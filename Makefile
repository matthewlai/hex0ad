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

ifeq ($(basename $(notdir $(CXX))), em++)
	EM_BUILD = 1
endif

ifeq ($(OS),Windows_NT)
	ifneq ($(EM_BUILD), 1)
		WINDOWS_BUILD = 1
	endif
endif

# Files and paths.
# All binaries.
BINS=bin/hex0ad bin/make_assets

ifeq ($(WINDOWS_BUILD), 1)
	BINS := $(BINS:%=%.exe)
endif

WEB_BIN=hex0ad

CXXFILES := $(wildcard src/*.cpp)
CXXFILES += third_party/tinyxml2/tinyxml2.cpp
CXXFILES += third_party/lodepng/lodepng.cpp

# We don't actually support C, so treat libimagequant differently.
# CFLAGS are copied from the official Makefile. We can use SSE here because this will not be built for the game
# itself.
IMAGEQUANT_CC = gcc
IMAGEQUANT_CFILES := $(wildcard third_party/libimagequant/*.c)
IMAGEQUANT_OBJS := $(IMAGEQUANT_CFILES:%.c=obj/%.o)
IMAGEQUANT_CFLAGS = -fno-math-errno -funroll-loops -fomit-frame-pointer -Wall -std=c99 -Wno-attributes
IMAGEQUANT_CFLAGS += -O3 -DNDEBUG -DUSE_SSE=1 -msse -mfpmath=sse -Wno-unknown-pragmas -fexcess-precision=fast
IMAGEQUANT_CFLAGS += -Ithird_party/libimagequant

WEB_FILES=$(WEB_BIN).js $(WEB_BIN).wasm $(WEB_BIN).html $(WEB_BIN).data

SHADERS := $(wildcard shaders/*)

FLATBUFFER_SCHEMAS := $(wildcard fb/*.fbs)

FLATBUFFER_GENERATED_FILES := $(FLATBUFFER_SCHEMAS:fb/%.fbs=fb/%_generated.h)

OBJS := $(CXXFILES:%.cpp=obj/%.o)
DEPS := $(CXXFILES:%.cpp=dep/%.d)
BIN_OBJS = $(BINS:bin/%=obj/src/%.o)

ifeq ($(WINDOWS_BUILD), 1)
	BIN_OBJS = $(BINS:bin/%.exe=obj/src/%.o)
endif

INCLUDES +=-Iinc -Ithird_party -Ifb -Ithird_party/libimagequant

# Platforms.
DEFAULT_TARGETS = $(BINS)

# Flags.
ifeq ($(EM_BUILD), 1)
	CXXFLAGS = $(INCLUDES) -std=gnu++17 -s USE_SDL=2 -O3
	LDFLAGS = --emrun -s WASM=1 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s ALLOW_MEMORY_GROWTH=1
	LDFLAGS += -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2
	LDFLAGS += --preload-file assets --preload-file shaders
	LDFLAGS += --shell-file em_shell.html
	DEFAULT_TARGETS = $(WEB_BIN).html
else
	CXXFLAGS += -Wall -Wextra -Wno-unused-function -std=gnu++17 -march=native -ffast-math -Wno-unused-const-variable -g -O3
	LDFLAGS = -lm -lFColladaSD

	ifeq ($(OS),Windows_NT)
		LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lFColladaSD
		LDFLAGS += -lopengl32 -lglew32
	else
		# For UNIX-like platforms.
		UNAME_S := $(shell uname -s)

		# Add homebrew include path for Mac
		ifeq ($(UNAME_S),Darwin)
			INCLUDES += -I$${HOME}/homebrew/include
		endif

		# SDL dependencies.
		CXXFLAGS += $(shell sdl2-config --cflags)
		CXXFLAGS_DEP = -std=gnu++17 $(shell sdl2-config --cflags)
		LDFLAGS = $(shell sdl2-config --libs)

		# OpenGL dependencies.
		ifeq ($(UNAME_S),Darwin)
			LDFLAGS += -framework OpenGL
		else
			LDFLAGS += -lGL
		endif

		LDFLAGS += -lFColladaSD
	endif
endif

.PHONY: clean run_web

default: $(DEFAULT_TARGETS)

fb/%_generated.h: fb/%.fbs
	$(Q) flatc --cpp -o fb/ $<

dep/%.d: %.cpp $(FLATBUFFER_GENERATED_FILES)
	$(Q) $(CXX) $(CXXFLAGS_DEP) $(INCLUDES) $< -MM -MT $(@:dep/%.d=obj/%.o) > $@
	
obj/%.o: %.cpp $(FLATBUFFER_GENERATED_FILES)
	$(Q) $(CXX) $(CXXFLAGS) $(INCLUDES) -c $(@:obj/%.o=%.cpp) -o $@

obj/third_party/libimagequant/%.o: third_party/libimagequant/%.c
	$(Q) $(IMAGEQUANT_CC) $(IMAGEQUANT_CFLAGS) -c $(@:obj/third_party/libimagequant/%.o=third_party/libimagequant/%.c) -o $@

bin/make_assets bin/make_assets.exe: $(filter-out $(BIN_OBJS), $(OBJS)) obj/src/make_assets.o $(IMAGEQUANT_OBJS)
	$(Q) $(CXX) $(CXXFLAGS) $(filter-out $(BIN_OBJS), $(OBJS)) $(IMAGEQUANT_OBJS) obj/src/make_assets.o -o $@ $(LDFLAGS)

bin/hex0ad bin/hex0ad.exe: $(filter-out $(BIN_OBJS), $(OBJS)) obj/src/hex0ad.o
	$(Q) $(CXX) $(CXXFLAGS) $(filter-out $(BIN_OBJS), $(OBJS)) obj/src/hex0ad.o -o $@ $(LDFLAGS)
	
clean:
	-$(Q) rm -f $(DEPS) $(OBJS) $(BINS) $(WEB_FILES)

$(WEB_BIN).html : $(filter-out $(BIN_OBJS), $(OBJS)) $(SHADERS) obj/src/hex0ad.o em_shell.html
	$(Q) $(CXX) $(CXXFLAGS) $(filter-out $(BIN_OBJS), $(OBJS)) $(@:%.html=obj/src/%.o) -o $@ $(LDFLAGS)

run_web: $(WEB_BIN).html
	$(Q) emrun $(WEB_BIN).html

no_deps = 
ifeq ($(MAKECMDGOALS),clean)
	no_deps = yes
endif

ifndef no_deps
	-include $(DEPS)
endif

