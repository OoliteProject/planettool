EXECUTABLE = planettool
include planettool-version.inc


CC = gcc
LD = $(CC)

CFLAGS = -I$(FPM_PATH) -I$(OOMATHS_PATH) -DOOMATHS_STANDALONE=1 -DPLANETTOOL_VERSION="\"$(PLANETTOOL_VERSION)\"" -g -ffast-math
LDFLAGS = -lpng -lz


ifndef scheduler
	scheduler = PThreadScheduler
endif

ifeq ($(scheduler),PThreadScheduler)
	LDFLAGS += -lpthread
endif


ifneq ($(debug),yes)
	CFLAGS += -O3 -ftree-vectorize -funroll-loops
endif


ifeq ($(asprintf),no)
	CFLAGS += -DNO_ASPRINTF
endif


ifndef srcdir
	srcdir = .
endif

FPM_PATH = $(srcdir)/FloatPixMap/

ifndef OOMATHS_PATH
	ifneq ($(inrepo),yes)
		OOMATHS_PATH = $(srcdir)/Oolite/
	else
		OOMATHS_PATH = $(srcdir)/../../src/Core/
	endif
endif
		


vpath %.h $(FPM_PATH):$(OOMATHS_PATH)
vpath %.c $(FPM_PATH)
vpath %.m $(OOMATHS_PATH)

.SUFFIXES: .m


CORE_OBJECTS = main.o SphericalPixelSource.o ReadLatLong.o ReadCube.o LatLongGridGenerator.o RenderToLatLong.o RenderToCube.o RenderToMercator.o RenderToGallPeters.o MatrixTransformer.o CosineBlurFilter.o $(scheduler).o PTPowerManagement.o
FPM_OBJECTS = FloatPixMap.o FPMGamma.o FPMImageOperations.o FPMPNG.o FPMQuantize.o FPMRaw.o
OOMATHS_OBJECTS = OOFastArithmetic.o OOMatrix.o OOQuaternion.o OOVector.o

OBJECTS = $(CORE_OBJECTS) $(FPM_OBJECTS) $(OOMATHS_OBJECTS)


planettool: $(OBJECTS)
	$(LD) -o $(EXECUTABLE) $(LDFLAGS) $(OBJECTS)


# Rule to compile Objective-C maths files as C.
.m.o:
	$(CC) -c -x c $(CFLAGS) -o $@ $<


# Core dependencies.
SphericalPixelSource.h: FloatPixMap.h
LatLongGridGenerator.h ReadLatLong.h ReadCube.h MatrixTransformer.h RenderToLatLong.h RenderToCube.h PlanetToolScheduler.h: SphericalPixelSource.h

main.o: FPMPNG.h LatLongGridGenerator.h ReadLatLong.h MatrixTransformer.h RenderToLatLong.h RenderToCube.h

SphericalPixelSource.o: SphericalPixelSource.h
ReadLatLong.o: ReadLatLong.h FPMImageOperations.h PlanetToolScheduler.h
ReadCube.o: ReadCube.h FPMImageOperations.h PlanetToolScheduler.h
RenderToLatLong.o: RenderToLatLong.h FPMImageOperations.h
RenderToMercator.o: RenderToMercator.h FPMImageOperations.h
RenderToGallPeters.o: RenderToGallPeters.h FPMImageOperations.h
LatLongGridGenerator.o: LatLongGridGenerator.h
RenderToCube.o: RenderToCube.h FPMImageOperations.h
MatrixTransformer.o: MatrixTransformer.h
CosineBlurFilter.o: CosineBlurFilter.h
SerialScheduler.o PListScheduler.o: PlanetToolScheduler.h PTPowerManagement.h
PTPowerManagement.o: PTPowerManagement.h


# FloatPixMap dependencies.
FloatPixMap.h FPMVector.h: FPMBasics.h
FPMPNG.h FPMGamma.h FPMImageOperations.h FPMQuantize.h FPMRaw.h : FloatPixMap.h
FPMPNG.h : FPMGamma.h FPMQuantize.h

FloatPixMap.o: FloatPixMap.h
FPMGamma.o: FPMGamma.h FPMImageOperations.h
FPMImageOperations.o: FPMImageOperations.h FPMVector.h
FPMPNG.o: FPMPNG.h
FPMQuantize.o: FPMQuantize.h FPMImageOperations.h
FPMRaw.o: FPMRaw.h FPMImageOperations.h



.PHONY: clean
clean:
	-rm -f *.o $(EXECUTABLE)
