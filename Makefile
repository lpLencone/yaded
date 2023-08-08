CC 		:= gcc
CFLAGS	:= -Wall -Wextra -std=c11 -pedantic -ggdb `pkg-config --cflags sdl2`
LIBS	:= `pkg-config --libs sdl2` -lm -lSDL2_ttf
SRC		:= $(shell find src -type f)
INCLUDE := -Iinclude

te: main.c
	$(CC) $(INCLUDE) $(CFLAGS) $(SRC) -o te main.c $(LIBS)