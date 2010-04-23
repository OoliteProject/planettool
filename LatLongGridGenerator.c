/*
	LatLongGridGenerator.c
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

#include "LatLongGridGenerator.h"


#if 1
static const FPMColor kLatGridEastColor = { 0, 0.5, 0.5, 1 };
static const FPMColor kLatGridWestColor = { 1, 0, 0, 1 };
static const FPMColor kLonGridNorthColor = { 0, 0, 1, 1 };
static const FPMColor kLonGridSouthColor = { 0, 1, 0, 1 };
#else
#define kLatGridEastColor kFPMColorBlack
#define kLatGridWestColor kFPMColorBlack
#define kLonGridNorthColor kFPMColorBlack
#define kLonGridSouthColor kFPMColorBlack
#endif
#define kBackgroundColor kFPMColorWhite


static FPMColor LatLongGridGenerator(Coordinates where, RenderFlags flags, void *context)
{
	float latitude, longitude;
	const float interval = 360.0f/36.0f;	// interval, degrees
	const float width = 0.5f;				// line width, degrees
	
	CoordsGetLatLongDeg(where, &latitude, &longitude);
	
	float modLat = (latitude < 0.0f) ? latitude + 360.0f : latitude;
	float modLong = (longitude < 0.0f) ? longitude + 360.0f : longitude;
	modLat = fmodf(modLat, interval);
	
	if (modLat < width / 2.0f || modLat > (interval - width / 2.0f))
	{
		return (longitude < 0) ? kLatGridWestColor : kLatGridEastColor;
	}
	
	modLong = fmodf(modLong, interval);
	float scaledWidth;
	scaledWidth = width;
	// This is supposed to give us equal thickness around the globe, but it don't.
//	scaledWidth = interval - (interval - width) * cosf(latitude * kDegToRad);
	
	if (modLong < scaledWidth / 2.0f || modLong > (interval - scaledWidth / 2.0f))
	{
		return (latitude < 0) ? kLonGridSouthColor : kLonGridNorthColor;
	}
	
	return kBackgroundColor;
}


bool LatLongGridGeneratorConstructor(FloatPixMapRef sourceImage, RenderFlags flags, SphericalPixelSourceFunction *source, void **context)
{
	*source = LatLongGridGenerator;
	return true;
}
