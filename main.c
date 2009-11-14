#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "FPMPNG.h"
#include "SphericalPixelSource.h"

// Sources
#include "LatLongGridGenerator.h"
#include "ReadLatLong.h"
#include "MatrixTransformer.h"

// Sinks
#include "RenderToLatLong.h"
#include "RenderToCube.h"


static void ErrorHandler(const char *message, bool isError);


typedef struct
{
	const char						*name;
	const char						shortcut;
} FilterEntryBase;


typedef struct
{
	FilterEntryBase					keys;
	SphericalPixelSourceFunction	source;
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
	bool							showHelp;
	bool							showVersion;
	bool							quiet;
	const char						*sourcePath;
	const char						*sinkPath;
} Settings;


static bool InterpretArguments(int argc, const char * argv[], Settings *settings);


static void PrintProgress(size_t numerator, size_t denominator, void *context);
static void SuppressProgress(size_t numerator, size_t denominator, void *context);


int main (int argc, const char * argv[])
{
	FPMInit();
	srandom(time(NULL));
	
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
		sourcePM = FPMCreateWithPNG(settings.sourcePath, kFPMGammaLinear, NULL);
		
		if (sourcePM == NULL)
		{
			fprintf(stderr, "Could not load %s\n", argv[1]);
			return EXIT_FAILURE;
		}
	}
	
	// Run source constructor.
	void *sourceContext = NULL;
	if (settings.source->constructor != NULL)
	{
		if (!settings.source->constructor(sourcePM, settings.flags, &sourceContext))
		{
			return EXIT_FAILURE;
		}
	}
	SphericalPixelSourceFunction source = settings.source->source;
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
	
	// Render.
	ProgressCallbackFunction progressCB;
	unsigned progressCtxt = 0;
	if (!settings.quiet)
	{
		printf("Rendering...\n");
		progressCB = PrintProgress;
	}
	else
	{
		progressCB = SuppressProgress;
	}
	
	FloatPixMapRef resultPM = settings.sink->sink(settings.size, settings.flags, source, sourceContext, progressCB, &progressCtxt);
	FPMRelease(&sourcePM);
	if (resultPM == NULL)
	{
		return EXIT_FAILURE;
	}
	
	// Destructimacate.
	if (destructor != NULL)  destructor(sourceContext);
	
	// Write output.
	if (!settings.quiet)  printf("Writing...\n");
	if (!FPMWritePNG(resultPM, "/tmp/planettool-test.png", kFPMWritePNGDither, kFPMGammaLinear, kFPMGammaSRGB, ErrorHandler))
	{
		return EXIT_FAILURE;
	}
	
	return 0;
	
	
#if 0
	bool fast = false;
	bool jitter = true;
	unsigned size = 1024;
	
	RenderFlags flags = 0;
	if (fast) flags |= kRenderFast;
	if (jitter) flags |= kRenderJitter;
	
	FloatPixMapRef pm = NULL;
	if (argc < 2)
	{
		printf("Rendering...\n");
		pm = RenderToCube(size, flags, LatLongGridGenerator, NULL, NULL, NULL);
	}
	else
	{
		void *context = NULL;
#if 0
		printf("Reading...\n");
		FloatPixMapRef source = FPMCreateWithPNG(argv[1], kFPMGammaLinear, NULL);
		if (source == NULL)
		{
			fprintf(stderr, "Could not load %s\n", argv[1]);
			return EXIT_FAILURE;
		}
		
		if (!ReadLatLongConstructor(source, flags, &context))
		{
			fprintf(stderr, "Could not set up rendering context.\n");
			return EXIT_FAILURE;
		}
#define SOURCE ReadLatLong
#define DESTRUCTOR ReadLatLongDestructor
#else
#define SOURCE LatLongGridGenerator
#define DESTRUCTOR NullDestructor
#endif
		
#if 0
		printf("Rendering...\n");
		pm = RenderToCubeCross(size, flags, SOURCE, context, NULL, NULL);
		
		DESTRUCTOR(context);
#else
		OOMatrix matrix = kIdentityMatrix;
		/*
		matrix = OOMatrixRotateZ(matrix, 180 * kDegToRad);
		matrix = OOMatrixRotateX(matrix, 20.0 * kDegToRad);
		 matrix = OOMatrixRotateZ(matrix, 40 * kDegToRad);*/
		matrix = OOMatrixRotateY(matrix, 45.1 * kDegToRad);
		matrix = OOMatrixRotateX(matrix, 54.8 * kDegToRad);
		if (!MatrixTransformerSetUp(SOURCE, DESTRUCTOR, context, matrix, &context))
		{
			fprintf(stderr, "Could not set up transformer context.\n");
			return EXIT_FAILURE;
		}
		
		printf("Rendering...\n");
		pm = RenderToCubeCross(size, flags, MatrixTransformer, context, NULL, NULL);
		
		MatrixTransformerDestructor(context);
#endif
	}
	
	if (pm != NULL)
	{
		printf("Writing...\n");
		FPMWritePNG(pm, "/tmp/planettool-test.png", kFPMWritePNGDither, kFPMGammaLinear, kFPMGammaSRGB, ErrorHandler);
		return 0;
	}
	else
	{
		fprintf(stderr, "Rendering failed.\n");
		return EXIT_FAILURE;
	}
#endif
}


static void ErrorHandler(const char *message, bool isError)
{
	fprintf(stderr, "%s: %s\n", isError ? "ERROR" : "WARNING", message);
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
static bool ParseRotate(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseHelp(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseVersion(int argc, const char *argv[], int *consumedArgs, Settings *settings);
static bool ParseQuiet(int argc, const char *argv[], int *consumedArgs, Settings *settings);


static const SourceEntry sGenerators[] =
{
	{ "grid1",				'g',	LatLongGridGenerator, NULL, NULL }
};

enum { sGeneratorCount = sizeof sGenerators / sizeof sGenerators[0] };


static const SourceEntry sReaders[] =
{
	{ "latlong",			'l',	ReadLatLong, ReadLatLongConstructor, ReadLatLongDestructor },
};

enum { sReaderCount = sizeof sReaders / sizeof sReaders[0] };


static const SinkEntry sSinks[] =
{
	{ "latlong",			'l',	RenderToLatLong, 2048 },
	{ "cube",				'c',	RenderToCube, 1024 },
	{ "cubex",				'x',	RenderToCubeCross, 1024 }
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
	char					*description;
	FilterEntryBase			*descTypes;
	off_t					descStride;
	unsigned				descTypeCount;
} ArgumentHandler;


static const ArgumentHandler sHandlers[] =
{
	{
		"output",		'o', 2, ParseOutput,
		"<outType> <outFile>", true, "Type and name of output file. Type must be one of: ", (FilterEntryBase *)sSinks, sizeof(*sSinks), sSinkCount
	},
	{
		"input",		'i', 2, ParseInput,
		"<inType> <inFile>",  false, "Type and name of input file. Type must be one of: ", (FilterEntryBase *)sReaders, sizeof(*sReaders), sReaderCount
	},
	{
		"generate",		'g', 1, ParseGenerate,
		"<generator>", false, "Type and name of generator. Type must be one of: ", (FilterEntryBase *)sGenerators, sizeof(*sGenerators), sGeneratorCount
	},
	{
		"size",			'S', 1, ParseSize,
		"<size>", false, "Size of output, in pixels. Interpretation depends on output type.", NULL, 0, 0
	},
	{
		"fast",			'F', 0, ParseFast,
		NULL, false, "Use faster, low-quality rendering.", NULL, 0, 0
	},
	{
		"jitter",		'J', 0, ParseJitter,
		NULL, false, "Use jittering for slower, slightly noisy rendering which may look better in some cases.", NULL, 0, 0
	},
	{
		"rotate",		'R', 3, ParseRotate,
		"<ry> <rx> <rz>", false, "Unimplemented.", NULL, 0, 0
	},
	{
		"help",			'H', 0, ParseHelp,
		NULL, false, "Show this helpful help.", NULL, 0, 0
	},
	{
		"version",		'V', 0, ParseVersion,
		NULL, false, "Show version number.", NULL, 0, 0
	},
	{
		"quiet",		'Q', 0, ParseQuiet,
		NULL, false, "Don't print progress information.", NULL, 0, 0
	},
};

static const unsigned sHandlerCount = sizeof sHandlers / sizeof sHandlers[0];


static void ShowHelp(void);
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
			int handlerIdx;
			bool match = false;
			for (handlerIdx = 0; handlerIdx < sHandlerCount && !match; handlerIdx++)
			{
				const ArgumentHandler *handler = &sHandlers[handlerIdx];
				
				if (shortcut != '\0')  match = (shortcut == handler->shortcut);
				else  match = (strcmp(keyword, handler->keyword) == '\0');
				
				if (match)
				{
					int handlerArgc = argc - index - 1;
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
		ShowHelp();
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
	for (i = 0; i != sReaderCount; i++)
	{
		const SourceEntry *source = &sReaders[i];
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


static bool ParseRotate(int argc, const char *argv[], int *consumedArgs, Settings *settings)
{
	*consumedArgs += 3;
	
	// FIXME: set up matrix.
	
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


static void ShowHelp(void)
{
//	printf("Pretend this is helpful.\n");
	printf("Planettool version <undefined>\nplanettool");
	
	// ACT I: the Synopsis. Dramatis personae: a gaggle of Shortcuts.
	unsigned i;
	const ArgumentHandler *handler = NULL;
	for (i = 0; i < sHandlerCount; i++)
	{
		handler = &sHandlers[i];
		
		printf(" ");
		if (!handler->required)  printf("[");
		printf("-%c", handler->shortcut);
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
		
		#define LABEL_SIZE 16
		char label[LABEL_SIZE];
		snprintf(label, LABEL_SIZE, "--%s, -%c", handler->keyword, handler->shortcut);
		
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
		   "Planettool reads a global map from an input file (in PNG format) or a generator\n"
		   "function, and writes it to an output file (in PNG format). In so doing, it may\n"
		   "change the projection and scale of the map, and may rotate it around the planet.\n"
		   "\n"
		   "Planettool's design is geared for flexibility and quality. As a side effect, it\n"
		   "is extremely slow. Don't be alarmed if it takes several minutes to do anything.\n"
		   "\n"
		   "EXAMPLES:\n"
		   "planettool --outout cube \"cubemap.png\" --input latlong \"original.png\" --size 512\n"
		   "    Reads original.png, treated as a latitude-longitude map, and remaps it to a\n"
		   "    cube map with a side length of 512 pixels.\n"
		   "\n"
		   "planettool -o c cubemap.png -i l original.png -S 512\n"
		   "    Same as above, only less legible for extra geek cred.\n"
		   "\n"
		   "planettool -o cube grid.png --generator grid1 --fast --rotate 0 30 0\n"
		   "    Generate a grid, tilted 30 degrees and projected onto a cube map at low\n"
		   "    quality.\n"
		   "\n"
		   "THE PROJECTION TYPES:\n"
		   "latlong: in this format, the intervals between pixels are constant steps of\n"
		   "         latitude and longitude. This is conceptually simple, but inefficent;\n"
		   "         lots of pixels are crammed together tightly near the poles.\n"
		   "   cube: The surface is divided into six equal areas, which are projected onto\n"
		   "         squares. These are then stacked vertically, in the following order:\n"
		   "         +x, -x, +y, -y, +z, -z.\n"
		   "  cubex: The same projection as cube, but the squares are rearranged into a more\n"
		   "         human-friendly layout (which can be printed and folded into a cube if\n"
		   "         you're bored).\n");
		// "=========|=========|=========|=========|=========|=========|=========|=========|\n"
}


static void ShowVersion(void)
{
	printf("Pretend we have a version number.\n");
}


static void PrintProgress(size_t numerator, size_t denominator, void *context)
{
	unsigned percentage = numerator * 100 / denominator;
	unsigned *last = context;
	
	if (percentage > *last)
	{
		printf("%u %%\n", percentage);
		*last = percentage;
	}
}


static void SuppressProgress(size_t numerator, size_t denominator, void *context)
{
	
}
