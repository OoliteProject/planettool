/*
 *  ReadLatLong.h
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-03.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#include "SphericalPixelSource.h"


bool ReadLatLongConstructor(FloatPixMapRef sourceImage, RenderFlags flags, void **context);
void ReadLatLongDestructor(void *context);

FPMColor ReadLatLong(Coordinates where, RenderFlags flags, void *context);
