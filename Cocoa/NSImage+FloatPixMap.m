/*
	NSImage+FloatPixMap.m
	Planet Tool
	
	
	Copyright © 2010 Jens Ayton

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

#import "NSImage+FloatPixMap.h"
#import "FPMPNG.h"


static void WriteToData(png_structp png, png_bytep bytes, png_size_t size);


@implementation NSImage (FloatPixMap)

+ (id) fpm_imageWithFloatPixMap:(FloatPixMapRef)fpm sourceGamma:(FPMGammaFactor)sourceGamma
{
	if (fpm == NULL)  return nil;
	
	NSMutableData *data = [[NSMutableData alloc] init];
	NSImage *result = nil;
	
	if (FPMWritePNGCustom(fpm, data, WriteToData, NULL, kFPMWritePNGDither, sourceGamma, kFPMGammaSRGB, NULL, NULL, NULL))
	{
		result = [[[NSImage alloc] initWithData:data] autorelease];
	}
	
	[data release];
	return result;
}


static void WriteToData(png_structp png, png_bytep bytes, png_size_t size)
{
	NSMutableData *data = png_get_io_ptr(png);
	[data appendBytes:bytes length:size];
}

@end
