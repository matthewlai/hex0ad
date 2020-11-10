ifndef CXX
	CXX=g++
endif

CXXFLAGS_BASE = -Wall -Wextra -Wno-unused-function -std=gnu++17 -march=native -ffast-math

# we will then extend this one with optimization flags
CXXFLAGS:= $(CXXFLAGS_BASE)

CXXFLAGS_DEP = -std=gnu++17

LDFLAGS=-lm -lSDL2 -lGL

CXXFILES := $(wildcard *.cpp)

INCLUDES=-I.

BIN=hex0ad

OBJS := $(CXXFILES:%.cpp=obj/%.o)
DEPS := $(CXXFILES:%.cpp=dep/%.d)

WEB_FILES=$(BIN).js $(BIN).wasm $(BIN).html $(BIN).data

ifeq ($(V),0)
	Q = @
else
	Q =
endif

ifeq ($(OS),Windows_NT)
	# mingw builds crash with LTO
	CXXFLAGS := $(filter-out -flto,$(CXXFLAGS))
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		# OSX needs workaround for AVX, and LTO is broken
		# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47785
		CXXFLAGS += -Wa,-q
		CXXFLAGS := $(filter-out -flto,$(CXXFLAGS))
	endif
endif

.PHONY: clean web

default: $(BIN)

dep/%.d: %.cpp
	$(Q) $(CXX) $(CXXFLAGS_DEP) $(INCLUDES) $< -MM -MT $(@:dep/%.d=obj/%.o) > $@
	
obj/%.o: %.cpp
	$(Q) $(CXX) $(CXXFLAGS) $(INCLUDES) -c $(@:obj/%.o=%.cpp) -o $@

$(BIN): $(OBJS)
	$(Q) $(CXX) $(CXXFLAGS) $(OBJS) -o $(BIN) $(LDFLAGS)
	
clean:
	-$(Q) rm -f $(DEPS) $(OBJS) $(BIN) $(WEB_FILES)

web:
	$(Q) emcc $(CXXFILES) -std=gnu++17 -O2 --emrun -s WASM=1 -s USE_SDL=2 -s \
	    USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s USE_WEBGL2=1 \
	    --preload-file assets --preload-file shaders -o $(BIN).html

no_deps = 
ifeq ($(MAKECMDGOALS),clean)
	no_deps = yes
endif

ifeq ($(MAKECMDGOALS),web)
	no_deps = yes
endif

ifndef no_deps
	-include $(DEPS)
endif

