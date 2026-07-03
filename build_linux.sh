#!/bin/bash
OUT_DIR="Debug"
OUT_EXE="BrokenProxy"
INCLUDES="-I. \
          -Ithird_party/imgui/include \
          -Ithird_party/portable_file_dialogs/include \
          -Ithird_party/nlohmann/include \
          -Ithird_party/nanosvg/include \
          -Ithird_party/mapbox/include \
          -Ithird_party/cjson/include \
          -Isrc/shared \
          -Isrc/proxy \
          $(pkg-config --cflags sdl3)"
SOURCES="third_party/imgui/src/imgui*.cpp \
         third_party/cjson/src/cJSON.c \
         src/shared/Clay/*.c \
         src/shared/Europa/*.c \
         src/shared/Styx/*.c \
         src/shared/Talos/*.c \
         src/proxy/bproxy.c \
         src/proxy/dns_resolution.c \
         src/proxy/network_tools.c \
         src/main.cpp"
LIBS="$(pkg-config --libs sdl3)"
mkdir -p ${OUT_DIR}
echo "Compiling..."
g++ -g ${INCLUDES} $SOURCES -o ${OUT_DIR}/${OUT_EXE} ${LIBS}
cp -r data ${OUT_DIR}/
echo "Build complete."
