/*	MatrixTransformer.h
	
	Pseudo-source which reads from another source after transforming
	coordinates by multiplying with a matrix.
	
	Works like a normal source, except it needs manual setup.
*/

#include "SphericalPixelSource.h"


bool MatrixTransformerSetUp(SphericalPixelSourceFunction source, SphericalPixelSourceDestructorFunction sourceDestructor, void *sourceContext, OOMatrix transform, void **context);
void MatrixTransformerDestructor(void *context);

FPMColor MatrixTransformer(Coordinates where, RenderFlags flags, void *context);
