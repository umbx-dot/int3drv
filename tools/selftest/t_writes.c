#include "ctx.h"

int st_writes(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;

	uint8_t ro8[8] = {0};
	struct int3_rw roww = {
		.remote = addrs[3],
		.local = (uint64_t)(uintptr_t)ro8,
		.len = sizeof(ro8),
	};
	memcpy(ro8, "PLAINWR1", 8);
	if (ioctl(fd, INT3_IOC_WRITE, &roww) == 0) {
		fprintf(stderr, "[selftest] FAIL: plain write to RO page succeeded\n");
		return st_fail(c);
	}
	printf("[selftest] ro write reject: PASS\n");

	memcpy(ro8, "FORCE_A1", 8);
	struct int3_rw fow = {
		.remote = addrs[3],
		.local = (uint64_t)(uintptr_t)ro8,
		.len = sizeof(ro8),
	};
	if (ioctl(fd, INT3_IOC_FORCE_WRITE, &fow) < 0) {
		fprintf(stderr, "[selftest] FAIL: force_write: %s\n", strerror(errno));
		return st_fail(c);
	}
	memset(ro8, 0, sizeof(ro8));
	if (ioctl(fd, INT3_IOC_READ, &roww) < 0 || memcmp(ro8, "FORCE_A1", 8) != 0) {
		fprintf(stderr, "[selftest] FAIL: force_write readback\n");
		return st_fail(c);
	}
	printf("[selftest] force_write: PASS\n");

	memcpy(ro8, "FORCE_B2", 8);
	struct int3_op fo[1] = {
		{.remote = addrs[3], .local = (uint64_t)(uintptr_t)ro8, .len = 8, .flags = INT3_OP_FORCE},
	};
	struct int3_batch fb = {
		.ops = (uint64_t)(uintptr_t)fo,
		.count = 1,
		.flags = INT3_BATCH_MIXED,
		.fail_idx = -1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_BATCH, &fb) < 0 || fb.fail_idx != -1) {
		fprintf(stderr, "[selftest] FAIL: batch force: %s\n", strerror(errno));
		return st_fail(c);
	}
	memset(ro8, 0, sizeof(ro8));
	if (ioctl(fd, INT3_IOC_READ, &roww) < 0 || memcmp(ro8, "FORCE_B2", 8) != 0) {
		fprintf(stderr, "[selftest] FAIL: batch force readback\n");
		return st_fail(c);
	}
	printf("[selftest] batch force: PASS\n");
	return 0;
}
