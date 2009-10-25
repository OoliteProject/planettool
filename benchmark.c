#include <stdio.h>
#include "FPMPNG.h"
#include "SphericalPixelSource.h"
#include <mach/mach_time.h>

// Sources
#include "LatLongGridGenerator.h"
#include "ReadLatLong.h"

// Sinks
#include "RenderToLatLong.h"
#include "RenderToCube.h"


#define BENCHMARK_ITERS			100
#define SIZE					64
#define FAST					1
#define JITTER					0

#if 1
#define REQUIRE_SOURCE			0
#define SOURCE					LatLongGridGenerator
#define CONSTRUCTOR				NULL
#define DESTRUCTOR				NULL
#else
#define REQUIRE_SOURCE			1
#define SOURCE					ReadLatLong
#define CONSTRUCTOR				ReadLatLongConstructor
#define DESTRUCTOR				ReadLatLongDestructor
#endif

#define SINK					RenderToCube


static double TimeDeltaSeconds(uint64_t startTime, uint64_t endTime);


int main (int argc, const char * argv[])
{
	FPMInit();
	srandomdev();
	
	RenderFlags flags = 0;
	if (FAST)  flags |= kRenderFast;
	if (JITTER)  flags |= kRenderJitter;
	
	SphericalPixelSourceConstructorFunction constructor = CONSTRUCTOR;
	SphericalPixelSourceDestructorFunction destructor = DESTRUCTOR;
	
	// For sources that require a file, read from argv[1]
	FloatPixMapRef sourcePM = NULL;
#if REQUIRE_SOURCE
	if (argc > 1)
	{
		printf("Loading source...\n");
		sourcePM = FPMCreateWithPNG(argv[1], kFPMGammaLinear, NULL);
		if (sourcePM == NULL)
		{
			fprintf(stderr, "Could not load source image from %s\n", argv[1]);
			return EXIT_FAILURE;
		}
	}
#endif
	
	unsigned i;
	uint64_t start = mach_absolute_time();
	for (i = 0; i < BENCHMARK_ITERS; i++)
	{
		printf("Rendering pass %u of %u...\n", i + 1, BENCHMARK_ITERS);
		
		void *context = NULL;
		
		if (constructor != NULL)  constructor(sourcePM, flags, &context);
		FloatPixMapRef pm = SINK(SIZE, 0, SOURCE, context);
		if (destructor != NULL)  destructor(context);
		
		FPMRelease(&pm);
	}
	uint64_t end = mach_absolute_time();
	double elapsed = TimeDeltaSeconds(start, end);
	printf("Rendered %u passes in %f seconds (%f s per).\n", BENCHMARK_ITERS, elapsed, elapsed / (double)BENCHMARK_ITERS);
	
	FPMRelease(&sourcePM);
	
	return 0;
}


static double TimeDeltaSeconds(uint64_t startTime, uint64_t endTime)
{
	uint64_t diff = endTime - startTime;
	static double conversion = 0.0;
	
	if (EXPECT_NOT(conversion == 0.0))
	{
		mach_timebase_info_data_t info;
		kern_return_t err = mach_timebase_info(&info);
		
		if (err == 0)
		{
			conversion = 1e-9 * (double)info.numer / (double)info.denom;
		}
	}
	
	return conversion * (double)diff;
}
