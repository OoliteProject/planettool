/*
	PlanetToolRenderer.h
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

#import <Foundation/Foundation.h>
#import "FloatPixMap.h"


typedef enum PlanetToolFormat
{
	kPlanetToolFormatLatLong,
	kPlanetToolFormatCube,
	kPlanetToolFormatCubeX,
	kPlanetToolFormatMercator,
	kPlanetToolFormatGallPeters
} PlanetToolFormat;


static inline BOOL IsValidPlanetToolInputFormat(PlanetToolFormat value)
{
	return value <= kPlanetToolFormatCubeX;
}


static inline BOOL IsValidPlanetToolOutputFormat(PlanetToolFormat value)
{
	return value <= kPlanetToolFormatGallPeters;
}


@protocol PlanetToolRendererDelegate;


@interface PlanetToolRenderer: NSObject
{
@private
	PlanetToolFormat				_inputFormat;
	PlanetToolFormat				_outputFormat;
	
	FloatPixMapRef					_sourcePixMap;
	BOOL							_isRendering;
	BOOL							_cancel;
	BOOL							_hadProgress;
	BOOL							_hadError;
	
#if !__OBJC2__
	size_t							_outputSize;
	BOOL							_flip;
	BOOL							_fast;
	BOOL							_jitter;
	float							_rotateX;
	float							_rotateY;
	float							_rotateZ;
	id <PlanetToolRendererDelegate>	_delegate;
#endif
}

@property PlanetToolFormat inputFormat;
@property PlanetToolFormat outputFormat;
@property size_t outputSize;
@property BOOL flip;
@property BOOL fast;
@property BOOL jitter;
@property float rotateX;
@property float rotateY;
@property float rotateZ;
@property id <PlanetToolRendererDelegate> delegate;
@property (readonly, getter=isRendering) BOOL rendering;

- (BOOL) asyncRenderFromImage:(FloatPixMapRef)inImage;
- (BOOL) asyncRenderFromGridGenerator;	// Ignores inputFormat.

- (void) cancelRendering;

@end


@protocol PlanetToolRendererDelegate <NSObject>
@required

- (void) planetToolRenderer:(PlanetToolRenderer *)renderer didCompleteImage:(FloatPixMapRef)image;
- (void) planetToolRenderer:(PlanetToolRenderer *)renderer failedWithMessage:(NSString *)message;

@optional

- (void) planetToolRendererCancelled:(PlanetToolRenderer *)renderer;

// IMPORTANT: this method will be called on a secondary thread.
// If it returns false, the renderer cancels.
- (BOOL) planetToolRenderer:(PlanetToolRenderer *)renderer progressUpdate:(float)progress;

@end
