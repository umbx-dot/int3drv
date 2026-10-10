#include "ctx.h"

int st_thread(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;

	if (c->ver < 7) {
		printf("[selftest] thread skipped (driver v%u < 7)\n", c->ver);
		return 0;
	}

	struct int3_thread th = {
		.fn = addrs[5],
		.arg = addrs[4],
		.timeout_ms = 5000,
		.tid = -1,
		.retval = 0,
	};
	if (ioctl(fd, INT3_IOC_THREAD, &th) < 0) {
		fprintf(stderr, "[selftest] FAIL: thread: %s\n", strerror(errno));
		return st_fail(c);
	}
	if (th.tid <= 0) {
		fprintf(stderr, "[selftest] FAIL: thread bad tid %d\n", th.tid);
		return st_fail(c);
	}
	unsigned long flag = 0;
	struct int3_rw fr = {
		.remote = addrs[4],
		.local = (uint64_t)(uintptr_t)&flag,
		.len = sizeof(flag),
	};
	if (ioctl(fd, INT3_IOC_READ, &fr) < 0 || flag != 0x1BADB002ul) {
		fprintf(stderr, "[selftest] FAIL: thread ran flag=0x%lx\n", flag);
		return st_fail(c);
	}
	if (th.retval != 0xFEEDFACEul) {
		fprintf(stderr, "[selftest] FAIL: thread retval=0x%" PRIx64 "\n", (uint64_t)th.retval);
		return st_fail(c);
	}
	printf("[selftest] thread: PASS (tid %d ran 0x%" PRIx64 ")\n", th.tid, addrs[5]);

	th.fn = 0;
	if (ioctl(fd, INT3_IOC_THREAD, &th) == 0) {
		fprintf(stderr, "[selftest] FAIL: thread accepted null fn\n");
		return st_fail(c);
	}
	printf("[selftest] thread validation: PASS\n");
	return 0;
}
