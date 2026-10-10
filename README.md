# int3drv — High-Performance Privileged Remote Memory Kernel Driver

`int3drv` is a high-performance Linux kernel character device driver (`/dev/int3`) engineered for high-throughput, low-latency remote memory access, binary instrumentation, and dynamic analysis.

It is designed as an ultra-fast drop-in acceleration layer for memory analysis frameworks, reversing tools, debuggers, and game introspection utilities.

---

## Key Advantages Over Standard Linux Interfaces

| Feature | Standard `process_vm_*` / `ptrace` | `int3drv` Kernel Driver |
| :--- | :--- | :--- |
| **Call Overhead** | 1 syscall per operation or vector | 1 ioctl per batch of 256 operations |
| **Discovery Hot Path** | 256 syscalls for 256 pointer reads | **1 syscall + 1 task lookup** |
| **Scalar Access** | Requires userspace bounce pointers | **Zero-allocation direct scalar register read/write** |
| **Pattern Scanning** | Must stream gigabytes of memory to userspace | **In-kernel sliding window scanner with hardware prefetch** |
| **Executable/RO Patching** | Blocked on read-only pages without `mprotect` | **Privileged `FOLL_FORCE` write bypassing page protections** |
| **VMA Region Lookup** | Parse text `/proc/<pid>/maps` via VFS | **Sub-microsecond direct kernel VMA tree query (`vma_lookup`)** |
| **Tracer Collisions** | Blocked if another tracer (e.g. anti-debug) attaches | **Operates via kernel page tables, bypassing ptrace restrictions** |

---

## Architecture & Codebase Layout

The project enforces clean architecture where every file has a single, well-defined responsibility:

```
int3-kernel/
├── build.sh                 # Complete build automation script (chmod +x)
├── run.sh                   # Driver loader and command runner (chmod +x)
├── test.sh                  # Verification suite runner: build check, selftest, live smoke (chmod +x)
├── Makefile                 # Top-level make interface (supports DKMS)
├── dkms.conf                # Dynamic Kernel Module Support configuration
├── RULES.md                 # Agent rules, Ponytail doctrine, and C23 guidelines
├── README.md                # Technical documentation and UAPI reference
├── include/
│   └── int3drv.h            # Canonical UAPI header (shared with userspace)
├── src/
│   ├── int3_internal.h      # Driver internal declarations and assertions
│   ├── init/
│   │   ├── device.c         # miscdevice registration and file operations table
│   │   └── module.c         # Module lifecycle (module_init / module_exit)
│   ├── core/
│   │   ├── context.c        # File descriptor context allocation and bounce buffer
│   │   ├── resolve.c        # Process resolution and task struct pinning
│   │   ├── validate.c       # Address boundaries and arithmetic wrap checks
│   │   └── dispatch.c       # Unlocked ioctl routing and session commands
│   ├── mem/
│   │   ├── copy_read.c      # Chunked remote-to-user read engine
│   │   ├── copy_write.c     # Chunked user-to-remote write engine
│   │   ├── rw.c             # Single-operation read/write handlers
│   │   ├── write_verify.c   # Write with in-kernel readback comparison
│   │   └── batch.c          # Multi-operation batch execution engine
│   └── ops/
│       ├── scalar.c         # Zero-allocation 32-bit and 64-bit scalar access
│       ├── deref.c          # Single-level remote pointer dereference plus displacement
│       ├── chain.c          # Multi-level pointer chain resolution
│       ├── probe.c          # Non-destructive address accessibility probe
│       ├── scan.c           # In-kernel sliding window pattern scanner
│       ├── scan_all.c       # Multi-match strided pattern scanner
│       ├── vscan.c          # In-kernel integer value scanner (eq/ne/gt/lt)
│       ├── cksum.c          # In-kernel range checksum (FNV-1a)
│       ├── copy.c           # In-kernel remote-to-remote copy
│       ├── alloc.c          # Remote memory allocation/release via mm worker thread
│       ├── thread.c         # Stop-and-hijack remote thread execution engine
│       ├── stub.S           # Position-independent hijack stub (call, flag, park)
│       ├── modbase.c        # Module base lookup by path substring
│       ├── vma.c            # Direct kernel VMA permission query
│       ├── vma_next.c       # Iterative VMA region enumeration
│       ├── maps_next.c      # VMA enumeration with backing-file paths (/proc/maps replacement)
│       └── proc_next.c      # Process enumeration with command names (/proc scan replacement)
└── tools/
    ├── Makefile             # C23 compiler driver for userspace utilities and libint3ctl.a
    ├── include/
    │   └── int3ctl.h        # Userspace tool definitions and command prototypes
    ├── util/
    │   ├── dev.c            # Device opening, validation, and shared write-request helper
    │   ├── hex.c            # Hexadecimal string parser
    │   └── dump.c           # Memory formatting and hexdump renderer
    ├── lib/
    │   └── api.c            # Thin libint3ctl wrappers (attach/read/write/read_u64/modbase)
    ├── manualmap/
    │   ├── mm.h             # Loader internal declarations
    │   ├── io.c             # File pread helper
    │   ├── elf.c            # Program header reader
    │   ├── open.c           # ELF open/validate/minv helper
    │   ├── dyn.c            # Dynamic section reader
    │   ├── copy.c           # Segment stager
    │   ├── hash.c           # GNU hash function
    │   ├── lookup.c         # Remote libc symbol lookup
    │   ├── off.c            # Vaddr-to-file-offset helper
    │   ├── reloc.c          # Relocation engine
    │   ├── sym.c            # File-local symbol offset lookup
    │   ├── load.c           # PT_LOAD stager with W^X permissions
    │   ├── run.c            # Load plus DT_INIT/INIT_ARRAY execution
    │   ├── dlfcn.c          # Load via the target's own dlopen
    │   ├── mprotectv.c      # Batched remote mprotect via syscall shellcode
    │   ├── mprotect.c       # Single-range remote mprotect
    │   └── testlib.c        # Selftest fixture .so source
    ├── commands/
    │   ├── cmd_version.c    # Driver version query
    │   ├── cmd_stats.c      # Session telemetry query
    │   ├── cmd_caps.c       # Capability bits query
    │   ├── cmd_manualmap.c  # Remote alloc/free/manual-map/thread commands
    │   ├── cmd_probe.c      # Address range accessibility prober
    │   ├── cmd_read.c       # Remote memory hex reading
    │   ├── cmd_write.c      # Remote memory raw writing
    │   ├── cmd_force.c      # Privileged force write to code pages
    │   ├── cmd_scalar.c     # Fast 64-bit scalar read/write
    │   ├── cmd_deref.c      # Remote pointer dereference
    │   ├── cmd_chain.c      # Multi-level pointer chain resolution
    │   ├── cmd_scan.c       # In-kernel signature search
    │   ├── cmd_scan_all.c   # Multi-match strided signature search
    │   ├── cmd_vscan.c      # In-kernel integer value search
    │   ├── cmd_cksum.c      # In-kernel range checksum
    │   ├── cmd_copy.c       # In-kernel remote-to-remote copy
    │   ├── cmd_vma.c        # Direct VMA metadata lookup
    │   ├── cmd_modbase.c    # Module base lookup by name
    │   ├── cmd_maps.c       # Full VMA region enumeration with backing paths
    │   ├── cmd_procs.c      # Full process enumeration with command names
    │   ├── cmd_write_verify.c # Write with in-kernel readback check
    │   ├── cmd_bench.c      # Performance and throughput benchmark
    │   └── cmd_selftest.c   # Selftest entry point (suites live in selftest/)
    ├── selftest/
    │   ├── ctx.h            # Shared selftest context and suite declarations
    │   ├── run.c            # Layout check, target spawn, suite sequencing, teardown
    │   ├── t_scalar.c       # Scalar and single read/write suites
    │   ├── t_batch.c        # Batch engine and validation suites
    │   ├── t_writes.c       # Force-write suites
    │   ├── t_validate.c     # Argument validation suite
    │   ├── t_query.c        # Probe/VMA/enumeration suites
    │   ├── t_scan.c         # Pattern scan suites
    │   ├── t_scanall.c      # Multi-match scan suites
    │   ├── t_deref.c        # Pointer dereference suites
    │   ├── t_verify.c       # Verified write and stress suites
    │   ├── t_maps_procs.c   # Maps/process enumeration suites
    │   ├── t_v5.c           # v5 ioctl suites (caps/modbase/chain/vscan/cksum/copy)
    │   ├── t_mmap.c         # v6 suites (alloc/free, end-to-end fixture load)
    │   ├── t_thread.c       # v7 suite (hijacked-thread fn execution)
    │   └── t_lifecycle.c    # Detach/attach lifecycle suite
    └── main.c               # CLI router and argument parser
```

### Build Artifact Isolation
All compiler intermediate objects (`.o`, `.cmd`, `.mod.o`, `.symvers`, `.order`) are isolated in the `build/` directory:
- `build/kernel/`: Kernel module intermediate build directory.
- `build/tools/`: Userspace CLI intermediate object directory.
- `bin/`: Final compiled binaries (`bin/int3drv.ko`, `bin/int3ctl`, `bin/libint3ctl.a`, `bin/libtestmap.so`).

---

## Quick Start

### 1. Build Everything
```bash
./build.sh
```

### 2. Load the Kernel Module
```bash
make load
# or manually:
sudo insmod int3drv.ko
```

### 2. Load & Verify Driver
```bash
./run.sh
```
`./run.sh` checks if `int3drv` is already loaded and responsive at `/dev/int3`. If not running, it loads the kernel module with `sudo insmod bin/int3drv.ko`, verifies the device node, reports status, and leaves the driver running.

You can also execute commands directly via `run.sh`:
```bash
./run.sh selftest
./run.sh version
./run.sh vma <pid> <address_hex>
```

### 3. Run Verification Tests
```bash
./test.sh
```
`test.sh` ensures build outputs exist (running `build.sh` if needed), attempts a passwordless driver load when `/dev/int3` is absent (never prompting for a password), runs `int3ctl selftest` under `timeout` when available, and adds live `version`/`stats` smoke checks when the driver is active. Without a loaded driver it runs layout-only checks and skips live tests.

### 4. Unload Driver
```bash
make unload
# or manually:
sudo rmmod int3drv
```

---

## Userspace CLI (`int3ctl`)

The repository includes a modern C23 command-line utility for interacting with the driver:

```bash
# Query driver version
./bin/int3ctl version

# Query session telemetry
./bin/int3ctl stats

# Query driver capability bits
./bin/int3ctl caps

# Find base address of a loaded module by path substring
./bin/int3ctl modbase <pid> <name>

# Follow a multi-level pointer chain and hexdump the target
./bin/int3ctl chain <pid> <ptr_hex> <len> <off1_hex> [off2_hex ...]

# Scan for an integer value (width 1, 2, 4 or 8; op 0=eq, 1=ne, 2=gt, 3=lt)
./bin/int3ctl vscan <pid> <start_hex> <end_hex> <width> <op> <value>

# Hash readable bytes in a range without streaming them out (FNV-1a)
./bin/int3ctl cksum <pid> <start_hex> <end_hex>

# Remote-to-remote copy without a userspace round-trip
./bin/int3ctl copy <pid> <src_hex> <dst_hex> <len>

# Allocate memory inside a remote process (prot letters, default RW)
./bin/int3ctl alloc <pid> <len> [RWX]

# Release a remote mapping
./bin/int3ctl free <pid> <addr_hex> <len>

# Manual-map a .so into a remote process (stages + relocates, prints init)
./bin/int3ctl manualmap <pid> <so_path>

# Manual-map a .so and run its init in one step
./bin/int3ctl load <pid> <so_path> [timeout_ms]

# Load a .so through the target's own dynamic linker
./bin/int3ctl load_libc <pid> <so_path> [timeout_ms]

# Run fn(arg) on a hijacked thread of a remote process
./bin/int3ctl thread <pid> <fn_hex> <arg_hex> [timeout_ms]

# Inspect VMA memory region and permissions
./bin/int3ctl vma <pid> <address_hex>

# Probe memory accessibility (read/write check)
./bin/int3ctl probe <pid> <address_hex> <length>

# Hexdump remote memory
./bin/int3ctl read <pid> <address_hex> <length>

# Write raw hex bytes to remote memory
./bin/int3ctl write <pid> <address_hex> <hex_string>

# Force write to read-only code pages (Ring 0 override)
./bin/int3ctl force_write <pid> <address_hex> <hex_string>

# Fast scalar read (64-bit integer)
./bin/int3ctl read64 <pid> <address_hex>

# Fast scalar write (64-bit integer)
./bin/int3ctl write64 <pid> <address_hex> <value_hex>

# In-kernel pattern scanning (with optional mask)
./bin/int3ctl scan <pid> <start_hex> <end_hex> <pattern_hex> [mask_hex]

# Multi-match pattern scanning with stride alignment (1, 2, 4 or 8)
./bin/int3ctl scan_all <pid> <start_hex> <end_hex> <pattern_hex> [mask_hex] [align]

# Write with in-kernel readback verification
./bin/int3ctl write_verify <pid> <address_hex> <hex_string>

# Follow a remote pointer plus displacement and hexdump the target
./bin/int3ctl deref <pid> <ptr_hex> <disp_hex> <len>

# List every VMA region of a process with backing-file paths (maps-like, no /proc parsing)
./bin/int3ctl maps <pid>

# List every process with its command name (no /proc scanning)
./bin/int3ctl procs

# Run performance benchmark
./bin/int3ctl bench <pid> <address_hex> [iterations]
```

---

## UAPI Specification & Programming Guide

Include the canonical header:
```c
#include "int3drv.h"
```

### 1. Attach to Target Process
Each open file descriptor maintains its own attached PID state:

```c
int fd = open("/dev/int3", O_RDWR | O_CLOEXEC);
int32_t pid = 1234;

if (ioctl(fd, INT3_IOC_ATTACH, &pid) < 0) {
    perror("attach failed");
}
```

### 2. Fast Scalar Read / Write (Zero Allocation)
Read a 64-bit value directly into the ioctl struct:

```c
struct int3_val val = {
    .remote = 0x7fff00100000ull,
    .val = 0
};

if (ioctl(fd, INT3_IOC_READ_U64, &val) == 0) {
    printf("Read value: 0x%016llx\n", val.val);
}
```

### 3. Batched Vector Transfers
Execute up to 256 reads, writes, or mixed operations in a single syscall:

```c
struct int3_op ops[2] = {
    {
        .remote = 0x7fff00100000ull,
        .local = (uint64_t)(uintptr_t)&buf1,
        .len = sizeof(buf1),
        .flags = INT3_OP_READ
    },
    {
        .remote = 0x7fff00200000ull,
        .local = (uint64_t)(uintptr_t)&buf2,
        .len = sizeof(buf2),
        .flags = INT3_OP_READ
    }
};

struct int3_batch batch = {
    .ops = (uint64_t)(uintptr_t)ops,
    .count = 2,
    .flags = INT3_BATCH_MIXED,
    .fail_idx = -1
};

if (ioctl(fd, INT3_IOC_BATCH, &batch) < 0) {
    printf("Batch failed at index: %d\n", batch.fail_idx);
}
```

### 4. In-Kernel Pattern Scanning
Scan a target virtual address range for a signature without transferring memory to userspace:

```c
uint8_t pattern[] = { 0x48, 0x89, 0x5c, 0x24 };
uint8_t mask[]    = { 0xff, 0xff, 0xff, 0xff };

struct int3_pattern scan = {
    .remote_start = 0x00400000ull,
    .remote_end   = 0x00800000ull,
    .match_addr   = 0,
    .pattern_ptr  = (uint64_t)(uintptr_t)pattern,
    .mask_ptr     = (uint64_t)(uintptr_t)mask,
    .pattern_len  = sizeof(pattern)
};

if (ioctl(fd, INT3_IOC_PATTERN_SCAN, &scan) == 0) {
    printf("Found signature at 0x%016llx\n", scan.match_addr);
}
```

### 5. Direct VMA Query
Query virtual memory region bounds and permission flags directly from the kernel memory descriptor:

```c
struct int3_vma vma = {
    .addr = 0x7fff00100000ull
};

if (ioctl(fd, INT3_IOC_VMA, &vma) == 0) {
    printf("VMA Range: [0x%llx - 0x%llx]\n", vma.vm_start, vma.vm_end);
    printf("Readable: %d, Writable: %d, Executable: %d\n",
           !!(vma.vm_flags & INT3_VM_READ),
           !!(vma.vm_flags & INT3_VM_WRITE),
           !!(vma.vm_flags & INT3_VM_EXEC));
}
```

### 6. VMA Region Enumeration
Walk the whole address space without parsing `/proc/<pid>/maps`. Pass `0` as the first cursor, then feed each returned `vm_end` back as the next cursor until `-ENOENT`:

```c
struct int3_vma_next nx = { .cursor = 0 };

while (ioctl(fd, INT3_IOC_VMA_NEXT, &nx) == 0) {
    printf("[0x%llx - 0x%llx]\n", nx.vm_start, nx.vm_end);
    nx.cursor = nx.vm_end;
}
```

The synthetic vsyscall gate shown by `/proc/<pid>/maps` is not a real VMA of the target and is therefore not enumerated.

### 7. Pointer Dereference
Resolve one pointer level plus an unsigned displacement and read the target in a single syscall. Composable: feed `out_ptr` back for deeper chains:

```c
uint8_t buf[64];

struct int3_deref d = {
    .ptr_addr = 0x7fff00100000ull,
    .disp = 0x28,
    .local = (uint64_t)(uintptr_t)buf,
    .out_ptr = 0,
    .len = sizeof(buf),
};

if (ioctl(fd, INT3_IOC_DEREF, &d) == 0) {
    printf("Resolved pointer: 0x%llx\n", d.out_ptr);
}
```

### 8. Multi-Match Strided Scan
Collect up to `INT3_MAX_MATCHES` hits at absolute addresses divisible by `align` (1, 2, 4 or 8). Returns `-ENOSPC` with a full buffer when more matches exist:

```c
uint64_t hits[256];

struct int3_scan_all sa = {
    .remote_start = 0x00400000ull,
    .remote_end   = 0x00800000ull,
    .match_ptr    = (uint64_t)(uintptr_t)hits,
    .pattern_ptr  = (uint64_t)(uintptr_t)pattern,
    .mask_ptr     = (uint64_t)(uintptr_t)mask,
    .match_cap    = 256,
    .pattern_len  = sizeof(pattern),
    .align        = 8,
};

if (ioctl(fd, INT3_IOC_SCAN_ALL, &sa) == 0) {
    printf("%u matches\n", sa.match_count);
}
```

### 9. Verified Write
Write bytes and have the kernel compare them back in the same call. Returns `-EIO` on mismatch. Also available per-operation inside batches via `INT3_OP_WRITE_VERIFY`:

```c
struct int3_rw rw = {
    .remote = 0x7fff00100000ull,
    .local = (uint64_t)(uintptr_t)buf,
    .len = sizeof(buf),
};

if (ioctl(fd, INT3_IOC_WRITE_VERIFY, &rw) < 0) {
    perror("verified write failed");
}
```

### 10. VMA Enumeration With Backing Paths
Walk the whole address space like section 6, but each region also carries its backing-file path (empty string for anonymous mappings), so callers can filter by path without parsing `/proc/<pid>/maps`. Pass `0` as the first cursor, then feed each returned `vm_end` back as the next cursor until `-ENOENT`:

```c
char path[512];

struct int3_map m = {
    .cursor = 0,
    .path_ptr = (uint64_t)(uintptr_t)path,
    .path_cap = sizeof(path),
};

while (ioctl(fd, INT3_IOC_MAPS_NEXT, &m) == 0) {
    printf("[0x%llx - 0x%llx] %s\n", m.vm_start, m.vm_end, path);
    m.cursor = m.vm_end;
}
```

`path_len` reports the bytes written including the `NUL` terminator. Paths resolve against the caller's mount namespace; mappings unreachable from it (or without a backing file) report an empty string.

### 11. Process Enumeration
List every process id with its command name without scanning `/proc`. Pass `0` as the first cursor, then feed each returned `pid` back as the next cursor until `-ENOENT`:

```c
struct int3_proc p = { .cursor = 0 };

while (ioctl(fd, INT3_IOC_PROC_NEXT, &p) == 0) {
    printf("%u %s\n", p.pid, p.comm);
    p.cursor = p.pid;
}
```

### 12. Module Base Lookup
Find the lowest base address whose backing-file path contains `name` as a substring, without parsing `/proc/<pid>/maps`:

```c
const char want[] = "libc";

struct int3_mod m = {
    .name_ptr = (uint64_t)(uintptr_t)want,
    .base = 0,
    .name_len = sizeof(want) - 1,
};

if (ioctl(fd, INT3_IOC_MOD_BASE, &m) == 0) {
    printf("base: 0x%llx\n", m.base);
}
```

Returns `-ENOENT` when no mapping matches. The name must be non-empty, at most `INT3_MAX_NAME` bytes, and contain no `NUL`.

### 13. Multi-Level Pointer Chain
Resolve `depth` pointer levels (up to `INT3_MAX_CHAIN`) plus per-level offsets in a single syscall, then read the final target. With `depth == 0` this reads `len` bytes at `*(ptr_addr)`:

```c
uint64_t offs[] = { 0x28, 0x10 };
uint8_t buf[64];

struct int3_chain c = {
    .ptr_addr = 0x7fff00100000ull,
    .local = (uint64_t)(uintptr_t)buf,
    .out_ptr = 0,
    .offsets = (uint64_t)(uintptr_t)offs,
    .len = sizeof(buf),
    .depth = 2,
};

if (ioctl(fd, INT3_IOC_CHAIN, &c) == 0) {
    printf("target: 0x%llx\n", c.out_ptr);
}
```

### 14. Integer Value Scan
Collect up to `match_cap` addresses holding an integer comparison hit, strided by the value width (1, 2, 4 or 8 bytes, little-endian). Operators are `INT3_VSCAN_EQ/NE/GT/LT` (unsigned). Same `-ENOSPC`-on-truncation contract as section 8:

```c
uint64_t hits[256];

struct int3_vscan vs = {
    .remote_start = 0x00400000ull,
    .remote_end   = 0x00800000ull,
    .match_ptr    = (uint64_t)(uintptr_t)hits,
    .value        = 100,
    .match_cap    = 256,
    .match_count  = 0,
    .width        = 4,
    .op           = INT3_VSCAN_EQ,
};

if (ioctl(fd, INT3_IOC_VSCAN, &vs) == 0) {
    printf("%u matches\n", vs.match_count);
}
```

### 15. Range Checksum
Hash the readable bytes of a range with FNV-1a without streaming them to userspace. Unreadable gaps are skipped like the scanners; returns `-ENOENT` when nothing was hashed:

```c
struct int3_cksum ck = {
    .remote_start = 0x00400000ull,
    .remote_end   = 0x00800000ull,
    .hash         = 0,
};

if (ioctl(fd, INT3_IOC_CHECKSUM, &ck) == 0) {
    printf("hash: 0x%llx\n", ck.hash);
}
```

### 16. Remote-to-Remote Copy
Copy bytes inside the attached target without a userspace round-trip:

```c
struct int3_copy cp = {
    .src = 0x7fff00100000ull,
    .dst = 0x7fff00200000ull,
    .len = 64,
};

if (ioctl(fd, INT3_IOC_COPY, &cp) < 0) {
    perror("remote copy failed");
}
```

### 17. Capability Bits
Query one bitmask instead of gating on version numbers. `INT3_CAPS_ALL` is the current full set (`MODBASE`, `CHAIN`, `VSCAN`, `CHECKSUM`, `COPY`, `ALLOC`):

```c
uint64_t caps = 0;

if (ioctl(fd, INT3_IOC_CAPS, &caps) == 0 && (caps & INT3_CAP_VSCAN)) {
    printf("value scan available\n");
}
```

### 18. Remote Allocation
Allocate anonymous memory inside the attached target without touching it first. The driver runs `vm_mmap` from a short-lived worker thread bound to the target mm (`kthread_use_mm`; all symbols long-exported, no version hacks). Pass `PROT_READ/WRITE/EXEC` bits; the hint is advisory, never `MAP_FIXED`:

```c
struct int3_alloc a = {
    .hint = 0,
    .addr = 0,
    .len = 65536,
    .prot = PROT_READ | PROT_WRITE,
};

if (ioctl(fd, INT3_IOC_ALLOC, &a) == 0) {
    printf("remote mapping: 0x%llx\n", a.addr);
}

struct int3_free f = { .addr = a.addr, .len = 65536 };

if (ioctl(fd, INT3_IOC_FREE, &f) < 0) {
    perror("remote free failed");
}
```

### 19. Manual .so Mapping
`mm_load()` (`tools/manualmap/manualmap.c`, also in `libint3ctl.a` consumers via `int3ctl.h`) stages an `ET_DYN` x86-64 object into memory allocated with section 18: it copies `PT_LOAD` segments, applies `RELATIVE` relocations, resolves `GLOB_DAT`/`JUMP_SLOT` externals against the target's libc through its GNU hash table (weak-undefined slots stay zero), and returns the base plus the `DT_INIT`/`DT_INIT_ARRAY[0]` entry point:

```c
uint64_t base = 0, init = 0, size = 0;

if (mm_load(fd, pid, "./payload.so", &base, &init, &size) == 0) {
    printf("mapped at 0x%llx, init at 0x%llx\n", base, init);
}
```

One call to map and run: `mm_load_run()` stages like `mm_load`, then executes `init` on a hijacked thread (section 20) and returns its address:

```c
uint64_t base = 0, init = 0;

if (mm_load_run(fd, pid, "./payload.so", 5000, &base, &init) == 0) {
    printf("loaded at 0x%llx, init 0x%llx ran\n", base, init);
}
```

Second mode, `mm_load_libc()`: instead of staging bytes, resolve the target's `dlopen` through its own GNU hash, stage the path plus a 19-byte `RTLD_NOW` trampoline, and run it on a hijacked thread. The real dynamic linker then handles everything the manual mapper ceilings out: TLS, dependencies, versioned symbols, `IFUNC` resolvers. Needs a glibc target and a path visible from the target's mount namespace:

```c
uint64_t handle = 0;

if (mm_load_libc(fd, pid, "./payload.so", 5000, &handle) == 0) {
    printf("dlopened: 0x%llx\n", handle);
}
```

Remote symbol lookup (`mm_lookup_remote()`, shared by both modes) reads the target's `.dynamic` and walks its GNU hash. Note it expects pre-slid address fields: current linkers emit relocations covering `.dynamic` itself, so a running object's table holds absolute addresses — the lookup uses them as-is and only adds the base as a fallback when the first read faults.

Deliberate ceilings: segments are staged `RW` and then protected per `PT_LOAD` flags (text ends `R-X`, no `RWX` footprint), static TLS is not relocated, only GNU-hash externals resolve, and `IRELATIVE` resolvers run with a baseline hwcap argument. Execution: `mm_load_run()` runs `DT_INIT` plus each `DT_INIT_ARRAY` entry through section 20, or point an existing hook (GOT entry, vtable slot, lifecycle callback) at `init`. `mm_sym_off()` resolves a file-local symbol offset (`.symtab`, then `.dynsym`) for locating flags and callbacks in the staged image.

Remote `mprotect` needs no kernel operation: `mm_mprotectv()` stages a tiny `mprotect` syscall shellcode (one per range, error-accumulating, single parked thread) and runs it through section 20, so per-range protections apply without ever reimplementing page-table surgery in the module:

### 20. Remote Thread Execution
Run `fn(arg)` in the attached target without `ptrace` (which fails against already-traced or anti-debug processes). The driver `SIGSTOP`s one sleeping thread, stages a position-independent stub plus stack into one fresh mapping, points the thread at it, and `SIGCONT`s. The stub calls `fn(arg)`, sets a done flag, and parks itself in a `nanosleep` loop:

```c
struct int3_thread t = {
    .fn = init_addr,
    .arg = 0,
    .timeout_ms = 5000,
    .tid = -1,
};

if (ioctl(fd, INT3_IOC_THREAD, &t) == 0) {
    printf("ran in tid %d\n", t.tid);
}
```

Deliberate ceilings, read before relying on this: x86-64 only; the whole thread group hitch-stops for milliseconds; the hijacked thread never resumes its old life (its abandoned syscall is gone, one thread plus 20 KiB stay parked per call until process exit, so do not spam it); `fn` must return (a hanging `fn` burns the timeout and leaves the parked thread); the stub entry carries a 2-byte NOP slide because the kernel rewinds `rip` by 2 when resuming a thread parked in a syscall. `tid` reports the hijacked thread. Never hijack the only thread of the process supervising the caller (e.g. your own shell waiting on `int3ctl`): the supervisor stays parked and the call appears to hang. Thread *creation* from a module stays refused: manufacturing a user-context thread needs `copy_process`, which is unexported, so any reimplementation would be version-locked fragility rather than this stop-and-hijack design built only on long-exported symbols.

---

## Userspace Library (`libint3ctl.a`)

`tools/lib/api.c` plus `tools/manualmap/` build a thin static library for downstream consumers instead of hand-rolling ioctl wrappers: attach/read/write/read_u64/modbase plus the full loader (`mm_load`, `mm_load_run`, `mm_mprotect`, `mm_sym_off`). No CLI dependency:

```c
int fd = int3_open_device();
int3_attach(fd, pid);
int3_read(fd, addr, buf, len);
int3_write(fd, addr, buf, len);
int3_read_u64(fd, addr, &val);
int3_modbase(fd, "libc", &base);
```

Link with `bin/libint3ctl.a` and include `tools/include/int3ctl.h` plus the canonical `include/int3drv.h`. More wrappers graduate here when a second in-tree consumer needs them.

---

## Design Notes

Driver version is `7`. All v2–v6 structs and ioctls are unchanged and keep working; new callers should gate the section 6–9 interfaces on `INT3_IOC_VERSION >= 3`, the sections 10–11 interfaces on `INT3_IOC_VERSION >= 4`, the sections 12–17 interfaces on `INT3_IOC_VERSION >= 5`, the sections 18–19 interfaces on `INT3_IOC_VERSION >= 6`, and section 20 on `INT3_IOC_VERSION >= 7` (or, preferably, on `INT3_IOC_CAPS` bits).

There is deliberately no protection-change (`mprotect`) operation: the kernel exports no remote-mprotect machinery to loadable modules (`mprotect_fixup`, `change_protection` and `do_mprotect_pkey` are all unexported; the latter is static), so an in-driver reimplementation would be version-locked fragility rather than a stable interface.

---

## DKMS Setup

To have `int3drv` automatically rebuild on kernel upgrades:

```bash
sudo cp -r . /usr/src/int3drv-7.0
sudo dkms add -m int3drv -v 7.0
sudo dkms build -m int3drv -v 7.0
sudo dkms install -m int3drv -v 7.0
```

---

## License

GPL-2.0
