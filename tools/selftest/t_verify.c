#include "ctx.h"

int st_verify(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;
	struct int3_val *vp = &c->val;

	uint64_t newval = 0xAABBCCDD11223344ull;
	struct int3_rw vw = {
		.remote = addrs[0],
		.local = (uint64_t)(uintptr_t)&newval,
		.len = sizeof(newval),
	};
	if (ioctl(fd, INT3_IOC_WRITE_VERIFY, &vw) < 0) {
		fprintf(stderr, "[selftest] FAIL: write_verify: %s\n", strerror(errno));
		return st_fail(c);
	}
	vp->val = 0;
	if (ioctl(fd, INT3_IOC_READ_U64, vp) < 0 || (uint64_t)vp->val != newval) {
		fprintf(stderr, "[selftest] FAIL: write_verify readback mismatch\n");
		return st_fail(c);
	}
	c->track_val = newval;
	printf("[selftest] write_verify: PASS\n");

	for (uint32_t it = 0; it < 1000; it++) {
		vp->val = 0;
		if (ioctl(fd, INT3_IOC_READ_U64, vp) < 0 || (uint64_t)vp->val != newval) {
			fprintf(stderr, "[selftest] FAIL: stress scalar iter %u\n", it);
			return st_fail(c);
		}
	}
	c->stress_val = 0;
	for (uint32_t it = 0; it < 10; it++) {
		c->stress_batch.count = 256;
		c->stress_batch.fail_idx = -1;
		if (ioctl(fd, INT3_IOC_BATCH, &c->stress_batch) < 0 || c->stress_batch.fail_idx != -1 ||
		    c->stress_val != newval) {
			fprintf(stderr, "[selftest] FAIL: stress batch iter %u\n", it);
			return st_fail(c);
		}
	}
	printf("[selftest] stress: PASS (1000 scalar + 2560 batch ops)\n");

	struct int3_stats st = {0};
	if (ioctl(fd, INT3_IOC_STATS, &st) < 0 || st.ops_completed == 0) {
		fprintf(stderr, "[selftest] FAIL: stats empty\n");
		return st_fail(c);
	}
	printf("[selftest] stats: PASS (ops_completed=%" PRIu64 ", bytes_read=%" PRIu64 ")\n",
	       (uint64_t)st.ops_completed, (uint64_t)st.bytes_read);
	return 0;
}
