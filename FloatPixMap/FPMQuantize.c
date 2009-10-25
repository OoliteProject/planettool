/*
 *  FPMQuantize.c
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-01.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#include "FPMQuantize.h"
#include "FPMImageOperations.h"


static inline void ForEachComponent(FPMColor *pixel, float(^block)(float value))
{
	pixel->r = block(pixel->r);
	pixel->g = block(pixel->g);
	pixel->b = block(pixel->b);
	pixel->a = block(pixel->a);
}


void FPMQuantize(FloatPixMapRef pm, float srcMin, float srcMax, float targetMin, float targetMax, unsigned steps, FPMQuantizeFlags options)
{
	if (pm == NULL)  return;
	
	float srcScale = srcMax - srcMin;
	float targetScale = targetMax - targetMin;
	float stepMax = steps - 1;
	float srcToStepsFactor = stepMax / srcScale;
	float srcToStepsOffset = srcToStepsFactor * srcMin;
	bool clip = options & kFMPQuantizeClip;
	float stepsToTargetFactor = targetScale / stepMax;
	
	// FIXME: implement dithering and jitter. De-blockify for lesser platforms.
	FPM_FOR_EACH_PIXEL(pm, true)
		ForEachComponent(pixel, ^float(float v)
		{
			v = v * srcToStepsFactor - srcToStepsOffset;
			if (clip)
			{
				v = fmaxf(0.0f, fminf(v, stepMax));
			}
			v = roundf(v);
			
			v = v * stepsToTargetFactor + targetMin;
			
			return v;
		});
	FPM_END_FOR_EACH_PIXEL
}
