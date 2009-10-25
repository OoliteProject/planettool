/*	FPMVector.h
	© 2009 Jens Ayton
	
	Macros controlling use of vector intrinsics. NOTE: runtime feature
	detection is currently not supported. FPM must be built requiring vector
	instructions or without support.
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
