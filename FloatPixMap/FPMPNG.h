/*
	FPMPNG.h
	FloatPixMap
	
	PNG input/output support for FloatPixMap.
	
	
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


#ifndef INCLUDED_FPMPNG_h
#define INCLUDED_FPMPNG_h

#include "FloatPixMap.h"
#include "FPMGamma.h"
#include "FPMQuantize.h"
#include "png.h"

FPM_BEGIN_EXTERN_C


enum
{
	kFPMWritePNGDither			= kFPMQuantizeDither,	// If set, error-diffusion dither is used when converting to output format. [UNIMPLEMENTED]
	kFPMWritePNGJitter			= kFPMQuantizeJitter,	// If set, jitter is used in dithering.
	kFPMWritePNG16BPC			= 0x00010000,			// If set, output will use sixteen bits per channel; if clear, eight bits per channel are used.
};
typedef uint32_t FPMWritePNGFlags;


/*	Callback type for PNG error handling. if isError is true, it's a libpng
	or FPMPNG error; if it's false, it's a libpng warning.
*/
typedef void FPMPNGErrorHandler(const char *message, bool isError, void *callbackContext);


/*	Callback type for PNG read/write progress.
*/
typedef void FPMPNGProgressHandler(float proportion, void *callbackContext);


/*	FPMCreateWithPNG()
	Load a PNG image from the specified path.
	DesiredGamma should usually be kFPMGammaLinear.
*/
FloatPixMapRef FPMCreateWithPNG(const char *path, FPMGammaFactor desiredGamma, FPMPNGErrorHandler errorHandler, FPMPNGProgressHandler progressHandler, void *callbackContext);

/*	FPMCreateWithPNGCustom()
	Load a PNG image using a callback to provide data. See libpng
	documentation for png_set_read_fn() for information about the parameters.
*/
FloatPixMapRef FPMCreateWithPNGCustom(png_voidp ioPtr, png_rw_ptr readDataFn, FPMGammaFactor desiredGamma, FPMPNGErrorHandler errorHandler, FPMPNGProgressHandler progressHandler, void *callbackContext);


/*	FPMWritePNG()
	Write an image to the specified path in PNG format.
	SourceGamma should usually be kFPMGammaLinear.
	FileGamma should generally be kFPMGammaSRGB for eight-bit files. (The PNG
	'sRGB' chunk will be used in that case, instead of 'gAMA'.) For sixteen-
	bit files, either kFPMGammaLinear or kFPMGammaSRGB are reasonable choices.
*/
bool FPMWritePNG(FloatPixMapRef pm, const char *path, FPMWritePNGFlags options, FPMGammaFactor sourceGamma, FPMGammaFactor fileGamma, FPMPNGErrorHandler errorHandler, FPMPNGProgressHandler progressHandler, void *callbackContext);

bool FPMWritePNGCustom(FloatPixMapRef pm, png_voidp ioPtr, png_rw_ptr writeDataFn, png_flush_ptr flushDataFn, FPMWritePNGFlags options, FPMGammaFactor sourceGamma, FPMGammaFactor fileGamma, FPMPNGErrorHandler errorHandler, FPMPNGProgressHandler progressHandler, void *callbackContext);


/*	FPMWritePNGSimple()
	Call FPMWritePNG() with the most common options: eight-bit data, dithering,
	linear source gamma, sRGB file gamma.
*/
FPM_INLINE bool FPMWritePNGSimple(FloatPixMapRef pm, const char *path)  { return FPMWritePNG(pm, path, kFPMWritePNGDither, kFPMGammaLinear, kFPMGammaSRGB, NULL, NULL, NULL); }


FPM_END_EXTERN_C
#endif	/* INCLUDED_FPMPNG_h */
