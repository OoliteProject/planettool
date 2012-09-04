/*
	ReadCube.c
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

#include "ReadCube.h"
#include "FPMImageOperations.h"


typedef struct
{
	FloatPixMapRef		pm;
	FPMSize				faceSize;
	float				halfWidth;
	float				halfHeight;
	float				maxX;
	float				maxY;
	// Top left corners of each face.
	FPMPoint			pxPos;
	FPMPoint			nxPos;
	FPMPoint			pyPos;
	FPMPoint			nyPos;
	FPMPoint			pzPos;
	FPMPoint			nzPos;
} ReadCubeContext;


static FPMColor ReadCube(Coordinates where, RenderFlags flags, void *context);
static FPMColor ReadCubeEdge(ReadCubeContext *context, float x, float y, Vector coordinates);


bool ReadCubeConstructor(FloatPixMapRef sourceImage, RenderFlags flags, SphericalPixelSourceFunction *source, void **context)
{
	if (sourceImage == NULL || context == NULL)  return false;
	
	ReadCubeContext *cx = malloc(sizeof (ReadCubeContext));
	if (cx == NULL)  return false;
	
	cx->pm = FPMRetain(sourceImage);
	
	FPMSize totalSize = FPMGetSize(sourceImage);
	if (totalSize.height % 6 != 0)
	{
		fprintf(stderr, "Cube map height must be a multiple of six pixels.\n");
		free(cx);
		FPMRelease(&sourceImage);
		return false;
	}
	
	cx->faceSize.width = totalSize.width;
	cx->faceSize.height = totalSize.height / 6;
	cx->halfWidth = (float)cx->faceSize.width / 2.0f;
	cx->halfHeight = (float)cx->faceSize.height / 2.0f;
	cx->maxX = (float)cx->faceSize.width - 1.0f;
	cx->maxY = (float)cx->faceSize.height - 1.0f;
	
	cx->pxPos = (FPMPoint){ 0, 0 * cx->faceSize.height };
	cx->nxPos = (FPMPoint){ 0, 1 * cx->faceSize.height };
	cx->pyPos = (FPMPoint){ 0, 2 * cx->faceSize.height };
	cx->nyPos = (FPMPoint){ 0, 3 * cx->faceSize.height };
	cx->pzPos = (FPMPoint){ 0, 4 * cx->faceSize.height };
	cx->nzPos = (FPMPoint){ 0, 5 * cx->faceSize.height };
	
	*context = cx;
	*source = ReadCube;
	return true;
}


bool ReadCubeCrossConstructor(FloatPixMapRef sourceImage, RenderFlags flags, SphericalPixelSourceFunction *source, void **context)
{
	if (sourceImage == NULL || context == NULL)  return false;
	
	ReadCubeContext *cx = malloc(sizeof (ReadCubeContext));
	if (cx == NULL)  return false;
	
	cx->pm = FPMRetain(sourceImage);
	
	FPMSize totalSize = FPMGetSize(sourceImage);
	if (totalSize.width % 4 != 0 || totalSize.height % 3 != 0)
	{
		fprintf(stderr, "Cross cube map width must be a multiple of four pixels and height must be a multiple of three pixels.\n");
		free(cx);
		FPMRelease(&sourceImage);
		return false;
	}
	
	cx->faceSize.width = totalSize.width / 4;
	cx->faceSize.height = totalSize.height / 3;
	cx->halfWidth = (float)cx->faceSize.width / 2.0;
	cx->halfHeight = (float)cx->faceSize.height / 2.0;
	cx->maxX = (float)cx->faceSize.width - 1.0f;
	cx->maxY = (float)cx->faceSize.height - 1.0f;
	
	cx->pxPos = (FPMPoint){ 2 * cx->faceSize.width, 1 * cx->faceSize.height };
	cx->nxPos = (FPMPoint){ 0 * cx->faceSize.width, 1 * cx->faceSize.height };
	cx->pyPos = (FPMPoint){ 1 * cx->faceSize.width, 0 * cx->faceSize.height };
	cx->nyPos = (FPMPoint){ 1 * cx->faceSize.width, 2 * cx->faceSize.height };
	cx->pzPos = (FPMPoint){ 1 * cx->faceSize.width, 1 * cx->faceSize.height };
	cx->nzPos = (FPMPoint){ 3 * cx->faceSize.width, 1 * cx->faceSize.height };
	
	*context = cx;
	*source = ReadCube;
	return true;
}


void ReadCubeDestructor(void *context)
{
	assert(context != NULL);
	ReadCubeContext *cx = context;
	
	FPMRelease(&cx->pm);
	free(cx);
}


static FPMColor ReadCube(Coordinates where, RenderFlags flags, void *context)
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
	
	assert(ax != 0.0f || ay != 0.0f || az != 0.0f);
	
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
	
#if 0
	if (x < 0.0 || x > (cx->halfWidth * 2.0 - 1.0))  printf("x: %g (width: %g)\n", x, cx->halfWidth * 2.0);
	if (y < 0.0 || y > (cx->halfHeight * 2.0 - 1.0))  printf("y: %g (height: %g)\n", y, cx->halfHeight * 2.0);
#endif
	if (1)//(1 <= x && x <= cx->maxX && 1 <= y && y <= cx->maxY)
	{
		FPMColor result = FPMSampleLinearClamp(cx->pm, x + faceOffset.x, y + faceOffset.y);
		return result;
	}
	else
	{
		// Deal with nasty edge cases.
		return ReadCubeEdge(cx, x, y, coords);
	}
}


static FPMColor ReadCubeEdge(ReadCubeContext *context, float x, float y, Vector coordinates)
{
	if (x < 2)  return (FPMColor){ 0, 0, 10, 1 };
	if (y < 2)  return (FPMColor){ 0, 10, 0, 1 };
	
	return (FPMColor){ 10, 0, 0, 1 };
}
