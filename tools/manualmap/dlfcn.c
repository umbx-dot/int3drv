#include "mm.h"

int mm_load_libc(int fd, int pid, const char *path, uint32_t timeout_ms, uint64_t *handle)
{
	size_t plen = strlen(path);
	struct int3_alloc a = {
		.hint = 0,
		.addr = 0,
		.len = 4096,
		.prot = PROT_READ | PROT_WRITE | PROT_EXEC,
		._rsv = 0,
	};
	uint8_t sc[19];
	struct int3_rw rw;
	struct int3_thread t;
	struct int3_free f;
	uint64_t dlopen = 0;
	uint64_t libc = 0;
	int rc = -1;

	if (!plen || plen > 4000)
		return -1;
	if (int3_attach(fd, pid) < 0)
		return -1;
	if (int3_modbase(fd, "libc", &libc) < 0)
		return -1;
	if (mm_lookup_remote(fd, libc, "dlopen", &dlopen) < 0 || !dlopen)
		return -1;
	memcpy(sc, "\xbe\x02\x00\x00\x00\xff\x25\x00\x00\x00\x00", 11);
	memcpy(sc + 11, &dlopen, 8);
	if (ioctl(fd, INT3_IOC_ALLOC, &a) < 0 || !a.addr)
		return -1;
	rw.remote = a.addr;
	rw.local = (uint64_t)(uintptr_t)sc;
	rw.len = sizeof(sc);
	if (ioctl(fd, INT3_IOC_WRITE, &rw) < 0)
		goto out;
	rw.remote = a.addr + sizeof(sc);
	rw.local = (uint64_t)(uintptr_t)path;
	rw.len = plen + 1;
	if (ioctl(fd, INT3_IOC_WRITE, &rw) < 0)
		goto out;
	t.fn = a.addr;
	t.arg = a.addr + sizeof(sc);
	t.timeout_ms = timeout_ms ? timeout_ms : 5000;
	t.tid = -1;
	t.retval = 0;
	if (ioctl(fd, INT3_IOC_THREAD, &t) < 0)
		goto out;
	if (!t.retval)
		goto out;
	*handle = t.retval;
	rc = 0;
out:
	f.addr = a.addr;
	f.len = 4096;
	(void)ioctl(fd, INT3_IOC_FREE, &f);
	return rc;
}
