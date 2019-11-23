CC=gcc
CFLAGS=-Wall -Werror -ggdb3 #-Wextra
DEPS = datatypes.h pingpong.h queue.h
OBJ = queue.o pingpong.o pingpong-preempcao.o

all: pingpong-preempcao

pingpong-preempcao: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)


.PHONY: clean

clean:
	rm -f *.o *~ core $(INCDIR)/*~ 
