###############################################################################
# To force rebuild with `clang++' (default):
#    make [-k] -B
# To force rebuild with `g++':
#    make [-k] -B GCC=1
###############################################################################
TESTNAME := worker

###############################################################################
#                         DO NOT EDIT BELOW THIS LINE                         #
###############################################################################

# OS
ifeq ($(OS),Windows_NT)
    detected_OS := Windows
	EXE_SUFFIX := .exe
else
	detected_OS := $(uname -o)
endif

# Compilers
ifdef GCC
CC = g++
TARGET_SUFFIX = _gcc$(EXE_SUFFIX)
else
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
TARGET = $(TESTNAME)$(TARGET_SUFFIX)
CPPFLAGS = -std=c++14 -Wall -pthread -O2
INCLUDES = -I../../..
OUTDIR = out

# Enable tracing in release build
# -v is for verbose output (to list all include paths etc)
# DEFS = -D_TEC_TRACE_RELEASE -v
DEFS = -D_TEC_TRACE_RELEASE

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
$(OUTDIR)/$(TARGET): test_$(TESTNAME).cpp
	$(CC) $(EXTRAS) $(CPPFLAGS) $(INCLUDES) $(DEFS) test_$(TESTNAME).cpp -o $(OUTDIR)/$(TARGET)

all: $(OUTDIR)/$(TARGET)
