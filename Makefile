ifndef CXX
	CXX=g++
endif

CXXFLAGS_BASE = -Wall -Wextra -Wno-unused-function -std=gnu++17 -march=native -ffast-math -Wno-unused-const-variable

# we will then extend this one with optimization flags
CXXFLAGS:= $(CXXFLAGS_BASE)

CXXFLAGS_DEP = -std=gnu++17

LDFLAGS=-lm

CXXFILES := $(wildcard *.cpp)
CXXFILES += third_party/tinyxml2/tinyxml2.cpp

INCLUDES=-I. -Ithird_party

BIN=hex0ad

OBJS := $(CXXFILES:%.cpp=obj/%.o)
DEPS := $(CXXFILES:%.cpp=dep/%.d)

WEB_FILES=$(BIN).js $(BIN).wasm $(BIN).html $(BIN).data

ifeq ($(V),0)
	Q = @
else
	Q =
endif

UNAME_S := $(shell uname -s)

# SDL dependencies.
CXXFLAGS += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs)

CXXFLAGS_DEP += $(shell sdl2-config --cflags)

# OpenGL dependencies.
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -framework OpenGL
else
	LDFLAGS += -lGL
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

