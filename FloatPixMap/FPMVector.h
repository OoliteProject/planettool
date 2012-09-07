/*
	FPMVector.h
	FloatPixMap
	
	Macros controlling use of vector intrinsics. NOTE: runtime feature
	detection is currently not supported. FPM must be built requiring vector
	instructions or without support.
	
	
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


#ifndef INCLUDED_FPMVector_h
#define INCLUDED_FPMVector_h

#include "FPMBasics.h"


FPM_BEGIN_EXTERN_C


#ifndef FPM_USE_ALTIVEC
#if __ALTIVEC__
#define FPM_USE_ALTIVEC 1
#else
#define FPM_USE_ALTIVEC 0
#endif
#endif


#ifndef FPM_USE_SSE
#if __SSE__
#define FPM_USE_SSE 1
#else
#define FPM_USE_SSE 0
#endif
#endif


#if FPM_USE_SSE && FPM_USE_ALTIVEC
#warning Set to use both SSE and Altivec, which makes no sense. Disabling both.
#undef FPM_USE_SSE
#define FPM_USE_SSE 0
#undef FPM_USE_ALTIVEC
#define FPM_USE_ALTIVEC 0
#endif


#ifndef FPM_USE_SSE2
#if __SSE2__
#define FPM_USE_SSE2 1
#else
#define FPM_USE_SSE2 0
#endif
#endif

#if FPM_USE_SSE2 && !FPM_USE_SSE
#warning Cannot use SSE2 but not SSE.
#undef FPM_USE_SSE2
#define FPM_USE_SSE2 0
#endif


#ifndef FPM_USE_SSE3
#if __SSE3__
#define FPM_USE_SSE3 1
#else
#define FPM_USE_SSE3 0
#endif
#endif

#if FPM_USE_SSE3 && !FPM_USE_SSE2
#warning Cannot use SSE3 but not SSE2.
#undef FPM_USE_SSE3
#define FPM_USE_SSE3 0
#endif


#if FPM_USE_ALTIVEC
#ifndef __APPLE_ALTIVEC__
// Work around the fact that altivec.h has a stupid non-standard definition of bool.
#undef bool
#include <altivec.h>
#undef bool
#define bool _Bool
#undef vector
#undef pixel
#endif
#endif


#if FPM_USE_SSE
#include <xmmintrin.h>
#endif


#if FPM_USE_SSE2
#include <emmintrin.h>
#endif


#if FPM_USE_SSE3
#include <pmmintrin.h>
#endif


#if FPM_USE_ALTIVEC

typedef __vector float FPMVFloat;

FPM_INLINE FPMVFloat FPMVectorFromColor(FPMColor color)
{
	FPMVFloat result = { color.r, color.g, color.b, color.a };
	return result;
}

#elif FPM_USE_SSE

typedef __m128 FPMVFloat;

FPM_INLINE FPMVFloat FPMVectorFromColor(FPMColor color)
{
	FPMVFloat result = { color.r, color.g, color.b, color.a };
	return result;
}

#endif	/* FPM_USE_ALTIVEC/FPM_USE_SSE */


FPM_END_EXTERN_C
#endif	/* INCLUDED_FPMVector_h */
