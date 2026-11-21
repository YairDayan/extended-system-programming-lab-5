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

int main(int argc, char **argv)
{
  int fd;
  struct stat st;
  void *map_start;
  Elf32_Ehdr *ehdr;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <ELF file>\n", argv[0]);
    return 1;
  }

  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  if (fstat(fd, &st) < 0) {
    perror("fstat");
    close(fd);
    return 1;
  }

  map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map_start == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return 1;
  }

  ehdr = (Elf32_Ehdr *)map_start;
  if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
      ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
    fprintf(stderr, "Not an ELF file\n");
    munmap(map_start, st.st_size);
    close(fd);
    return 1;
  }

  foreach_phdr(map_start, print_phdr_info, 0);

  munmap(map_start, st.st_size);
  close(fd);
  return 0;
}
