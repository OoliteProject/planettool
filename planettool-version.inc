PLANETTOOL_VERSION = 1.0.1
ifeq ($(sse2),yes)
	PLANETTOOL_VERSION += (SSE2 optimized)
endif
