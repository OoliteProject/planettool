/*
 *  FPMPNG.h
 *  planettool
 *
 *  Created by Jens Ayton on 2009-09-30.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
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
	kFPMWritePNGDither			= kFPMQuantizeDither,	// If set, error-diffusion dither is used when converting to output format.
	kFPMWritePNGJitter			= kFPMQuantizeJitter,	// If set, jitter is used in dithering.
	kFPMWritePNG16BPC			= 0x00010000,			// If set, output will use sixteen bits per channel; if clear, eight bits per channel are used.
};
typedef uint32_t FPMWritePNGFlags;


/*	Callback type for PNG error handling. if isError is true, it's a libpng
	or FPMPNG error; if it's false, it's a libpng warning.
*/
typedef void FPMPNGErrorHandler(const char *message, bool isError);


/*	FPMCreateWithPNG()
	Load a PNG image from the specified path.
	DesiredGamma should usually be kFPMGammaLinear.
*/
FloatPixMapRef FPMCreateWithPNG(const char *path, FPMGammaFactor desiredGamma, FPMPNGErrorHandler errorHandler);

/*	FPMCreateWithPNGCustom()
	Load a PNG image using a callback to provide data. See libpng
	documentation for png_set_read_fn() for information about the parameters.
*/
FloatPixMapRef FPMCreateWithPNGCustom(png_voidp ioPtr, png_rw_ptr readDataFn, FPMGammaFactor desiredGamma, FPMPNGErrorHandler errorHandler);


/*	FPMWritePNG()
	Write an image to the specified path in PNG format.
	SourceGamma should usually be kFPMGammaLinear.
	FileGamma should generally be kFPMGammaSRGB for eight-bit files. (The PNG
	'sRGB' chunk will be used in that case, instead of 'gAMA'.) For sixteen-
	bit files, either kFPMGammaLinear or kFPMGammaSRGB are reasonable choices.
*/
bool FPMWritePNG(FloatPixMapRef pm, const char *path, FPMWritePNGFlags options, FPMGammaFactor sourceGamma, FPMGammaFactor fileGamma, FPMPNGErrorHandler errorHandler);

bool FPMWritePNGCustom(FloatPixMapRef pm, png_voidp ioPtr, png_rw_ptr writeDataFn, png_flush_ptr flushDataFn, FPMWritePNGFlags options, FPMGammaFactor sourceGamma, FPMGammaFactor fileGamma, FPMPNGErrorHandler errorHandler);


/*	FPMWritePNGSimple()
	Call FPMWritePNG() with the most common options: eight-bit data, dithering,
	linear source gamma, sRGB file gamma.
*/
FPM_INLINE bool FPMWritePNGSimple(FloatPixMapRef pm, const char *path)  { return FPMWritePNG(pm, path, kFPMWritePNGDither, kFPMGammaLinear, kFPMGammaSRGB, NULL); }


FPM_END_EXTERN_C
#endif	/* INCLUDED_FPMPNG_h */
