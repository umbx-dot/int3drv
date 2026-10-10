#include "mm.h"

int mm_load_run(int fd, int pid, const char *path, uint32_t timeout_ms,
		uint64_t *base, uint64_t *init, uint64_t *size)
{
	uint64_t entry = 0;
	struct int3_thread t;

	if (mm_load(fd, pid, path, base, &entry, size) < 0 || !entry)
		return -1;
	t.fn = entry;
	t.arg = 0;
	t.timeout_ms = timeout_ms ? timeout_ms : 5000;
	t.tid = -1;
	t.retval = 0;
	if (ioctl(fd, INT3_IOC_THREAD, &t) < 0)
		return -1;
	*init = entry;
	{
		int ffd = -1;
		Elf64_Ehdr eh;
		Elf64_Phdr *ph = NULL;
		uint16_t n = 0;
		uint64_t minv = 0;
		Elf64_Dyn *dyn = NULL;
		uint64_t arr = 0;
		uint64_t arrsz = 0;

		if (mm_open_elf(path, &ffd, &eh, &ph, &n, &minv))
			goto no_array;
		if (mm_read_dyn(ffd, ph, n, &dyn))
			goto no_array_fd;
		if (dyn) {
			for (Elf64_Dyn *e = dyn; e->d_tag; e++) {
				if (e->d_tag == DT_INIT_ARRAY && e->d_un.d_ptr)
					arr = *base + (e->d_un.d_ptr - minv);
				else if (e->d_tag == DT_INIT_ARRAYSZ)
					arrsz = e->d_un.d_val;
			}
			free(dyn);
			dyn = NULL;
		}
		free(ph);
		ph = NULL;
		close(ffd);
		if (arr && arrsz && arrsz <= 64 * 8) {
			uint64_t fns[64];
			size_t count = (size_t)arrsz / 8;
			struct int3_rw rw = {
				.remote = arr,
				.local = (uint64_t)(uintptr_t)fns,
				.len = arrsz,
			};
			if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
				return -1;
			for (size_t i = 0; i < count; i++) {
				if (!fns[i] || fns[i] == entry)
					continue;
				t.fn = fns[i];
				t.tid = -1;
				t.retval = 0;
				if (ioctl(fd, INT3_IOC_THREAD, &t) < 0)
					return -1;
			}
		}
		return 0;
no_array_fd:
		free(ph);
		free(dyn);
		close(ffd);
no_array:
		return 0;
	}
}
