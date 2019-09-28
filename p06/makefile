CC=gcc
CFLAGS=-Wall -Werror -ggdb3 #-Wextra
DEPS = datatypes.h pingpong.h queue.h
OBJ = queue.o pingpong.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: pingpong-contab pingpong-contab-prio

pingpong-contab: $(OBJ) pingpong-contab.o
	$(CC) -o $@ $^ $(CFLAGS)

pingpong-contab-prio: $(OBJ) pingpong-contab-prio.o
	$(CC) -o $@ $^ $(CFLAGS)


.PHONY: clean

clean:
	rm -f *.o *~ core $(INCDIR)/*~ 
