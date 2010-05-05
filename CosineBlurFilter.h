/*
	CosineBlurFilter.h
	planettool
	
	Pseudo-source which applies a cosine-profile blur to the image (that is,
	a blur which weighs every input pixel by the dot product of its vector and
	the output pixel's vector, or zero, whichever is highest).
	
	Since this notionally samples every single input pixel for every output
	pixel, it's rather slow, even by planettool standards.
	
	"Every pixel" is defined in terms of the size parameter, and the
	assumption that we're transforming from a cube map to a cube map of the
	same size. Said size should generally be quite small.
	
	The alpha channel of the input is used to weight individual pixels. 0 in
	the alpha channel corresponds to a weight of 1.0, while 1 in the alpha
	channel corresponds to the maxWeight parameter.
	
	
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

#include "SphericalPixelSource.h"


bool CosineBlurFilterSetUp(SphericalPixelSourceFunction source, SphericalPixelSourceDestructorFunction sourceDestructor, void *sourceContext, FPMDimension size, float unmaskedScale, float maskedScale, void **context);
void CosineBlurFilterDestructor(void *context);

FPMColor CosineBlurFilter(Coordinates where, RenderFlags flags, void *context);
