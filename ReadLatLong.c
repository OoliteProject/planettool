/*
	ReadLatLong.c
	planettool
	
	
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

#include "ReadLatLong.h"
#include "FPMImageOperations.h"


typedef struct
{
	FloatPixMapRef		pm;
	size_t				pwidth;
	float				width;
	float				height;
} ReadLatLongContext;


static FPMColor ReadLatLong(Coordinates where, RenderFlags flags, void *context);
static FPMColor ReadLatLongFast(Coordinates where, RenderFlags flags, void *context);


bool ReadLatLongConstructor(FloatPixMapRef sourceImage, RenderFlags flags, SphericalPixelSourceFunction *source, void **context)
{
	if (sourceImage == NULL || context == NULL)  return false;
	
	ReadLatLongContext *cx = malloc(sizeof (ReadLatLongContext));
	if (cx == NULL)  return false;
	
	cx->pm = FPMRetain(sourceImage);
	cx->pwidth = FPMGetWidth(sourceImage);
	cx->width = (float)cx->pwidth / (2.0f * kPiF);
	cx->height = (float)FPMGetHeight(sourceImage) / kPiF;
	
	if (flags & kRenderFast)  *source = ReadLatLongFast;
	else  *source = ReadLatLong;
	
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


static FPMColor ReadLatLong(Coordinates where, RenderFlags flags, void *context)
{
	ReadLatLongContext *cx = context;
	
	float rlon, rlat, lon, lat;
	CoordsGetLatLongRad(where, &rlat, &rlon);
	lon = (rlon + kPiF) * cx->width;
	lat = (kPiF / 2.0f - rlat) * cx->height;
	
	return FPMSampleLinear(cx->pm, lon, lat, kFPMWrapRepeat, kFPMWrapClamp);
}


static FPMColor ReadLatLongFast(Coordinates where, RenderFlags flags, void *context)
{
	ReadLatLongContext *cx = context;
	
	float rlon, rlat, lon, lat;
	CoordsGetLatLongRad(where, &rlat, &rlon);
	lon = (rlon + kPiF) * cx->width;
	lat = (kPiF / 2.0f - rlat) * cx->height;
	
	return FPMGetPixelC(cx->pm, (size_t)lon % cx->pwidth, lat);
}
