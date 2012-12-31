/*
	FloatPixMap.c
	
	
 	Copyright © 2009–2013 Jens Ayton
 
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

#include "FloatPixMap.h"
#include <assert.h>
#include <string.h>


const FPMColor kFPMColorInvalid     = { -INFINITY, -INFINITY, -INFINITY, -INFINITY };
const FPMColor kFPMColorClear       = { 0.0f, 0.0f, 0.0f, 0.0f };
const FPMColor kFPMColorBlack       = { 0.0f, 0.0f, 0.0f, 1.0f };
const FPMColor kFPMColorWhite       = { 1.0f, 1.0f, 1.0f, 1.0f };

const FPMPoint kFPMPointZero		= { 0, 0 };

const FPMSize kFPMSizeZero			= { 0, 0 };

const FPMRect kFPMRectZero			= {{ 0, 0 }, { 0, 0, }};


static bool sInited = false;


#if FPM_EXTRA_VALIDATION
/*	FPM_INTERNAL_ASSERT()
	Used only to assert preconditions of internal, static functions
	(generally, pm != NULL). Plain assert() is used to check public
	functions where relevant.
*/
#define FPM_INTERNAL_ASSERT(x) assert(x)
#else
#define FPM_INTERNAL_ASSERT(x) do {} while (0)
#endif


typedef struct FloatPixMap
{
	size_t					retainCount;
	size_t					width;
	size_t					height;
	size_t					rowCount;
	FPMColor				*pixels;
	FloatPixMapRef			master;
} FloatPixMap;


bool FPMInit(void)
{
	// For future needs.
	sInited = true;
	return true;
}


static bool PointInRange(FloatPixMapRef pm, FPMPoint pt)
{
	FPM_INTERNAL_ASSERT(pm != NULL);
	
	return 0 <= pt.x && (FPMDimension)pt.x < pm->width &&
	0 <= pt.y && (FPMDimension)pt.y < pm->height;
}


static off_t PixelIndex(FloatPixMapRef pm, FPMPoint pt)
{
	FPM_INTERNAL_ASSERT(pm != NULL);
	FPM_INTERNAL_ASSERT(PointInRange(pm, pt));
	
	return pm->rowCount * pt.y + pt.x;
}


static size_t GetPixelCount(FloatPixMapRef pm)
{
	FPM_INTERNAL_ASSERT(pm != NULL);
	
	return pm->width * pm->height;
}


/*	NOTE: the Clang Static Analyzer claims callers of this function are leaking
	the “pixels” parameter. This is untrue, but there doesn’t seem to be a way
	to annotate functions as consuming a parameter outside of ObjC or
	CoreFoundation code.
*/
static FloatPixMapRef MakeFPM(FPMSize size, FPMDimension rowCount, FPMColor *pixels, FloatPixMapRef master)
{
	FloatPixMapRef result = malloc(sizeof (FloatPixMap));
	if (result != NULL)
	{
		FPM_INTERNAL_ASSERT(FPMSizeArea(size) == 0 || pixels != NULL);
		FPM_INTERNAL_ASSERT(size.width <= rowCount);
		
		result->retainCount = 1;
		result->width = size.width;
		result->height = size.height;
		result->rowCount = rowCount;
		result->pixels = pixels;
		result->master = FPMRetain(master);
	}
	
	return result;
}


static FloatPixMapRef MakeEmptyFPM(FPMSize nominalSize)
{
	return MakeFPM(nominalSize, nominalSize.width, NULL, NULL);
}


FloatPixMapRef FPMCreate(FPMSize size)
{
	assert(sInited);
	
	if (FPMSizeArea(size) == 0)  return NULL;
	FPMColor *pixels = NULL;
	
	// Calculate area and check for overflow.
	uintmax_t area = FPMSizeArea(size);
	if ((size_t)area * sizeof *pixels < area)
	{
		return NULL;
	}
	
	pixels = calloc(area, sizeof *pixels);
	if (pixels == NULL)  return NULL;
	
	return MakeFPM(size, size.width, pixels, NULL);
}


FloatPixMapRef FPMRetain(FloatPixMapRef pm)
{
	if (pm != NULL)
	{
		assert(pm->retainCount < SIZE_MAX);
		pm->retainCount++;
	}
	
	return pm;
}


static void Destroy(FloatPixMapRef pm)
{
	FPM_INTERNAL_ASSERT(pm != NULL);
	
	// Only "masterless" FPMs own their pixels.
	if (pm->master == NULL)
	{
		free(pm->pixels);
	}
	else
	{
		FPMRelease(&pm->master);
	}
	
	free(pm);
}


void FPMRelease(FloatPixMapRef *pm)
{
	if (pm != NULL && *pm != NULL)
	{
		assert((*pm)->retainCount > 0);
		if (--(*pm)->retainCount == 0)
		{
			Destroy(*pm);
		}
		*pm = NULL;
	}
}


uintptr_t FPMGetRetainCount(FloatPixMapRef pm)
{
	if (pm != NULL)
	{
		return pm->retainCount;
	}
	else
	{
		return 0;
	}
}


FloatPixMapRef FPMCopy(FloatPixMapRef pm)
{
	if (pm != NULL)
	{
		size_t byteCount = GetPixelCount(pm) * sizeof (FPMColor);
		if (byteCount == 0)
		{
			// One empty pixmap of a given size is the same as another, so we'll stick with the one instead of making another.
			assert(pm->pixels == NULL);
			return FPMRetain(pm);
		}
		
		assert(pm->pixels != NULL);
		
		void *pixels = malloc(byteCount);
		if (pixels == NULL)  return NULL;
		
		if (pm->rowCount == pm->width)
		{
			// No padding to skip.
			memcpy(pixels, pm->pixels, byteCount);
		}
		else
		{
			// Original is padded, copy row by row.
			FPMColor *srcPx = pm->pixels;
			FPMColor *dstPx = pixels;
			size_t count = pm->height;
			
			do
			{
				memcpy(dstPx, srcPx, sizeof (FPMColor) * pm->width);
				srcPx += pm->rowCount;
				dstPx += pm->width;
			} while (--count);
		}
		
		return MakeFPM(FPMMakeSize(pm->width, pm->height), pm->width, pixels, NULL);
	}
	else
	{
		return NULL;
	}
}


FloatPixMapRef FPMCreateSub(FloatPixMapRef pm, FPMRect rect)
{
	if (pm != NULL)
	{
		rect = FPMClipRectToFPM(pm, rect);
		if (FPMRectArea(rect) != 0)
		{
			// Build sub-pixmap
			return MakeFPM(rect.size, pm->rowCount, FPMGetPixelPointer(pm, rect.origin), pm);
		}
		else
		{
			// Empty rect -> empty pixmap.
			return MakeEmptyFPM(rect.size);
		}
	}
	else
	{
		return NULL;
	}
}


FloatPixMapRef FPMCopySub(FloatPixMapRef pm, FPMRect rect)
{
	FloatPixMapRef sub = FPMCreateSub(pm, rect);
	FloatPixMapRef result = FPMCopy(sub);
	FPMRelease(&sub);
	return result;
}


FPMDimension FPMGetWidth(FloatPixMapRef pm)
{
	if (pm != NULL)
	{
		return pm->width;
	}
	else
	{
		return 0;
	}
}


FPMDimension FPMGetHeight(FloatPixMapRef pm)
{
	if (pm != NULL)
	{
		return pm->height;
	}
	else
	{
		return 0;
	}
}


FPMSize FPMGetSize(FloatPixMapRef pm)
{
	if (pm != NULL)
	{
		return FPMMakeSize(pm->width, pm->height);
	}
	else
	{
		return kFPMSizeZero;
	}
}


bool FPMPointInRange(FloatPixMapRef pm, FPMPoint pt)
{
	if (pm != NULL)
	{
		return PointInRange(pm, pt);
	}
	else
	{
		return false;
	}
}


FPMPoint FPMClipPointToFPM(FloatPixMapRef pm, FPMPoint pt)
{
	if (pm != NULL)
	{
		return FPMClipPointToSize(pt, FPMMakeSize(pm->width, pm->height));
	}
	else
	{
		return kFPMPointZero;
	}
}


FPMRect FPMClipRectToFPM(FloatPixMapRef pm, FPMRect pt)
{
	if (pm != NULL)
	{
		return FPMClipRectToRect(pt, FPMMakeRectC(0, 0, pm->width, pm->height));
	}
	else
	{
		return kFPMRectZero;
	}
}


FPMColor FPMGetPixel(FloatPixMapRef pm, FPMPoint pt)
{
	if (pm != NULL && PointInRange(pm, pt))
	{
		assert(pm->pixels != NULL);
		
		return pm->pixels[PixelIndex(pm, pt)];
	}
	else
	{
		return kFPMColorInvalid;
	}
}


void FPMSetPixel(FloatPixMapRef pm, FPMPoint pt, FPMColor px)
{
	if (pm != NULL && PointInRange(pm, pt))
	{
		assert(pm->pixels != NULL);
		
		pm->pixels[PixelIndex(pm, pt)] = px;
	}
}


FPMColor *FPMGetPixelPointer(FloatPixMapRef pm, FPMPoint pt)
{
	if (pm != NULL && PointInRange(pm, pt))
	{
		assert(pm->pixels != NULL);
		
		return pm->pixels + PixelIndex(pm, pt);
	}
	else
	{
		return NULL;
	}
}


FPMDimension FPMGetRowPixelCount(FloatPixMapRef pm)
{
	if (pm != NULL)
	{
		return pm->rowCount;
	}
	else
	{
		return 0;
	}
}


void FPMGetIterationInformation(FloatPixMapRef pm, FPMColor **bufferStart, FPMDimension *width, FPMDimension *height, size_t *rowOffset)
{
	assert(bufferStart != NULL && width != NULL && height != NULL && rowOffset != NULL);
	
	if (pm != NULL)
	{
		assert(pm->pixels != NULL || pm->width == 0 || pm->height == 0);
		
		*bufferStart = pm->pixels;
		*width = pm->width;
		*height = pm->height;
		*rowOffset = pm->rowCount - pm->width;
	}
	else
	{
		*bufferStart = NULL;
		*width = 0;
		*height = 0;
		*rowOffset = 0;
	}
}


FPMRect FPMMakeRectWithPoints(FPMPoint a, FPMPoint b)
{
	FPMCoordinate minX = fminf(a.x, b.x);
	FPMCoordinate maxX = fmaxf(a.x, b.x);
	FPMCoordinate minY = fminf(a.y, b.y);
	FPMCoordinate maxY = fmaxf(a.y, b.y);
	return FPMMakeRectC(minX, minY, maxX - minX, maxY - minY);
}


FPMRect FPMMakeRectWith3Points(FPMPoint a, FPMPoint b, FPMPoint c)
{
	FPMCoordinate minX = fminf(a.x, fminf(b.x, c.x));
	FPMCoordinate maxX = fmaxf(a.x, fmaxf(b.x, c.x));
	FPMCoordinate minY = fminf(a.y, fminf(b.y, c.y));
	FPMCoordinate maxY = fmaxf(a.y, fmaxf(b.y, c.y));
	return FPMMakeRectC(minX, minY, maxX - minX, maxY - minY);
}


FPMRect FPMRectAddPoint(FPMRect rect, FPMPoint pt)
{
	return FPMMakeRectWith3Points(rect.origin, FPMRectBottomRight(rect), pt);
}


FPMPoint FPMClipPointToRect(FPMPoint pt, FPMRect clipRect)
{
	pt.x = fminf(fmaxf(pt.x, clipRect.origin.x), FPMRectRight(clipRect));
	pt.y = fminf(fmaxf(pt.y, clipRect.origin.y), FPMRectBottom(clipRect));
	return pt;
}


FPMRect FPMClipRectToRect(FPMRect rect, FPMRect clipRect)
{
	FPMCoordinate left = rect.origin.x;
	FPMCoordinate right = left + rect.size.width;
	FPMCoordinate top = rect.origin.y;
	FPMCoordinate bottom = top + rect.size.height;
	
	if (left < clipRect.origin.x)  left = clipRect.origin.x;
	if (FPMRectRight(clipRect) < right)  right = FPMRectRight(clipRect);
	if (top < clipRect.origin.y)  top = clipRect.origin.y;
	if (FPMRectBottom(clipRect) < bottom)  bottom = FPMRectBottom(clipRect);
	
	return FPMMakeRectWithPointsC(left, top, right, bottom);
}
