/*
	main.c
	planettool
	
	
	Copyright © 2009–2010 Jens Ayton

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "FPMPNG.h"
#include "SphericalPixelSource.h"
#include "PTPowerManagement.h"

// Sources
#include "LatLongGridGenerator.h"
#include "ReadLatLong.h"
#include "ReadCube.h"
#include "MatrixTransformer.h"
#include "CosineBlurFilter.h"

// Sinks
#include "RenderToLatLong.h"
#include "RenderToCube.h"
#include "RenderToMercator.h"
#include "RenderToGallPeters.h"


static void LoadErrorHandler(const char *message, bool isError, void *context);
static void RenderErrorHandler(const char *message, void *context);


typedef struct
{
	const char						*name;
	const char						shortcut;
} FilterEntryBase;


typedef struct
{
	FilterEntryBase					keys;
	SphericalPixelSourceConstructorFunction	constructor;
	SphericalPixelSourceDestructorFunction	destructor;
} SourceEntry;


typedef struct
{
	FilterEntryBase					keys;
	SphericalPixelSinkFunction		sink;
	size_t							defaultSize;
} SinkEntry;


typedef struct
{
	RenderFlags						flags;
	const SourceEntry				*source;
	const SinkEntry					*sink;
	size_t							size;
	size_t							defaultSize;
	OOMatrix						transform;
	double							cosBlurBackFactor;
	double							cosBlurFrontFactor;
	bool							showHelp;
	bool							showVersion;
	bool							quiet;
	bool							sixteenBit;
	bool							cosBlur;
	const char						*sourcePath;
	const char						*sinkPath;
} Settings;


static bool InterpretArguments(int argc, const char * argv[], Settings *settings);


static bool PrintProgress(size_t numerator, size_t denominator, void *context);


int main (int argc, const char * argv[])
{
	FPMInit();
	srand(time(NULL));
	PTStartPreventingSleep();
	
	// Work out what the user wants.
	Settings settings;
	memset(&settings, 0, sizeof settings);
	settings.transform = kIdentityMatrix;
	
	if (!InterpretArguments(argc, argv, &settings))
	{
		return (settings.showHelp || settings.showVersion) ? 0 : EXIT_FAILURE;
	}
	assert(settings.source != NULL && settings.sink != NULL);
	
	// Read input file, if any.
	FloatPixMapRef sourcePM = NULL;
	if (settings.sourcePath != NULL)
	{
		if (!settings.quiet)  printf("Reading...\n");
		sourcePM = FPMCreateWithPNG(settings.sourcePath, kFPMGammaLinear, NULL, NULL, NULL);
		
		if (sourcePM == NULL)
		{
			fprintf(stderr, "Could not load %s\n", settings.sourcePath);
			return EXIT_FAILURE;
		}
	}
	
	// Run source constructor.
	void *sourceContext = NULL;
	SphericalPixelSourceFunction source = NULL;
	if (settings.source->constructor != NULL)
	{
		if (!settings.source->constructor(sourcePM, settings.flags, &source, &sourceContext))
		{
			return EXIT_FAILURE;
		}
	}
	SphericalPixelSourceDestructorFunction destructor = settings.source->destructor;
	
	// Set up matrix filter if necessary.
	if (!OOMatrixIsIdentity(settings.transform))
	{
		void *transformContext = NULL;
		if (!MatrixTransformerSetUp(source, destructor, sourceContext, settings.transform, &transformContext))
		{
			return EXIT_FAILURE;
		}
		
		source = MatrixTransformer;
		destructor = MatrixTransformerDestructor;
		sourceContext = transformContext;
	}
	
	// Set up cos blur filter if requested.
	if (settings.cosBlur)
	{
		void *cosBlurContext = NULL;
		if (!CosineBlurFilterSetUp(source, destructor, sourceContext, settings.size, settings.cosBlurBackFactor, settings.cosBlurFrontFactor, &cosBlurContext))
		{
			return EXIT_FAILURE;
		}
		
		source = CosineBlurFilter;
		destructor = CosineBlurFilterDestructor;
		sourceContext = cosBlurContext;
	}
	
	// Render.
	ProgressCallbackFunction progressCB = NULL;
	unsigned progressCtxt = 0;
	if (!settings.quiet)
	{
		printf("Rendering...\n");
		progressCB = PrintProgress;
	}
	
	FloatPixMapRef resultPM = settings.sink->sink(settings.size, settings.flags, source, sourceContext, progressCB, RenderErrorHandler, &progressCtxt);
	FPMRelease(&sourcePM);
	if (!settings.quiet)  printf("\n");
	
	if (resultPM == NULL)
	{
		fprintf(stderr, "Rendering failed.\n");
		return EXIT_FAILURE;
	}
	
	// Destructimacate.
	if (destructor != NULL)  destructor(sourceContext);
	
	// Write output.
	if (!settings.quiet)  printf("Writing...\n");
	FPMWritePNGFlags flags = kFPMWritePNGDither;
	if (settings.sixteenBit)  flags = kFPMWritePNG16BPC;
	if (!FPMWritePNG(resultPM, settings.sinkPath, flags, kFPMGammaLinear, kFPMGammaSRGB, LoadErrorHandler, NULL, NULL))
	{
		return EXIT_FAILURE;
	}
	
	if (!settings.quiet)  printf("Done.\n");
	return 0;
}


static void LoadErrorHandler(const char *message, bool isError, void *context)
{
	fprintf(stderr, "%s: %s\n", isError ? "ERROR" : "WARNING", message);
}


static void RenderErrorHandler(const char *message, void *context)
{
	fprintf(stderr, "%s\n", message);
}


/*	Argument parser functions.
	These will never be called with NULL arguments, and argv will have at least
	minArgs (as specified in the Arguments struct) elements.
*/
typedef bool (*ArgumentParserFunction)(int argc, const char *argv[], int *consumedArgs, Settings *settings);

static bool ParseInput(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseGenerate(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseOutput(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseSize(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseFast(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseJitter(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseSixteenBit(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseRotate(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseCosBlur(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseFlip(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseHelp(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseVersion(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseQuiet(int argc, const char *argv[], int *consumedArgs, Settings *settings);


static const SourceEntry sGenerators[] =
{
	{{ "grid1",				'g', },	LatLongGridGeneratorConstructor, NULL }
};

enum { sGeneratorCount = sizeof sGenerators / sizeof sGenerators[0] };


static const SourceEntry sReaders[] =
{
	{{ "latlong",			'l', },	ReadLatLongConstructor, ReadLatLongDestructor },
	{{ "cube",				'c', },	ReadCubeConstructor, ReadCubeDestructor },
	{{ "cubex",				'x', },	ReadCubeCrossConstructor, ReadCubeDestructor },
};

enum { sReaderCount = sizeof sReaders / sizeof sReaders[0] };


static const SinkEntry sSinks[] =
{
	{{ "latlong",			'l', },	RenderToLatLong, 2048 },
	{{ "cube",				'c', },	RenderToCube, 1024 },
	{{ "cubex",				'x', },	RenderToCubeCross, 1024 },
	{{ "mercator",			'm', },	RenderToMercator, 2048 },
	{{ "gall-peters",		'g', },	RenderToGallPeters, 2048 }
};

enum { sSinkCount = sizeof sSinks / sizeof sSinks[0] };


typedef struct
{
	char					*keyword;
	char					shortcut;
	unsigned				minArgs;
	ArgumentParserFunction	handler;
	
	// Stuff used to generate documentation, such as it is.
	char					*paramNames;
	bool					required;
	bool					hidden;
	char					*description;
	FilterEntryBase			*descTypes;
	off_t					descStride;
	unsigned				descTypeCount;
} ArgumentHandler;


static const ArgumentHandler sHandlers[] =
{
	{
		"output",		'o', 2, ParseOutput,
		"<outType> <outFile>", true, false, "Type and name of output file. Type must be one of: ", (FilterEntryBase *)sSinks, sizeof(*sSinks), sSinkCount
	},
	{
		"input",		'i', 2, ParseInput,
		"<inType> <inFile>",  false, false, "Type and name of input file. Type must be one of: ", (FilterEntryBase *)sReaders, sizeof(*sReaders), sReaderCount
	},
	{
		"generate",		'g', 1, ParseGenerate,
		"<generator>", false, false, "Type and name of generator. Type must be one of: ", (FilterEntryBase *)sGenerators, sizeof(*sGenerators), sGeneratorCount
	},
	{
		"size",			'S', 1, ParseSize,
		"<size>", false, false, "Size of output, in pixels. Interpretation depends on output type.", NULL, 0, 0
	},
	{
		"fast",			'F', 0, ParseFast,
		NULL, false, false, "Use faster, low-quality rendering.", NULL, 0, 0
	},
	{
		"jitter",		'J', 0, ParseJitter,
		NULL, false, false, "Use jittering for slower, slightly noisy rendering which may look better in some cases.", NULL, 0, 0
	},
	{
		"sixteen-bit",	0, 0, ParseSixteenBit,
		NULL, false, false, "Save in sixteen bit per channel format (instead of eight-bit-per-channel format).", NULL, 0, 0
	},
	{
		"flip",			'L', 0, ParseFlip,
		NULL, false, false, "Mirror the texture in 3D space (through the YZ plane) while rendering. This produces an \"inside-out\" texture.", NULL, 0, 0
	},
	{
		"rotate",		'R', 3, ParseRotate,
		"<ry> <rx> <rz>", false, false, "Rotate the texture around the planet while rendering. The ry axis corresponds to the planet's axis of rotation.", NULL, 0, 0
	},
	{
		"cosblur",		0, 2, ParseCosBlur,
		"<unmaskedscale> <maskedscale>", false, true, "Apply cosine blur (converts environment map into diffuse light map).", NULL, 0, 0
	},
	{
		"help",			'H', 0, ParseHelp,
		NULL, false, false, "Show this helpful help.", NULL, 0, 0
	},
	{
		"version",		'V', 0, ParseVersion,
		NULL, false, false, "Show version number.", NULL, 0, 0
	},
	{
		"quiet",		'Q', 0, ParseQuiet,
		NULL, false, false, "Don't print progress information.", NULL, 0, 0
	},
};

static const unsigned sHandlerCount = sizeof sHandlers / sizeof sHandlers[0];


static void ShowHelp(bool showHidden);
static void ShowVersion(void);


static bool InterpretArguments(int argc, const char * argv[], Settings *settings)
{
	bool error = false;
	
	int index;
	for (index = 1; index < argc && !error; index++)
	{
		const char *keyword = argv[index];
		if (keyword[0] == '-')
		{
			keyword++;
			
			// Check if it's a single-character shortcut, like -I for --input
			char shortcut = '\0';
			if (keyword[0] != 0 && keyword[1] == 0)
			{
				shortcut = keyword[0];
			}
			else
			{
				// Otherwise, treat -foo and --foo the same.
				if (keyword[0] == '-')  keyword++;
			}
			
			// Search for an appropriate handler.
			unsigned handlerIdx;
			bool match = false;
			for (handlerIdx = 0; handlerIdx < sHandlerCount && !match; handlerIdx++)
			{
				const ArgumentHandler *handler = &sHandlers[handlerIdx];
				
				if (shortcut != '\0')  match = (shortcut == handler->shortcut);
				else  match = (strcmp(keyword, handler->keyword) == '\0');
				
				if (match)
				{
					unsigned handlerArgc = argc - index - 1;
					if (handlerArgc < handler->minArgs)
					{
						fprintf(stderr, "Option %s requires %u arguments.\n", argv[index], handler->minArgs);
						error = true;
						break;
					}
					
					int consumedArgs = 0;
					error = !handler->handler(handlerArgc, argv + index + 1, &consumedArgs, settings);
					if (consumedArgs > 0)  index += consumedArgs;
				}
			}
			
			if (!match && !error)
			{
				fprintf(stderr, "Unknown option %s.\n", argv[index]);
				error = true;
			}
		}
		else
		{
			fprintf(stderr, "Expected option identifier, got \"%s\".\n", argv[index]);
			error = true;
		}
	}
	
	if (settings->showHelp)
	{
		ShowHelp(false);
	}
	else if (settings->showVersion)
	{
		ShowVersion();
	}
	
	if (!error && settings->source == NULL)
	{
		if (!settings->showHelp || settings->showVersion)  fprintf(stderr, "No %s specified. Try planettool --help for help.\n", "input");
		error = true;
	}
	if (!error && settings->sink == NULL)
	{
		if (!settings->showHelp || settings->showVersion)  fprintf(stderr, "No %s specified. Try planettool --help for help.\n", "output");
		error = true;
	}
	
	if (settings->size == 0)  settings->size = settings->defaultSize;
	
	return !error;
}


static bool ParseInput(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	const char *inputType = argv[0];
	const char *inputPath = argv[1];
	*consumedArgs = 2;
	
	char shortcut = '\0';
	if (inputType[0] != '\0' && inputType[1] == '\0')
	{
		shortcut = inputType[0];
	}
	
	unsigned i;
	bool match = false;
	for (i = 0; i != sReaderCount; i++)
	{
		const SourceEntry *source = &sReaders[i];
		if (shortcut != '\0')  match = (shortcut == source->keys.shortcut);
		else  match = (strcmp(inputType, source->keys.name) == 0);
		
		if (match)
		{
			settings->source = source;
			settings->sourcePath = inputPath;
			break;
		}
	}
	
	if (!match)
	{
		fprintf(stderr, "Unknown input type \"%s\"\n", inputType);
	}
	
	return match;
}


static bool ParseGenerate(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	const char *name = argv[0];
	*consumedArgs = 1;
	
	char shortcut = '\0';
	if (name[0] != '\0' && name[1] == '\0')
	{
		shortcut = name[0];
	}
	
	unsigned i;
	bool match = false;
	for (i = 0; i != sGeneratorCount; i++)
	{
		const SourceEntry *source = &sGenerators[i];
		if (shortcut != '\0')  match = (shortcut == source->keys.shortcut);
		else  match = (strcmp(name, source->keys.name) == 0);
		
		if (match)
		{
			settings->source = source;
			settings->sourcePath = NULL;
			break;
		}
	}
	
	if (!match)
	{
		fprintf(stderr, "Unknown generator \"%s\"\n", name);
	}
	
	return match;
}


static bool ParseOutput(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	const char *outputType = argv[0];
	const char *outputPath = argv[1];
	*consumedArgs = 2;
	
	char shortcut = '\0';
	if (outputType[0] != '\0' && outputType[1] == '\0')
	{
		shortcut = outputType[0];
	}
	
	unsigned i;
	bool match = false;
	for (i = 0; i != sSinkCount; i++)
	{
		const SinkEntry *sink = &sSinks[i];
		if (shortcut != '\0')  match = (shortcut == sink->keys.shortcut);
		else  match = (strcmp(outputType, sink->keys.name) == 0);
		
		if (match)
		{
			settings->sink = sink;
			settings->sinkPath = outputPath;
			settings->defaultSize = sink->defaultSize;
			break;
		}
	}
	
	if (!match)
	{
		fprintf(stderr, "Unknown output type \"%s\"\n", outputType);
	}
	
	return match;
}


static bool ParseSize(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	char *end = NULL;
	long size = strtoul(argv[0], &end, 10);
	*consumedArgs += 1;
	
	if (*end != '\0' || errno == ERANGE || errno == EINVAL)
	{
		fprintf(stderr, "Could not interpret size argument \"%s\" as a positive integer.\n", argv[0]);
		return false;
	}
	
	if (size < 1)
	{
		fprintf(stderr, "Size may not be zero.\n");
		return false;
	}
	
	settings->size = size;
	return true;
}


static bool ParseFast(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	settings->flags |= kRenderFast;
	return true;
}


static bool ParseJitter(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	settings->flags |= kRenderJitter;
	return true;
}


static bool ParseSixteenBit(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	settings->sixteenBit = true;
	return true;
}


static bool ParseOneFloat(const char *string, double *value)
{
	assert(string != NULL && value != NULL);
	
	char *end = NULL;
	*value = strtof(string, &end);
	if (end != string)  return true;
	else
	{
		fprintf(stderr, "Could not interpret argument \"%s\" as a number.\n", string);
		return false;
	}
}


static bool ParseRotate(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	*consumedArgs += 3;
	
	double rx, ry, rz;
	if (!ParseOneFloat(argv[0], &rx))  return false;
	if (!ParseOneFloat(argv[1], &ry))  return false;
	if (!ParseOneFloat(argv[2], &rz))  return false;
	
	OOMatrix rotation = kIdentityMatrix;
	rotation = OOMatrixRotateX(rotation, rx * kDegToRad);
	rotation = OOMatrixRotateZ(rotation, rz * kDegToRad);
	
	// Y axis (planetary axis) rotation is deliberately applied last. This makes it rotate around the *original* axis of rotation.
	rotation = OOMatrixRotateY(rotation, ry * kDegToRad);
	
	settings->transform = OOMatrixMultiply(settings->transform, rotation);
	
	return true;
}


static bool ParseCosBlur(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	*consumedArgs += 2;
	if (!ParseOneFloat(argv[0], &settings->cosBlurBackFactor))  return false;
	if (!ParseOneFloat(argv[1], &settings->cosBlurFrontFactor))  return false;
	settings->cosBlur = true;
	return true;
}


static bool ParseFlip(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	settings->transform = OOMatrixScale(settings->transform, -1, 1, 1);
	return true;
}


static bool ParseHelp(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	settings->showHelp = true;
	return true;
}


static bool ParseVersion(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	settings->showVersion = true;
	return true;
}


static bool ParseQuiet(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	settings->quiet = true;
	return true;
}


static void ShowHelp(bool showHidden)
{
	printf("Planettool version %s\nplanettool", PLANETTOOL_VERSION);
	
	// ACT I: the Synopsis. Dramatis personae: a gaggle of Shortcuts.
	unsigned i;
	const ArgumentHandler *handler = NULL;
	for (i = 0; i < sHandlerCount; i++)
	{
		handler = &sHandlers[i];
		if (handler->hidden && !showHidden)  continue;
		
		printf(" ");
		if (!handler->required)  printf("[");
		if (handler->shortcut != '\0')
		{
			printf("-%c", handler->shortcut);
		}
		else
		{
			printf("--%s", handler->keyword);
		}
		if (handler->paramNames != NULL)
		{
			printf(" %s", handler->paramNames);
		}
		if (!handler->required)  printf("]");
	}
	printf("\n\n");
	
	// ACT II: the Descriptions. Dramatis personae: several Description Strings, divers DescTypes; also Peasblossom, a Faerie.
	for (i = 0; i < sHandlerCount; i++)
	{
		handler = &sHandlers[i];
		if (handler->hidden && !showHidden)  continue;
		
		#define LABEL_SIZE 16
		char label[LABEL_SIZE];
		if (handler->shortcut != '\0')
		{
			snprintf(label, LABEL_SIZE, "--%s, -%c", handler->keyword, handler->shortcut);
		}
		else
		{
			snprintf(label, LABEL_SIZE, "--%s", handler->keyword);
		}
		
		printf("%*s:  %s", LABEL_SIZE, label, handler->description);
		
		if (handler->descTypes != NULL)
		{
			unsigned j;
			for (j = 0; j < handler->descTypeCount; j++)
			{
				FilterEntryBase *entry = (FilterEntryBase *)((char *)handler->descTypes + handler->descStride * j);
				if (j != 0)  printf(", ");
				printf("\"%s\" (%c)", entry->name, entry->shortcut);
			}
		}
		printf("\n");
	}
	
	// ACT III: the Expository Text.
	printf("\n"
		// "=========|=========|=========|=========|=========|=========|=========|=========|\n"
		   "Planettool reads a texture map from an input file (in PNG format) or a generator\n"
		   "function, and writes it to an output file (in PNG format). In so doing, it may\n"
		   "change the projection and scale of the map, and may rotate it around the planet.\n"
		   "\n"
		   "Planettool's design is geared for flexibility and quality. As a side effect, it\n"
		   "is extremely slow. Don't be alarmed if it takes several minutes to do anything.\n"
		   "\n"
		   "EXAMPLES:\n"
		   "planettool --output cube \"cubemap.png\" --input latlong \"original.png\" --size 512\n"
		   "    Reads original.png, treated as a latitude-longitude map, and remaps it to a\n"
		   "    cube map with a side length of 512 pixels.\n"
		   "\n"
		   "planettool -o c cubemap.png -i l original.png -S 512\n"
		   "    Same as above, only less legible for extra geek cred.\n"
		   "\n"
		   "planettool -o cube grid.png --generator grid1 --fast --rotate 30 0 0 --flip\n"
		   "    Generate a grid, tilted 30 degrees and projected onto an inside-out cube map\n"
		   "    at low quality.\n"
		   "\n"
		   "THE PROJECTION TYPES:\n"
		// "=========|=========|=========|=========|=========|=========|=========|=========|\n"
		   "    latlong: Equirectangular projection. In this format, the intervals between\n"
		   "             pixels are constant steps of latitude and longitude. This is\n"
		   "             conceptually simple, but inefficent; lots of pixels are crammed\n"
		   "             together tightly near the poles.\n"
		   "       cube: The surface is divided into six equal areas, which are projected\n"
		   "             onto squares. These are then stacked vertically, in the following\n"
		   "             order:\n"
		   "             +x, -x, +y, -y, +z, -z.\n"
		   "      cubex: The same projection as cube, but the squares are rearranged into a\n"
		   "             more human-friendly layout (which can be printed and folded into a\n"
		   "             cube if you're bored).\n"
		   "   mercator: An angle-preserving map projection. The traditional projection for\n"
		   "             sailors and people who can't be bothered to choose a more approp-\n"
		   "             riate projection for whatever they're doing. Entirely unsuitable\n"
		   "             for texturing, but possibly useful if you want a wall map.\n"
		   "gall-peters: A variant of the cylindric equal-area projection, in which the\n"
		   "             proportions between different areas are preserved.\n"
		   "\n"
		   "THE GENERATORS:\n"
		   "      grid1: A grid with lines spaced ten degrees apart. Longitude lines are\n"
		   "             green in the northern hemisphere, blue in the south. Latitude lines\n"
		   "             are red in the western hemisphere, teal in the east.\n"
		// "=========|=========|=========|=========|=========|=========|=========|=========|\n"
		   );
}


static void ShowVersion(void)
{
	printf("%s\n", PLANETTOOL_VERSION);
}


static bool PrintProgress(size_t numerator, size_t denominator, void *context)
{
	size_t percentage = (numerator * 100) / denominator;
	unsigned *last = context;
	
	if (percentage > *last)
	{
		printf("\r%zu %%", percentage);
		fflush(stdout);
		*last = percentage;
	}
	
	return true;
}
