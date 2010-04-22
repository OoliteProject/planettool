/*
 *  RenderToLatLong.c
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-03.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#include "RenderToLatLong.h"
#include "FPMImageOperations.h"
#include "PlanetToolScheduler.h"


FPM_INLINE void GetLatLong(float x, float y, float size, float *lat, float *lon)
{
	*lat = ((size - y) / size - 0.5f) * kPiF;
	*lon = (x / size - 1.0f) * kPiF;
}


#define SAMPLE_GRID_SIZE_FAST	3	// Should be odd.
#define SAMPLE_GRID_SIZE_HIGHQ	11	// Should be odd.

#define HALF_WIDTH			0.5f


static bool RenderLatLongLine(size_t lineIndex, size_t lineCount, void *vcontext);


typedef struct RenderCubeFaceContext
{
	FloatPixMapRef					pm;
	size_t							size;
	
	unsigned						sampleGridSize;
	float							*weights;
	
	SphericalPixelSourceFunction	source;
	void							*sourceContext;
	
	RenderFlags						flags;
	
} RenderLatLongContext;


FloatPixMapRef RenderToLatLong(size_t size, RenderFlags flags, SphericalPixelSourceFunction source, void *sourceContext, ProgressCallbackFunction progress, void *progressContext)
{
	if (size < 1)
	{
		fprintf(stderr, "Size must be non-zero.\n");
		return NULL;
	}
	
	FloatPixMapRef pm = FPMCreateC(size * 2, size);
	if (pm == NULL)
	{
		fprintf(stderr, "Could not create a %llu by %llu pixel pixmap.\n", (unsigned long long)size * 2, (unsigned long long)size);
		return NULL;
	}
	
	unsigned sampleGridSize = (flags & kRenderFast) ? SAMPLE_GRID_SIZE_FAST : SAMPLE_GRID_SIZE_HIGHQ;
	float weights[sampleGridSize];
	BuildGaussTable(sampleGridSize, weights);
	
#if 0
	FPM_FOR_EACH_PIXEL(pm, true)
		float latMin, lonMin, latMax, lonMax, latDiff, lonDiff;
		GetLatLong((float)x - HALF_WIDTH + 0.5f, (float)y - HALF_WIDTH + 0.5f, size, &latMin, &lonMin);
		GetLatLong((float)x + HALF_WIDTH + 0.5f, (float)y + HALF_WIDTH + 0.5f, size, &latMax, &lonMax);
		
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
		
		*pixel = FPMColorMultiply(accum, 1.0f / totalWeight);
	FPM_END_FOR_EACH_PIXEL
#else
	RenderLatLongContext context =
	{
		.pm = pm,
		.size = size,
		.sampleGridSize = sampleGridSize,
		.weights = weights,
		.source = source,
		.sourceContext = sourceContext,
		.flags = flags
	};
	
	ScheduleRender(RenderLatLongLine, &context, size, 0, 1, progress, progressContext);
#endif
	
	return pm;
}


static bool RenderLatLongLine(size_t lineIndex, size_t lineCount, void *vcontext)
{
	RenderLatLongContext *context = vcontext;
	
	size_t size = context->size;
	
	SphericalPixelSourceFunction source = context->source;
	void *sourceContext = context->sourceContext;
	
	unsigned sampleGridSize = context->sampleGridSize;
	float *weights = context->weights;
	
	RenderFlags flags = context->flags;
	
	FPMColor *pixel = FPMGetPixelPointerC(context->pm, 0, lineIndex);
	FPMDimension x, y = lineIndex;
	
	for (x = 0; x < size * 2; x++)
	{
		float latMin, lonMin, latMax, lonMax, latDiff, lonDiff;
		GetLatLong((float)x - HALF_WIDTH + 0.5f, (float)y - HALF_WIDTH + 0.5f, size, &latMin, &lonMin);
		GetLatLong((float)x + HALF_WIDTH + 0.5f, (float)y + HALF_WIDTH + 0.5f, size, &latMax, &lonMax);
		
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
