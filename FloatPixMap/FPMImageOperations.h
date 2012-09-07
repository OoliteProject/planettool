/*
	FPMImageOperations.h
	FloatPixMap
	
	Simple full-buffer manipulations.
	To work on a rectangular subregion of an image, use FPMCreateSub().
	
	
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


#ifndef INCLUDED_FPMImageOperations_h
#define INCLUDED_FPMImageOperations_h

#include "FloatPixMap.h"

FPM_BEGIN_EXTERN_C


/*	FPMForEachPixel()
	Call a block or callback function for each pixel in an image. A simple,
	albeit not hugely efficient, way to process images.
*/
typedef void (*FPMForEachPixelFunc)(FloatPixMapRef pm, FPMColor *pixel, FPMPoint coords, void *info);

void FPMForEachPixelF(FloatPixMapRef pm, FPMForEachPixelFunc callback, void *info);
#if __BLOCKS__
void FPMForEachPixel(FloatPixMapRef pm, void(^block)(FPMColor *pixel, FPMPoint coords));
#endif


/*	FPMSaturate()
	Clamp all values to [0..1].
*/
void FPMSaturate(FloatPixMapRef pm, bool saturateAlpha);

/*	FPMScaleValues()
	Multiply and add each component by specified values.
*/
void FPMScaleValues(FloatPixMapRef pm, FPMColor scale, FPMColor bias);

/*	FPMFindExtremes()
	Find highest and lowest values for each channel. Infinities and NaNs are ignored.
	Both outMax and outMin are optional.
*/
void FPMFindExtremes(FloatPixMapRef pm, FPMColor *outMin, FPMColor *outMax);

/*	FPMNormalize()
	Scale all values by most extreme.
	Uses same extremes for r, g, and b, but separate for a.
	
	If retainZero is true, zero is kept the same and any negative values are
	clamped. Otherwise, the lowest observed value (wether positive or negative)
	is used as the black point.
	
	If perChannel is true, normalizing is done separately for each channel. If
	it is false, the same scaling is applied to r, g and b.
	
	If normalizeAlpha is true, a will be normalized (separately from r, g and
	b). If it is false, a will be left alone.
*/
void FPMNormalize(FloatPixMapRef pm, bool retainZero, bool perChannel, bool normalizeAlpha);

/*	FPMFill()
	Fill all pixels with specified value.
*/
FPM_INLINE void FPMFill(FloatPixMapRef pm, FPMColor value)  { FPMScaleValues(pm, kFPMColorClear, value); }
FPM_INLINE void FPMFillV(FloatPixMapRef pm, FPMComponent r, FPMComponent g, FPMComponent b, FPMComponent a)  { FPMFill(pm, FPMMakeColor(r, g, b, a)); }


typedef enum
{
	kFPMWrapClamp,
	kFPMWrapRepeat,
	kFPMWrapMirror
} FPMWrapMode;

/*	FPMSampleLinear()
	Sample pixels at specified point, with bilinear interpolation. The wrap
	arguments specify what to do with out-of-range values.
*/
FPMColor FPMSampleLinear(FloatPixMapRef pm, float x, float y, FPMWrapMode wrapx, FPMWrapMode wrapy);
FPM_INLINE FPMColor FPMSampleLinearClamp(FloatPixMapRef pm, float x, float y)  { return FPMSampleLinear(pm, x, y, kFPMWrapClamp, kFPMWrapClamp); }

/*	FPMSampleCubicHermite()
	Sample pixels at specified point, with bicubic hermite interpolation. The
	wrap arguments specify what to do with out-of-range values.
*/
FPMColor FPMSampleCubicHermite(FloatPixMapRef pm, float x, float y, FPMWrapMode wrapx, FPMWrapMode wrapy);
FPM_INLINE FPMColor FPMSampleCubicHermiteClamp(FloatPixMapRef pm, float x, float y)  { return FPMSampleCubicHermite(pm, x, y, kFPMWrapClamp, kFPMWrapClamp); }


/*	FPM_FOR_EACH_PIXEL()
	FPM_END_FOR_EACH_PIXEL
	Like FPMForEachPixel(), but nasty and inline and nasty.
	
	pm_ is a pixmap. willWrite_ is a compile-time boolean specifying whether
	you intend to write to the data.
	
	Between FPM_FOR_EACH_PIXEL() and FPM_END_FOR_EACH_PIXEL, the following
	variables are available:
		FPMColor        *pixel;         // Points to the current pixel.
		FPMDimension    x, y;           // The location of the current pixel.
		FPMDimension    width, height;  // The dimension of the pixmap.
	
	Example:
		FPM_FOR_EACH_PIXEL(pixMap)
			if (x < width / 2)  *pixel = kFPMColorWhite;
			else  *pixel = kFPMColorBlack;
		FPM_END_FOR_EACH_PIXEL
*/
#define FPM_FOR_EACH_PIXEL(pm_, willWrite_)  do { \
	FPMColor *pixel; FPMDimension x, y; FPMDimension width, height; size_t rowOffset_; \
	FPMGetIterationInformation(pm_, &pixel, &width, &height, &rowOffset_); \
	for (y = 0; y < height; y++) { \
		FPM_GCC_PREFETCH(pixel, willWrite_, 1); \
		for (x = 0; x < width; x++) {
		
#define FPM_END_FOR_EACH_PIXEL ; ++pixel; } pixel += rowOffset_; }} while (0);


FPM_END_EXTERN_C
#endif	/* INCLUDED_FPMImageOperations_h */
