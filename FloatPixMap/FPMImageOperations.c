#include "FPMImageOperations.h"
#include "FPMVector.h"
#include <assert.h>


void FPMForEachPixelF(FloatPixMapRef pm, FPMForEachPixelFunc callback, void *info)
{
	FPM_FOR_EACH_PIXEL(pm, true)
		callback(pm, pixel, FPMMakePoint(x, y), info);
	FPM_END_FOR_EACH_PIXEL
}


void FPMForEachPixel(FloatPixMapRef pm, void(^block)(FPMColor *pixel, FPMPoint coords))
{
	FPM_FOR_EACH_PIXEL(pm, true)
		block(pixel, FPMMakePoint(x, y));
	FPM_END_FOR_EACH_PIXEL
}


void FPMSaturate(FloatPixMapRef pm, bool saturateAlpha)
{
	if (saturateAlpha)
	{
		FPM_FOR_EACH_PIXEL(pm, true)
			*pixel = FPMClampColor(*pixel);
		FPM_END_FOR_EACH_PIXEL
	}
	else
	{
		FPM_FOR_EACH_PIXEL(pm, true)
			*pixel = FPMClampColorNotAlpha(*pixel);
		FPM_END_FOR_EACH_PIXEL
	}
}


#if FPM_USE_ALTIVEC

void FPMScaleValues(FloatPixMapRef pm, FPMColor scale, FPMColor bias)
{
	FPMSize size = FPMGetSize(pm);
	
	if (pm != NULL && !FPMSizeEmpty(size))
	{
		FPMVFloat *px = (FPMVFloat *)FPMGetBufferPointer(pm);
		assert(px != NULL);
		size_t xCount = size.width;
		
		// Start a prefetch stream for first row as soon as possible.
		uint32_t controlWidth = xCount / 32 + 1;
		if (controlWidth >= 256)  controlWidth = 0;
		uint32_t controlWord = (controlWidth << 16) | 16;
		vec_dststt(px, controlWord, 0);
		
		size_t x, y, yCount = size.height;
		size_t rowCount = FPMGetRowPixelCount(pm);
		size_t rowOffset = rowCount - xCount;
		
		FPMVFloat scalev = FPMVectorFromColor(scale);
		FPMVFloat biasv = FPMVectorFromColor(bias);
		
		for (y = 0; y < yCount; y++)
		{
			// Prefetch next row.
			vec_dststt(px + rowCount, controlWord, 1);
			
			for (x = 0; x < xCount; x++)
			{
				*px = vec_madd(*px, scalev, biasv);
				px++;
			}
			px += rowOffset;
		}
		
		// Stop prefetch.
		vec_dss(1);
	}
}

#elif FPM_USE_SSE && 0

void FPMScaleValues(FloatPixMapRef pm, FPMColor scale, FPMColor bias)
{
	FPMSize size = FPMGetSize(pm);
	
	if (pm != NULL && !FPMSizeEmpty(size))
	{
		FPMVFloat *px = (FPMVFloat *)FPMGetBufferPointer(pm);
		assert(px != NULL);
		
		size_t x, y, xCount = size.width, yCount = size.height;
		size_t rowOffset = FPMGetRowPixelCount(pm) - xCount;
		
		FPMVFloat scalev = FPMVectorFromColor(scale);
		FPMVFloat biasv = FPMVectorFromColor(bias);
		
		for (y = 0; y < yCount; y++)
		{
			for (x = 0; x < xCount; x++)
			{
				FPMVFloat val = _mm_mul_ps(scalev, *px);
				*px = _mm_add_ps(val, biasv);
				
				++px;
			}
			px += rowOffset;
		}
	}
}

#else

void FPMScaleValues(FloatPixMapRef pm, FPMColor scale, FPMColor bias)
{
	FPM_FOR_EACH_PIXEL(pm, true)
		pixel->r = pixel->r * scale.r + bias.r;
		pixel->g = pixel->g * scale.g + bias.g;
		pixel->b = pixel->b * scale.b + bias.b;
		pixel->a = pixel->a * scale.a + bias.a;
	FPM_END_FOR_EACH_PIXEL
}

#endif


static void FindOneExtreme(float value, float *min, float *max)
{
	if (isfinite(value) && !isnan(value))
	{
		*min = fminf(*min, value);
		*max = fmaxf(*max, value);
	}
}

void FPMFindExtremes(FloatPixMapRef pm, FPMColor *outMin, FPMColor *outMax)
{
	if (pm != NULL && outMin != NULL && outMax != NULL && FPMGetArea(pm) != 0)
	{
		FPMColor min = { INFINITY, INFINITY, INFINITY, INFINITY };
		FPMColor max = { -INFINITY, -INFINITY, -INFINITY, -INFINITY };
		
		FPM_FOR_EACH_PIXEL(pm, false)
			FindOneExtreme(pixel->r, &min.r, &max.r);
			FindOneExtreme(pixel->g, &min.g, &max.g);
			FindOneExtreme(pixel->b, &min.b, &max.b);
			FindOneExtreme(pixel->a, &min.a, &max.a);
		FPM_END_FOR_EACH_PIXEL
		
		if (outMin != NULL)  *outMin = min;
		if (outMax != NULL)  *outMax = max; 
	}
	else
	{
		if (outMin != NULL)  *outMin = kFPMColorInvalid;
		if (outMax != NULL)  *outMax = kFPMColorInvalid;
	}
}


void FPMNormalize(FloatPixMapRef pm, bool retainZero, bool perChannel, bool normalizeAlpha)
{
	FPMColor min, max, refMin, scale, bias;
	FPMFindExtremes(pm, &min, &max);
	
	if (!perChannel)
	{
		max.r = fmaxf(max.r, fmaxf(max.g, max.b));
		max.b = max.g = max.r;
		min.r = fminf(min.r, fminf(min.g, min.b));
		min.b = min.g = min.r;
	}
	
	if (retainZero)  refMin = kFPMColorClear;
	else  refMin = min;
	
	scale.r = 1.0f / (max.r - refMin.r);
	scale.g = 1.0f / (max.g - refMin.g);
	scale.b = 1.0f / (max.b - refMin.b);
	scale.a = normalizeAlpha ? (1.0f / (max.a - refMin.a)) : 1.0;
	
	if (retainZero)
	{
		bias = kFPMColorClear;
	}
	else
	{
		bias.r = -min.r * scale.r;
		bias.g = -min.g * scale.g;
		bias.b = -min.b * scale.b;
		bias.a = normalizeAlpha ? (-min.a * scale.a) : 0.0;
	}
	
	FPMScaleValues(pm, scale, bias);
	
	if (retainZero && (min.r < 0.0f || min.g < 0.0f || min.b < 0.0f || (normalizeAlpha && min.a < 0.0f)))
	{
		FPMSaturate(pm, normalizeAlpha);
	}
}


FPM_INLINE FPMCoordinate Wrap(FPMCoordinate coord, FPMCoordinate max, FPMWrapMode mode)
{
	// NOTE: max is FPMCoordinate rather than FMPDimension because it needs to be signed in various places.
	if (0 <= coord)
	{
		switch (mode)
		{
			case kFPMWrapClamp:  return (coord < max) ? coord : max - 1;
			case kFPMWrapRepeat:  return coord % max;
			case kFPMWrapMirror:
				max -= 1;
				coord = coord % (max * 2);
				if (coord >= max)  return (max * 2) - coord;
				return coord;
		}
	}
	
	switch (mode)
	{
		case kFPMWrapClamp:  return 0;
		case kFPMWrapRepeat:  return (coord % max + max) % max;
		case kFPMWrapMirror:
			max -= 1;
			coord = coord % (max * 2);
			if (coord < -max)  return coord - max * -2;
			return -coord;
	}
	
	assert(0);
}


FPM_INLINE FPMColor Sample(FPMColor *buffer, size_t rowCount, size_t x, size_t y)
{
	// No range checking here, that's Wrap's job.
	return buffer[x + y * rowCount];
}


FPMColor FPMSampleLinear(FloatPixMapRef pm, float x, float y, FPMWrapMode wrapx, FPMWrapMode wrapy)
{
	x -= 0.5;
	y -= 0.5;
	FPMColor *buffer;
	FPMDimension width, height;
	size_t rowCount;
	FPMGetIterationInformation(pm, &buffer, &width, &height, &rowCount);
	rowCount += width;	// Undo optimization not useful in this particular case.
	
	float flrx = floorf(x);
	float flry = floorf(y);
	
	FPMCoordinate lowx = flrx;
	FPMCoordinate lowy = flry;
	FPMCoordinate highx = lowx + 1;
	FPMCoordinate highy = lowy + 1;
	
	lowx = Wrap(lowx, width, wrapx);
	highx = Wrap(highx, width, wrapy);
	lowy = Wrap(lowy, height, wrapy);
	highy = Wrap(highy, height, wrapy);
	
	FPMColor ll = Sample(buffer, rowCount, lowx, lowy);
	FPMColor lh = Sample(buffer, rowCount, lowx, highy);
	FPMColor hl = Sample(buffer, rowCount, highx, lowy);
	FPMColor hh = Sample(buffer, rowCount, highx, highy);
	
	float alphax = x - flrx;
	float alphay = y - flry;
	ll = FPMColorBlend(hl, ll, alphax);
	hl = FPMColorBlend(hh, lh, alphax);
	return FPMColorBlend(hl, ll, alphay);
}


FPM_INLINE float Cubic(float f)
{
	return f * f * (3.0f - 2.0f * f);
}


FPMColor FPMSampleCubic(FloatPixMapRef pm, float x, float y, FPMWrapMode wrapx, FPMWrapMode wrapy)
{
	x -= 0.5;
	y -= 0.5;
	FPMColor *buffer;
	FPMDimension width, height;
	size_t rowCount;
	FPMGetIterationInformation(pm, &buffer, &width, &height, &rowCount);
	rowCount += width;	// Undo optimization not useful in this particular case.
	
	float flrx = floorf(x);
	float flry = floorf(y);
	
	FPMCoordinate lowx = flrx;
	FPMCoordinate lowy = flry;
	FPMCoordinate highx = lowx + 1;
	FPMCoordinate highy = lowy + 1;
	
	lowx = Wrap(lowx, width, wrapx);
	highx = Wrap(highx, width, wrapy);
	lowy = Wrap(lowy, height, wrapy);
	highy = Wrap(highy, height, wrapy);
	
	FPMColor ll = Sample(buffer, rowCount, lowx, lowy);
	FPMColor lh = Sample(buffer, rowCount, lowx, highy);
	FPMColor hl = Sample(buffer, rowCount, highx, lowy);
	FPMColor hh = Sample(buffer, rowCount, highx, highy);
	
	float alphax = Cubic(x - flrx);
	float alphay = Cubic(y - flry);
	ll = FPMColorBlend(hl, ll, alphax);
	hl = FPMColorBlend(hh, lh, alphax);
	return FPMColorBlend(hl, ll, alphay);
}
