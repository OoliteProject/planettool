/*
 *  FPMQuantize.h
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-01.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */


#ifndef INCLUDED_FPMQuantize_h
#define INCLUDED_FPMQuantize_h

#include "FloatPixMap.h"

FPM_BEGIN_EXTERN_C


enum
{
	kFPMQuantizeDither			= 0x00000001,	// If set, error-diffusion dither is used.
	kFPMQuantizeJitter			= 0x00000002,	// If set, jitter is used in dithering. Ignored if not dithering.
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
