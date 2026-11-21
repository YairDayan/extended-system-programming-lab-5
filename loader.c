#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

/* Forward declarations so functions can be used before their definitions. */
const char *phdr_type_name(Elf32_Word type);
void flags_to_string(Elf32_Word flags, char *out);
int phdr_to_prot(Elf32_Word flags);
int phdr_to_mmap_flags(void);
void print_phdr(Elf32_Phdr *phdr, int arg);
void print_phdr_info(Elf32_Phdr *phdr, int arg);
void load_phdr(Elf32_Phdr *phdr, int fd);

/* Defined in startup.o. Builds argc/argv on the stack, then jumps to start. */
int startup(int argc, char **argv, void (*start)());

/*
 * Iterate over every program header in the mapped ELF file.
 * map_start - address where the file is mapped in virtual memory
 * func      - function pointer applied to each program header
 * arg       - extra argument passed to func (file descriptor in task 2)
 */
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
  /* program header table starts at file offset e_phoff */
  Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);
  int i;

  for (i = 0; i < ehdr->e_phnum; i++) {
    func(&phdr[i], arg);
  }
  return 0;
}

/* Convert p_type to a readable name (like readelf -l). */
const char *phdr_type_name(Elf32_Word type)
{
  switch (type) {
  case PT_NULL:
    return "NULL";
  case PT_LOAD:
    return "LOAD";
  case PT_DYNAMIC:
    return "DYNAMIC";
  case PT_INTERP:
    return "INTERP";
  case PT_NOTE:
    return "NOTE";
  case PT_SHLIB:
    return "SHLIB";
  case PT_PHDR:
    return "PHDR";
  default:
    return "UNKNOWN";
  }
}

/* Convert p_flags (PF_R / PF_W / PF_X) into a string like "R E" or "RW ". */
void flags_to_string(Elf32_Word flags, char *out)
{
  out[0] = (flags & PF_R) ? 'R' : ' ';
  out[1] = (flags & PF_W) ? 'W' : ' ';
  out[2] = (flags & PF_X) ? 'E' : ' ';
  out[3] = '\0';
}

/*
 * Task 1b: convert ELF p_flags to mmap protection flags.
 * Bit positions differ, so PF_* cannot be passed to mmap directly.
 */
int phdr_to_prot(Elf32_Word flags)
{
  int prot = 0;
  if (flags & PF_R)
    prot |= PROT_READ;
  if (flags & PF_W)
    prot |= PROT_WRITE;
  if (flags & PF_X)
    prot |= PROT_EXEC;
  return prot;
}

/* Mapping flags used when loading PT_LOAD segments in this lab. */
int phdr_to_mmap_flags(void)
{
  return MAP_PRIVATE | MAP_FIXED;
}

/* Task 0 verification: print index and address of each program header. */
void print_phdr_info(Elf32_Phdr *phdr, int arg)
{
  static int i = 0;
  (void)arg; /* required by the iterator signature; unused in task 0 */
  printf("Program header number %d at address %p\n", i, (void *)phdr);
  i++;
}

/* Task 1a + 1b: print program header fields; for PT_LOAD also print mmap flags. */
void print_phdr(Elf32_Phdr *phdr, int arg)
{
  char flags[4];
  (void)arg;

  flags_to_string(phdr->p_flags, flags);

  printf("%-8s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %s 0x%x\n", phdr_type_name(phdr->p_type),
         phdr->p_offset, phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz, phdr->p_memsz, flags,
         phdr->p_align);

  /* Only PT_LOAD segments are mapped into memory. */
  if (phdr->p_type == PT_LOAD) {
    int prot = phdr_to_prot(phdr->p_flags);
    int map_flags = phdr_to_mmap_flags();

    printf("  mmap prot: ");
    if (prot & PROT_READ)
      printf("PROT_READ ");
    if (prot & PROT_WRITE)
      printf("PROT_WRITE ");
    if (prot & PROT_EXEC)
      printf("PROT_EXEC ");
    printf("\n");

    printf("  mmap flags: ");
    if (map_flags & MAP_PRIVATE)
      printf("MAP_PRIVATE ");
    if (map_flags & MAP_FIXED)
      printf("MAP_FIXED ");
    printf("\n");
  }
}

/*
 * Task 2b: map a PT_LOAD segment into virtual memory.
 * fd is the open executable, passed through foreach_phdr as arg.
 */
void load_phdr(Elf32_Phdr *phdr, int fd)
{
  void *vaddr;
  off_t offset;
  size_t padding;
  void *map;
  int prot;
  int map_flags;

  if (phdr->p_type != PT_LOAD) {
    return;
  }

  print_phdr(phdr, fd);

  /* mmap with MAP_FIXED must start on a page boundary (0x1000). */
  vaddr = (void *)(phdr->p_vaddr & 0xfffff000);
  offset = phdr->p_offset & 0xfffff000;
  padding = phdr->p_vaddr & 0xfff;

  prot = phdr_to_prot(phdr->p_flags);
  map_flags = phdr_to_mmap_flags();

  map = mmap(vaddr, phdr->p_memsz + padding, prot, map_flags, fd, offset);
  if (map == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
}

int main(int argc, char **argv)
{
  int fd;
  struct stat st;
  void *map_start;
  Elf32_Ehdr *ehdr;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <ELF file> [args...]\n", argv[0]);
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

  /* Map the whole file into virtual memory so we can read headers via pointers. */
  map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map_start == MAP_FAILED) {
    perror("mmap");
    close(fd);
    return 1;
  }

  /* Optional safety check: first 4 bytes of the ELF header must be 0x7F 'E' 'L' 'F'. */
  ehdr = (Elf32_Ehdr *)map_start;
  if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
      ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
    fprintf(stderr, "Not an ELF file\n");
    munmap(map_start, st.st_size);
    close(fd);
    return 1;
  }

  /*
   * Task 2b: map every PT_LOAD segment. fd stays open and the file
   * mapping stays in place until the loaded program has run.
   */
  printf("Type     Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align\n");
  foreach_phdr(map_start, load_phdr, fd);

  /* Task 2d: pass the loaded program's argv, then jump to its entry point. */
  startup(argc - 1, argv + 1, (void *)(ehdr->e_entry));

  return 0;
}
