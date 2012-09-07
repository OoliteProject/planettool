/*
	FPMPNG.c
	FloatPixMap
	
	
	Copyright © 2009–2012 Jens Ayton
 
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

#include "FPMPNG.h"
#include <assert.h>


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


static void PNGReadFile(png_structp png, png_bytep bytes, png_size_t size);
static void PNGWriteFile(png_structp png, png_bytep bytes, png_size_t size);
static void PNGError(png_structp png, png_const_charp message);
static void PNGWarning(png_structp png, png_const_charp message);
FloatPixMapRef ConvertPNGData(void *data, size_t width, size_t height, size_t rowBytes, uint8_t depth, uint8_t colorType);
static FloatPixMapRef ConvertPNGDataRGBA8(FloatPixMapRef pm, uint8_t *data, size_t width, size_t height, size_t rowBytes);
static FloatPixMapRef ConvertPNGDataRGBA16(FloatPixMapRef pm, uint8_t *data, size_t width, size_t height, size_t rowBytes);
static FloatPixMapRef ConvertPNGDataRGB16(FloatPixMapRef pm, uint8_t *data, size_t width, size_t height, size_t rowBytes);
typedef void (*RowTransformer)(void *data, size_t width);
static void TransformRow16(void *data, size_t width);
static void TransformRow8(void *data, size_t width);


typedef struct
{
	FPMPNGErrorHandler		*errorCB;
	void					*errorCBContext;
} ErrorInfo;


FloatPixMapRef FPMCreateWithPNG(const char *path, FPMGammaFactor desiredGamma, FPMPNGErrorHandler errorHandler, FPMPNGProgressHandler progressHandler, void *callbackContext)
{
	if (path != NULL)
	{
		// Attempt to open file.
		FILE *file = fopen(path, "rb");
		if (file == NULL)
		{
			if (errorHandler != NULL)  errorHandler("file not found.", true, callbackContext);
			return NULL;
		}
		
		png_byte bytes[8];
		if (fread(bytes, 8, 1, file) < 1)
		{
			if (errorHandler != NULL)  errorHandler("could not read file.", true, callbackContext);
			return NULL;
		}
		if (png_sig_cmp(bytes, 0, 8) != 0)
		{
			if (errorHandler != NULL)  errorHandler("not a PNG.", true, callbackContext);
			return NULL;
		}
		
		if (fseek(file, 0, SEEK_SET) != 0)
		{
			if (errorHandler != NULL)  errorHandler("could not read file.", true, callbackContext);
			return NULL;
		}
		
		FloatPixMapRef result = FPMCreateWithPNGCustom(file, PNGReadFile, desiredGamma, errorHandler, progressHandler, callbackContext);
		fclose(file);
		return result;
	}
	else
	{
		return NULL;
	}
}


FloatPixMapRef FPMCreateWithPNGCustom(png_voidp ioPtr, png_rw_ptr readDataFn, FPMGammaFactor desiredGamma, FPMPNGErrorHandler errorHandler, FPMPNGProgressHandler progressHandler, void *callbackContext)
{
	png_structp			png = NULL;
	png_infop			pngInfo = NULL;
	png_infop			pngEndInfo = NULL;
	FloatPixMapRef		result = NULL;
	png_uint_32			i, width, height, rowBytes;
	int					depth, colorType;
	void				*data = NULL;
	ErrorInfo			errInfo = { errorHandler, callbackContext };
	
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, &errInfo, PNGError, PNGWarning);
	if (png == NULL)  goto FAIL;
	
	pngInfo = png_create_info_struct(png);
	if (pngInfo == NULL)  goto FAIL;
	
	pngEndInfo = png_create_info_struct(png);
	if (pngEndInfo == NULL)  goto FAIL;
	
	if (setjmp(png_jmpbuf(png)))
	{
		// libpng will jump here on error.
		goto FAIL;
	}
	
	png_set_read_fn(png, ioPtr, readDataFn);
	
	png_read_info(png, pngInfo);
	if (!png_get_IHDR(png, pngInfo, &width, &height, &depth, &colorType, NULL, NULL, NULL))  goto FAIL;
	
#if FPM_LITTLE_ENDIAN
	if (depth == 16)  png_set_swap(png);
#endif
	if (depth <= 8 && !(colorType & PNG_COLOR_MASK_ALPHA))
	{
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	}
	png_set_gray_to_rgb(png);
	if (depth < 8)
	{
		png_set_packing(png);
		png_set_expand(png);
	}
	
	if (colorType & PNG_COLOR_MASK_PALETTE)
	{
		png_set_palette_to_rgb(png);
	}
	
	int j, passCount = png_set_interlace_handling(png);
	
	png_read_update_info(png, pngInfo);
	rowBytes = png_get_rowbytes(png, pngInfo);
	
	data = malloc(rowBytes * height);
	if (data == NULL)  goto FAIL;
	
	// Read image data.
	float progressNumerator = 0.0f, progressDenominator = passCount * height;
	for (j = 0; j < passCount; j++)
	{
		for (i = 0; i < height; i++)
		{
			png_read_row(png, (png_bytep)data + i * rowBytes, NULL);
			
			if (progressHandler)
			{
				progressNumerator++;
				progressHandler(progressNumerator / progressDenominator, callbackContext);
			}
		}
	}
	
	png_read_end(png, pngEndInfo);
	
	result = ConvertPNGData(data, width, height, rowBytes, png_get_bit_depth(png, pngInfo), png_get_color_type(png, pngInfo));
	
	if (result != NULL)
	{
		double invGamma = 1.0/kFPMGammaSRGB;
		png_get_gAMA(png, pngInfo, &invGamma);
		FPMApplyGamma(result, 1.0/invGamma, desiredGamma, png_get_bit_depth(png, pngInfo) == 16 ? 65536 : 256);
	}
	else
	{
		if (errorHandler != NULL)  errorHandler("Could not convert PNG data to native representation.", true, callbackContext);
	}

	
	png_destroy_read_struct(&png, &pngInfo, &pngEndInfo);
	free(data);
	
	return result;
	
FAIL:
	FPMRelease(&result);
	if (png != NULL)  png_destroy_read_struct(&png, &pngInfo, &pngEndInfo);
	free(data);
	
	return NULL;
}


bool FPMWritePNG(FloatPixMapRef pm, const char *path, FPMWritePNGFlags options, FPMGammaFactor sourceGamma, FPMGammaFactor fileGamma, FPMPNGErrorHandler errorHandler, FPMPNGProgressHandler progressHandler, void *callbackContext)
{
	if (pm != NULL && path != NULL)
	{
		// Attempt to open file.
		FILE *file = fopen(path, "wb");
		if (file == NULL)
		{
			if (errorHandler != NULL)  errorHandler("file not found.", true, callbackContext);
			return NULL;
		}
		
		bool result = FPMWritePNGCustom(pm, file, PNGWriteFile, NULL, options, sourceGamma, fileGamma, errorHandler, progressHandler, callbackContext);
		fclose(file);
		return result;
	}
	else
	{
		return false;
	}
}


bool FPMWritePNGCustom(FloatPixMapRef srcPM, png_voidp ioPtr, png_rw_ptr writeDataFn, png_flush_ptr flushDataFn, FPMWritePNGFlags options, FPMGammaFactor sourceGamma, FPMGammaFactor fileGamma, FPMPNGErrorHandler errorHandler, FPMPNGProgressHandler progressHandler, void *callbackContext)
{
	if (srcPM != NULL)
	{
		bool				success = false;
		png_structp			png = NULL;
		png_infop			pngInfo = NULL;
		FloatPixMapRef		pm = NULL;
		ErrorInfo			errInfo = { errorHandler, callbackContext };
		
		// Prepare data.
		FPMDimension width = FPMGetWidth(srcPM);
		FPMDimension height = FPMGetHeight(srcPM);
		if (width > UINT32_MAX || height > UINT32_MAX)
		{
			if (errorHandler != NULL)  errorHandler("image is too large for PNG format.", true, callbackContext);
			return false;
		}
		
		pm = FPMCopy(srcPM);
		if (pm == NULL)  return false;
		
		unsigned steps = (options & kFPMWritePNG16BPC) ? 0x10000 : 0x100;
		FPMApplyGamma(pm, sourceGamma, fileGamma, steps);
		FPMQuantize(pm, 0.0f, 1.0f, 0.0f, steps - 1, steps, (options & kFPMQuantizeDither & kFPMQuantizeJitter) | kFMPQuantizeClip | kFMPQuantizeAlpha);
		
		png = png_create_write_struct(PNG_LIBPNG_VER_STRING, &errInfo, PNGError, PNGWarning);
		if (png == NULL)  goto FAIL;
		
		pngInfo = png_create_info_struct(png);
		if (pngInfo == NULL)  goto FAIL;
		
		if (setjmp(png_jmpbuf(png)))
		{
			// libpng will jump here on error.
			goto FAIL;
		}
		
		png_set_write_fn(png, ioPtr, writeDataFn, flushDataFn);
		
		png_set_IHDR(png, pngInfo, width, height, (options & kFPMWritePNG16BPC) ? 16 : 8,PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		
		if (fileGamma == kFPMGammaSRGB)
		{
			png_set_sRGB_gAMA_and_cHRM(png, pngInfo, PNG_sRGB_INTENT_PERCEPTUAL);
		}
		else
		{
			png_set_gAMA(png, pngInfo, fileGamma);
		}
		
		/*	Select function used to transform a row of data to PNG-friendly
			format. NOTE: these work in place,and overwrite the data in pm,
			which is OK since we copied it.
		*/
		RowTransformer transformer = NULL;
		if (options & kFPMWritePNG16BPC)
		{
			transformer = TransformRow16;
		}
		else
		{
			transformer = TransformRow8;
		}
		
		png_write_info(png, pngInfo);
		
		size_t i;
		size_t rowOffset = FPMGetRowByteCount(pm);
		png_bytep row = (png_bytep)FPMGetBufferPointer(pm);
		for (i = 0; i < height; i++)
		{
			transformer(row, width);
			png_write_row(png, row);
			row += rowOffset;
			if (progressHandler)  progressHandler((float)(i + 1) / (float)height, callbackContext);
		}
		png_write_end(png, pngInfo);
		success = true;
		
	FAIL:
		if (png != NULL)  png_destroy_write_struct(&png, &pngInfo);
		FPMRelease(&pm);
		
		return success;
	}
	else
	{
		return false;
	}
}


static void PNGReadFile(png_structp png, png_bytep bytes, png_size_t size)
{
	FILE *file = png_get_io_ptr(png);
	if (fread(bytes, size, 1, file) < 1)
	{
		png_error(png, "read failed.");
	}
}


static void PNGWriteFile(png_structp png, png_bytep bytes, png_size_t size)
{
	FILE *file = png_get_io_ptr(png);
	if (fwrite(bytes, size, 1, file) < 1)
	{
		png_error(png, "write failed.");
	}
}


static void PNGError(png_structp png, png_const_charp message)
{
	ErrorInfo *errorInfo = png_get_error_ptr(png);
	if (errorInfo != NULL && errorInfo->errorCB != NULL)
	{
		errorInfo->errorCB(message, true, errorInfo->errorCBContext);
	}
}


static void PNGWarning(png_structp png, png_const_charp message)
{
	ErrorInfo *errorInfo = png_get_error_ptr(png);
	if (errorInfo != NULL && errorInfo->errorCB != NULL)
	{
		errorInfo->errorCB(message, false, errorInfo->errorCBContext);
	}
}


FloatPixMapRef ConvertPNGData(void *data, size_t width, size_t height, size_t rowBytes, uint8_t depth, uint8_t colorType)
{
	FloatPixMapRef result = FPMCreateC(width, height);
	if (result == NULL)  return NULL;
	
	// Libpng transformations should have given us one of these formats.
	if (depth == 8 && (colorType == PNG_COLOR_TYPE_RGB_ALPHA || colorType == PNG_COLOR_TYPE_RGB))
	{
		return ConvertPNGDataRGBA8(result, data, width, height, rowBytes);
	}
	if (depth == 16 && colorType == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		return ConvertPNGDataRGBA16(result, data, width, height, rowBytes);
	}
	if (depth == 16 && colorType == PNG_COLOR_TYPE_RGB)
	{
		return ConvertPNGDataRGB16(result, data, width, height, rowBytes);
	}
	
	fprintf(stderr, "Unexpected PNG depth/colorType combination: %u, 0x%X\n", depth, colorType);
	FPMRelease(&result);
	return NULL;
}


static FloatPixMapRef ConvertPNGDataRGBA8(FloatPixMapRef pm, uint8_t *data, size_t width, size_t height, size_t rowBytes)
{
	uint8_t				*src = NULL;
	float				*dst = NULL;
	size_t				x, y;
	
	for (y = 0; y < height; y++)
	{
		src = data + rowBytes * y;
		dst = (float *)FPMGetPixelPointerC(pm, 0, y);
		FPM_INTERNAL_ASSERT(src != NULL && dst != NULL);
		
		for (x = 0; x < width * 4; x++)
		{
			*dst++ = (float)*src++ * 1.0f/255.0f;
		}
	}
	
	return pm;
}


static FloatPixMapRef ConvertPNGDataRGBA16(FloatPixMapRef pm, uint8_t *data, size_t width, size_t height, size_t rowBytes)
{
	uint16_t			*src = NULL;
	float				*dst = NULL;
	size_t				x, y;
	
	for (y = 0; y < height; y++)
	{
		src = (uint16_t *)(data + rowBytes * y);
		dst = (float *)FPMGetPixelPointerC(pm, 0, y);
		FPM_INTERNAL_ASSERT(src != NULL && dst != NULL);
		
		for (x = 0; x < width * 4; x++)
		{
			*dst++ = (float)*src++ * 1.0f/65535.0f;
		}
	}
	
	return pm;
}


static FloatPixMapRef ConvertPNGDataRGB16(FloatPixMapRef pm, uint8_t *data, size_t width, size_t height, size_t rowBytes)
{
	uint16_t			*src = NULL;
	float				*dst = NULL;
	size_t				x, y;
	
	for (y = 0; y < height; y++)
	{
		src = (uint16_t *)(data + rowBytes * y);
		dst = (float *)FPMGetPixelPointerC(pm, 0, y);
		FPM_INTERNAL_ASSERT(src != NULL && dst != NULL);
		
		for (x = 0; x < width * 3; x++)
		{
			*dst++ = (float)*src++ * 1.0f/65535.0f;
		}
	}
	
	return pm;
}


static void TransformRow16(void *data, size_t width)
{
	assert(data != NULL);
	
	float *src = (float *)data;
#if FPM_LITTLE_ENDIAN
	uint8_t *dst = (uint8_t *)data;
#elif FPM_BIG_ENDIAN
	uint16_t *dst = (uint16_t *)data;
#endif
	
	size_t count = width * 4;
	while (count--)
	{
		// value should already be scaled to appropriate range.
		uint16_t value = *src++;
		
#if FPM_LITTLE_ENDIAN
		*dst++ = value >> 8;
		*dst++ = value & 0xFF;
#elif FPM_BIG_ENDIAN
		*dst++ = value;
#endif
	}
}


static void TransformRow8(void *data, size_t width)
{
	assert(data != NULL);
	
	float *src = (float *)data;
	uint8_t *dst = (uint8_t *)data;
	
	size_t count = width * 4;
	while (count--)
	{
		// value should already be scaled to appropriate range.
		uint8_t value = *src++;
		*dst++ = value;
	}
}
