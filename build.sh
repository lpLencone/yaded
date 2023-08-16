#!/bin/sh

set -xe

CC="${CXX:-cc}"
PKGS="sdl2 glew freetype2"
CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"
LIBS=-lm
SRC=$(find . -name "*.c")
INCLUDE=-Iinclude

if [ `uname` = "Darwin" ]; then
    CFLAGS+=" -framework OpenGL"
fi

$CC $CFLAGS $INCLUDE `pkg-config --cflags $PKGS` -o yaded $SRC $LIBS `pkg-config --libs $PKGS`