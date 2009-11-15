/*
 *  ReadCube.h
 *  planettool
 *
 *  Created by Jens Ayton on 2009-11-15.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#include "SphericalPixelSource.h"


bool ReadCubeConstructor(FloatPixMapRef sourceImage, RenderFlags flags, void **context);
bool ReadCubeCrossConstructor(FloatPixMapRef sourceImage, RenderFlags flags, void **context);
void ReadCubeDestructor(void *context);

FPMColor ReadCube(Coordinates where, RenderFlags flags, void *context);
