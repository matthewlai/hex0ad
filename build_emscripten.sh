#!/bin/bash

set -e

make clean
rm -rf assets/art
OPT=-O3 make -j8
bin/make_assets
make clean
OPT=-O3 emmake make -j8 hex0ad.html
mv hex0ad.html hex0ad.data hex0ad.js hex0ad.wasm web_build