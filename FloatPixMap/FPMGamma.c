/*
 *  FPMGamma.c
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-01.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#include "FPMGamma.h"
#include "FPMImageOperations.h"


const FPMGammaFactor kFPMGammaLinear			= 1.0f;
const FPMGammaFactor kFPMGammaSRGB				= 2.2f;
const FPMGammaFactor kFPMGammaTraditionalMac	= 1.8f;


#ifndef FPM_GAMMA_USE_LUT
#define FPM_GAMMA_USE_LUT 0
#endif


void ApplyGammaDirect(FloatPixMapRef pm, FPMColor *pixel, FPMPoint coords, void *info)
{
	FPMGammaFactor gamma = *(FPMGammaFactor *)info;
	pixel->r = powf(pixel->r, gamma);
	pixel->g = powf(pixel->g, gamma);
	pixel->b = powf(pixel->b, gamma);
}


#if FPM_GAMMA_USE_LUT

typedef struct ApplyGammaLUTInfo
{
	float			scale;
	unsigned		entries;
	float			*lut;
} ApplyGammaLUTInfo;


FPM_INLINE float ApplyGammaOne(float value, ApplyGammaLUTInfo *info)
{
	if (value > 0.0f)
	{
		unsigned index = value * info->scale;
		if (index < info->entries)  value = info->lut[index];
	}
	return value;
}


void ApplyGammaLUT(FloatPixMapRef pm, FPMColor *pixel, FPMPoint coords, void *info)
{
	pixel->r = ApplyGammaOne(pixel->r, info);
	pixel->g = ApplyGammaOne(pixel->g, info);
	pixel->b = ApplyGammaOne(pixel->b, info);
}

#endif


void FPMApplyGamma(FloatPixMapRef pm, FPMGammaFactor currentGamma, FPMGammaFactor desiredGamma, unsigned steps)
{
	if (pm != NULL && currentGamma != desiredGamma && steps > 1)
	{
		FPMGammaFactor resultingGamma = desiredGamma / currentGamma;
		
#if FPM_GAMMA_USE_LUT
		if (steps >= FPMGetArea(pm) * 5)	// 5 is a fudge factor; 3 would strictly minimize number of powf() calls.
		{
			// No point in building a LUT if picture is not much bigger than LUT.
			FPMForEachPixelF(pm, ApplyGammaDirect, &resultingGamma);
			return;
		}
		
		static float *lookupTable = NULL;
		static unsigned lastTableSize = 0;
		static float lastGamma;
		
		if (lastTableSize != steps || resultingGamma != lastGamma)
		{
			free(lookupTable);
			lookupTable = malloc(steps * sizeof (float));
			if (lookupTable == NULL)
			{
				// Fallback: apply gamma the slow way.
				FPMForEachPixelF(pm, ApplyGammaDirect, &resultingGamma);
				return;
			}
			
			unsigned i;
			for (i = 0; i < steps; i++)
			{
				lookupTable[i] = powf((float)i / (float)steps, resultingGamma);
			}
		}
		
		ApplyGammaLUTInfo info =
		{
			steps,
			steps,
			lookupTable
		};
		
		FPMForEachPixelF(pm, ApplyGammaLUT, &info);
#else
		FPMForEachPixelF(pm, ApplyGammaDirect, &resultingGamma);
#endif
	}
}
