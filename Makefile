# Build
# Mac/Linux: make
# Emscripten: emmake make
# Emscripten build and run in browser: emmake make run_web

# Defaults.
ifndef CXX
	CXX=g++
endif

ifeq ($(V),0)
	Q = @
else
	Q =
endif

# Files and paths.
# All binaries.
BINS=bin/hex0ad bin/make_assets

WEB_BIN=hex0ad

CXXFILES := $(wildcard src/*.cpp)
CXXFILES += third_party/tinyxml2/tinyxml2.cpp
CXXFILES += third_party/lodepng/lodepng.cpp

WEB_FILES=$(WEB_BIN).js $(WEB_BIN).wasm $(WEB_BIN).html $(WEB_BIN).data

FLATBUFFER_SCHEMAS := $(wildcard fb/*.fbs)

OBJS := $(CXXFILES:%.cpp=obj/%.o) 
DEPS := $(CXXFILES:%.cpp=dep/%.d)
BIN_OBJS = $(BINS:bin/%=obj/src/%.o)

INCLUDES=-Iinc -Ithird_party -Ifb

# Platforms.
DEFAULT_TARGETS = $(BINS)

# Flags.
ifeq ($(notdir $(CXX)), em++)
	CXXFLAGS = $(INCLUDES) -std=gnu++17 -s USE_SDL=2
	LDFLAGS = --emrun -s WASM=1 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]'
	LDFLAGS += -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2
	LDFLAGS += --preload-file assets --preload-file shaders
	DEFAULT_TARGETS = $(WEB_BIN).html
else
	CXXFLAGS = -Wall -Wextra -Wno-unused-function -std=gnu++17 -march=native -ffast-math -Wno-unused-const-variable -g
	LDFLAGS = -lm

	ifeq ($(OS),Windows_NT)
		LDFLAGS += -lmingw32 -lSDL2main -lSDL2
		LDFLAGS += -lopengl32 -lglew32
	else
		# For UNIX-like platforms.
		# SDL dependencies.
		CXXFLAGS += $(shell sdl2-config --cflags)
		CXXFLAGS_DEP = -std=gnu++17 $(shell sdl2-config --cflags)
		LDFLAGS = $(shell sdl2-config --libs)

		# OpenGL dependencies.
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Darwin)
			LDFLAGS += -framework OpenGL
		else
			LDFLAGS += -lGL
		endif
	endif
endif

.PHONY: clean run_web

default: $(DEFAULT_TARGETS)

fb/%_generated.h: fb/%.fbs
	$(Q) flatc --cpp -o fb/ $<

dep/%.d: %.cpp
	$(Q) $(CXX) $(CXXFLAGS_DEP) $(INCLUDES) $< -MM -MT $(@:dep/%.d=obj/%.o) > $@
	
obj/%.o: %.cpp
	$(Q) $(CXX) $(CXXFLAGS) $(INCLUDES) -c $(@:obj/%.o=%.cpp) -o $@

bin/make_assets: $(OBJS)
	$(Q) $(CXX) $(CXXFLAGS) $(filter-out $(BIN_OBJS), $(OBJS)) obj/src/make_assets.o -o $@ $(LDFLAGS) -lassimp

bin/hex0ad: $(OBJS)
	$(Q) $(CXX) $(CXXFLAGS) $(filter-out $(BIN_OBJS), $(OBJS)) obj/src/hex0ad.o -o $@ $(LDFLAGS)
	
clean:
	-$(Q) rm -f $(DEPS) $(OBJS) $(BINS) $(WEB_FILES)

$(WEB_BIN).html : $(OBJS)
	$(Q) $(CXX) $(CXXFLAGS) $(filter-out $(BIN_OBJS), $(OBJS)) $(@:%.html=obj/src/%.o) -o $@ $(LDFLAGS)

run_web:
	$(Q) emrun $(WEB_BIN).html

no_deps = 
ifeq ($(MAKECMDGOALS),clean)
	no_deps = yes
endif

ifeq ($(MAKECMDGOALS),run_web)
	no_deps = yes
endif

ifndef no_deps
	-include $(DEPS)
endif

