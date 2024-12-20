CC = gcc

SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(SOURCES))

edb: $(OBJECTS)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $^ -o $@ -g

clean :
	rm -rf *.o edb
