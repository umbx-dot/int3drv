#include "ctx.h"

int st_scalar(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;
	struct int3_val *vp = &c->val;

	if (ioctl(fd, INT3_IOC_READ_U64, vp) < 0 || (uint64_t)vp->val != 0x1122334455667788ull) {
		fprintf(stderr, "[selftest] FAIL: read_u64 got 0x%" PRIx64 "\n", (uint64_t)vp->val);
		return st_fail(c);
	}
	printf("[selftest] read_u64: PASS (0x%" PRIx64 ")\n", (uint64_t)vp->val);

	vp->val = 0x9988776655443322ull;
	if (ioctl(fd, INT3_IOC_WRITE_U64, vp) < 0) {
		fprintf(stderr, "[selftest] FAIL: write_u64 failed\n");
		return st_fail(c);
	}

	vp->val = 0;
	if (ioctl(fd, INT3_IOC_READ_U64, vp) < 0 || (uint64_t)vp->val != 0x9988776655443322ull) {
		fprintf(stderr, "[selftest] FAIL: re-read write_u64 mismatch\n");
		return st_fail(c);
	}
	printf("[selftest] write_u64 + verify: PASS\n");

	struct int3_val v32 = {.remote = addrs[0], .val = 0xDEADBEEFull};
	if (ioctl(fd, INT3_IOC_WRITE_U32, &v32) < 0) {
		fprintf(stderr, "[selftest] FAIL: write_u32 failed\n");
		return st_fail(c);
	}
	v32.val = 0;
	if (ioctl(fd, INT3_IOC_READ_U32, &v32) < 0 || (uint64_t)v32.val != 0xDEADBEEFull) {
		fprintf(stderr, "[selftest] FAIL: read_u32 got 0x%" PRIx64 "\n", (uint64_t)v32.val);
		return st_fail(c);
	}
	printf("[selftest] u32: PASS\n");

	vp->val = 0x9988776655443322ull;
	if (ioctl(fd, INT3_IOC_WRITE_U64, vp) < 0) {
		fprintf(stderr, "[selftest] FAIL: u32 restore failed\n");
		return st_fail(c);
	}

	uint64_t rv = 0;
	struct int3_rw rr = {
		.remote = addrs[0],
		.local = (uint64_t)(uintptr_t)&rv,
		.len = sizeof(rv),
	};
	if (ioctl(fd, INT3_IOC_READ, &rr) < 0 || rv != 0x9988776655443322ull) {
		fprintf(stderr, "[selftest] FAIL: single read got 0x%" PRIx64 "\n", rv);
		return st_fail(c);
	}
	uint64_t wv = 0x0102030405060708ull;
	struct int3_rw ww = {
		.remote = addrs[0],
		.local = (uint64_t)(uintptr_t)&wv,
		.len = sizeof(wv),
	};
	if (ioctl(fd, INT3_IOC_WRITE, &ww) < 0) {
		fprintf(stderr, "[selftest] FAIL: single write: %s\n", strerror(errno));
		return st_fail(c);
	}
	rv = 0;
	if (ioctl(fd, INT3_IOC_READ, &rr) < 0 || rv != wv) {
		fprintf(stderr, "[selftest] FAIL: single readback mismatch\n");
		return st_fail(c);
	}
	wv = 0x9988776655443322ull;
	if (ioctl(fd, INT3_IOC_WRITE, &ww) < 0) {
		fprintf(stderr, "[selftest] FAIL: single write restore\n");
		return st_fail(c);
	}
	printf("[selftest] rw single: PASS\n");
	return 0;
}
