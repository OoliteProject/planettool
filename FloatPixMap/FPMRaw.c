/*
 *  FPMRaw.c
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-02.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#include "FPMRaw.h"
#include "FPMImageOperations.h"


bool FPMWriteRaw(FloatPixMapRef pm, const char *path, float max)
{
	if (pm == NULL || path == NULL)  return false;
	
	FILE *file = fopen(path, "wb");
	if (file == NULL)  return false;
	
	FPMForEachPixel(pm, ^(FPMColor *pixel, FPMPoint coords)
	{
		FPMColor px = FPMClampColorRange(*pixel, 0, max);
		uint8_t bytes[4] = {
			px.a * 255.0f / max,
			px.r * 255.0f / max,
			px.g * 255.0f / max,
			px.b * 255.0f / max
		};
		
		fwrite(&bytes, 1, 4, file);
	});
	
	fclose(file);
	return true;
}
