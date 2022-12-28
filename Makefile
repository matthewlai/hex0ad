# Build
# Mac/Linux: OPT=-O3 make
# Emscripten: OPT=-O3 emmake make
# Emscripten build and run in browser: OPT=-O3 emmake make run_web

# Defaults.
ifndef CXX
	CXX=g++
endif

ifeq ($(V),0)
	Q = @
else
	Q =
endif

ifndef OPT
	OPT=-O1
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
CXXFILES += third_party/lodepng/lodepng.cpp
ifndef EM_BUILD
	CXXFILES += third_party/tinyxml2/tinyxml2.cpp
	CXXFILES += third_party/0ad/decompose.cpp
endif

# We don't actually support C, so treat libimagequant differently.
# CFLAGS are copied from the official Makefile.
IMAGEQUANT_CC = gcc
IMAGEQUANT_CFILES := $(wildcard third_party/libimagequant/*.c)
IMAGEQUANT_OBJS := $(IMAGEQUANT_CFILES:%.c=obj/%.o)
IMAGEQUANT_CFLAGS = -fno-math-errno -funroll-loops -fomit-frame-pointer -Wall -std=c99 -Wno-attributes
IMAGEQUANT_CFLAGS += -O3 -DNDEBUG -Wno-unknown-pragmas -fexcess-precision=fast
IMAGEQUANT_CFLAGS += -Ithird_party/libimagequant

WEB_FILES=$(WEB_BIN).js $(WEB_BIN).wasm $(WEB_BIN).html $(WEB_BIN).data

SHADERS := $(wildcard shaders/*)

FLATBUFFER_SCHEMAS := $(wildcard fb/*.fbs)

FLATBUFFER_GENERATED_FILES := $(FLATBUFFER_SCHEMAS:fb/%.fbs=fb/%_generated.h)

OBJS := $(CXXFILES:%.cpp=obj/%.o)
DEPS := $(CXXFILES:%.cpp=dep/%.d)
BIN_OBJS = $(BINS:bin/%=obj/src/%.o)

ifdef EM_BUILD
	DEPS := $(filter-out dep/src/make_assets.d, $(DEPS))
endif

ifeq ($(WINDOWS_BUILD), 1)
	BIN_OBJS = $(BINS:bin/%.exe=obj/src/%.o)
endif

INCLUDES +=-Iinc -Ithird_party -Ifb -Ithird_party/libimagequant

# Platforms.
DEFAULT_TARGETS = $(BINS)

CXXFLAGS_DEP = -std=gnu++20

# Flags.
ifeq ($(EM_BUILD), 1)
	PORTS =  -s USE_SDL=2 -s USE_SDL_TTF=2
	CXXFLAGS = -std=gnu++20 $(PORTS) $(OPT)
	LDFLAGS =  $(PORTS) -s WASM=1 -s ALLOW_MEMORY_GROWTH=1
	LDFLAGS += -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2
	LDFLAGS += --preload-file assets --preload-file shaders
	LDFLAGS += --shell-file em_shell.html
	DEFAULT_TARGETS = $(WEB_BIN).html
else
	CXXFLAGS += -Wall -Wextra -Wno-unused-function -std=gnu++20 -ffast-math -Wno-unused-const-variable -g $(OPT)
	LDFLAGS = -lm

	ifeq ($(OS),Windows_NT)
		LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lFColladaSD
		LDFLAGS += -lopengl32 -lglew32
	else
		# For UNIX-like platforms.
		UNAME_S := $(shell uname -s)

		# Add homebrew include path for Mac
		ifeq ($(UNAME_S),Darwin)
			INCLUDES += -I$${HOME}/homebrew/include
		endif

		# SDL dependencies.
		INCLUDES += $(shell sdl2-config --cflags)
		LDFLAGS = $(shell sdl2-config --libs)

		# SDL2_ttf dependencies.
		INCLUDES += $(shell pkg-config --cflags SDL2_ttf)
		LDFLAGS += $(shell pkg-config --libs SDL2_ttf)

		# FCollada dependencies.
		INCLUDES += $(shell pkg-config --cflags fcollada)
		LDFLAGS += $(shell pkg-config --libs fcollada)

		# libxml2 dependencies.
		INCLUDES += $(shell pkg-config --cflags libxml-2.0)
		LDFLAGS += $(shell pkg-config --libs libxml-2.0)

		INCLUDES += -Ithird_party/0ad

		# OpenGL dependencies.
		ifeq ($(UNAME_S),Darwin)
			LDFLAGS += -framework OpenGL
		else
			LDFLAGS += -lGL
		endif

		ifeq ($(UNAME_S),Linux)
			CXXFLAGS += -Wno-psabi
			LDFLAGS += -ldl -pthread
		endif
	endif
endif

ifdef PROFILE
	CXXFLAGS += -pg
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
	-$(Q) rm -f $(DEPS) $(OBJS) $(BINS) $(WEB_FILES) $(FLATBUFFER_GENERATED_FILES)

$(WEB_BIN).html : $(filter-out $(BIN_OBJS), $(OBJS)) $(SHADERS) obj/src/hex0ad.o em_shell.html
	$(Q) $(CXX) $(CXXFLAGS) $(filter-out $(BIN_OBJS), $(OBJS)) $(@:%.html=obj/src/%.o) -o $@ $(LDFLAGS)

run_web: $(WEB_BIN).html
	$(Q) emrun $(WEB_BIN).html --no_browser

no_deps = 
ifeq ($(MAKECMDGOALS),clean)
	no_deps = yes
endif

ifndef no_deps
	-include $(DEPS)
endif

