CC 		:= gcc
CFLAGS	:= -Wall -Wextra -std=c11 -pedantic -ggdb
LIBS	:= -lSDL2 -lSDL2_ttf -lm
SRC		:= $(shell find src -type f)
INCLUDE := -Iinclude

te: main.c
	$(CC) $(INCLUDE) $(CFLAGS) $(SRC) -o te main.c $(LIBS)