# Lab 5 — Loader

Extended System Programming Laboratory.

32-bit static ELF loader: walk program headers, map `PT_LOAD` segments with `mmap`, then transfer control to the loaded entry point (via `startup` for argv).

## Build (final loader)

Needs a 32-bit toolchain (`gcc -m32`, `nasm`, `ld`).

```bash
make
./loader loadme
./loader mission
./loader encoder arg1 arg2
make clean
```

Linking uses the custom `linking_script` so the loader is mapped below the usual `0x08048000` load address and does not collide with the loaded program.

Verify with:

```bash
readelf -h loader
```

## Earlier tasks (snapshots)

`loader.c` is the final program (task 2d). In that file, `main` only prints headers through `load_phdr`, so only `PT_LOAD` lines appear, then the file is mapped and control is transferred.

Earlier stages, where `main` still printed every program header (or only the task-0 message), are kept as separate snapshots so they stay visible after later tasks overwrote `main`:

| File | Stage | What `main` does |
|------|-------|------------------|
| `TASK 0.c` | Task 0 | Calls `print_phdr_info` for each header; one ELF argument |
| `TASK 1a.c` | Task 1a | Prints all program headers like `readelf -l` |
| `TASK 1b.c` | Task 1b | Same as 1a, plus `mmap` prot/flags for every `PT_LOAD` |

Build and run a snapshot with plain `gcc` (no custom linker script needed yet):

```bash
gcc -m32 -Wall -o task0 "TASK 0.c"
./task0 loadme

gcc -m32 -Wall -o task1a "TASK 1a.c"
./task1a loadme

gcc -m32 -Wall -o task1b "TASK 1b.c"
./task1b loadme
```

## Tasks

| Task | Description | Source |
|------|-------------|--------|
| 0 | Iterator `foreach_phdr` over program headers | `TASK 0.c` |
| 1a | Print program headers like `readelf -l` | `TASK 1a.c` |
| 1b | Print `mmap` protection and mapping flags per header | `TASK 1b.c` |
| 2a | Link with `linking_script`, `start.o` | `makefile`, `linking_script`, `start.s` |
| 2b | `load_phdr` — map each `PT_LOAD` segment | `loader.c` |
| 2c | Jump to the loaded entry point | `loader.c` |
| 2d | Pass argv through `startup(argc-1, argv+1, entry)` | `loader.c`, `startup.s` |

Header printing also names common GNU types (`GNU_EH_FRAME`, `GNU_STACK`, `GNU_RELRO`) when present.

## Project layout

```
.
├── loader.c                    # Final loader (tasks 2b–2d)
├── TASK 0.c                    # Snapshot of task 0
├── TASK 1a.c                   # Snapshot of task 1a
├── TASK 1b.c                   # Snapshot of task 1b
├── start.s                     # _start + system_call glue
├── startup.s                   # startup() — builds argv stack for the loaded program
├── startup.o                   # Provided object of startup.s
├── linking_script              # Custom linker script (low load address)
├── makefile
├── loadme                      # Provided static test ELF
├── mission                     # Provided static test ELF
├── encoder                     # Provided static test ELF (needs argv via startup)
├── Lab 5 tasks.pdf             # Lab tasks
├── Lab 5 Reading material.pdf  # Reading material
├── Chart1.png                  # Figure from the reading material
├── object.jpg                  # Figure from the reading material
└── loaded-object.jpg           # Figure from the reading material
```

## Notes

- Compile and link as 32-bit only (`-m32` / `elf32`).
- From task 2 onward, only static executables are required (no `DYNAMIC` / `INTERP`).
- Keep the ELF file descriptor open while segments stay mapped.
- `MAP_FIXED` mappings must be page-aligned; use the padding scheme from the reading material.
