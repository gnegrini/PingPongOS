CC=gcc
CFLAGS=-Wall -Werror -ggdb3 #-Wextra
DEPS = datatypes.h pingpong.h
OBJ = pingpong.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: pingpong-tasks1 pingpong-tasks2 pingpong-tasks3

pingpong-tasks1: $(OBJ) pingpong-tasks1.o
	$(CC) -o $@ $^ $(CFLAGS)

pingpong-tasks2: $(OBJ) pingpong-tasks2.o 
	$(CC) -o $@ $^ $(CFLAGS)

pingpong-tasks3: $(OBJ) pingpong-tasks3.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core $(INCDIR)/*~ 
