/*
	PlanetToolRenderer.m
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

#import "PlanetToolRenderer.h"

// Business logiccy stuff.
#include "SphericalPixelSource.h"

// Sources
#include "LatLongGridGenerator.h"
#include "ReadLatLong.h"
#include "ReadCube.h"
#include "MatrixTransformer.h"

// Sinks
#include "RenderToLatLong.h"
#include "RenderToCube.h"
#include "RenderToMercator.h"
#include "RenderToGallPeters.h"


@interface PlanetToolRenderer ()

- (void) failRenderWithMessage:(NSString *)message;
- (void) failRenderWithMessageAndFormat:(NSString *)message, ...;
- (BOOL) updateProgress:(float)value;

- (void) performRenderWithSourceConstructor:(SphericalPixelSourceConstructorFunction)sourceConstructor
								 destructor:(SphericalPixelSourceDestructorFunction)sourceDestructor;

- (OOMatrix) transform;

@end


static bool ProgressCB(size_t numerator, size_t denominator, void *context);
static void ErrorCB(const char *message, void *context);


@implementation PlanetToolRenderer

@synthesize outputSize = _outputSize;
@synthesize flip = _flip;
@synthesize fast = _fast;
@synthesize jitter = _jitter;
@synthesize rotateX = _rotateX;
@synthesize rotateY = _rotateY;
@synthesize rotateZ = _rotateZ;
@synthesize delegate = _delegate;


- (BOOL) isRendering
{
	return _isRendering;
}


- (void) finalize
{
	FPMRelease(&_sourcePixMap);
	
	[super finalize];
}


- (PlanetToolFormat) inputFormat
{
	return _inputFormat;
}


- (void) setInputFormat:(PlanetToolFormat)value
{
	if (IsValidPlanetToolInputFormat(value))
	{
		_inputFormat = value;
	}
}


- (PlanetToolFormat) outputFormat
{
	return _outputFormat;
}


- (void) setOutputFormat:(PlanetToolFormat)value
{
	if (IsValidPlanetToolOutputFormat(value))
	{
		_outputFormat = value;
	}
}


- (BOOL) asyncRenderFromImage:(FloatPixMapRef)inImage
{
	if (_isRendering)  return NO;
	if (self.delegate == nil)  return NO;
	
	_sourcePixMap = FPMRetain(inImage);
	_isRendering = YES;
	
	[NSThread detachNewThreadSelector:@selector(asyncRenderImage) toTarget:self withObject:nil];
	return YES;
}


- (BOOL) asyncRenderFromGridGenerator
{
	if (_isRendering)  return NO;
	if (self.delegate == nil)  return NO;
	
	_isRendering = YES;
	
	[NSThread detachNewThreadSelector:@selector(asyncRenderGenerator) toTarget:self withObject:nil];
	return YES;
}


- (void) cancelRendering
{
	_cancel = YES;
	if ([self.delegate respondsToSelector:@selector(planetToolRendererCancelled:)])
	{
		[self.delegate planetToolRendererCancelled:self];
	}
}


- (void) asyncRenderImage
{
	SphericalPixelSourceConstructorFunction		sourceConstructor = NULL;
	SphericalPixelSourceDestructorFunction		sourceDestructor = NULL;
	
	switch (self.inputFormat)
	{
		case kPlanetToolFormatLatLong:
			sourceConstructor = ReadLatLongConstructor;
			sourceDestructor = ReadLatLongDestructor;
			break;
			
		case kPlanetToolFormatCube:
			sourceConstructor = ReadCubeConstructor;
			sourceDestructor = ReadCubeDestructor;
			break;
			
		case kPlanetToolFormatCubeX:
			sourceConstructor = ReadCubeCrossConstructor;
			sourceDestructor = ReadCubeDestructor;
			break;
			
		case kPlanetToolFormatMercator:
		case kPlanetToolFormatGallPeters:
			break;
	}
	
	if (sourceConstructor == NULL)
	{
		[self failRenderWithMessageAndFormat:NSLocalizedString(@"Internal error: unknown input format %u.", NULL), self.inputFormat];
		return;
	}
	
	[self performRenderWithSourceConstructor:sourceConstructor destructor:sourceDestructor];
}


- (void) asyncRenderGenerator
{
	[self performRenderWithSourceConstructor:LatLongGridGeneratorConstructor destructor:NULL];
}


- (void) performRenderWithSourceConstructor:(SphericalPixelSourceConstructorFunction)sourceConstructor destructor:(SphericalPixelSourceDestructorFunction)sourceDestructor
{
	SphericalPixelSinkFunction					sink = NULL;
	SphericalPixelSourceFunction				source = NULL;
	void										*sourceContext = NULL;
	RenderFlags									flags = 0;
	
	switch (self.outputFormat)
	{
		case kPlanetToolFormatLatLong:
			sink = RenderToLatLong;
			break;
			
		case kPlanetToolFormatCube:
			sink = RenderToCube;
			break;
			
		case kPlanetToolFormatCubeX:
			sink = RenderToCubeCross;
			break;
			
		case kPlanetToolFormatMercator:
			sink = RenderToMercator;
			break;
			
		case kPlanetToolFormatGallPeters:
			sink = RenderToGallPeters;
			break;
	}
	
	if (sink == NULL)
	{
		[self failRenderWithMessageAndFormat:NSLocalizedString(@"Internal error: unknown output format %u.", NULL), self.inputFormat];
		FPMRelease(&_sourcePixMap);
		return;
	}
	
	if (self.fast)  flags |= kRenderFast;
	if (self.jitter)  flags |= kRenderJitter;
	
	if (!sourceConstructor(_sourcePixMap, flags, &source, &sourceContext))
	{
		[self failRenderWithMessage:NSLocalizedString(@"Internal error: failed to set up render source.", NULL)];
		FPMRelease(&_sourcePixMap);
		return;
	}
	
	OOMatrix transform = [self transform];
	if (!OOMatrixIsIdentity(transform))
	{
		void *transformContext = NULL;
		if (!MatrixTransformerSetUp(source, sourceDestructor, sourceContext, transform, &transformContext))
		{
			[self failRenderWithMessage:NSLocalizedString(@"Internal error: failed to set up transformation matrix.", NULL)];
			FPMRelease(&_sourcePixMap);
			return;
		}
		
		source = MatrixTransformer;
		sourceDestructor = MatrixTransformerDestructor;
		sourceContext = transformContext;
	}
	
	_hadProgress = NO;
	_hadError = NO;
	_cancel = NO;
	
	FloatPixMapRef result = sink(self.outputSize, flags, source, sourceContext, ProgressCB, ErrorCB, self);
	if (sourceDestructor != NULL)  sourceDestructor(sourceContext);
	
	FPMRelease(&_sourcePixMap);
	_isRendering = NO;
	
	if (result != NULL)
	{
		// Send result to client, waiting for completion so client can take ownership.
		[self performSelectorOnMainThread:@selector(sendCompletedPixMap:)
							   withObject:[NSValue valueWithPointer:result]
							waitUntilDone:YES];
	}
	else if (!_hadProgress && !_hadError)
	{
		[self failRenderWithMessage:NSLocalizedString(@"Internal error: renderer failed.", NULL)];
	}
	FPMRelease(&result);
}


- (OOMatrix) transform
{
	OOMatrix transform = kIdentityMatrix;
	
	if (self.flip)  transform = OOMatrixScale(transform, -1, 1, 1);
	
	transform = OOMatrixRotateX(transform, self.rotateX * kDegToRad);
	transform = OOMatrixRotateZ(transform, self.rotateZ * kDegToRad);
	
	// Y axis (planetary axis) rotation is deliberately applied last. This makes it rotate around the *original* axis of rotation.
	transform = OOMatrixRotateY(transform, self.rotateY * kDegToRad);
	
	return transform;
}


- (void) failRenderWithMessage:(NSString *)message
{
	_hadError = YES;
	FPMRelease(&_sourcePixMap);
	
	[self performSelectorOnMainThread:@selector(sendFailNotification:) withObject:message waitUntilDone:NO];
}


- (void) failRenderWithMessageAndFormat:(NSString *)message, ...
{
	va_list args;
	va_start(args, message);
	message = [[NSString alloc] initWithFormat:message arguments:args];
	va_end(args);
	
	[self failRenderWithMessage:message];
}


- (void) sendCompletedPixMap:(NSValue *)value
{
	[self.delegate planetToolRenderer:self didCompleteImage:[value pointerValue]];
}


- (void) sendFailNotification:(NSString *)message
{
	[self.delegate planetToolRenderer:self failedWithMessage:message];
}


- (BOOL) updateProgress:(float)progress
{
	_hadProgress = YES;
	if ([self.delegate respondsToSelector:@selector(planetToolRenderer:progressUpdate:)])
	{
		if (![self.delegate planetToolRenderer:self progressUpdate:progress])  _cancel = YES;
	}
	return !_cancel;
}

@end


static bool ProgressCB(size_t numerator, size_t denominator, void *context)
{
	return [(PlanetToolRenderer *)context updateProgress:(float)numerator / (float)denominator];
}


static void ErrorCB(const char *message, void *context)
{
	NSString *messageNS = [NSString stringWithUTF8String:message];
	if (messageNS != NULL)  messageNS = [[NSString alloc] initWithCString:message encoding:NSISOLatin1StringEncoding];
	
	[(PlanetToolRenderer *)context failRenderWithMessage:messageNS];
}
