/*
	PlanetToolDocument.h
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

#import <Cocoa/Cocoa.h>
#import "PlanetToolRenderer.h"


@interface PlanetToolDocument: NSDocument
{
@private
	FloatPixMapRef					_sourcePixMap;
	PlanetToolFormat				_inputFormat;
	PlanetToolFormat				_outputFormat;
	NSTimeInterval					_lastProgressUpdate;
	
	PlanetToolRenderer				*_previewRenderer;
	PlanetToolRenderer				*_finalRenderer;
	
	NSInteger						_outputSize;
	NSString						*_outputPath;
	FloatPixMapRef					_outputImage;
	
#if !__OBJC2__	
	NSWindow						*_progressSheet;
	NSTextField						*_progressLabel;
	NSProgressIndicator				*_progressBar;
	NSButton						*_progressCancelButton;
	
	NSString						*_sourceDocumentName;
	BOOL							_flip;
	BOOL							_fast;
	BOOL							_jitter;
	BOOL							_isRenderingPreview;
	float							_rotateX;
	float							_rotateY;
	float							_rotateZ;
#endif
}

@property IBOutlet NSWindow *progressSheet;
@property IBOutlet NSTextField *progressLabel;
@property IBOutlet NSProgressIndicator *progressBar;
@property IBOutlet NSButton *progressCancelButton;

@property (readonly) NSString *sourceDocumentName;
@property NSUInteger inputFormat;
@property NSUInteger outputFormat;
@property NSInteger outputSize;
@property (readonly) NSString *outputSizeContext;
@property BOOL flip;
@property BOOL fast;
@property BOOL jitter;
@property float rotateX;
@property float rotateY;
@property float rotateZ;
@property (readonly) NSImage *previewImage;
@property (readonly) BOOL isRenderingPreview;

- (IBAction) performFinalRender:(id)sender;
- (IBAction) cancelLoading:(id)sender;

@end
