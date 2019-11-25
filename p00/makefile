CC=gcc
CFLAGS=-Wall -Werror -ggdb3
DEPS = queue.h
OBJ = queue.o testafila.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

testafila: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core $(INCDIR)/*~ 
