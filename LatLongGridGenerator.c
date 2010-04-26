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
	// Grid spacing in degrees.
	const float kInterval	= 10.0f;
	const float kWidth		= 0.5f;
	
	/*	To avoid nasty fmods, or even moderately nasty integer mods, we use
		an integer coordinate scheme where 0x01000000 corresponds to 10
		degrees.
	*/
	float flat, flon;
	CoordsGetLatLongDeg(where, &flat, &flon);
	
	/*	Get rid of negative values and convert to ints. Vales will range from
		approximately 180 degrees to 540 degrees, or 0x12000000 to 0x36000000.
	*/
	const float kConversionFactor = (float)0x01000000 / kInterval;
	uint_fast32_t lat = (flat + 360.0f) * kConversionFactor;
	
	//	Get latitude + half of width modulo ten degrees in integer coord system.
	const uint_fast32_t kIntWidth = kWidth * kConversionFactor + 0.5f;
	const uint_fast32_t kIntHalfWidth = kWidth / 2.0f * kConversionFactor + 0.5f;
	
	uint_fast32_t mlat = (lat + kIntHalfWidth) & 0x00FFFFFF;
	
	if (mlat < kIntWidth)
	{
		return (flon < 0.0f) ? kLatGridWestColor : kLatGridEastColor;
	}
	
	// Same for longitude.
	uint_fast32_t lon = (flon + 360.0f) * kConversionFactor;
	uint_fast32_t mlon = (lon + kIntHalfWidth) & 0x00FFFFFF;
	if (mlon < kIntWidth)
	{
		const uint_fast32_t midLat = 360.0f * kConversionFactor;
		return (lat < midLat) ? kLonGridSouthColor : kLonGridNorthColor;
	}
	
	return kBackgroundColor;
}


bool LatLongGridGeneratorConstructor(FloatPixMapRef sourceImage, RenderFlags flags, SphericalPixelSourceFunction *source, void **context)
{
	*source = LatLongGridGenerator;
	return true;
}
