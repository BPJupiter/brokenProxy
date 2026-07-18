#!/bin/bash
OUT_DIR="Debug"
OUT_EXE="BrokenProxy"
INCLUDES="-I. \
          -Ithird_party/imgui/include \
          -Ithird_party/portable_file_dialogs/include \
          -Ithird_party/nlohmann/include \
          -Ithird_party/nanosvg/include \
          -Ithird_party/mapbox/include \
          -Isrc/shared \
          -Isrc/shared/bplibxx/include \
          -Isrc/proxy \
          $(pkg-config --cflags sdl3)"
SOURCES="third_party/imgui/src/imgui*.cpp \
         src/shared/bplibxx/src/*.cpp \
         src/shared/Clay/*.c \
         src/shared/Europa/*.c \
         src/shared/Styx/*.c \
         src/shared/Talos/*.c \
         src/proxy/broken_proxy.c \
         src/proxy/dns_resolution.c \
         src/proxy/network_tools.c \
         src/main.cpp"
LIBS="$(pkg-config --libs sdl3)"
FLAGS="-mshstk"
NO_WARNINGS="-Wno-write-strings"
mkdir -p ${OUT_DIR}
echo "Compiling..."
g++ -g ${FLAGS} ${NO_WARNINGS} ${INCLUDES} $SOURCES -o ${OUT_DIR}/${OUT_EXE} ${LIBS}
cp -r assets ${OUT_DIR}/
echo "Build complete."
