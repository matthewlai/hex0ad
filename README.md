# hex0ad
Tile-based strategy game using 0ad assets

# BUILD
## Requirements

### Linux (Debian-based, including Raspberry Pi 4)
* SDL2 SDL ttf, GLEW, CMake
	* `sudo apt install libsdl2-dev libsdl2-ttf-dev libglew-dev cmake`
* Flatc compiler (https://github.com/google/flatbuffers/releases)
	* `git clone https://github.com/google/flatbuffers.git`
	* `cd flatbuffers`
	* `cmake . -DFLATBUFFERS_BUILD_TESTS=OFF -DFLATBUFFERS_PACKAGE_DEBIAN=ON`
	* `make -j6 package`
	* `sudo dpkg -i flatbuffers_*.deb`
	* `cp -r include/flatbuffers fb/`
* FCollada for converting assets (https://github.com/matthewlai/fcollada)
	* `git clone https://github.com/matthewlai/fcollada.git`
	* `cd fcollada`
	* `cmake . -DCMAKE_INSTALL_PREFIX=/usr`
	* `make MAKE="make -j6" && make install`

### Windows
* MinGW-W64 (https://sourceforge.net/projects/mingw-w64/, use online installer)
	* Add both [MinGW installation directory]/x86_64-w64-mingw32/bin and [MinGW installation directory]/bin to PATH
* Make (http://gnuwin32.sourceforge.net/packages/make.htm)
* Coreutils (http://gnuwin32.sourceforge.net/packages/coreutils.htm)
* SDL2 (https://www.libsdl.org/download-2.0.php)
	* Download Windows development library (MinGW 32/64-bit)
	* Copy into [MinGW installation directory]/x86_64-w64-mingw32
* SDL ttf (https://www.libsdl.org/projects/SDL_ttf/)
	* Download Windows development library (MinGW 32/64-bit)
	* Copy into [MinGW installation directory]/
* GLEW (http://www.grhmedia.com/glew.html)
	* Download the latest mingw-w64 version
	* Copy into [MinGW installation directory]/x86_64-w64-mingw32
	* Copy lib/glew32[mx].dll to [MinGW installation directory]/x86_64-w64-mingw32/bin
* Flatc compiler (https://github.com/google/flatbuffers/releases)
	* Download the latest Windows binary
	* Copy flatc.exe into [mingw-w64 bin directory]/bin
	* Download the source package, and copy the content of include/flatbuffers to fb/flatbuffers
* CMake (https://cmake.org/download/)
	* Use the win64-x64 installer and add CMake to PATH
* FCollada for converting assets (https://github.com/matthewlai/fcollada)
	* `git clone https://github.com/matthewlai/fcollada.git`
	* `cd fcollada`
	* `cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=build .`
	* Build with `make MAKE="make -j 24" && make install` (or as appropriate for how many CPU cores you have)
	* Copy build/include, lib, and bin to [MinGW installation directory]/x86_64-w64-mingw32

### macOS
* Install Homebrew (https://brew.sh/)
	* Follow instructions on https://brew.sh
* Install dependencies
	* `brew install sdl2 sdl2_ttf flatbuffers libxml2 cmake pkg-config`
* Copy flatbuffers headers into the project
	* ``cp -r `pkg-config --cflags-only-I flatbuffers|cut -c 3-`/flatbuffers fb/``
* Set up libxml2 for pkg-config to find
	* `brew info libxml2` to see instructions (set PKG_CONFIG_PATH in $HOME/.bash_profile or $HOME/.zshrc)
* FCollada for converting assets (https://github.com/matthewlai/fcollada)
	* `git clone https://github.com/matthewlai/fcollada.git`
	* `cd fcollada`
	* `cmake .`
	* Build with `make -j 6 && make install` (or as appropriate for how many CPU cores you have)

## Download and convert 0ad Assets (All Platforms)
* Download https://0adassets.s3-us-west-2.amazonaws.com/assets.7z, and extract into 0ad_assets, so you should end up with:
	* 0ad_assets/art
	* 0ad_assets/audio
	* ...
* Convert assets to hex0ad format:
	* `make`
	* `bin/make_assets`

## Build the game
* `make`

## Run the game
* `bin/hex0ad`
