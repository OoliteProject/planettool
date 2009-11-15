/*
 *  ReadCube.c
 *  planettool
 *
 *  Created by Jens Ayton on 2009-11-15.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
 */

#include "ReadCube.h"
#include "FPMImageOperations.h"


typedef struct
{
	FloatPixMapRef		pm;
	FPMSize				faceSize;
	float				halfWidth;
	float				halfHeight;
	// Top left corners of each face.
	FPMPoint			pxPos;
	FPMPoint			nxPos;
	FPMPoint			pyPos;
	FPMPoint			nyPos;
	FPMPoint			pzPos;
	FPMPoint			nzPos;
} ReadCubeContext;


bool ReadCubeConstructor(FloatPixMapRef sourceImage, RenderFlags flags, void **context)
{
	if (sourceImage == NULL || context == NULL)  return false;
	
	ReadCubeContext *cx = malloc(sizeof (ReadCubeContext));
	if (cx == NULL)  return false;
	
	cx->pm = FPMRetain(sourceImage);
	
	FPMSize totalSize = FPMGetSize(sourceImage);
	if (totalSize.height % 6 != 0)
	{
		fprintf(stderr, "Cube map height must be a multiple of six pixels.\n");
		return false;
	}
	
	cx->faceSize.width = totalSize.width;
	cx->faceSize.height = totalSize.height / 6;
	cx->halfWidth = (float)cx->faceSize.width / 2.0;
	cx->halfHeight = (float)cx->faceSize.height / 2.0;
	
	cx->pxPos = (FPMPoint){ 0, 0 * cx->faceSize.height };
	cx->nxPos = (FPMPoint){ 0, 1 * cx->faceSize.height };
	cx->pyPos = (FPMPoint){ 0, 2 * cx->faceSize.height };
	cx->nyPos = (FPMPoint){ 0, 3 * cx->faceSize.height };
	cx->pzPos = (FPMPoint){ 0, 4 * cx->faceSize.height };
	cx->nzPos = (FPMPoint){ 0, 5 * cx->faceSize.height };
	
	*context = cx;
	return true;
}


bool ReadCubeCrossConstructor(FloatPixMapRef sourceImage, RenderFlags flags, void **context)
{
	if (sourceImage == NULL || context == NULL)  return false;
	
	ReadCubeContext *cx = malloc(sizeof (ReadCubeContext));
	if (cx == NULL)  return false;
	
	cx->pm = FPMRetain(sourceImage);
	
	FPMSize totalSize = FPMGetSize(sourceImage);
	if (totalSize.width % 4 != 0 || totalSize.height % 3 != 0)
	{
		fprintf(stderr, "Cross cube map width must be a multiple of four pixels and height must be a multiple of three pixels.\n");
		return false;
	}
	
	cx->faceSize.width = totalSize.width / 4;
	cx->faceSize.height = totalSize.height / 3;
	cx->halfWidth = (float)cx->faceSize.width / 2.0;
	cx->halfHeight = (float)cx->faceSize.height / 2.0;
	
	cx->pxPos = (FPMPoint){ 2 * cx->faceSize.width, 1 * cx->faceSize.height };
	cx->nxPos = (FPMPoint){ 0 * cx->faceSize.width, 1 * cx->faceSize.height };
	cx->pyPos = (FPMPoint){ 1 * cx->faceSize.width, 0 * cx->faceSize.height };
	cx->nyPos = (FPMPoint){ 1 * cx->faceSize.width, 2 * cx->faceSize.height };
	cx->pzPos = (FPMPoint){ 1 * cx->faceSize.width, 1 * cx->faceSize.height };
	cx->nzPos = (FPMPoint){ 3 * cx->faceSize.width, 1 * cx->faceSize.height };
	
	*context = cx;
	return true;
}


void ReadCubeDestructor(void *context)
{
	assert(context != NULL);
	ReadCubeContext *cx = context;
	
	FPMRelease(&cx->pm);
	free(cx);
}


FPMColor ReadCube(Coordinates where, RenderFlags flags, void *context)
{
	assert(context != NULL);
	ReadCubeContext *cx = context;
	
	// The largest coordinate component determines which face we’re looking at.
	Vector coords = CoordsGetVector(where);
	float ax = fabsf(coords.x);
	float ay = fabsf(coords.y);
	float az = fabsf(coords.z);
	FPMPoint faceOffset;
	float x, y;
	
	assert(ax * ay * az != 0);
	
	if (ax > ay && ax > az)
	{
		x = coords.z / ax;
		y = -coords.y / ax;
		if (0 < coords.x)
		{
			x = -x;
			faceOffset = cx->pxPos;
		}
		else
		{
			faceOffset = cx->nxPos;
		}
	}
	else if (ay > ax && ay > az)
	{
		x = coords.x / ay;
		y = coords.z / ay;
		if (0 < coords.y)
		{
			faceOffset = cx->pyPos;
		}
		else
		{
			y = -y;
			faceOffset = cx->nyPos;
		}
	}
	else
	{
		x = coords.x / az;
		y = -coords.y / az;
		if (0 < coords.z)
		{
			faceOffset = cx->pzPos;
		}
		else
		{
			x = -x;
			faceOffset = cx->nzPos;
		}
	}
	
	x = x * cx->halfWidth + cx->halfWidth;
	y = y * cx->halfHeight + cx->halfHeight;
	
	// FIXME: for seamless cube mapping, we need some nasty special cases for the edges.
	return FPMSampleLinearClamp(cx->pm, x + faceOffset.x, y + faceOffset.y);
}
