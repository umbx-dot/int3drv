#ifndef MM_H
#define MM_H

#include "int3ctl.h"

#include <elf.h>

int mm_read_at(int ffd, void *buf, size_t len, off_t off);
int mm_phdrs(int ffd, const Elf64_Ehdr *eh, Elf64_Phdr **out, uint16_t *out_n);
int mm_copy_load(int fd, int ffd, uint64_t base, uint64_t minv, const Elf64_Phdr *ph,
		 uint16_t n);
uint64_t mm_gnu_hash(const char *s);
int mm_lookup_remote(int fd, uint64_t libc, const char *name, uint64_t *sym);
int64_t mm_file_off(uint64_t vaddr, const Elf64_Phdr *ph, uint16_t n);
int mm_relocate(int fd, int ffd, uint64_t base, uint64_t minv, const Elf64_Phdr *ph,
		uint16_t n, Elf64_Dyn *d);
int mm_open_elf(const char *path, int *ffd, Elf64_Ehdr *eh, Elf64_Phdr **ph, uint16_t *n,
		uint64_t *minv);
int mm_read_dyn(int ffd, const Elf64_Phdr *ph, uint16_t n, Elf64_Dyn **dyn);

#endif
