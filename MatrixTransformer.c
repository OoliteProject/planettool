/*
 *  MatrixTransformer.c
 *  planettool
 *
 *  Created by Jens Ayton on 2009-10-04.
 *  Copyright 2009 Jens Ayton. All rights reserved.
 *
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
