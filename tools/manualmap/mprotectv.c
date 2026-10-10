#include "mm.h"

int mm_mprotectv(int fd, int pid, const uint64_t *addrs, const uint64_t *lens,
		 const uint32_t *prots, size_t n)
{
	uint8_t sc[16 * 40];
	struct int3_alloc a = {
		.hint = 0,
		.addr = 0,
		.len = 4096,
		.prot = PROT_READ | PROT_WRITE | PROT_EXEC,
		._rsv = 0,
	};
	struct int3_thread t;
	struct int3_rw rw;
	struct int3_free f;
	size_t off = 0;
	int rc = -1;

	if (!n || n > 16)
		return -1;
	if (int3_attach(fd, pid) < 0)
		return -1;
	memcpy(sc + off, "\x4d\x31\xff", 3);
	off += 3;
	for (size_t i = 0; i < n; i++) {
		uint64_t addr = addrs[i];
		uint64_t len = lens[i];
		uint32_t prot = prots[i];

		if (!len || addr + len < addr)
			return -1;
		addr &= ~(uint64_t)4095;
		len = (len + 4095) & ~(uint64_t)4095;
		memcpy(sc + off, "\xb8\x0a\x00\x00\x00\x48\xbf", 7);
		off += 7;
		memcpy(sc + off, &addr, 8);
		off += 8;
		memcpy(sc + off, "\x48\xbe", 2);
		off += 2;
		memcpy(sc + off, &len, 8);
		off += 8;
		memcpy(sc + off, "\xba", 1);
		off += 1;
		memcpy(sc + off, &prot, 4);
		off += 4;
		memcpy(sc + off, "\x0f\x05\x49\x09\xc7", 5);
		off += 5;
	}
	memcpy(sc + off, "\x4c\x89\xf8\xc3", 4);
	off += 4;
	if (ioctl(fd, INT3_IOC_ALLOC, &a) < 0 || !a.addr)
		return -1;
	rw.remote = a.addr;
	rw.local = (uint64_t)(uintptr_t)sc;
	rw.len = off;
	if (ioctl(fd, INT3_IOC_WRITE, &rw) < 0)
		goto out;
	t.fn = a.addr;
	t.arg = 0;
	t.timeout_ms = 5000;
	t.tid = -1;
	t.retval = 0;
	if (ioctl(fd, INT3_IOC_THREAD, &t) < 0)
		goto out;
	if ((int64_t)t.retval < 0)
		goto out;
	rc = 0;
out:
	f.addr = a.addr;
	f.len = 4096;
	(void)ioctl(fd, INT3_IOC_FREE, &f);
	return rc;
}
