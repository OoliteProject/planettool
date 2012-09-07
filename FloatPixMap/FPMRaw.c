/*
	FPMRaw.c
	FloatPixMap
	
	
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

#include "FPMRaw.h"
#include "FPMImageOperations.h"


bool FPMWriteRaw(FloatPixMapRef pm, const char *path, float max)
{
	if (pm == NULL || path == NULL)  return false;
	
	FILE *file = fopen(path, "wb");
	if (file == NULL)  return false;
	
	FPM_FOR_EACH_PIXEL(pm, true)
		FPMColor px = FPMClampColorRange(*pixel, 0, max);
		uint8_t bytes[4] = {
			px.a * 255.0f / max,
			px.r * 255.0f / max,
			px.g * 255.0f / max,
			px.b * 255.0f / max
		};
		fwrite(&bytes, 1, 4, file);
	FPM_END_FOR_EACH_PIXEL
	
	fclose(file);
	return true;
}
