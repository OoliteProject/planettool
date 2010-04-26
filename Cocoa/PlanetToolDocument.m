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
#import "NSImage+FloatPixMap.h"


@interface PlanetToolDocument () <PlanetToolRendererDelegate>

@property (readwrite) BOOL renderingPreview;
@property (readwrite) BOOL gridGenerator;
@property (readwrite, assign) NSImage *previewImage;

- (void) asyncLoadImage:(NSString *)path;

- (void) loadingComplete;
- (void) loadingFailedWithMessage:(NSString *)message;

- (void) showProgressSheetWithMessage:(NSString *)message cancelAction:(SEL)cancelAction;
- (void) updateProgress:(float)value;
- (void) removeProgressSheetWithSuccess:(BOOL)success;
- (BOOL) startRenderer:(PlanetToolRenderer *)renderer;

- (void) startPreviewRender;
- (NSUInteger) smallPreviewSize;
- (NSUInteger) largePreviewSize;

@end


static void LoadErrorHandler(const char *message, bool isError, void *context);
static void LoadProgressHandler(float proportion, void *context);


@implementation PlanetToolDocument

@synthesize progressSheet = _progressSheet;
@synthesize progressLabel = _progressLabel;
@synthesize progressBar = _progressBar;
@synthesize progressCancelButton = _progressCancelButton;

@synthesize inputFormatPopUp  = _inputFormatPopUp;

@synthesize outputSize = _outputSize;
@synthesize flip = _flip;
@synthesize fast = _fast;
@synthesize jitter = _jitter;
@synthesize sixteenBitPerChannel = _sixteenBitPerChannel;
@synthesize rotateX = _rotateX;
@synthesize rotateY = _rotateY;
@synthesize rotateZ = _rotateZ;
@synthesize previewImage = _previewImage;
@synthesize renderingPreview = _isRenderingPreview;

@synthesize gridGenerator = _gridGenerator;


+ (void) initialize
{
	FPMInit();
}


- (id) initAsGridGenerator
{
	if ((self = [self init]))
	{
		self.gridGenerator = YES;
		self.outputSize = 1024;
	}
	return self;
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
	
	if (self.gridGenerator)
	{
		NSPopUpButton *popUp = self.inputFormatPopUp;
		[popUp setEnabled:NO];
		[popUp removeAllItems];
		[popUp addItemWithTitle:NSLocalizedString(@"Grid Generator", NULL)];
		[self startPreviewRender];
	}
}


- (void) deferredAwakeFromNib
{
	if (!self.gridGenerator && _sourcePixMap == nil)
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


- (BOOL) isDocumentEdited
{
	return NO;
}



#pragma mark -
#pragma mark Properties

- (NSUInteger) inputFormat
{
	return _inputFormat;
}


- (void) setInputFormat:(NSUInteger)value
{
	if (IsValidPlanetToolInputFormat(value) && _inputFormat != value)
	{
		_inputFormat = value;
		[self startPreviewRender];
	}
}


- (NSUInteger) outputFormat
{
	return _outputFormat;
}


- (void) setOutputFormat:(NSUInteger)value
{
	if (IsValidPlanetToolOutputFormat(value) && _outputFormat != value)
	{
		_outputFormat = value;
		[self startPreviewRender];
	}
}


- (void) setOutputSize:(NSInteger)size
{
	if (size > 0)
	{
		NSUInteger previewSize = [self largePreviewSize];
		_outputSize = size;
		
		if (previewSize != [self largePreviewSize])
		{
			[self startPreviewRender];
		}
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


- (void) setFlip:(BOOL)value
{
	if (value != _flip)
	{
		_flip = value;
		[self startPreviewRender];
	}
}


static inline float ClampDegrees(float value)
{
	return fmodl(value + 180.0f, 360.0) - 180.0f;
}


- (void) setRotateX:(float)value
{
	value = ClampDegrees(value);
	if (value != _rotateX)
	{
		_rotateX = value;
		[self startPreviewRender];
	}
}


- (void) setRotateY:(float)value
{
	value = ClampDegrees(value);
	if (value != _rotateY)
	{
		_rotateY = value;
		[self startPreviewRender];
	}
}


- (void) setRotateZ:(float)value
{
	value = ClampDegrees(value);
	if (value != _rotateZ)
	{
		_rotateZ = value;
		[self startPreviewRender];
	}
}


- (BOOL) canUseCubeSource
{
	return FPMGetWidth(_sourcePixMap) % 6 == 0;
}


- (BOOL) canUseCubeXSource
{
	return FPMGetWidth(_sourcePixMap) % 4 == 0 && FPMGetHeight(_sourcePixMap) % 3 == 0;
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
	savePanel.canSelectHiddenExtension = YES;
	[savePanel setExtensionHidden:NO];
	
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
	
	_outputDisplayName = [sheet.filename lastPathComponent];
	if ([sheet isExtensionHidden])  _outputDisplayName = [_outputDisplayName stringByDeletingPathExtension];
	
	NSString *message = [NSString stringWithFormat:NSLocalizedString(@"Rendering \"%@\"...", NULL), _outputDisplayName];
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
	
	if (![self startRenderer:_finalRenderer])
	{
		[self planetToolRenderer:_finalRenderer failedWithMessage:NSLocalizedString(@"Unknown error.", NULL)];
	}
}


- (BOOL) startRenderer:(PlanetToolRenderer *)renderer
{
	if (self.gridGenerator)
	{
		return [renderer asyncRenderFromGridGenerator];
	}
	else
	{
		return [renderer asyncRenderFromImage:_sourcePixMap];
	}
}


- (void) cancelRender:(id)sender
{
	[_finalRenderer cancelRendering];
}


- (void) planetToolRenderer:(PlanetToolRenderer *)renderer didCompleteImage:(FloatPixMapRef)image
{
	if (renderer == _finalRenderer)
	{
		_outputImage = FPMRetain(image);
		[NSThread detachNewThreadSelector:@selector(writeOutputFile) toTarget:self withObject:nil];
		self.progressLabel.stringValue = [NSString stringWithFormat:NSLocalizedString(@"Writing \"%@\"...", NULL), _outputDisplayName];
	}
	else if (renderer == _previewRenderer)
	{
		if (renderer.outputSize < [self largePreviewSize])
		{
			self.previewImage = [NSImage fpm_imageWithFloatPixMap:image sourceGamma:kFPMGammaLinear];
			renderer.outputSize = [self largePreviewSize];
			[self startRenderer:renderer];
		}
		else if (renderer == _previewRenderer)
		{
			self.previewImage = [NSImage fpm_imageWithFloatPixMap:image sourceGamma:kFPMGammaLinear];
			_previewRenderer = nil;
			self.renderingPreview = NO;
		}
		// Else it's an old high-res preview, which we ignore.
	}

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
	FPMWritePNGFlags flags = kFPMWritePNGDither;
	if (self.sixteenBitPerChannel)  flags |= kFPMWritePNG16BPC;
	if (FPMWritePNG(_outputImage, [_outputPath fileSystemRepresentation], flags, kFPMGammaLinear, kFPMGammaSRGB, WriteErrorHandler, NULL, self))
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
	[self removeProgressSheetWithSuccess:NO];
	[NSAlert alertWithMessageText:[NSString stringWithFormat:NSLocalizedString(@"The document \"%@\" could not be saved.", NULL), _outputDisplayName]
					defaultButton:nil
				  alternateButton:nil
					  otherButton:nil
		informativeTextWithFormat:@"%@", message];
}


- (void) planetToolRenderer:(PlanetToolRenderer *)renderer failedWithMessage:(NSString *)message
{
	if (renderer == _finalRenderer)
	{
		[self removeProgressSheetWithSuccess:NO];
		[NSAlert alertWithMessageText:NSLocalizedString(@"Rendering failed.", NULL)
						defaultButton:nil
					  alternateButton:nil
						  otherButton:nil
			informativeTextWithFormat:@"%@", message];
	}
	else	
	{
		self.renderingPreview = NO;
	}
}


- (void) planetToolRendererCancelled:(PlanetToolRenderer *)renderer
{
	if (renderer == _finalRenderer)
	{
		[self removeProgressSheetWithSuccess:NO];
	}
}


- (BOOL) planetToolRenderer:(PlanetToolRenderer *)renderer progressUpdate:(float)progress
{
	if (renderer == _finalRenderer)
	{
		[self updateProgress:progress];
		return YES;
	}
	else
	{
		// Cancel dangling renderers caused by modifying parameters while preview still rendering.
		return renderer == _previewRenderer;
	}
	
}



#pragma mark -
#pragma mark Preview rendering

- (void) startPreviewRender
{
	self.renderingPreview = YES;
	[_previewRenderer cancelRendering];
	
	if (self.outputSize == 0)
	{
		self.renderingPreview = NO;
		return;
	}
	
	_previewRenderer = [PlanetToolRenderer new];
	_previewRenderer.fast = YES;
	_previewRenderer.delegate = self;
	_previewRenderer.inputFormat = self.inputFormat;
	_previewRenderer.outputFormat = self.outputFormat;
	_previewRenderer.outputSize = [self smallPreviewSize];
	_previewRenderer.flip = self.flip;
	_previewRenderer.rotateX = self.rotateX;
	_previewRenderer.rotateY = self.rotateY;
	_previewRenderer.rotateZ = self.rotateZ;
	
	[self startRenderer:_previewRenderer];
}


- (NSUInteger) smallPreviewSize
{
	NSUInteger result = 128;
	switch (self.outputFormat)
	{
		case kPlanetToolFormatLatLong:
			result = 128;
			break;
			
		case kPlanetToolFormatMercator:
		case kPlanetToolFormatGallPeters:
			result = 256;
			break;
			
		case kPlanetToolFormatCube:
		case kPlanetToolFormatCubeX:
			result = 64;
			break;
	}
	
	if (result > (NSUInteger)self.outputSize)  result = self.outputSize;
	return result;
}


- (NSUInteger) largePreviewSize
{
	NSUInteger result = 256;
	switch (self.outputFormat)
	{
		case kPlanetToolFormatLatLong:
		case kPlanetToolFormatMercator:
		case kPlanetToolFormatGallPeters:
			result = 512;
			break;
			
		case kPlanetToolFormatCube:
			result = 128;
			break;
			
		case kPlanetToolFormatCubeX:
			result = 256;
			break;
	}
	
	if (result > (NSUInteger)self.outputSize)  result = self.outputSize;
	return result;
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


- (IBAction) resetRotation:(id)sender
{
	self.rotateX = 0.0f;
	self.rotateY = 0.0f;
	self.rotateZ = 0.0f;
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
	float aspectRatio = FPMGetWidth(_sourcePixMap) / FPMGetHeight(_sourcePixMap);
	NSUInteger size = 0;
	
	if (aspectRatio < 1.0f && self.canUseCubeSource)
	{
		self.inputFormat = kPlanetToolFormatCube;
		self.outputFormat = kPlanetToolFormatLatLong;
		size = FPMGetWidth(_sourcePixMap) * 2;
	}
	else if (aspectRatio < 1.5f && self.canUseCubeXSource)
	{
		self.inputFormat = kPlanetToolFormatCubeX;
		self.outputFormat = kPlanetToolFormatLatLong;
		size = FPMGetWidth(_sourcePixMap) / 2;
	}
	else
	{
		self.inputFormat = kPlanetToolFormatLatLong;
		self.outputFormat = kPlanetToolFormatCube;
		size = FPMGetHeight(_sourcePixMap) / 2;
	}
	
	self.outputSize = OORoundUpToPowerOf2(size);
	
	[self startPreviewRender];
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
