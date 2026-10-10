#ifndef INT3CTL_H
#define INT3CTL_H

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "int3drv.h"

[[nodiscard]] int int3_open_device(void);
int int3_write_req(int pid, uint64_t addr, const uint8_t *buf, size_t len, unsigned long req,
		   const char *err, const char *ok_fmt);
int int3_scan_print(int fd, unsigned long req, void *s, const uint64_t *matches,
		    uint32_t *count, const char *err);
[[nodiscard]] bool parse_prot(const char *s, uint32_t *prot);
[[nodiscard]] bool parse_hex_bytes(const char *hex, uint8_t *out, size_t *out_len, size_t max_len);
void print_hexdump(uint64_t base_addr, const uint8_t *data, size_t len);

int cmd_version(void);
int cmd_stats(void);
int cmd_probe(int pid, uint64_t addr, uint64_t len);
int cmd_read(int pid, uint64_t addr, uint64_t len);
int cmd_write(int pid, uint64_t addr, const char *hex_str);
int cmd_force_write(int pid, uint64_t addr, const char *hex_str);
int cmd_read64(int pid, uint64_t addr);
int cmd_write64(int pid, uint64_t addr, uint64_t val);
int cmd_scan(int pid, uint64_t start, uint64_t end, const char *pat_hex, const char *mask_hex);
int cmd_scan_all(int pid, uint64_t start, uint64_t end, const char *pat_hex, const char *mask_hex,
		 uint32_t align);
int cmd_vma(int pid, uint64_t addr);
int cmd_modbase(int pid, const char *name);
int cmd_chain(int pid, uint64_t ptr_addr, uint64_t len, uint32_t depth, const uint64_t *offs);
int cmd_vscan(int pid, uint64_t start, uint64_t end, uint32_t width, uint32_t op, uint64_t val);
int cmd_cksum(int pid, uint64_t start, uint64_t end);
int cmd_copy(int pid, uint64_t src, uint64_t dst, uint64_t len);
int cmd_caps(void);
int cmd_manualmap(int pid, const char *path);
int cmd_load(int pid, const char *path, uint32_t timeout_ms);
int cmd_load_libc(int pid, const char *path, uint32_t timeout_ms);
int cmd_alloc(int pid, uint64_t len, uint32_t prot);
int cmd_free(int pid, uint64_t addr, uint64_t len);
int cmd_mprotect(int pid, uint64_t addr, uint64_t len, uint32_t prot);
int cmd_thread(int pid, uint64_t fn, uint64_t arg, uint32_t timeout_ms);
int cmd_maps(int pid);
int cmd_procs(void);
int cmd_deref(int pid, uint64_t ptr_addr, uint64_t disp, uint64_t len);
int cmd_write_verify(int pid, uint64_t addr, const char *hex_str);
int cmd_bench(int pid, uint64_t addr, uint32_t iterations);
int cmd_selftest(void);

int int3_attach(int fd, int pid);
int int3_open_attached(int pid);
int int3_read(int fd, uint64_t addr, void *buf, uint64_t len);
int int3_write(int fd, uint64_t addr, const void *buf, uint64_t len);
int int3_read_u64(int fd, uint64_t addr, uint64_t *val);
int int3_modbase(int fd, const char *name, uint64_t *base);

int mm_load(int fd, int pid, const char *path, uint64_t *base, uint64_t *init,
	    uint64_t *size);
int mm_load_run(int fd, int pid, const char *path, uint32_t timeout_ms,
		uint64_t *base, uint64_t *init, uint64_t *size);
int mm_load_libc(int fd, int pid, const char *path, uint32_t timeout_ms, uint64_t *handle);
uint64_t mm_sym_off(const char *path, const char *name);
int mm_mprotect(int fd, int pid, uint64_t addr, uint64_t len, uint32_t prot);
int mm_mprotectv(int fd, int pid, const uint64_t *addrs, const uint64_t *lens,
		 const uint32_t *prots, size_t n);

#endif
