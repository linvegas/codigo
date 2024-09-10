CFLAGS = -Wall -Wextra -pedantic -ggdb

build: main.c
	$(CC) $(CFLAGS) -o codigo main.c buffer.c -lraylib && ./codigo
