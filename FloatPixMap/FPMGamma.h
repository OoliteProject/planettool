/*
 *  FPMGamma.h
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-01.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */


#ifndef INCLUDED_FPMGamma_h
#define INCLUDED_FPMGamma_h

#include "FloatPixMap.h"

FPM_BEGIN_EXTERN_C


typedef float FPMGammaFactor;

extern const FPMGammaFactor kFPMGammaLinear;			// 1.0f
extern const FPMGammaFactor kFPMGammaSRGB;				// 2.2f
extern const FPMGammaFactor kFPMGammaTraditionalMac;	// 1.8f (standard for Mac screens prior to Snow Leopard)


/*	FPMApplyGamma()
	Apply gamma transformation to pixels, i.e. raise each colour component
	to the power desiredGamma/currentGamma. Alpha components are unaffected.
	
	To avoid taking years to process, a lookup table may be used. The steps
	parameter defines the size of the table. Table entries are distributed
	evenly between 0 and 1 in the source space.
	
	The use of the lookup table is not guaranteed, so relying on its
	quantifying effect would be a bad idea.
	
	The effect on values outside the range 0..1 is undefined. If necessary,
	clamp or scale data before calling FPMApplyGamma().
*/
void FPMApplyGamma(FloatPixMapRef pm, FPMGammaFactor currentGamma, FPMGammaFactor desiredGamma, unsigned steps);


FPM_END_EXTERN_C
#endif	/* INCLUDED_FPMGamma_h */
