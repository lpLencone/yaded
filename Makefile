CC 		:= gcc
SRC		:= $(shell find src -type f)

PKGS	:= sdl2 glew
LIBS	:= `pkg-config --libs $(PKGS)` -lm
CFLAGS	:= -Wall -Wextra -std=c11 -pedantic -ggdb `pkg-config --cflags $(PKGS)`
INCLUDE := -Iinclude

.SILENT: te

te: main.c
	$(CC) $(INCLUDE) $(CFLAGS) $(SRC) -o te main.c $(LIBS)