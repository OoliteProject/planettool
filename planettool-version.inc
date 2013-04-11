PLANETTOOL_VERSION = 0.4.4
ifeq ($(sse2),yes)
	PLANETTOOL_VERSION += (SSE2 optimized)
endif
