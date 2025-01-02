#
# Copyright 2015 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# Portions Copyright (c) 2022-2024 The Emacs Cat
#

###############################################################################
# To force rebuild with `clang++' (default):
#    make [-k] -B
# To force rebuild with `g++':
#    make [-k] -B GCC=1
###############################################################################

# OS
ifeq ($(OS),Windows_NT)
detected_OS := Windows
EXE_SUFFIX := .exe
else
detected_OS := $(uname -o)
endif

ifdef DEBUG
EXTRA_FLAGS := -ggdb -D_DEBUG=1
else
EXTRA_FLAGS := -O2
endif

# Just for fun - colorize compiler output
ifdef GCC
EXTRA_FLAGS += -fdiagnostics-color=always
else
ifdef CLANG
EXTRA_FLAGS += -fcolor-diagnostics
endif
endif

ifdef GCC
CXX = g++
SUFFIX = _gcc
else
CXX = clang++
SUFFIX = _clang
endif

# Create output directory if needed
ifeq ($(detected_OS),Windows)
MKDIR_P = MKDIR
else
MKDIR_P = mkdir -p
endif

OUTDIR = ./out

ifeq ($(detected_OS),Windows)
$(shell if not exist $(OUTDIR) (${MKDIR_P} $(OUTDIR)))
else
$(info $(shell ${MKDIR_P} $(OUTDIR)))
endif


TEC_CXXFLAGS := -I$(HOME)/lib/grpc/include -I$(HOME)/workspace/devel -D_TEC_TRACE_ON
HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++17 $(EXTRA_FLAGS) $(TEC_CXXFLAGS)

# System-dependant libraries
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs --static protobuf grpc++`\
           -pthread\
           -lgrpc++_reflection\
           -ldl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs --static protobuf grpc++`\
           -pthread\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl -lsystemd
endif


########################################################################
# Buffer Protocol compiler and plugins
########################################################################

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = .

vpath %.proto $(PROTOS_PATH)


########################################################################
#                           Build
########################################################################

# all: system-check greeter_server greeter_client
all: greeter_server greeter_client

greeter_server: helloworld.pb.o helloworld.grpc.pb.o greeter_server.o
	$(CXX) $^ $(LDFLAGS) -o $(OUTDIR)/$@$(SUFFIX)

greeter_client: helloworld.pb.o helloworld.grpc.pb.o greeter_client.o
	$(CXX) $^ $(LDFLAGS) -o $(OUTDIR)/$@$(SUFFIX)


########################################################################
#                Protocol Buffer compiler
########################################################################

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

########################################################################
#                       Clean up
########################################################################
clean:
	rm -f *.o *.pb.cc *.pb.h out/*
