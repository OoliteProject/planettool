/*
 *  LatLongGridGenerator.c
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-03.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
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


FPMColor LatLongGridGenerator(Coordinates where, RenderFlags flags, void *context)
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
