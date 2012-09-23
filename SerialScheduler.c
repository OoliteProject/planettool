/*
	SerialScheduler.c
	planettool
	
	Simple single-threaded implementation of PlanetToolScheduler interface.
	
	
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


#include "PlanetToolScheduler.h"
#include "PTPowerManagement.h"


static bool DoScheduleRender(RenderCallback renderCB, void *renderContext, size_t lineCount, size_t subRenderIndex, size_t subRenderCount, ProgressCallbackFunction progressCB, void *cbContext)
{
	if (renderCB == NULL)  return false;
	
	size_t progressNumerator = subRenderIndex * lineCount;
	size_t progressDenominator = subRenderCount * lineCount;
	
	size_t i;
	for (i = 0; i < lineCount; i++)
	{
		if (EXPECT_NOT(!renderCB(i, lineCount, renderContext)))  return false;
		
		if (progressCB != NULL)
		{
			if (EXPECT_NOT(!progressCB(++progressNumerator, progressDenominator, cbContext)))  return false;
		}
	}
	
	return true;
}


bool ScheduleRender(RenderCallback renderCB, void *renderContext, size_t lineCount, size_t subRenderIndex, size_t subRenderCount, ProgressCallbackFunction progressCB, void *cbContext)
{
	PTStartPreventingSleep();
	DoScheduleRender();
	PTStopPreventingSleep();
}

