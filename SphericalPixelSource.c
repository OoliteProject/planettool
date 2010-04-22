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
	v = vector_normal(v);
	float las = v.y;
	if (las != 1.0f)
	{
		float lat = asinf(las);
		float rlac = OOInvSqrtf(1.0f - las * las);	// Equivalent to abs(1/cos(lat))
		
		if (latitude != NULL)  *latitude = lat;
		if (longitude != NULL)
		{
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
	}
	else
	{
		// Straight up, avoid divide-by-zero
		if (latitude != NULL)  *latitude = kPiF / 2.0f;
		if (longitude != NULL)  *longitude = 0.0f;	// arbitrary
	}
}


void VectorToCoordsDeg(Vector v, float *latitude, float *longitude)
{
	VectorToCoordsRad(v, latitude, longitude);
	if (latitude != NULL)  *latitude *= 180.0f / kPiF;
	if (longitude != NULL)  *longitude *= 180.0f / kPiF;
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


void DummyProgressCallback(size_t numerator, size_t denominator, void *context)
{
	// Do nothing.
}
