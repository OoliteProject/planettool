/*
	MatrixTransformer.c
	planettool
	
	
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

#include "MatrixTransformer.h"


typedef struct
{
	SphericalPixelSourceFunction			source;
	SphericalPixelSourceDestructorFunction	sourceDestructor;
	void									*sourceContext;
	OOMatrix								transform;
} MatrixTransformerContext;


bool MatrixTransformerSetUp(SphericalPixelSourceFunction source, SphericalPixelSourceDestructorFunction sourceDestructor, void *sourceContext, OOMatrix transform, void **context)
{
	assert(source != NULL && context != NULL);
	
	MatrixTransformerContext *cx = malloc(sizeof (MatrixTransformerContext));
	if (cx == NULL)  return false;
	
	cx->source = source;
	cx->sourceDestructor = sourceDestructor;
	cx->sourceContext = sourceContext;
	cx->transform = transform;
	
	*context = cx;
	return true;
}


void MatrixTransformerDestructor(void *context)
{
	MatrixTransformerContext *cx = context;
	if (cx != NULL)
	{
		if (cx->sourceDestructor)
		{
			cx->sourceDestructor(cx->sourceContext);
		}
		
		free(cx);
	}
}


FPMColor MatrixTransformer(Coordinates where, RenderFlags flags, void *context)
{
	MatrixTransformerContext *cx = context;
	
	Vector v = CoordsGetVector(where);
	v = OOVectorMultiplyMatrix(v, cx->transform);
	where = MakeCoordsVector(v);
	
	return cx->source(where, flags, cx->sourceContext);
}
