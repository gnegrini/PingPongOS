CC=gcc
CFLAGS=-Wall -Werror -ggdb3 #-Wextra
DEPS = datatypes.h pingpong.h queue.h
OBJ = queue.o pingpong.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: pingpong-sleep

pingpong-sleep: $(OBJ) pingpong-sleep.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core $(INCDIR)/*~ 
