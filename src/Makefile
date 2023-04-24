CC=gcc
CFLAGS=-std=c++11 -lstdc++ -I.
DEPS = *.h

mmu: main.cc
	$(CC) main.cc -o mmu $(CFLAGS)

.PHONY: clean

clean:
	rm -f mmu *~ *o