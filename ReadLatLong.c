/*
 *  ReadLatLong.c
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-03.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#include "ReadLatLong.h"
#include "FPMImageOperations.h"


typedef struct
{
	FloatPixMapRef		pm;
	size_t				pwidth;
	float				width;
	float				height;
} ReadLatLongContext;


bool ReadLatLongConstructor(FloatPixMapRef sourceImage, RenderFlags flags, void **context)
{
	if (sourceImage == NULL || context == NULL)  return false;
	
	ReadLatLongContext *cx = malloc(sizeof (ReadLatLongContext));
	if (cx == NULL)  return false;
	
	cx->pm = FPMRetain(sourceImage);
	cx->pwidth = FPMGetWidth(sourceImage);
	cx->width = (float)cx->pwidth / (2.0f * kPiF);
	cx->height = (float)FPMGetHeight(sourceImage) / kPiF;
	
	*context = cx;
	return true;
}


void ReadLatLongDestructor(void *context)
{
	assert(context != NULL);
	ReadLatLongContext *cx = context;
	
	FPMRelease(&cx->pm);
	free(cx);
}


FPMColor ReadLatLong(Coordinates where, RenderFlags flags, void *context)
{
	ReadLatLongContext *cx = context;
	
	float rlon, rlat, lon, lat;
	CoordsGetLatLongRad(where, &rlat, &rlon);
	lon = (rlon + kPiF) * cx->width;
	lat = (kPiF / 2.0f - rlat) * cx->height;
	
	FPMColor sample;
	if (!(flags & kRenderFast))
	{
		sample = FPMSampleLinear(cx->pm, lon, lat, kFPMWrapClamp, kFPMWrapRepeat);
	}
	else
	{
		sample = FPMGetPixelC(cx->pm, (size_t)lon % cx->pwidth, lat);
	}
	return sample;
}
