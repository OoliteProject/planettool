/*
	CosineBlurFilter.c
	planettool
	
	
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

#include "CosineBlurFilter.h"


typedef struct
{
	SphericalPixelSourceFunction			source;
	SphericalPixelSourceDestructorFunction	sourceDestructor;
	void									*sourceContext;
	FPMDimension							size;
	float									scaleBias;
	float									scaleOffset;
} CosineBlurFilterContext;


bool CosineBlurFilterSetUp(SphericalPixelSourceFunction source, SphericalPixelSourceDestructorFunction sourceDestructor, void *sourceContext, FPMDimension size, float unmaskedScale, float maskedScale, void **context)
{
	assert(source != NULL && context != NULL);
	
	CosineBlurFilterContext *cx = malloc(sizeof (CosineBlurFilterContext));
	if (cx == NULL)  return false;
	
	cx->source = source;
	cx->sourceDestructor = sourceDestructor;
	cx->sourceContext = sourceContext;
	cx->size = size;
	cx->scaleBias = unmaskedScale;
	cx->scaleOffset = maskedScale - unmaskedScale;
	
	*context = cx;
	return true;
}


void CosineBlurFilterDestructor(void *context)
{
	CosineBlurFilterContext *cx = context;
	if (cx != NULL)
	{
		if (cx->sourceDestructor)
		{
			cx->sourceDestructor(cx->sourceContext);
		}
		
		free(cx);
	}
}


static bool IsValidColor(FPMColor c) PURE_FUNC;
static bool IsValidColor(FPMColor c)
{
	return isfinite(c.r) && isfinite(c.g) && isfinite(c.b) && isfinite(c.a);
}


static void SampleFace(Vector outV, Vector xv, Vector yv, Vector zv, CosineBlurFilterContext *context, RenderFlags flags, FPMColor *colorAccum, float *weightAccum)
{
	assert(colorAccum != NULL && weightAccum != NULL);
	
	FPMDimension size = context->size;
	float scaleBias = context->scaleBias;
	float scaleOffset = context->scaleOffset;
	
	float incr = 2.0f / size;
	
	FPMDimension x, y;
	for (y = 0; y < size; y++)
	{
		float fy = (float)y * incr - 1.0f;
		float fx = -1.0f;
		
		for (x = 0; x < size; x++, fx += incr)
		{
			Vector v = vector_add(vector_multiply_scalar(xv, fx), vector_add(vector_multiply_scalar(yv, fy), zv));
			v = vector_normal(v);
			float weight = dot_product(v, outV);
			if (weight <= 0.0f)  continue;
			
			FPMColor color = context->source(MakeCoordsVector(v), flags, context->sourceContext);
			if (!IsValidColor(color))
			{
				continue;
			}
			float localWeight = (scaleBias + color.a * scaleOffset);
			color = FPMColorMultiply(color, weight * localWeight);
			
			*colorAccum = FPMColorAdd(*colorAccum, color);
			*weightAccum += weight;
		}
	}
}


FPMColor CosineBlurFilter(Coordinates where, RenderFlags flags, void *context)
{
	Vector outV = CoordsGetVector(where);
	FPMColor colorAccum = kFPMColorClear;
	float weightAccum = 0.0f;
	
	SampleFace(outV, make_vector(0, 0, 1), make_vector(0, 1, 0), make_vector( 1, 0, 0), context, flags, &colorAccum, &weightAccum);
	SampleFace(outV, make_vector(0, 0, 1), make_vector(0, 1, 0), make_vector(-1, 0, 0), context, flags, &colorAccum, &weightAccum);
	SampleFace(outV, make_vector(1, 0, 0), make_vector(0, 0, 1), make_vector(0,  1, 0), context, flags, &colorAccum, &weightAccum);
	SampleFace(outV, make_vector(1, 0, 0), make_vector(0, 0, 1), make_vector(0, -1, 0), context, flags, &colorAccum, &weightAccum);
	SampleFace(outV, make_vector(0, 1, 0), make_vector(1, 0, 0), make_vector(0, 0,  1), context, flags, &colorAccum, &weightAccum);
	SampleFace(outV, make_vector(0, 1, 0), make_vector(1, 0, 0), make_vector(0, 0, -1), context, flags, &colorAccum, &weightAccum);
	
	weightAccum = 1.0f / weightAccum;
	colorAccum.r *= weightAccum;
	colorAccum.g *= weightAccum;
	colorAccum.b *= weightAccum;
	colorAccum.a = 1.0f;
	
	return colorAccum;
}
