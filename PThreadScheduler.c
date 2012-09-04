/*
	PThreadScheduler.c
	planettool
	
	POSIX thread PlanetToolScheduler interface.
	
	
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
#include <pthread.h>

#ifdef __WIN32__
#include <windows.h>
#endif


typedef struct PlanetToolSchedulerContext
{
	RenderCallback				renderCB;
	void						*renderContext;
	size_t						lineCount;
	size_t						subRenderIndex;
	size_t						subRenderCount;
	
	pthread_mutex_t				indexLock;
	volatile size_t				index;
	
	pthread_mutex_t				notificationMutex;
	pthread_cond_t				notificationCond;
	
	size_t						progressNumerator;
	size_t						progressDenominator;
	ProgressCallbackFunction	progressCB;
	void						*cbContext;
	
	volatile bool				stop;
} PlanetToolSchedulerContext;


static bool RunRenderTask(PlanetToolSchedulerContext *context);
static void *RenderThreadTask(void *vcontext);
static unsigned ThreadCount(void);


bool ScheduleRender(RenderCallback renderCB, void *renderContext, size_t lineCount, size_t subRenderIndex, size_t subRenderCount, ProgressCallbackFunction progressCB, void *cbContext)
{
	PlanetToolSchedulerContext threadContext =
	{
		.renderCB = renderCB,
		.renderContext = renderContext,
		.lineCount = lineCount,
		.subRenderIndex = subRenderIndex,
		.subRenderCount = subRenderCount,
		.indexLock = PTHREAD_MUTEX_INITIALIZER,
		.index = 0,
		.notificationCond = PTHREAD_COND_INITIALIZER,
		.notificationMutex = PTHREAD_MUTEX_INITIALIZER,
		.progressNumerator = subRenderIndex * lineCount,
		.progressDenominator = subRenderCount * lineCount,
		.progressCB = NULL,
		.cbContext = cbContext,
		.stop = false
	};
	
	bool result = false;
	
	if (pthread_mutex_init(&threadContext.indexLock, NULL) == 0)
	{
		if (pthread_mutex_init(&threadContext.notificationMutex, NULL) == 0)
		{
			if (pthread_cond_init(&threadContext.notificationCond, NULL) == 0)
			{
				threadContext.progressCB = progressCB;
			}
		}
		
		result = RunRenderTask(&threadContext);
		
		if (threadContext.progressCB != NULL)
		{
			pthread_cond_destroy(&threadContext.notificationCond);
			pthread_mutex_destroy(&threadContext.notificationMutex);
		}
		
		pthread_mutex_destroy(&threadContext.indexLock);
	}
	else
	{
		fprintf(stderr, "Failed to create work item indexing mutex.\n");
	}

	
	return result;
}


static bool RunRenderTask(PlanetToolSchedulerContext *context)
{
	unsigned i, threadCount = ThreadCount();
	if (threadCount > context->lineCount)  threadCount = context->lineCount;	// Prepare for the mega-manycore monsters of the mfuture!
	assert(threadCount > 0);
	
	pthread_t threads[threadCount];
	int err = 0;
	
	for (i = 0; i < threadCount; i++)
	{
		err = pthread_create(&threads[i], NULL, RenderThreadTask, context);
		if (err != 0)
		{
			if (i == 0)
			{
				fprintf(stderr, "Failed to create work threads.\n");
				return false;
			}
			else
			{
				threadCount = i;
				break;
			}
		}
	}
	
	// Receive progress notifications if relevant.
	if (context->progressCB != NULL)
	{
		while (context->index < context->lineCount && !context->stop)
		{
			pthread_mutex_lock(&context->notificationMutex);
			pthread_cond_wait(&context->notificationCond, &context->notificationMutex);
			pthread_mutex_unlock(&context->notificationMutex);
			
			if (context->index <= context->lineCount)
			{
				if (EXPECT_NOT(!context->progressCB(context->progressNumerator + context->index, context->progressDenominator, context->cbContext)))
				{
					context->stop = true;
				}
			}
		}
		
		if (!context->stop)  context->progressCB(context->progressNumerator + context->lineCount, context->progressDenominator, context->cbContext);
	}
	
	// Wait for completion.
	for (i = 0; i < threadCount; i++)
	{
		void *junk = NULL;
		err = pthread_join(threads[i], &junk);
		if (err != 0)
		{
			fprintf(stderr, "\nThread join error: %i\n", err);
			abort();
		}
	}
	
	return !context->stop;
}


static void *RenderThreadTask(void *vcontext)
{
	PlanetToolSchedulerContext *context = vcontext;
	
	for (;;)
	{
		// Select an index.
		pthread_mutex_lock(&context->indexLock);
		size_t idx = context->index++;
		pthread_mutex_unlock(&context->indexLock);
		
		if (idx < context->lineCount && !context->stop)
		{
			bool success = context->renderCB(idx, context->lineCount, context->renderContext);
			if (EXPECT_NOT(!success))  context->stop = true;
			
			// If there's a progress callback, signal main thread to update.
			if (context->progressCB != NULL)
			{
				pthread_cond_signal(&context->notificationCond);
			}
		}
		
		if (idx >= context->lineCount || context->stop)  break;
	}
	
	return NULL;
}


#include <unistd.h>

#if FORCE_SINGLE_THREAD

static unsigned ThreadCount(void)
{
	// For debugging.
	return 1;
}

#elif __APPLE__
#include <sys/sysctl.h>

static unsigned ThreadCount(void)
{
	int		value = 0;
	size_t	size = sizeof value;
	
	if (sysctlbyname("hw.logicalcpu", &value, &size, NULL, 0) == 0)
	{
		if (1 <= value)  return value;
	}
	if (sysctlbyname("hw.ncpu", &value, &size, NULL, 0) == 0)
	{
		if (1 <= value)  return value;
	}
	
	return 1;
}

#elif defined _SC_NPROCESSORS_ONLN

static unsigned ThreadCount(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

#elif defined __WIN32__

static unsigned ThreadCount(void)
{
	SYSTEM_INFO	sysInfo;
	
	GetSystemInfo(&sysInfo);
	return sysInfo.dwNumberOfProcessors;
}

#else

#warning Cannot determine number of processors on this system, rendering will be single-threaded.

static unsigned ThreadCount(void)
{
	return 1;
}

#endif
