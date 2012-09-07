/*
	FloatPixMap.h
	
	Basic manipulation of floating-point pixmaps.
	
	A FloatPixMap consists of a 2D array of FPMColors, each FPMColor being
	four floats in r, g, b, a order. FloatPixMaps are reference counted,
	following the conventions of Apple's CoreFoundation and related libraries:
	• A function whose name contains Create, Retain, or Copy creates an owning
	  reference.
	• Each owning reference must be released by calling FPMRelease.
	
	FloatPixMap components are assumed to use a linear colour space, with 0..1
	being the standard range for low dynamic range operations. FloatPixMaps
	are suitable for high dynamic range operations, but no HDR-oriented
	functionality is provided (in particular, no tone mapping other than
	FPMNormalize()).
	
	
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


#ifndef INCLUDED_FloatPixMap_h
#define INCLUDED_FloatPixMap_h

#include "FPMBasics.h"

FPM_BEGIN_EXTERN_C


typedef struct FloatPixMap *FloatPixMapRef;


/*** Initialization (required) ***/

bool FPMInit(void);


/*** Creation and memory management ***/

FloatPixMapRef FPMCreate(FPMSize size);
FPM_INLINE FloatPixMapRef FPMCreateC(FPMDimension width, FPMDimension height)
{
	return FPMCreate(FPMMakeSize(width, height));
}

FloatPixMapRef FPMRetain(FloatPixMapRef pm);
void FPMRelease(FloatPixMapRef *pm);
uintptr_t FPMGetRetainCount(FloatPixMapRef pm);

FloatPixMapRef FPMCopy(FloatPixMapRef pm);

/*	FPMCreateSub()
	Create a pixmap representing a rectangle within a larger pixmap, sharing
	the same data. (That is, modifying the sub-pixmap will also modify the
	relevant region of the original pixmap, and vice versa.) The caller is not
	required to ensure that the original pixmap outlives the sub-pixmap.
*/
FloatPixMapRef FPMCreateSub(FloatPixMapRef pm, FPMRect rect);
FPM_INLINE FloatPixMapRef FPMCreateSubC(FloatPixMapRef pm, FPMCoordinate x, FPMCoordinate y, FPMDimension width, FPMDimension height)
{
	return FPMCreateSub(pm, FPMMakeRectC(x, y, width, height));
}

/*	FPMCopySub()
	Create a pixmap based on a copy of the data in a rectangular region of an
	existing pixmap.
*/
FloatPixMapRef FPMCopySub(FloatPixMapRef pm, FPMRect rect);
FPM_INLINE FloatPixMapRef FPMCopySubC(FloatPixMapRef pm, FPMCoordinate x, FPMCoordinate y, FPMDimension width, FPMDimension height)
{
	return FPMCopySub(pm, FPMMakeRectC(x, y, width, height));
}


/*** Accessors ***/

FPMDimension FPMGetWidth(FloatPixMapRef pm) FPM_PURE;
FPMDimension FPMGetHeight(FloatPixMapRef pm) FPM_PURE;
FPMSize FPMGetSize(FloatPixMapRef pm) FPM_PURE;
FPM_INLINE FPMDimension FPMGetArea(FloatPixMapRef pm) FPM_PURE;
FPM_INLINE FPMDimension FPMGetArea(FloatPixMapRef pm)  { return FPMSizeArea(FPMGetSize(pm)); }

bool FPMPointInRange(FloatPixMapRef pm, FPMPoint pt) FPM_PURE;
FPM_INLINE bool FPMPointInRangeC(FloatPixMapRef pm, FPMCoordinate x, FPMCoordinate y) FPM_PURE;
FPM_INLINE bool FPMPointInRangeC(FloatPixMapRef pm, FPMCoordinate x, FPMCoordinate y)  { return FPMPointInRange(pm, FPMMakePoint(x, y)); }

FPMPoint FPMClipPointToFPM(FloatPixMapRef pm, FPMPoint pt) FPM_PURE;
FPMRect FPMClipRectToFPM(FloatPixMapRef pm, FPMRect pt) FPM_PURE;

FPMColor FPMGetPixel(FloatPixMapRef pm, FPMPoint pt);
FPM_INLINE FPMColor FPMGetPixelC(FloatPixMapRef pm, FPMCoordinate x, FPMCoordinate y)  { return FPMGetPixel(pm, FPMMakePoint(x, y)); }

void FPMSetPixel(FloatPixMapRef pm, FPMPoint pt, FPMColor px);
FPM_INLINE void FPMSetPixelC(FloatPixMapRef pm, FPMCoordinate x, FPMCoordinate y, FPMColor px)  { return FPMSetPixel(pm, FPMMakePoint(x, y), px); }
FPM_INLINE void FPMSetPixelV(FloatPixMapRef pm, FPMPoint pt, FPMComponent r, FPMComponent g, FPMComponent b, FPMComponent a)  { return FPMSetPixel(pm, pt, FPMMakeColor(r, g, b, a)); }
FPM_INLINE void FPMSetPixelCV(FloatPixMapRef pm, FPMCoordinate x, FPMCoordinate y, FPMComponent r, FPMComponent g, FPMComponent b, FPMComponent a)  { return FPMSetPixel(pm, FPMMakePoint(x, y), FPMMakeColor(r, g, b, a)); }


/*	FPMGetPixelPointer()
	FPMGetBufferPointer()
	Get a pointer to pixel data. NOTE: when iterating over pixel data, you
	MUST take FPMGetRowComponentCount() into account. ALSO NOTE: if the pixmap
	is empty, i.e. width or height are 0, FPMGetBufferPointer() may be NULL.
*/
FPMColor *FPMGetPixelPointer(FloatPixMapRef pm, FPMPoint pt) FPM_PURE;
FPM_INLINE FPMColor *FPMGetPixelPointerC(FloatPixMapRef pm, FPMCoordinate x, FPMCoordinate y) FPM_PURE;
FPM_INLINE FPMColor *FPMGetPixelPointerC(FloatPixMapRef pm, FPMCoordinate x, FPMCoordinate y)  { return FPMGetPixelPointer(pm, FPMMakePoint(x, y)); };
FPM_INLINE FPMColor *FPMGetBufferPointer(FloatPixMapRef pm) FPM_PURE;
FPM_INLINE FPMColor *FPMGetBufferPointer(FloatPixMapRef pm)  { return FPMGetPixelPointerC(pm, 0, 0); };

/*	FPMGetRowPixelCount()
	FPMGetRowComponentCount()
	FPMGetRowByteCount()
	The number of components/pixels/bytes per row, i.e. the offset between the
	same column in each row. FPMGetRowPixelCount() is not guaranteed to be
	equal to width, but for obvious reasons can't be smaller.
*/
FPMDimension FPMGetRowPixelCount(FloatPixMapRef pm) FPM_PURE;
FPM_INLINE FPMDimension FPMGetRowComponentCount(FloatPixMapRef pm) FPM_PURE;
FPM_INLINE FPMDimension FPMGetRowComponentCount(FloatPixMapRef pm)  { return FPMGetRowPixelCount(pm) * 4; }
FPM_INLINE size_t FPMGetRowByteCount(FloatPixMapRef pm) FPM_PURE;
FPM_INLINE size_t FPMGetRowByteCount(FloatPixMapRef pm)  { return FPMGetRowComponentCount(pm) * sizeof (float); }

/*	FPMGetIterationInformation()
	Quick accessor to get information needed to iterate over pixels. All
	arguments must be non-NULL.
	
	bufferStart is set to point to the top left pixel. NOTE: if the pixmap is
	empty, i.e. width or height are 0, bufferStart may be NULL.
	width and height are set to the dimensions of the pixmap.
	rowOffset is set to width - rowCount.
*/
void FPMGetIterationInformation(FloatPixMapRef pm, FPMColor **bufferStart, FPMDimension *width, FPMDimension *height, size_t *rowOffset) FPM_NON_NULL_ALL;


FPM_END_EXTERN_C
#endif	// INCLUDED_FloatPixMap_h
