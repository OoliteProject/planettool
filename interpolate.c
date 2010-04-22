#include "FPMImageOperations.h"
#include "FPMPNG.h"
#include <assert.h>


enum
{
	kOutWidth = 173,
	kOutHeight = 219
};


#define kWorkingSpace kFPMGammaLinear


int main (int argc, const char * argv[])
{
	FPMInit();
	
	assert(argc > 1);
	FloatPixMapRef source = FPMCreateWithPNG(argv[1], kWorkingSpace, NULL);
	FloatPixMapRef target = FPMCreateC(kOutWidth, kOutHeight);
	
	float xscale = (float)FPMGetWidth(source) / (float)kOutWidth;
	float yscale = (float)FPMGetHeight(source) / (float)kOutHeight;
	
	FPM_FOR_EACH_PIXEL(target, true)
	{
		float fx = ((float)x) * xscale;
		float fy = ((float)y) * yscale;
		
	//	printf("(%i, %i) -> (%g, %g)\n", x, y, fx, fy);
		
		*pixel = FPMSampleCubic(source, fx, fy, kFPMWrapClamp, kFPMWrapClamp);
	}
	FPM_END_FOR_EACH_PIXEL
	
	FPMWritePNG(target, "/tmp/fpm-sample-test.png", kFPMWritePNGDither, kWorkingSpace, kFPMGammaSRGB, NULL);
}
