CC=gcc
CFLAGS=-Wall -Werror -ggdb3 #-Wextra
DEPS = datatypes.h pingpong.h queue.h
OBJ = queue.o pingpong.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: pingpong-racecond pingpong-semaphore .PHONY

pingpong-racecond: $(OBJ) pingpong-racecond.o
	$(CC) -o $@ $^ $(CFLAGS)

pingpong-semaphore: $(OBJ) pingpong-semaphore.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core $(INCDIR)/*~ 
