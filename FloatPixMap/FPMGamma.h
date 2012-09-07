/*
	FPMGamma.h
	FloatPixMap
	
	Gamma conversion function for FloatPixMap.
	
	
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
	quantising effect would be a bad idea.
	
	The effect on values outside the range 0..1 is undefined. If necessary,
	clamp or scale data before calling FPMApplyGamma().
*/
void FPMApplyGamma(FloatPixMapRef pm, FPMGammaFactor currentGamma, FPMGammaFactor desiredGamma, unsigned steps);


FPM_END_EXTERN_C
#endif	/* INCLUDED_FPMGamma_h */
