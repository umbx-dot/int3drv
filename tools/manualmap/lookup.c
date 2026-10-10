#include "mm.h"

int mm_lookup_remote(int fd, uint64_t libc, const char *name, uint64_t *sym)
{
	Elf64_Ehdr eh;
	Elf64_Phdr *ph = NULL;
	uint16_t n = 0;
	uint64_t dyn = 0;
	uint64_t dynsz = 0;
	Elf64_Dyn *d = NULL;
	uint64_t strtab = 0;
	uint64_t symtab = 0;
	uint64_t hashtab = 0;
	uint64_t syment = 0;
	struct int3_rw rw;
	int rc = -1;

	rw.remote = libc;
	rw.local = (uint64_t)(uintptr_t)&eh;
	rw.len = sizeof(eh);
	if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
		goto out;
	if (memcmp(eh.e_ident, ELFMAG, SELFMAG) || eh.e_phnum == 0 || eh.e_phnum > 64)
		goto out;
	ph = malloc((size_t)eh.e_phnum * sizeof(*ph));
	if (!ph)
		goto out;
	rw.remote = libc + eh.e_phoff;
	rw.local = (uint64_t)(uintptr_t)ph;
	rw.len = (size_t)eh.e_phnum * sizeof(*ph);
	if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
		goto out;
	n = eh.e_phnum;
	for (uint16_t i = 0; i < n; i++) {
		if (ph[i].p_type != PT_DYNAMIC || !ph[i].p_memsz)
			continue;
		dyn = libc + ph[i].p_vaddr;
		dynsz = ph[i].p_memsz;
	}
	if (!dyn)
		goto out;
	d = malloc((size_t)dynsz);
	if (!d)
		goto out;
	rw.remote = dyn;
	rw.local = (uint64_t)(uintptr_t)d;
	rw.len = dynsz;
	if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
		goto out;
	for (Elf64_Dyn *e = d; e->d_tag; e++) {
		if (e->d_tag == DT_STRTAB)
			strtab = e->d_un.d_ptr;
		else if (e->d_tag == DT_SYMTAB)
			symtab = e->d_un.d_ptr;
		else if (e->d_tag == DT_GNU_HASH)
			hashtab = e->d_un.d_ptr;
		else if (e->d_tag == DT_SYMENT)
			syment = e->d_un.d_val;
	}
	if (!strtab || !symtab || !hashtab || syment != sizeof(Elf64_Sym))
		goto out;
	uint32_t head[4] = {0};
	rw.remote = hashtab;
	rw.local = (uint64_t)(uintptr_t)head;
	rw.len = sizeof(head);
	if (ioctl(fd, INT3_IOC_READ, &rw) < 0) {
		strtab += libc;
		symtab += libc;
		hashtab += libc;
		rw.remote = hashtab;
		if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
			goto out;
	}
	uint32_t nbucket = head[0];
	uint32_t symoff = head[1];
	uint32_t maskwords = head[2];
	uint64_t bloom_at = hashtab + 16;
	uint64_t buckets_at = bloom_at + (uint64_t)maskwords * 8;
	uint64_t chain_at = buckets_at + (uint64_t)nbucket * 4;
	uint64_t h = mm_gnu_hash(name);
	uint32_t h32 = (uint32_t)h;
	uint32_t bkt = 0;
	rw.remote = buckets_at + (uint64_t)(h32 % nbucket) * 4;
	rw.local = (uint64_t)(uintptr_t)&bkt;
	rw.len = sizeof(bkt);
	if (ioctl(fd, INT3_IOC_READ, &rw) < 0 || !bkt)
		goto out;
	for (uint32_t idx = bkt;; idx++) {
		Elf64_Sym s = {0};
		uint32_t hc = 0;
		char sname[256];
		size_t nl = strlen(name);

		rw.remote = symtab + (uint64_t)idx * sizeof(s);
		rw.local = (uint64_t)(uintptr_t)&s;
		rw.len = sizeof(s);
		if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
			goto out;
		rw.remote = chain_at + (uint64_t)(idx - symoff) * 4;
		rw.local = (uint64_t)(uintptr_t)&hc;
		rw.len = sizeof(hc);
		if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
			goto out;
		if ((hc | 1u) != (h32 | 1u)) {
			if (hc & 1u)
				break;
			continue;
		}
		if (nl > sizeof(sname) - 1 || !s.st_name)
			goto out;
		rw.remote = strtab + s.st_name;
		rw.local = (uint64_t)(uintptr_t)sname;
		rw.len = nl + 1;
		if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
			goto out;
		sname[sizeof(sname) - 1] = '\0';
		if (!memcmp(sname, name, nl + 1)) {
			uint64_t v = s.st_value;
			if (s.st_shndx == SHN_UNDEF || !v)
				goto out;
			if (v < 0x100000000ull)
				v += libc;
			*sym = v;
			rc = 0;
			goto out;
		}
		if (hc & 1u)
			break;
	}
out:
	free(ph);
	free(d);
	return rc;
}
