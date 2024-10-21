CFLAGS = -Wall -Wextra -pedantic -ggdb

all: build run

build: main.c
	$(CC) $(CFLAGS) -o codigo main.c buffer.c -lraylib

run: ./codigo
	./codigo $(ARGS)

tags:
	ctags --c-kinds=+p --fields=+iaS --format=2 *.h /usr/include/raylib.h
