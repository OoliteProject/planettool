/*
	PlanetToolDocument.m
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


#import "PlanetToolDocument.h"
#import "FPMPNG.h"
#import "OOMaths.h"


@interface PlanetToolDocument () <PlanetToolRendererDelegate>

@property (readwrite) BOOL isRenderingPreview;

- (void) asyncLoadImage:(NSString *)path;

- (void) loadingComplete;
- (void) loadingFailedWithMessage:(NSString *)message;

- (void) showProgressSheetWithMessage:(NSString *)message cancelAction:(SEL)cancelAction;
- (void) updateProgress:(float)value;
- (void) removeProgressSheetWithSuccess:(BOOL)success;

@end


static void LoadErrorHandler(const char *message, bool isError, void *context);
static void LoadProgressHandler(float proportion, void *context);


@implementation PlanetToolDocument

@synthesize progressSheet = _progressSheet;
@synthesize progressLabel = _progressLabel;
@synthesize progressBar = _progressBar;
@synthesize progressCancelButton = _progressCancelButton;

@synthesize sourceDocumentName = _sourceDocumentName;
@synthesize outputSize = _outputSize;
@synthesize flip = _flip;
@synthesize fast = _fast;
@synthesize jitter = _jitter;
@synthesize rotateX = _rotateX;
@synthesize rotateY = _rotateY;
@synthesize rotateZ = _rotateZ;
@synthesize isRenderingPreview = _isRenderingPreview;


+ (void) initialize
{
	FPMInit();
}


- (id)initWithType:(NSString *)typeName error:(NSError **)outError
{
	return nil;
}


- (void) finalize
{
	FPMRelease(&_sourcePixMap);
	FPMRelease(&_outputImage);
	
	[super finalize];
}


- (NSString *)windowNibName
{
	return @"PlanetToolDocument";
}


- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{
	if (![absoluteURL isFileURL])  return NO;
	
	[NSThread detachNewThreadSelector:@selector(asyncLoadImage:) toTarget:self withObject:absoluteURL.path];
	
	return YES;
}


- (void) awakeFromNib
{
	// Defer this because -windowForSheet will otherwise try to reload the nib.
	[self performSelector:@selector(deferredAwakeFromNib) withObject:nil afterDelay:0.0f];
}


- (void) deferredAwakeFromNib
{
	if (_sourcePixMap == nil)
	{
		NSString *message = [NSString stringWithFormat:NSLocalizedString(@"Loading \"%@\"...", NULL), [[NSFileManager defaultManager] displayNameAtPath:self.displayName]];
		[self showProgressSheetWithMessage:message cancelAction:@selector(cancelLoading:)];
	}
}


- (void) showProgressSheetWithMessage:(NSString *)message cancelAction:(SEL)cancelAction
{
	self.progressLabel.stringValue = message;
	self.progressBar.usesThreadedAnimation = YES;
	[self.progressBar startAnimation:nil];
	self.progressBar.doubleValue = 0.0;
	self.progressCancelButton.action = cancelAction;
	self.progressCancelButton.target = self;
	
	[NSApp beginSheet:self.progressSheet modalForWindow:self.windowForSheet modalDelegate:self didEndSelector:NULL contextInfo:nil];
}


- (void) removeProgressSheetWithSuccess:(BOOL)success
{
	[NSApp endSheet:self.progressSheet returnCode:success ? NSOKButton : NSCancelButton];
	[self.progressSheet orderOut:nil];
}


- (void) updateProgressAsync:(NSNumber *)value
{
	float floatValue = [value floatValue];
	if (floatValue >= 1.0)
	{
		[self.progressBar setIndeterminate:YES];
		[self.progressBar startAnimation:nil];
	}
	else
	{
		[self.progressBar setIndeterminate:NO];
		self.progressBar.doubleValue = floatValue;
	}
}


- (void) updateProgress:(float)value
{
	NSTimeInterval now = [[NSDate date] timeIntervalSinceReferenceDate];
	if (now - _lastProgressUpdate >= 0.05 || value == 1.0)
	{
		[self performSelectorOnMainThread:@selector(updateProgressAsync:) withObject:[NSNumber numberWithDouble:value] waitUntilDone:NO];
		_lastProgressUpdate = now;
	}
}



#pragma mark -
#pragma mark Properties

- (NSUInteger) inputFormat
{
	return _inputFormat;
}


- (void) setInputFormat:(NSUInteger)value
{
	if (IsValidPlanetToolInputFormat(value))
	{
		_inputFormat = value;
	}
}


- (NSUInteger) outputFormat
{
	return _outputFormat;
}


- (void) setOutputFormat:(NSUInteger)value
{
	if (IsValidPlanetToolOutputFormat(value))
	{
		_outputFormat = value;
	}
}


- (void) setOutputSize:(NSInteger)size
{
	if (size > 0)
	{
		_outputSize = size;
	}
}


- (NSString *) outputSizeContext
{
	switch (self.outputFormat)
	{
		case kPlanetToolFormatLatLong:
			return NSLocalizedString(@"pixels high", NULL);
			
		case kPlanetToolFormatCube:
		case kPlanetToolFormatCubeX:
			return NSLocalizedString(@"pixels per edge", NULL);
			
		case kPlanetToolFormatMercator:
		case kPlanetToolFormatGallPeters:
			return NSLocalizedString(@"pixels wide", NULL);
	}
	
	return @"?";
}


+ (NSSet *) keyPathsForValuesAffectingOutputSizeContext
{
	return [NSSet setWithObject:@"outputFormat"];
}


- (NSImage *) previewImage
{
	return nil;
}


- (NSSet *) keyPathsForValuesAffectingPreviewImage
{
	return [NSSet setWithObjects:@"inputFormat", @"outputFormat", @"flip", @"rotateX", @"rotateY", @"rotateZ", nil];
}



#pragma mark -
#pragma mark Rendering

- (NSString *) suggestedRenderName
{
	NSString *name = [self displayName];
	if ([[name.pathExtension lowercaseString] isEqualToString:@"png"])
	{
		name = [name stringByDeletingPathExtension];
	}
	
	switch (self.outputFormat)
	{
		case kPlanetToolFormatLatLong:
			name = [name stringByAppendingString:@"-latlong"];
			break;
			
		case kPlanetToolFormatCube:
			name = [name stringByAppendingString:@"-cube"];
			break;
			
		case kPlanetToolFormatCubeX:
			name = [name stringByAppendingString:@"-cubex"];
			break;
			
		case kPlanetToolFormatMercator:
			name = [name stringByAppendingString:@"-mercator"];
			break;
			
		case kPlanetToolFormatGallPeters:
			name = [name stringByAppendingString:@"-gall-peters"];
			break;
	}
	
	return [name stringByAppendingPathExtension:@"png"];
}


- (IBAction) performFinalRender:(id)sender
{
	NSSavePanel *savePanel = [NSSavePanel savePanel];
	savePanel.requiredFileType = @"png";
	[savePanel beginSheetForDirectory:nil
								 file:[self suggestedRenderName]
					   modalForWindow:self.windowForSheet
						modalDelegate:self
					   didEndSelector:@selector(renderSaveSheetDidEnd:returnCode:contextInfo:)
						  contextInfo:nil];
}


- (void) renderSaveSheetDidEnd:(NSSavePanel *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
	[sheet orderOut:nil];
	
	if (returnCode != NSFileHandlingPanelOKButton)  return;
	
	_outputPath = sheet.filename;
	
	NSString *displayName = [sheet.filename lastPathComponent];
	if ([sheet isExtensionHidden])  displayName = [displayName stringByDeletingPathExtension];
	
	NSString *message = [NSString stringWithFormat:NSLocalizedString(@"Rendering \"%@\"...", NULL), displayName];
	[self showProgressSheetWithMessage:message cancelAction:@selector(cancelRender:)];
	
	_lastProgressUpdate = 0.0;
	
	_finalRenderer = [PlanetToolRenderer new];
	_finalRenderer.inputFormat = self.inputFormat;
	_finalRenderer.outputFormat = self.outputFormat;
	_finalRenderer.outputSize = self.outputSize;
	_finalRenderer.flip = self.flip;
	_finalRenderer.fast = self.fast;
	_finalRenderer.jitter = self.jitter;
	_finalRenderer.rotateX = self.rotateX;
	_finalRenderer.rotateY = self.rotateY;
	_finalRenderer.rotateZ = self.rotateZ;
	
	_finalRenderer.delegate = self;
	
	if (![_finalRenderer asyncRenderFromImage:_sourcePixMap])
	{
		[self planetToolRenderer:_finalRenderer failedWithMessage:NSLocalizedString(@"Unknown error.", NULL)];
	}
}


- (void) cancelRender:(id)sender
{
	[_finalRenderer cancelRendering];
}


- (void) planetToolRenderer:(PlanetToolRenderer *)renderer didCompleteImage:(FloatPixMapRef)image
{
	_outputImage = FPMRetain(image);
	[NSThread detachNewThreadSelector:@selector(writeOutputFile) toTarget:self withObject:nil];
}


static void WriteErrorHandler(const char *message, bool isError, void *context)
{
	if (isError)
	{
		NSString *string = [NSString stringWithUTF8String:message];
		if (string == nil)  string = [[NSString alloc] initWithCString:message encoding:NSISOLatin1StringEncoding];
		
		[(PlanetToolDocument *)context performSelectorOnMainThread:@selector(writingFailedWithMessage:)
														withObject:string
													 waitUntilDone:NO];
	}
}


- (void) writeOutputFile
{
	if (FPMWritePNG(_outputImage, [_outputPath fileSystemRepresentation], kFPMWritePNGDither, kFPMGammaLinear, kFPMGammaSRGB, WriteErrorHandler, NULL, self))
	{
		[self removeProgressSheetWithSuccess:YES];
	}
	else
	{
		[self removeProgressSheetWithSuccess:NO];
	}
	
	FPMRelease(&_outputImage);
}


- (void) writingFailedWithMessage:(NSString *)message
{
	// FIXME: need an alert here.
	[self removeProgressSheetWithSuccess:NO];
	[NSAlert alertWithMessageText:NSLocalizedString(@"The document could not be saved.", NULL)
					defaultButton:nil
				  alternateButton:nil
					  otherButton:nil
		informativeTextWithFormat:@"%@", message];
}


- (void) planetToolRenderer:(PlanetToolRenderer *)renderer failedWithMessage:(NSString *)message
{
	// FIXME: need an alert here.
	[self removeProgressSheetWithSuccess:NO];
	[NSAlert alertWithMessageText:NSLocalizedString(@"Rendering failed.", NULL)
					defaultButton:nil
				  alternateButton:nil
					  otherButton:nil
		informativeTextWithFormat:@"%@", message];
}


- (void) planetToolRendererCancelled:(PlanetToolRenderer *)renderer
{
	[self removeProgressSheetWithSuccess:NO];
}


- (void) planetToolRenderer:(PlanetToolRenderer *)renderer progressUpdate:(float)progress
{
	[self updateProgress:progress];
}


#pragma mark -
#pragma mark Loading

- (void) asyncLoadImage:(NSString *)path
{
	_sourcePixMap = FPMCreateWithPNG([path fileSystemRepresentation], kFPMGammaLinear, LoadErrorHandler, LoadProgressHandler, self);
	[self performSelectorOnMainThread:@selector(loadingComplete) withObject:nil waitUntilDone:NO];
}


- (IBAction) cancelLoading:(id)sender
{
	[self removeProgressSheetWithSuccess:NO];
	[self close];
}


static void LoadErrorHandler(const char *message, bool isError, void *context)
{
	if (isError)
	{
		NSString *string = [NSString stringWithUTF8String:message];
		if (string == nil)  string = [[NSString alloc] initWithCString:message encoding:NSISOLatin1StringEncoding];
		
		[(PlanetToolDocument *)context performSelectorOnMainThread:@selector(loadingFailedWithMessage:)
														withObject:string
													 waitUntilDone:NO];
	}
}


static void LoadProgressHandler(float proportion, void *context)
{
	[(PlanetToolDocument *)context updateProgress:proportion];
}


- (void) loadingComplete
{
	[self removeProgressSheetWithSuccess:YES];
	
	// Select input and output format based on aspect ratio.
	float aspectRatio = (float)FPMGetWidth(_sourcePixMap) / (float)FPMGetHeight(_sourcePixMap);
	NSUInteger size = 0;
	if (aspectRatio >= 1.5)
	{
		self.inputFormat = kPlanetToolFormatLatLong;
		self.outputFormat = kPlanetToolFormatCube;
		size = FPMGetHeight(_sourcePixMap) / 2;
	}
	else if (aspectRatio >= 1)
	{
		self.inputFormat = kPlanetToolFormatCubeX;
		self.outputFormat = kPlanetToolFormatLatLong;
		size = FPMGetWidth(_sourcePixMap) / 2;
	}
	else
	{
		self.inputFormat = kPlanetToolFormatCube;
		self.outputFormat = kPlanetToolFormatLatLong;
		size = FPMGetWidth(_sourcePixMap) * 2;
	}
	self.outputSize = OORoundUpToPowerOf2(size);
}


- (void) loadingFailedWithMessage:(NSString *)message
{
	[self removeProgressSheetWithSuccess:NO];
	
	NSAlert *alert = [NSAlert alertWithMessageText:NSLocalizedString(@"The image could not be loaded.", NULL)
									 defaultButton:nil
								   alternateButton:nil
									   otherButton:nil
						 informativeTextWithFormat:@"%@", message];
	[alert beginSheetModalForWindow:self.windowForSheet
					  modalDelegate:self
					 didEndSelector:@selector(loadingFailedSheetDidEnd:returnCode:contextInfo:)
						contextInfo:NULL];
}


- (void) loadingFailedSheetDidEnd:(NSAlert *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
	[self close];
}

@end
