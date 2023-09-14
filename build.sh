#!/bin/sh

set -xe

CC="${CXX:-cc}"
PKGS="sdl2 glew freetype2"
CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"
LIBS=-lm
SRC=$(find src -name "*.c")
INCLUDE=-Iinclude

if [ `uname` = "Darwin" ]; then
    CFLAGS+=" -framework OpenGL"
fi

$CC $CFLAGS $INCLUDE `pkg-config --cflags $PKGS` main.c -o medo $SRC $LIBS `pkg-config --libs $PKGS`
