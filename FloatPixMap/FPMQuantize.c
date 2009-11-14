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


static float QuantizeComponentClip(float v, float srcToStepsFactor, float srcToStepsOffset, float stepsToTargetFactor, float stepMax, float targetMin)
{
	v = v * srcToStepsFactor - srcToStepsOffset;
	v = fmaxf(0.0f, fminf(v, stepMax));
	v = roundf(v);
	
	v = v * stepsToTargetFactor + targetMin;
	
	return v;
}


static float QuantizeComponent(float v, float srcToStepsFactor, float srcToStepsOffset, float stepsToTargetFactor, float targetMin)
{
	v = v * srcToStepsFactor - srcToStepsOffset;
	v = roundf(v);
	
	v = v * stepsToTargetFactor + targetMin;
	
	return v;
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
	
	// FIXME: implement dithering and jitter.
	if (clip)
	{
		FPM_FOR_EACH_PIXEL(pm, true)
			pixel->r = QuantizeComponentClip(pixel->r, srcToStepsFactor, srcToStepsOffset, stepsToTargetFactor, stepMax, targetMin);
			pixel->g = QuantizeComponentClip(pixel->g, srcToStepsFactor, srcToStepsOffset, stepsToTargetFactor, stepMax, targetMin);
			pixel->b = QuantizeComponentClip(pixel->b, srcToStepsFactor, srcToStepsOffset, stepsToTargetFactor, stepMax, targetMin);
			pixel->a = QuantizeComponentClip(pixel->a, srcToStepsFactor, srcToStepsOffset, stepsToTargetFactor, stepMax, targetMin);
		FPM_END_FOR_EACH_PIXEL
	}
	else
	{
		FPM_FOR_EACH_PIXEL(pm, true)
			pixel->r = QuantizeComponent(pixel->r, srcToStepsFactor, srcToStepsOffset, stepsToTargetFactor, targetMin);
			pixel->g = QuantizeComponent(pixel->g, srcToStepsFactor, srcToStepsOffset, stepsToTargetFactor, targetMin);
			pixel->b = QuantizeComponent(pixel->b, srcToStepsFactor, srcToStepsOffset, stepsToTargetFactor, targetMin);
			pixel->a = QuantizeComponent(pixel->a, srcToStepsFactor, srcToStepsOffset, stepsToTargetFactor, targetMin);
		FPM_END_FOR_EACH_PIXEL
	}
}
