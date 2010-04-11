/*
 *  RenderToCube.c
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-03.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#include "RenderToCube.h"
#include "FPMImageOperations.h"


#define SAMPLE_GRID_SIZE_FAST	3	// Should be odd.
#define SAMPLE_GRID_SIZE_HIGHQ	11	// Should be odd.
//#define SAMPLE_MIDDLE			((SAMPLE_GRID_SIZE - 1) / 2)
//#define SAMPLE_MIDDLE_F			((float)SAMPLE_MIDDLE)

#define SAMPLE_WIDTH				1.2f


static void RenderCubeFace(FloatPixMapRef pm, size_t size, unsigned xoff, unsigned yoff, Vector outVector, Vector downVector, bool mirror, RenderFlags flags, unsigned sampleGridSize, float *weights, SphericalPixelSourceFunction source, void *context);


FloatPixMapRef RenderToCube(size_t size, RenderFlags flags, SphericalPixelSourceFunction source, void *sourceContext, ProgressCallbackFunction progress, void *progressContext)
{
	if (size < 1)
	{
		fprintf(stderr, "Size must be non-zero.\n");
		return NULL;
	}
	
	FloatPixMapRef pm = FPMCreateC(size, size * 6);
	if (pm == NULL)
	{
		fprintf(stderr, "Could not create a %llu by %llu pixel pixmap.\n", (unsigned long long)size, (unsigned long long)size * 6);
		return NULL;
	}
	
	unsigned sampleGridSize = (flags & kRenderFast) ? SAMPLE_GRID_SIZE_FAST : SAMPLE_GRID_SIZE_HIGHQ;
	float weights[sampleGridSize];
	BuildGaussTable(sampleGridSize, weights);
	
	// Render faces:
	// +x
	RenderCubeFace(pm, size, 0, 0, kBasisXVector, vector_flip(kBasisYVector), false, flags, sampleGridSize, weights, source, sourceContext);
	// -x
	RenderCubeFace(pm, size, 0, 1, vector_flip(kBasisXVector), vector_flip(kBasisYVector), false, flags, sampleGridSize, weights, source, sourceContext);
	// +y
	RenderCubeFace(pm, size, 0, 2, kBasisYVector, kBasisZVector, false, flags, sampleGridSize, weights, source, sourceContext);
	// -y
	RenderCubeFace(pm, size, 0, 3, vector_flip(kBasisYVector), vector_flip(kBasisZVector), false, flags, sampleGridSize, weights, source, sourceContext);
	// +z
	RenderCubeFace(pm, size, 0, 4, kBasisZVector, vector_flip(kBasisYVector), false, flags, sampleGridSize, weights, source, sourceContext);
	// -z
	RenderCubeFace(pm, size, 0, 5, vector_flip(kBasisZVector), vector_flip(kBasisYVector), false, flags, sampleGridSize, weights, source, sourceContext);
	
	return pm;
}


FloatPixMapRef RenderToCubeFlipped(size_t size, RenderFlags flags, SphericalPixelSourceFunction source, void *sourceContext, ProgressCallbackFunction progress, void *progressContext)
{
	if (size < 1)
	{
		fprintf(stderr, "Size must be non-zero.\n");
		return NULL;
	}
	
	FloatPixMapRef pm = FPMCreateC(size, size * 6);
	if (pm == NULL)
	{
		fprintf(stderr, "Could not create a %llu by %llu pixel pixmap.\n", (unsigned long long)size, (unsigned long long)size * 6);
		return NULL;
	}
	
	unsigned sampleGridSize = (flags & kRenderFast) ? SAMPLE_GRID_SIZE_FAST : SAMPLE_GRID_SIZE_HIGHQ;
	float weights[sampleGridSize];
	BuildGaussTable(sampleGridSize, weights);
	
	// Render faces:
	// -x
	RenderCubeFace(pm, size, 0, 0, vector_flip(kBasisXVector), vector_flip(kBasisYVector), true, flags, sampleGridSize, weights, source, sourceContext);
	// +x
	RenderCubeFace(pm, size, 0, 1, kBasisXVector, vector_flip(kBasisYVector), true, flags, sampleGridSize, weights, source, sourceContext);
	// +y
	RenderCubeFace(pm, size, 0, 2, kBasisYVector, kBasisZVector, true, flags, sampleGridSize, weights, source, sourceContext);
	// -y
	RenderCubeFace(pm, size, 0, 3, vector_flip(kBasisYVector), vector_flip(kBasisZVector), true, flags, sampleGridSize, weights, source, sourceContext);
	// +z
	RenderCubeFace(pm, size, 0, 4, kBasisZVector, vector_flip(kBasisYVector), true, flags, sampleGridSize, weights, source, sourceContext);
	// -z
	RenderCubeFace(pm, size, 0, 5, vector_flip(kBasisZVector), vector_flip(kBasisYVector), true, flags, sampleGridSize, weights, source, sourceContext);
	
	return pm;
}


FloatPixMapRef RenderToCubeCross(size_t size, RenderFlags flags, SphericalPixelSourceFunction source, void *sourceContext, ProgressCallbackFunction progress, void *progressContext)
{
	if (size < 1)
	{
		fprintf(stderr, "Size must be non-zero.\n");
		return NULL;
	}
	
	FloatPixMapRef pm = FPMCreateC(size * 4, size * 3);
	if (pm == NULL)
	{
		fprintf(stderr, "Could not create a %llu by %llu pixel pixmap.\n", (unsigned long long)size * 4, (unsigned long long)size * 3);
		return NULL;
	}
	
	unsigned sampleGridSize = (flags & kRenderFast) ? SAMPLE_GRID_SIZE_FAST : SAMPLE_GRID_SIZE_HIGHQ;
	float weights[sampleGridSize];
	BuildGaussTable(sampleGridSize, weights);
	
	// Render faces:
	// +x
	RenderCubeFace(pm, size, 2, 1, kBasisXVector, vector_flip(kBasisYVector), false, flags, sampleGridSize, weights, source, sourceContext);
	// -x
	RenderCubeFace(pm, size, 0, 1, vector_flip(kBasisXVector), vector_flip(kBasisYVector), false, flags, sampleGridSize, weights, source, sourceContext);
	// +y
	RenderCubeFace(pm, size, 1, 0, kBasisYVector, kBasisZVector, false, flags, sampleGridSize, weights, source, sourceContext);
	// -y
	RenderCubeFace(pm, size, 1, 2, vector_flip(kBasisYVector), vector_flip(kBasisZVector), false, flags, sampleGridSize, weights, source, sourceContext);
	// +z
	RenderCubeFace(pm, size, 1, 1, kBasisZVector, vector_flip(kBasisYVector), false, flags, sampleGridSize, weights, source, sourceContext);
	// -z
	RenderCubeFace(pm, size, 3, 1, vector_flip(kBasisZVector), vector_flip(kBasisYVector), false, flags, sampleGridSize, weights, source, sourceContext);
	
	return pm;
}


static void RenderCubeFace(FloatPixMapRef pm, size_t size, unsigned xoff, unsigned yoff, Vector outVector, Vector downVector, bool mirror, RenderFlags flags, unsigned sampleGridSize, float *weights, SphericalPixelSourceFunction source, void *context)
{
	FloatPixMapRef subPM = FPMCreateSubC(pm, size * xoff, size * yoff, size, size);
	assert(subPM != NULL);
	
	Vector rightVector = cross_product(outVector, downVector);
	if (mirror)	 rightVector = vector_flip(rightVector);
	
	float scale = 2.0f / (float)size;
	float fdiff = (2.0f * SAMPLE_WIDTH / (float)sampleGridSize) * scale;
	
	bool jitter = (flags & kRenderJitter);
	
	FPM_FOR_EACH_PIXEL(subPM, true)
	{
		float fx = x;
		float fy = y;
		float fminx, fminy;
		if (!jitter)
		{
			fminx = (fx - SAMPLE_WIDTH) * scale - 1.0f;
			fminy = (fy - SAMPLE_WIDTH) * scale - 1.0f;
		}
		else
		{
			fminx = fx * scale - 1.0f;
			fminy = fy * scale - 1.0f;
		}

		
		FPMColor accum = kFPMColorClear;
		float totalWeight = 0.0f;
		float weight, yw;
		unsigned sx, sy;
		
		fy = fminy;
		for (sy = 0; sy < sampleGridSize; sy++)
		{
			fx = fminx;
			yw = weights[sy];
			for (sx = 0; sx < sampleGridSize; sx++)
			{
				if (!jitter){} else
				{
					fx = fminx + RandF2() * SAMPLE_WIDTH * 0.5 * scale;
					fy = fminy + RandF2() * SAMPLE_WIDTH * 0.5 * scale;
				}
				
				Vector coordv = vector_multiply_scalar(rightVector, fx);
				coordv = vector_add(coordv, vector_multiply_scalar(downVector, fy));
				coordv = vector_add(coordv, outVector);
				coordv = vector_normal(coordv);
				
				Coordinates coord = MakeCoordsVector(coordv);
				
				FPMColor sample = source(coord, flags, context);
				
				if (!jitter)
				{
					weight = yw * weights[sx];
					fx += fdiff;
				}
				else
				{
					weight = GaussTableLookup2D(fx, fminx, fy, fminy, SAMPLE_WIDTH * 0.5, sampleGridSize, weights);
				}

				
				accum = FPMColorAdd(FPMColorMultiply(sample, weight), accum);
				totalWeight += weight;
			}
			fy += fdiff;
		}
		*pixel = FPMColorMultiply(accum, 1.0f / totalWeight);
	}
	FPM_END_FOR_EACH_PIXEL
}
