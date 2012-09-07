/*
	FPMQuantize.c
	FloatPixMap
	
	
	Copyright © 2009 Jens Ayton
 
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
