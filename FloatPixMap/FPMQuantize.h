/*
	FPMQuantize.h
	FloatPixMap
	
	Quantization function for FloatPixMap.
	
	
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


#ifndef INCLUDED_FPMQuantize_h
#define INCLUDED_FPMQuantize_h

#include "FloatPixMap.h"

FPM_BEGIN_EXTERN_C


enum
{
	kFPMQuantizeDither			= 0x00000001,	// If set, error-diffusion dither is used. [UNIMPLEMENTED]
	kFPMQuantizeJitter			= 0x00000002,	// If set, jitter is used in dithering. Ignored if not dithering. [UNIMPLEMENTED]
	kFMPQuantizeClip			= 0x00000004,	// If set, out-of-range values are clipped, otherwise they're scaled by the same amount as in-range values.
	kFMPQuantizeAlpha			= 0x00000008,	// If set, alpha channel is quantized too.
};
typedef uint32_t FPMQuantizeFlags;


/*	FPMQuantize()
	Scale and round off pixel component values so that values from srcMin to
	srcMax are assigned one of steps values between targetMin and targetMax.
	
	Examples, given srcMin/targetMin = 0, srcMax/targetMax = 1, steps = 3:
		0		0
		0.1		0
		0.3		0.5
		0.5		0.5
		0.9		1.0
	
	The typical use of this is to prepare data for writing to an integer-based
	file format.
*/
void FPMQuantize(FloatPixMapRef pm, float srcMin, float srcMax, float targetMin, float targetMax, unsigned steps, FPMQuantizeFlags options);


FPM_END_EXTERN_C
#endif	/* INCLUDED_FPMQuantize_h */
