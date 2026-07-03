#!/bin/bash

OUT_DIR="Debug"
OUT_EXE="example_sdl3_sdlrenderer3"
INCLUDES="-I. \
          -Ithird_party/imgui/include \
          -Ithird_party/portable_file_dialogs/include \
          -Ithird_party/nlohmann/include \
          -Ithird_party/nanosvg/include \
          -Ithird_party/mapbox/include \
          $(pkg-config --cflags sdl3)"
SOURCES="main.cpp third_party/imgui/src/imgui*.cpp"
LIBS="$(pkg-config --libs sdl3)"
mkdir -p ${OUT_DIR}
echo "Compiling..."
g++ -g ${INCLUDES} $SOURCES -o ${OUT_DIR}/${OUT_EXE} ${LIBS}
cp -r data ${OUT_DIR}/
echo "Build complete."
