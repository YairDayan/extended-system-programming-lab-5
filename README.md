# Lab 5 — Loader

Extended System Programming Laboratory.

32-bit static ELF loader: walk program headers, map `PT_LOAD` segments with `mmap`, then transfer control to the loaded entry point (via `startup` for argv).

## Build

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

`loader.c` is the final loader (task 2d). Earlier stages that `main` later overwrote are kept as snapshots:

- `TASK 0.c` — `main` calls `print_phdr_info` (one ELF argument)
- `TASK 1a.c` — `main` prints all program headers like `readelf -l`
- `TASK 1b.c` — same, plus `mmap` prot/flags for every `PT_LOAD`

## Project layout

```
.
├── loader.c           # Final loader (tasks 2b–2d)
├── TASK 0.c           # Snapshot of task 0
├── TASK 1a.c          # Snapshot of task 1a
├── TASK 1b.c          # Snapshot of task 1b
├── start.s            # _start + system_call glue
├── startup.s          # startup() — builds argv stack for the loaded program
├── linking_script     # Custom linker script (low load address)
├── makefile
├── loadme             # Provided static test ELF
├── mission            # Provided static test ELF
├── encoder            # Provided static test ELF (needs argv via startup)
└── workingLoader      # Provided reference loader binary
```

## Notes

- Compile and link as 32-bit only (`-m32` / `elf32`).
- From task 2 onward, only static executables are required (no `DYNAMIC` / `INTERP`).
- Keep the ELF file descriptor open while segments stay mapped.
- `MAP_FIXED` mappings must be page-aligned; use the padding scheme from the reading material.
