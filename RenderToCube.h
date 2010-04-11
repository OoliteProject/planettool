/*
 *  RenderToCube.h
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-03.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */


#include "SphericalPixelSource.h"


FloatPixMapRef RenderToCube(size_t size, RenderFlags flags, SphericalPixelSourceFunction source, void *sourceContext, ProgressCallbackFunction progress, void *progressContext);
FloatPixMapRef RenderToCubeFlipped(size_t size, RenderFlags flags, SphericalPixelSourceFunction source, void *sourceContext, ProgressCallbackFunction progress, void *progressContext);

FloatPixMapRef RenderToCubeCross(size_t size, RenderFlags flags, SphericalPixelSourceFunction source, void *sourceContext, ProgressCallbackFunction progress, void *progressContext);
