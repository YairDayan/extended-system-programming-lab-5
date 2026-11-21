CC = gcc
CFLAGS = -m32 -c
NASM = nasm
NASMFLAGS = -f elf32
LD = ld
LDFLAGS = -m elf_i386 -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

all: loader

loader.o: loader.c
	$(CC) $(CFLAGS) loader.c -o loader.o

start.o: start.s
	$(NASM) $(NASMFLAGS) start.s -o start.o

startup.o: startup.s
	$(NASM) $(NASMFLAGS) startup.s -o startup.o

loader: loader.o startup.o start.o linking_script
	$(LD) -o loader loader.o startup.o start.o $(LDFLAGS)

clean:
	rm -f loader loader.o start.o startup.o

.PHONY: all clean
