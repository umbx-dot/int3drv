#include "mm.h"

int mm_copy_load(int fd, int ffd, uint64_t base, uint64_t minv, const Elf64_Phdr *ph,
			uint16_t n)
{
	uint8_t *zero = calloc(1, 65536);
	uint8_t *buf = malloc(65536);
	struct int3_rw rw;
	int rc = -1;

	if (!zero || !buf)
		goto out;
	for (uint16_t i = 0; i < n; i++) {
		uint64_t va;
		uint64_t left;
		uint64_t off;

		if (ph[i].p_type != PT_LOAD || !ph[i].p_memsz)
			continue;
		va = base + (ph[i].p_vaddr - minv);
		left = ph[i].p_filesz;
		off = ph[i].p_offset;
		while (left) {
			size_t c = left > 65536 ? 65536 : (size_t)left;
			if (mm_read_at(ffd, buf, c, (off_t)off))
				goto out;
			rw.remote = va;
			rw.local = (uint64_t)(uintptr_t)buf;
			rw.len = c;
			if (ioctl(fd, INT3_IOC_WRITE, &rw) < 0)
				goto out;
			va += c;
			off += c;
			left -= c;
		}
		left = ph[i].p_memsz - ph[i].p_filesz;
		while (left) {
			size_t c = left > 65536 ? 65536 : (size_t)left;
			rw.remote = va;
			rw.local = (uint64_t)(uintptr_t)zero;
			rw.len = c;
			if (ioctl(fd, INT3_IOC_WRITE, &rw) < 0)
				goto out;
			va += c;
			left -= c;
		}
	}
	rc = 0;
out:
	free(zero);
	free(buf);
	return rc;
}
