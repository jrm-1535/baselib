
#
# Makefile for basic data management library
#

LIBS :=
DEBUG := -g
OPTIMIZE := #-O3
CFLAGS := -Wall -std=c99 -pedantic $(OPTIMIZE) $(PROFILE) $(DEBUG)
CC := gcc $(GDEFS)

all:    baselib.a

clean:
	   rm *.o baselib.a

baselib.a:  vector.o slice.o heap.o fnv1a.o map.o queue.o fifo.o
	   /usr/bin/ar csr $@ $^

vector.o:   vector.c vector.h _vector.h

slice.o:    slice.c slice.h _slice.h vector.h _vector.h

heap.o:     heap.c heap.h slice.h _slice.h vector.h _vector.h

fnv1a.o:    fnv1a.c fnv.h

map.o:      map.c map.h

queue.o:    queue.c queue.h

fifo.o:     fifo.c fifo.h

