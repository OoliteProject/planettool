/*
 *  FPMRaw.h
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-02.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */


#ifndef INCLUDED_FPMRaw_h
#define INCLUDED_FPMRaw_h

#include "FloatPixMap.h"

FPM_BEGIN_EXTERN_C


/*	FPMWriteRaw()
	Simplistic output of eight-bit ARGB pixel data with no header and naïve
	quantization, mostly useful for debugging other output handlers.
*/
bool FPMWriteRaw(FloatPixMapRef pm, const char *path, float max);


FPM_END_EXTERN_C
#endif	/* INCLUDED_FPMRaw_h */
