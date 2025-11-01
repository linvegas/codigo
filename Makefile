CFLAGS = -Wall -Wextra -ggdb
# CFLAGS += $(shell pkg-config --cflags raylib)
CFLAGS += -I./raylib-5.5/include/

LIBS = -lm
# LIBS += $(shell pkg-config --libs raylib)
LIBS += -L./raylib-5.5/lib/ -l:libraylib.a

main: main.c
	cc $(CFLAGS) -o codigo main.c $(LIBS)
