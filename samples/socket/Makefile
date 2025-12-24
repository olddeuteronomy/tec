###############################################################################
# Time-stamp: <Last changed 2025-12-24 15:51:12 by magnolia>
#
# To force rebuild with `g++' (default):
#    make -k -B -j4
#
# To force rebuild with `clang++':
#    make -k -B -j4 CLANG=1
#
# To force rebuild with `mingw++':
#    make -k -B -j4 MINGW=1
#
# To build a release (-O2, no debug info)
#    make -k -B -j4 REL=1
#
###############################################################################

# OS
ifeq ($(OS),Windows_NT)
	detected_OS := Windows
	EXE_SUFFIX := .exe
else
	detected_OS := $(uname -o)
endif

# Default dependencies
all_headers := $(wildcard ../../*.hpp)

# Compilers
ifdef CLANG
# clang++ is optional
	CXX = clang++
	TARGET_SUFFIX = _clang$(EXE_SUFFIX)
else
ifdef MINGW
# mingw++ compiler is optional
	CXX = x86_64-w64-mingw32-g++
	TARGET_SUFFIX = _win$(EXE_SUFFIX)
else
# gcc++ is default
	GCC = 1
	CXX = g++
	TARGET_SUFFIX = _gcc$(EXE_SUFFIX)
endif
endif


# Platform dependent commands
ifeq ($(detected_OS),Windows)
	MKDIR_P = MKDIR
else
	MKDIR_P = mkdir -p
endif

# Compilation parameters
CPPFLAGS := -std=c++17 -Wall -pthread
ifdef REL
	CPPFLAGS += -O2
else
	CPPFLAGS += -O0 -g
endif

INCLUDES := -I../../..
OUTDIR := out

# Enable tracing in release build.
# Add -v is for verbose output (to list all include paths etc)
DEFS := -D_TEC_TRACE_ON

# Just for fun - colorize compiler output
ifdef GCC
	EXTRAS := -fdiagnostics-color=always
else
ifdef CLANG
	EXTRAS := -fcolor-diagnostics
endif
endif

# Create output directory if needed
ifeq ($(detected_OS),Windows)
$(shell if not exist $(OUTDIR) (${MKDIR_P} $(OUTDIR)))
else
$(info $(shell ${MKDIR_P} $(OUTDIR)))
endif


# Build the targets
all: tcp_client tcp_server tcp_client_nd tcp_server_nd

tcp_client: tcp_client.cpp
	$(CXX) $(EXTRAS) $(CPPFLAGS) $(INCLUDES) $(DEFS) $^ -o $(OUTDIR)/$@$(TARGET_SUFFIX)

tcp_server: tcp_server.cpp
	$(CXX) $(EXTRAS) $(CPPFLAGS) $(INCLUDES) $(DEFS) $^ -o $(OUTDIR)/$@$(TARGET_SUFFIX)

tcp_client_nd: tcp_client_nd.cpp
	$(CXX) $(EXTRAS) $(CPPFLAGS) $(INCLUDES) $(DEFS) $^ -lz -o $(OUTDIR)/$@$(TARGET_SUFFIX)

tcp_server_nd: tcp_server_nd.cpp
	$(CXX) $(EXTRAS) $(CPPFLAGS) $(INCLUDES) $(DEFS) $^ -lz -o $(OUTDIR)/$@$(TARGET_SUFFIX)
