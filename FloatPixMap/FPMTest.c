#include <stdio.h>
#include "FPMPNG.h"
#include "FPMImageOperations.h"


static void ErrorHandler(const char *message, bool isError);


int main (int argc, const char * argv[])
{
	if (argc < 3)
	{
		fprintf(stderr, "Need an input file path and an output file path.\n");
		return EXIT_FAILURE;
	}
	
	FPMInit();
	
	printf("Reading from %s\n", argv[1]);
	FloatPixMapRef pm = FPMCreateWithPNG(argv[1], kFPMGammaLinear, ErrorHandler);
	if (pm == NULL)  return EXIT_FAILURE;
	
	FPMRect rect = FPMMakeRectC(64, 64, 128, 128);
	FloatPixMapRef pm2 = FPMCreateSub(pm, rect);
//	FPMRelease(&pm);
	
	FPMScaleValues(pm2, FPMMakeColorGrey(0.2, 1.0), FPMMakeColorGrey(0.7, 0.0));
	
	printf("Writing to %s\n", argv[2]);
	FPMWritePNG(pm, argv[2], kFPMWritePNGDither, kFPMGammaLinear, kFPMGammaSRGB, ErrorHandler);
	
	FPMRelease(&pm);
	FPMRelease(&pm2);
    return 0;
}


static void ErrorHandler(const char *message, bool isError)
{
	fprintf(stderr, "%s: %s\n", isError ? "ERROR" : "WARNING", message);
}
