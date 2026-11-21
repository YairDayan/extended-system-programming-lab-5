#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
  Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);
  int i;

  for (i = 0; i < ehdr->e_phnum; i++) {
    func(&phdr[i], arg);
  }
  return 0;
}

void print_phdr_info(Elf32_Phdr *phdr, int arg)
{
  static int i = 0;
  (void)arg;
  printf("Program header number %d at address %p\n", i, (void *)phdr);
  i++;
}
