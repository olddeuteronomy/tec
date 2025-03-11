###############################################################################
# To force rebuild with `clang++' (default):
#    make [-k] -B TEST=worker|server
# To force rebuild with `g++':
#    make [-k] -B GCC=1 TEST=worker|server
###############################################################################

ifndef TEST
$(error TEST is not defined! Use make -k -B TEST=testname)
endif

# OS
ifeq ($(OS),Windows_NT)
	detected_OS := Windows
	EXE_SUFFIX := .exe
else
	detected_OS := $(uname -o)
endif

# Compilers
# gcc is optional
ifdef GCC
	CC = g++
	TARGET_SUFFIX = _gcc$(EXE_SUFFIX)
else
# clang is default
	CLANG = 1
	CC = clang++
	TARGET_SUFFIX = _clang$(EXE_SUFFIX)
endif


# Platform dependent commands
ifeq ($(detected_OS),Windows)
	MKDIR_P = MKDIR
else
	MKDIR_P = mkdir -p
endif

# Compilation parameters
TARGET = $(TEST)$(TARGET_SUFFIX)
CPPFLAGS = -std=c++17 -Wall -pthread -O2 -g
INCLUDES = -I../../..
OUTDIR = out

# Enable tracing in release build.
# Add -v is for verbose output (to list all include paths etc)
DEFS = -D_TEC_TRACE_ON

# Just for fun - colorize compiler output
ifdef GCC
	EXTRAS = -fdiagnostics-color=always
else
ifdef CLANG
	EXTRAS = -fcolor-diagnostics
endif
endif

# Create output directory if needed
ifeq ($(detected_OS),Windows)
$(shell if not exist $(OUTDIR) (${MKDIR_P} $(OUTDIR)))
else
$(info $(shell ${MKDIR_P} $(OUTDIR)))
endif


# Compile the target
$(OUTDIR)/$(TARGET): test_$(TEST).cpp
	$(CC) $(EXTRAS) $(CPPFLAGS) $(INCLUDES) $(DEFS) test_$(TEST).cpp -o $(OUTDIR)/$(TARGET)

all: $(OUTDIR)/$(TARGET)
