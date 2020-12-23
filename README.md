# hex0ad
Tile-based strategy game using 0ad assets

# BUILD
## Requirements
### Windows
* MinGW-W64 (https://sourceforge.net/projects/mingw-w64/, use online installer)
	* Add both [MinGW installation directory]/x86_64-w64-mingw32/bin and [MinGW installation directory]/bin to PATH
* Make (http://gnuwin32.sourceforge.net/packages/make.htm)
* Coreutils (http://gnuwin32.sourceforge.net/packages/coreutils.htm)
* SDL2 (https://www.libsdl.org/download-2.0.php)
	* Download Windows development library (MinGW 32/64-bit)
	* Copy into [MinGW installation directory]/x86_64-w64-mingw32
* GLEW (http://www.grhmedia.com/glew.html)
	* Download the latest mingw-w64 version
	* Copy into [MinGW installation directory]/x86_64-w64-mingw32
	* Copy lib/glew32[mx].dll to [MinGW installation directory]/x86_64-w64-mingw32/bin
* Flatc compiler (https://github.com/google/flatbuffers/releases)
	* Download the latest Windows binary
	* Copy flatc.exe into [mingw-w64 bin directory]/bin
* CMake (https://cmake.org/download/)
	* Use the win64-x64 installer and add CMake to PATH
* AssImp for converting assets (https://github.com/assimp/assimp/releases)
	* Download a source code release or git commit (many broken revisions near 5.0 because of a few bugs like #2431, #2977, #3144, and #3533)
	* Commit 5f5f1cf works: `git clone https://github.com/assimp/assimp.git` `cd assimp` `git checkout 5f5f1cf`
	* Generate Makefile with `cmake -G "MinGW Makefiles" ./`
	* Build with `make MAKE="make -j 24"` (or as appropriate for how many CPU cores you have)
	* Copy include, lib, and bin to [MinGW installation directory]/x86_64-w64-mingw32

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