/*
	SphericalPixelSource.c
	planettool
	
	
	Copyright © 2009–2010 Jens Ayton

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the “Software”),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#include "SphericalPixelSource.h"
#include <assert.h>
#include <stdarg.h>


Vector VectorFromCoordsRad(float latitude, float longitude)
{
	float las = sinf(latitude);
	float lac = cosf(latitude);
	float los = sinf(longitude);
	float loc = cosf(longitude);
	
	Vector result = make_vector(los * lac, las, loc * lac);
	assert(fabsf(magnitude(result) - 1.0f) < 0.01f);
	
	return result;
}


void VectorToCoordsRad(Vector v, float *latitude, float *longitude)
{
	assert(latitude != NULL && longitude != NULL);
	
	v = vector_normal(v);
	float las = v.y;
	if (EXPECT(las != 1.0f))
	{
		float lat = asinf(las);
		*latitude = lat;
		
		float rlac = fabsf(1.0f / cosf(lat));
		//	float rlac = OOInvSqrtf(1.0f - las * las);	// Equivalent to abs(1/cos(lat)), but turns out to be slower.
		
		float los = v.x * rlac;
		float lon = asinf(fminf(1.0f, fmaxf(-1.0f, los)));
		
		// Quadrant rectification.
		if (v.z < 0.0f)
		{
			// We're beyond 90 degrees of longitude in some direction.
			if (v.x < 0.0f)
			{
				// ...specifically, west.
				lon = -kPiF - lon;
			}
			else
			{
				// ...specifically, east.
				lon = kPiF - lon;
			}
		}
		
		*longitude = lon;
	}
	else
	{
		// Straight up, avoid divide-by-zero
		*latitude = kPiF / 2.0f;
		*longitude = 0.0f;	// arbitrary
	}
}


void VectorToCoordsDeg(Vector v, float *latitude, float *longitude)
{
	assert(latitude != NULL && longitude != NULL);
	
	VectorToCoordsRad(v, latitude, longitude);
	*latitude *= 180.0f / kPiF;
	*longitude *= 180.0f / kPiF;
}


#define GAUSS_WIDTH			2.2f

void BuildGaussTable(unsigned size, float *table)
{
	unsigned wi;
	float middle = (float)size * 0.5f - 0.5f;
	const float factor = GAUSS_WIDTH * GAUSS_WIDTH * 0.5f;
	
	for (wi = 0; wi < size; wi++)
	{
		float x = (middle - (float)wi) / middle;
		table[wi] = expf(-x * x * factor);
	}
}


FPM_INLINE float GSample(float *table, unsigned tblSize, int index)
{
	if (index > 0)
	{
		if (index < (int)tblSize)
		{
			return table[index];
		}
	}
	return 0.0f;
}


float GaussTableLookup(float value, float mid, float halfWidth, unsigned tblSize, float *table)
{
	float index = roundf(value - mid / halfWidth) + (float)((tblSize - 1) / 2);
	int lowIdx = index;
	
	float lowSample = GSample(table, tblSize, lowIdx);
	float highSample = GSample(table, tblSize, lowIdx);
	
	float alpha = index - lowIdx;
	return lowSample * alpha + highSample * (1.0f - alpha);
}


float GaussTableLookup2D(float x, float xmid, float y, float ymid, float halfWidth, unsigned tblSize, float *table)
{
	return GaussTableLookup(x, xmid, halfWidth, tblSize, table) * GaussTableLookup(y, ymid, halfWidth, tblSize, table);
}


bool DummyProgressCallback(size_t numerator, size_t denominator, void *context)
{
	return true;
}


void CallErrorCallbackWithFormat(ErrorCallbackFunction callback, void *cbContext, const char *format, ...)
{
	if (callback == NULL)  return;
	
	va_list args;
	char *message = NULL;
	va_start(args, format);
#ifndef NO_ASPRINTF
	vasprintf(&message, format, args);
#else
	enum { kMaxMessageSize = 4096 };
	message = malloc(kMaxMessageSize);
	if (message != NULL)  vsnprintf(message, kMaxMessageSize, format, args);
#endif
	
	if (message != NULL)  callback(message, cbContext);
	else  callback(format, cbContext);
	
	free(message);
}


FloatPixMapRef ValidateAndCreatePixMap(uintmax_t nominalSize, uintmax_t width, uintmax_t height, ErrorCallbackFunction errorCB, void *cbContext)
{
	if (nominalSize < 1)
	{
		CallErrorCallbackWithFormat(errorCB, cbContext, "Size must be non-zero.\n");
		return NULL;
	}
	
	if (width > FPM_DIMENSION_MAX || height > FPM_DIMENSION_MAX)
	{
		size_t maxSize;
		if (width > height)  maxSize = FPM_DIMENSION_MAX / (width / nominalSize);
		else  maxSize = FPM_DIMENSION_MAX / (height / nominalSize);
		
		CallErrorCallbackWithFormat(errorCB, cbContext, "Size must be no greater than %zu.\n", maxSize);
		return NULL;
	}
	
	FloatPixMapRef pm = FPMCreateC(width, height);
	if (pm == NULL)
	{
		CallErrorCallbackWithFormat(errorCB, cbContext, "Could not create a %llu by %llu pixel pixmap.\n", (unsigned long long)width, (unsigned long long)height);
		return NULL;
	}
	
	return pm;
}
