/*
	FPMBasics.h
	FloatPixMap
	
	Basic types and macros used by FloatPixMap library.
	
	
	Copyright © 2009–2010 Jens Ayton
 
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


#ifndef INCLUDED_FPMBasics_h
#define INCLUDED_FPMBasics_h

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


#if __cplusplus
#define FPM_INLINE			inline
#define FPM_BEGIN_EXTERN_C	extern "C" {
#define FPM_END_EXTERN_C	}
#else
#define FPM_INLINE static	inline
#define FPM_BEGIN_EXTERN_C
#define FPM_END_EXTERN_C
#endif


#if __GNUC__ || __clang__
#define FPM_PURE __attribute__((pure))
#define FPM_CONST __attribute__((const))
#ifdef NDEBUG
#define FPM_FORCE_INLINE __attribute__((always_inline))
#else
#define FPM_FORCE_INLINE
#endif
#define FPM_NON_NULL_ALL __attribute__((nonnull))
#define FPM_GCC_PREFETCH __builtin_prefetch
#define FPM_EXPECT(x)  __builtin_expect((x), 1)
#define FPM_EXPECT_NOT(x)  __builtin_expect((x), 0)
#else
#define FPM_PURE
#define FPM_CONST
#define FPM_FORCE_INLINE
#define FPM_NON_NULL_ALL
#define FPM_GCC_PREFETCH(...) do {} while (0)
#define FPM_EXPECT(x)  (x)
#define FPM_EXPECT_NOT(x)  (x)
#endif


#if __BIG_ENDIAN__
#define FPM_BIG_ENDIAN		1
#endif
#if __LITTLE_ENDIAN__
#define FPM_LITTLE_ENDIAN	1
#endif


#if !defined(FPM_BIG_ENDIAN) && !defined(FPM_LITTLE_ENDIAN)
#if defined(__i386__) || defined(__amd64__) || defined(__x86_64__)
#define FPM_LITTLE_ENDIAN	1
#endif

#if defined(__sgi__) || defined(__mips__) 
#define FPM_BIG_ENDIAN		1 
#endif
#endif


#ifndef FPM_BIG_ENDIAN
#define FPM_BIG_ENDIAN		0
#endif

#ifndef FPM_LITTLE_ENDIAN
#define FPM_LITTLE_ENDIAN	0
#endif


#if !FPM_BIG_ENDIAN && !FPM_LITTLE_ENDIAN
#error Neither FPM_BIG_ENDIAN nor FPM_LITTLE_ENDIAN is defined as nonzero!
#elif FPM_BIG_ENDIAN && FPM_LITTLE_ENDIAN
#error Both FPM_BIG_ENDIAN and FPM_LITTLE_ENDIAN are defined as nonzero!
#endif


FPM_BEGIN_EXTERN_C


typedef float		FPMComponent;
typedef int32_t		FPMCoordinate;
typedef uint32_t	FPMDimension;
#define FPM_DIMENSION_MAX UINT32_MAX


// FPMColor: an RGBA colour value.
typedef struct FPMColor
{
	FPMComponent		r, g, b, a;
} FPMColor;

extern const FPMColor kFPMColorInvalid;	// { -INFINITY, -INFINITY, -INFINITY, -INFINITY } -- used as a placeholder, e.g. when calling FPMGetPixel with a NULL pixmap.
extern const FPMColor kFPMColorClear;	// { 0, 0, 0, 0 }
extern const FPMColor kFPMColorBlack;	// { 0, 0, 0, 1 }
extern const FPMColor kFPMColorWhite;	// { 1, 1, 1, 1 }


FPM_INLINE FPMColor FPMMakeColor(FPMComponent r, FPMComponent g, FPMComponent b, FPMComponent a) FPM_CONST;
FPM_INLINE FPMColor FPMMakeColor(FPMComponent r, FPMComponent g, FPMComponent b, FPMComponent a)
{
	FPMColor px = { r, g, b, a };
	return px;
}

FPM_INLINE FPMColor FPMMakeColorGrey(FPMComponent g, FPMComponent a) FPM_CONST FPM_FORCE_INLINE;
FPM_INLINE FPMColor FPMMakeColorGrey(FPMComponent g, FPMComponent a)
{
	return FPMMakeColor(g, g, g, a);
}

FPM_INLINE bool FPMColorsEqual(FPMColor a, FPMColor b) FPM_CONST;
FPM_INLINE bool FPMColorsEqual(FPMColor a, FPMColor b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

FPM_INLINE FPMComponent FPMClampComponentRange(FPMComponent c, float min, float max) FPM_CONST;
FPM_INLINE FPMComponent FPMClampComponentRange(FPMComponent c, float min, float max)
{
	return fmaxf(min, fminf(c, max));
}

FPM_INLINE FPMColor FPMClampColorRange(FPMColor c, float min, float max) FPM_CONST;
FPM_INLINE FPMColor FPMClampColorRange(FPMColor c, float min, float max)
{
	return FPMMakeColor(FPMClampComponentRange(c.r, min, max), FPMClampComponentRange(c.g, min, max), FPMClampComponentRange(c.b, min, max), FPMClampComponentRange(c.a, min, max));
}

FPM_INLINE FPMColor FPMClampColorRangeNotAlpha(FPMColor c, float min, float max) FPM_CONST;
FPM_INLINE FPMColor FPMClampColorRangeNotAlpha(FPMColor c, float min, float max)
{
	return FPMMakeColor(FPMClampComponentRange(c.r, min, max), FPMClampComponentRange(c.g, min, max), FPMClampComponentRange(c.b, min, max), c.a);
}

FPM_INLINE FPMComponent FPMClampComponent(FPMComponent c) FPM_CONST FPM_FORCE_INLINE;
FPM_INLINE FPMComponent FPMClampComponent(FPMComponent c)
{
	return FPMClampComponentRange(c, 0.0f, 1.0f);
}

FPM_INLINE FPMColor FPMClampColor(FPMColor c) FPM_CONST;
FPM_INLINE FPMColor FPMClampColor(FPMColor c)
{
	return FPMMakeColor(FPMClampComponent(c.r), FPMClampComponent(c.g), FPMClampComponent(c.b), FPMClampComponent(c.a));
}

FPM_INLINE FPMColor FPMClampColorNotAlpha(FPMColor c) FPM_CONST;
FPM_INLINE FPMColor FPMClampColorNotAlpha(FPMColor c)
{
	return FPMMakeColor(FPMClampComponent(c.r), FPMClampComponent(c.g), FPMClampComponent(c.b), c.a);
}

FPM_INLINE FPMColor FPMColorAdd(FPMColor a, FPMColor b) FPM_CONST;
FPM_INLINE FPMColor FPMColorAdd(FPMColor a, FPMColor b)
{
	return FPMMakeColor(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
}

FPM_INLINE FPMColor FPMColorBlend(FPMColor a, FPMColor b, float fraction) FPM_CONST;
FPM_INLINE FPMColor FPMColorBlend(FPMColor a, FPMColor b, float fraction)
{
#define FPM_LERP(comp)  (b.comp + fraction * (a.comp - b.comp))
	return FPMMakeColor(FPM_LERP(r), FPM_LERP(g), FPM_LERP(b), FPM_LERP(a));
#undef FPM_LERP
}

FPM_INLINE FPMColor FPMColorMultiply(FPMColor col, float scale) FPM_CONST;
FPM_INLINE FPMColor FPMColorMultiply(FPMColor col, float scale)
{
	return FPMMakeColor(col.r * scale, col.g * scale, col.b * scale, col.a * scale);
}


// FMPPoint: a point in two-dimensional space. 0, 0 is the top left of a pixmap.
typedef struct FPMPoint
{
	FPMCoordinate		x, y;
} FPMPoint;

extern const FPMPoint kFPMPointZero;	// { 0, 0 }

FPM_INLINE FPMPoint FPMMakePoint(FPMCoordinate x, FPMCoordinate y) FPM_CONST;
FPM_INLINE FPMPoint FPMMakePoint(FPMCoordinate x, FPMCoordinate y)
{
	FPMPoint pt = { x, y };
	return pt;
}

FPM_INLINE bool FPMPointsEqual(FPMPoint a, FPMPoint b) FPM_CONST;
FPM_INLINE bool FPMPointsEqual(FPMPoint a, FPMPoint b)
{
	return a.x == b.x && a.y == b.y;
}


// FPMSize: dimensions in two-dimensional space.
typedef struct FPMSize
{
	FPMDimension		width, height;
} FPMSize;

extern const FPMSize kFPMSizeZero;		// { 0, 0 }

FPM_INLINE FPMSize FPMMakeSize(FPMDimension width, FPMDimension height) FPM_CONST;
FPM_INLINE FPMSize FPMMakeSize(FPMDimension width, FPMDimension height)
{
	FPMSize dim = { width, height };
	return dim;
}

FPM_INLINE bool FPMSizesEqual(FPMSize a, FPMSize b) FPM_CONST;
FPM_INLINE bool FPMSizesEqual(FPMSize a, FPMSize b)
{
	return a.width == b.width && a.height == b.height;
}

FPM_INLINE bool FPMSizeEmpty(FPMSize sz) FPM_CONST;
FPM_INLINE bool FPMSizeEmpty(FPMSize sz)
{
	return sz.width == 0 && sz.height == 0;
}

FPM_INLINE uintmax_t FPMSizeArea(FPMSize size) FPM_CONST;
FPM_INLINE uintmax_t FPMSizeArea(FPMSize size)
{
	return (uintmax_t)size.width * (uintmax_t)size.height;
}


// FPMRect: an axis-aligned rectangle in two-dimensional space.
typedef struct FPMRect
{
	FPMPoint			origin;
	FPMSize				size;
} FPMRect;

extern const FPMRect kFPMRectZero;		// {{ 0, 0 }, { 0, 0 }}

FPM_INLINE FPMRect FPMMakeRect(FPMPoint origin, FPMSize dim) FPM_CONST;
FPM_INLINE FPMRect FPMMakeRect(FPMPoint origin, FPMSize dim)
{
	FPMRect rect = { origin, dim };
	return rect;
}

FPM_INLINE FPMRect FPMMakeRectC(FPMCoordinate x, FPMCoordinate y, FPMDimension width, FPMDimension height) FPM_CONST;
FPM_INLINE FPMRect FPMMakeRectC(FPMCoordinate x, FPMCoordinate y, FPMDimension width, FPMDimension height)
{
	return FPMMakeRect(FPMMakePoint(x, y), FPMMakeSize(width, height));
}

// Make rectangle containing two points.
FPMRect FPMMakeRectWithPoints(FPMPoint a, FPMPoint b) FPM_CONST;

FPM_INLINE FPMRect FPMMakeRectWithPointsC(FPMCoordinate x1, FPMCoordinate y1, FPMCoordinate x2, FPMCoordinate y2) FPM_CONST;
FPM_INLINE FPMRect FPMMakeRectWithPointsC(FPMCoordinate x1, FPMCoordinate y1, FPMCoordinate x2, FPMCoordinate y2)
{
	return FPMMakeRectWithPoints(FPMMakePoint(x1, y1), FPMMakePoint(x2, y2));
}

// Make rectangle containing three points.
FPMRect FPMMakeRectWith3Points(FPMPoint a, FPMPoint b, FPMPoint c) FPM_CONST;

FPM_INLINE FPMRect FPMMakeRectWith3PointsC(FPMCoordinate x1, FPMCoordinate y1, FPMCoordinate x2, FPMCoordinate y2, FPMCoordinate x3, FPMCoordinate y3) FPM_CONST;
FPM_INLINE FPMRect FPMMakeRectWith3PointsC(FPMCoordinate x1, FPMCoordinate y1, FPMCoordinate x2, FPMCoordinate y2, FPMCoordinate x3, FPMCoordinate y3)
{
	return FPMMakeRectWith3Points(FPMMakePoint(x1, y1), FPMMakePoint(x2, y2), FPMMakePoint(x3, y3));
}

// Expand rect, if necessary, to include pt.
FPMRect FPMRectAddPoint(FPMRect rect, FPMPoint pt) FPM_CONST;

FPM_INLINE FPMRect FPMRectAddPointC(FPMRect rect, FPMCoordinate x, FPMCoordinate y) FPM_CONST;
FPM_INLINE FPMRect FPMRectAddPointC(FPMRect rect, FPMCoordinate x, FPMCoordinate y)
{
	return FPMRectAddPoint(rect, FPMMakePoint(x, y));
}

FPM_INLINE bool FPMRectsEqual(FPMRect a, FPMRect b) FPM_CONST;
FPM_INLINE bool FPMRectsEqual(FPMRect a, FPMRect b)
{
	return FPMPointsEqual(a.origin, b.origin) && FPMSizesEqual(a.size, b.size);
}

FPM_INLINE bool FPMRectEmpty(FPMRect rect) FPM_CONST FPM_FORCE_INLINE;
FPM_INLINE bool FPMRectEmpty(FPMRect rect)
{
	return FPMSizeEmpty(rect.size);
}

FPM_INLINE FPMDimension FPMRectArea(FPMRect rect) FPM_CONST FPM_FORCE_INLINE;
FPM_INLINE FPMDimension FPMRectArea(FPMRect rect)
{
	return FPMSizeArea(rect.size);
}

FPM_INLINE FPMCoordinate FPMRectRight(FPMRect rect) FPM_CONST;
FPM_INLINE FPMCoordinate FPMRectRight(FPMRect rect)
{
	return rect.origin.x + rect.size.width;
}

FPM_INLINE FPMCoordinate FPMRectBottom(FPMRect rect) FPM_CONST;
FPM_INLINE FPMCoordinate FPMRectBottom(FPMRect rect)
{
	return rect.origin.y + rect.size.height;
}

FPM_INLINE FPMPoint FPMRectBottomRight(FPMRect rect) FPM_CONST;
FPM_INLINE FPMPoint FPMRectBottomRight(FPMRect rect)
{
	return FPMMakePoint(FPMRectRight(rect), FPMRectBottom(rect));
}


// Clipping.
FPM_INLINE FPMPoint FPMClipPointToSize(FPMPoint pt, FPMSize size) FPM_CONST;
FPM_INLINE FPMPoint FPMClipPointToSize(FPMPoint pt, FPMSize size)
{
	pt.x = fminf(pt.x, size.width);
	pt.y = fminf(pt.y, size.height);
	return pt;
}

FPMPoint FPMClipPointToRect(FPMPoint pt, FPMRect clipRect) FPM_CONST;
FPMRect FPMClipRectToRect(FPMRect rect, FPMRect clipRect) FPM_CONST;


FPM_END_EXTERN_C
#endif	/* INCLUDED_FPMBasics_h */
