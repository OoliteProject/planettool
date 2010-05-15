/*
	RenderToMercator.c
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

#include "RenderToMercator.h"
#include "FPMImageOperations.h"
#include "PlanetToolScheduler.h"


FPM_INLINE void GetLatLong(float x, float y, float size, float *lat, float *lon)
{
	float adjY = ((size * 3 / 2 - y) / size - 0.5f) * kPiF;
	*lat = 2 * atanf(expf(adjY)) - (kPiF / 2.0f);
	*lon = (x / size - 1.0f) * kPiF;
}


#define SAMPLE_GRID_SIZE_FAST	3	// Should be odd.
#define SAMPLE_GRID_SIZE_HIGHQ	11	// Should be odd.

#define HALF_WIDTH			0.5f


static bool RenderMercatorLine(size_t lineIndex, size_t lineCount, void *vcontext);


typedef struct RenderMercatorContext
{
	FloatPixMapRef					pm;
	size_t							size;
	
	unsigned						sampleGridSize;
	float							*weights;
	
	SphericalPixelSourceFunction	source;
	void							*sourceContext;
	
	RenderFlags						flags;
	
} RenderMercatorContext;


FloatPixMapRef RenderToMercator(uintmax_t size, RenderFlags flags, SphericalPixelSourceFunction source, void *sourceContext, ProgressCallbackFunction progress, ErrorCallbackFunction error, void *cbContext)
{
	FloatPixMapRef pm = ValidateAndCreatePixMap(size, size, size, error, cbContext);
	if (pm == NULL)  return NULL;
	
	unsigned sampleGridSize = (flags & kRenderFast) ? SAMPLE_GRID_SIZE_FAST : SAMPLE_GRID_SIZE_HIGHQ;
	float weights[sampleGridSize];
	BuildGaussTable(sampleGridSize, weights);
	
	RenderMercatorContext context =
	{
		.pm = pm,
		.size = size,
		.sampleGridSize = sampleGridSize,
		.weights = weights,
		.source = source,
		.sourceContext = sourceContext,
		.flags = flags
	};
	
	if (!ScheduleRender(RenderMercatorLine, &context, size, 0, 1, progress, cbContext))
	{
		FPMRelease(&pm);
	}
	
	return pm;
}


static bool RenderMercatorLine(size_t lineIndex, size_t lineCount, void *vcontext)
{
	RenderMercatorContext *context = vcontext;
	
	size_t size = context->size;
	
	SphericalPixelSourceFunction source = context->source;
	void *sourceContext = context->sourceContext;
	
	unsigned sampleGridSize = context->sampleGridSize;
	float *weights = context->weights;
	
	RenderFlags flags = context->flags;
	
	FPMColor *pixel = FPMGetPixelPointerC(context->pm, 0, lineIndex);
	FPMDimension x, y = lineIndex;
	
	for (x = 0; x < size; x++)
	{
		float latMin, lonMin, latMax, lonMax, latDiff, lonDiff;
		GetLatLong((float)x - HALF_WIDTH + 0.5f, (float)y - HALF_WIDTH + 0.5f, size / 2, &latMin, &lonMin);
		GetLatLong((float)x + HALF_WIDTH + 0.5f, (float)y + HALF_WIDTH + 0.5f, size / 2, &latMax, &lonMax);
		
		latDiff = latMax - latMin;
		lonDiff = lonMax - lonMin;
		float latStep = latDiff * 1.0f / (float)(sampleGridSize - 1);
		float lonStep = lonDiff * 1.0f / (float)(sampleGridSize - 1);
		
		FPMColor accum = kFPMColorClear;
		float totalWeight = 0.0f;
		float lat = latMin, lon;
		float weight, yw;
		unsigned sx, sy;
		
		for (sy = 0; sy < sampleGridSize; sy++)
		{
			lon = lonMin;
			yw = weights[sy];
			for (sx = 0; sx < sampleGridSize; sx++)
			{
				FPMColor sample = source(MakeCoordsLatLongRad(lat, lon), flags, sourceContext);
				weight = yw * weights[sx];
				
				accum = FPMColorAdd(FPMColorMultiply(sample, weight), accum);
				totalWeight += weight;
				
				lon += lonStep;
			}
			lat += latStep;
		}
		
		*pixel++ = FPMColorMultiply(accum, 1.0f / totalWeight);
	}
	
	return true;
}
