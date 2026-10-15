#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
command -v emcc >/dev/null || { echo 'Install Emscripten 3.1.69 and activate emsdk_env.sh first (see WEB.md).' >&2; exit 1; }
RAYLIB_COMMIT=c1ab645ca298a2801097931d1079b10ff7eb9df8
if [[ ! -d build/web/raylib ]]; then
    mkdir -p build/web
    git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git build/web/raylib
fi
[[ "$(git -C build/web/raylib rev-parse HEAD)" == "$RAYLIB_COMMIT" ]]
if [[ ! -f build/web/raylib/.webgl2-manual-build ]]; then
    make -C build/web/raylib/src clean
    make -C build/web/raylib/src -j4 PLATFORM=PLATFORM_WEB GRAPHICS=GRAPHICS_API_OPENGL_ES3 CC=emcc AR=emar CUSTOM_CFLAGS=-DSUPPORT_CUSTOM_FRAME_CONTROL
    touch build/web/raylib/.webgl2-manual-build
fi
make -f web.mk -j4
cp web/index.html dist/index.html
chmod 644 dist/*
echo 'Static site ready in dist/. Serve over HTTP or deploy dist/ with Vercel.'
