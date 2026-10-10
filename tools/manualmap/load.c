#include "mm.h"

int mm_load(int fd, int pid, const char *path, uint64_t *base, uint64_t *init,
	    uint64_t *size)
{
	int ffd = -1;
	Elf64_Ehdr eh;
	Elf64_Phdr *ph = NULL;
	uint16_t n = 0;
	uint64_t minv = 0;
	uint64_t maxv = 0;
	uint64_t span;
	uint64_t addr = 0;
	Elf64_Dyn *dyn = NULL;
	int rc = -1;

	if (mm_open_elf(path, &ffd, &eh, &ph, &n, &minv))
		return -1;
	for (uint16_t i = 0; i < n; i++) {
		if (ph[i].p_type != PT_LOAD || !ph[i].p_memsz)
			continue;
		if (ph[i].p_vaddr + ph[i].p_memsz > maxv)
			maxv = ph[i].p_vaddr + ph[i].p_memsz;
	}
	if (!maxv || minv >= maxv)
		goto out;
	maxv = (maxv + 4095) & ~(uint64_t)4095;
	span = maxv - minv;
	if (!span || span > INT3_MAX_SINGLE)
		goto out;
	if (int3_attach(fd, pid) < 0)
		goto out;
	{
		struct int3_alloc a = {
			.hint = 0,
			.addr = 0,
			.len = span,
			.prot = PROT_READ | PROT_WRITE,
			._rsv = 0,
		};
		if (ioctl(fd, INT3_IOC_ALLOC, &a) < 0 || !a.addr)
			goto out;
		addr = a.addr;
	}
	if (mm_copy_load(fd, ffd, addr, minv, ph, n))
		goto fail_mapped;
	if (mm_read_dyn(ffd, ph, n, &dyn))
		goto fail_mapped;
	if (dyn && mm_relocate(fd, ffd, addr, minv, ph, n, dyn))
		goto fail_mapped;
	{
		uint64_t paddrs[16];
		uint64_t plens[16];
		uint32_t pprots[16];
		size_t pcnt = 0;

		for (uint16_t i = 0; i < n; i++) {
			uint32_t prot = 0;

			if (ph[i].p_type != PT_LOAD || !ph[i].p_memsz)
				continue;
			if (pcnt >= 16)
				goto fail_mapped;
			if (ph[i].p_flags & PF_R)
				prot |= PROT_READ;
			if (ph[i].p_flags & PF_W)
				prot |= PROT_WRITE;
			if (ph[i].p_flags & PF_X)
				prot |= PROT_EXEC;
			paddrs[pcnt] = addr + (ph[i].p_vaddr - minv);
			plens[pcnt] = ph[i].p_memsz;
			pprots[pcnt] = prot;
			pcnt++;
		}
		if (mm_mprotectv(fd, pid, paddrs, plens, pprots, pcnt))
			goto fail_mapped;
	}
	*base = addr;
	*size = span;
	*init = 0;
	if (dyn) {
		for (Elf64_Dyn *e = dyn; e->d_tag; e++) {
			if (e->d_tag == DT_INIT && e->d_un.d_ptr) {
				*init = addr + (e->d_un.d_ptr - minv);
				break;
			}
		}
		if (!*init) {
			for (Elf64_Dyn *e = dyn; e->d_tag; e++) {
				if (e->d_tag == DT_INIT_ARRAY && e->d_un.d_ptr) {
					uint64_t fn = 0;
					struct int3_rw rw = {
						.remote = addr + (e->d_un.d_ptr - minv),
						.local = (uint64_t)(uintptr_t)&fn,
						.len = sizeof(fn),
					};
					if (ioctl(fd, INT3_IOC_READ, &rw) == 0 && fn)
						*init = fn;
					break;
				}
			}
		}
	}
	rc = 0;
	goto out_d;
fail_mapped:
	{
		struct int3_free f = {.addr = addr, .len = span};
		(void)ioctl(fd, INT3_IOC_FREE, &f);
	}
out_d:
	free(dyn);
out:
	free(ph);
	if (ffd >= 0)
		close(ffd);
	return rc;
}
