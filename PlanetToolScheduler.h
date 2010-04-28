/*
	PlanetToolScheduler.h
	planettool
	
	Interface for schedulers.
	
	A scheduler takes a callback and a line count, and invokes the callback
	for each line value from zero to line count - 1. This may be done in
	parallel.
	
	
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

#ifndef INCLUDED_PlanetToolScheduler_h
#define INCLUDED_PlanetToolScheduler_h


#include "SphericalPixelSource.h"
#include <stdlib.h>
#include <stdbool.h>


/*	Type for render callback.
	The callback is called for each output line. Lines may be rendered in any
	order, potentially in parallel, and must not be independent.
	
	If a render callback returns false, ScheduleRender SHALL return false and
	MAY skip other lines. Since order of rendering is not specified, the
	cancellation of particular lines cannot be guaranteed.
*/
typedef bool (*RenderCallback)(size_t lineIndex, size_t lineCount, void *context);


/*	Scheduler interface.
	
	renderCB is the function to call for each line.
	
	renderContext is an arbitrary parameter to the render callback.
	
	lineCount specifies the number of lines to be rendered.
	
	subRenderIndex and subRenderCount may be used to specify that multiple
	calls to ScheduleRender() are being used, and will determine the parameters
	passed to progressCB. For instance, when rendering to lat/long
	ScheduleRender() will be called once, so subRenderIndex is 0 and
	subRenderCount is 1, but when rendering to a cube map six invocations will
	be used, so subRenderIndex will range from 0 to 5. It is assumed lineCount
	will be the same for each invocation; if not, the caller will have to
	provide an intermediate progress callback to translate.
	
	progressCB is called for each line to report progress.
	
	cbContext is an arbitrary parameter to the progress callback.
*/
bool ScheduleRender(RenderCallback renderCB, void *renderContext, size_t lineCount, size_t subRenderIndex, size_t subRenderCount, ProgressCallbackFunction progressCB, void *cbContext);


#endif	/* INCLUDED_PlanetToolScheduler_h */
