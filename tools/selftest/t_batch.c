#include "ctx.h"

int st_batch(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;
	struct int3_val *vp = &c->val;

	uint64_t b0 = 0;
	uint64_t b1 = 0;
	uint8_t b2[8] = {0};
	struct int3_op bo[3] = {
		{.remote = addrs[0], .local = (uint64_t)(uintptr_t)&b0, .len = 8, .flags = INT3_OP_READ},
		{.remote = addrs[2], .local = (uint64_t)(uintptr_t)&b1, .len = 8, .flags = INT3_OP_READ},
		{.remote = addrs[1], .local = (uint64_t)(uintptr_t)b2, .len = 8, .flags = INT3_OP_READ},
	};
	struct int3_batch bb = {
		.ops = (uint64_t)(uintptr_t)bo,
		.count = 3,
		.flags = INT3_BATCH_READ,
		.fail_idx = -1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_BATCH, &bb) < 0 || bb.fail_idx != -1 ||
	    b0 != 0x9988776655443322ull || b1 != addrs[1] || memcmp(b2, "int3_fas", 8) != 0) {
		fprintf(stderr, "[selftest] FAIL: batch read fail_idx=%d\n", bb.fail_idx);
		return st_fail(c);
	}
	printf("[selftest] batch read: PASS\n");

	uint64_t bw = 0x7777777777777777ull;
	struct int3_op wo[1] = {
		{.remote = addrs[0], .local = (uint64_t)(uintptr_t)&bw, .len = 8, .flags = 0},
	};
	struct int3_batch wb = {
		.ops = (uint64_t)(uintptr_t)wo,
		.count = 1,
		.flags = INT3_BATCH_WRITE,
		.fail_idx = -1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_BATCH, &wb) < 0 || wb.fail_idx != -1) {
		fprintf(stderr, "[selftest] FAIL: batch write: %s\n", strerror(errno));
		return st_fail(c);
	}
	vp->val = 0;
	if (ioctl(fd, INT3_IOC_READ_U64, vp) < 0 || (uint64_t)vp->val != bw) {
		fprintf(stderr, "[selftest] FAIL: batch write confirm\n");
		return st_fail(c);
	}
	printf("[selftest] batch write: PASS\n");

	struct int3_stats before = {0};
	if (ioctl(fd, INT3_IOC_STATS, &before) < 0) {
		fprintf(stderr, "[selftest] FAIL: stats snapshot\n");
		return st_fail(c);
	}
	uint64_t mv = 0x5555555555555555ull;
	uint64_t vv = 0x6666666666666666ull;
	b0 = 0;
	struct int3_op mo[3] = {
		{.remote = addrs[0], .local = (uint64_t)(uintptr_t)&b0, .len = 8, .flags = INT3_OP_READ},
		{.remote = addrs[0], .local = (uint64_t)(uintptr_t)&mv, .len = 8, .flags = INT3_OP_WRITE},
		{.remote = addrs[0], .local = (uint64_t)(uintptr_t)&vv, .len = 8, .flags = INT3_OP_WRITE_VERIFY},
	};
	struct int3_batch mb = {
		.ops = (uint64_t)(uintptr_t)mo,
		.count = 3,
		.flags = INT3_BATCH_MIXED,
		.fail_idx = -1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_BATCH, &mb) < 0 || mb.fail_idx != -1 || b0 != bw) {
		fprintf(stderr, "[selftest] FAIL: batch mixed fail_idx=%d\n", mb.fail_idx);
		return st_fail(c);
	}
	struct int3_stats after = {0};
	if (ioctl(fd, INT3_IOC_STATS, &after) < 0 ||
	    after.ops_completed != before.ops_completed + 3 ||
	    after.bytes_read != before.bytes_read + 8 ||
	    after.bytes_written != before.bytes_written + 16) {
		fprintf(stderr, "[selftest] FAIL: stats delta completed=%" PRIu64 " read=%" PRIu64 " written=%" PRIu64 "\n",
			(uint64_t)(after.ops_completed - before.ops_completed),
			(uint64_t)(after.bytes_read - before.bytes_read),
			(uint64_t)(after.bytes_written - before.bytes_written));
		return st_fail(c);
	}
	vp->val = 0;
	if (ioctl(fd, INT3_IOC_READ_U64, vp) < 0 || (uint64_t)vp->val != vv) {
		fprintf(stderr, "[selftest] FAIL: batch mixed confirm\n");
		return st_fail(c);
	}
	printf("[selftest] batch mixed + stats delta: PASS\n");

	struct int3_op vo[1] = {
		{.remote = addrs[0], .local = (uint64_t)(uintptr_t)&vv, .len = 8, .flags = INT3_OP_WRITE_VERIFY},
	};
	struct int3_batch vb = {
		.ops = (uint64_t)(uintptr_t)vo,
		.count = 1,
		.flags = INT3_BATCH_WRITE,
		.fail_idx = -1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_BATCH, &vb) < 0 || vb.fail_idx != -1) {
		fprintf(stderr, "[selftest] FAIL: batch write-mode verify: %s\n", strerror(errno));
		return st_fail(c);
	}
	printf("[selftest] batch write-mode verify: PASS\n");

	for (uint32_t k = 0; k < 256; k++)
		c->stress_ops[k] = (struct int3_op){
			.remote = addrs[0],
			.local = (uint64_t)(uintptr_t)&c->stress_val,
			.len = 8,
			.flags = 0,
		};
	c->stress_batch = (struct int3_batch){
		.ops = (uint64_t)(uintptr_t)c->stress_ops,
		.count = 256,
		.flags = INT3_BATCH_READ,
		.fail_idx = -1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_BATCH, &c->stress_batch) < 0 || c->stress_batch.fail_idx != -1 ||
	    c->stress_val != vv) {
		fprintf(stderr, "[selftest] FAIL: batch 256 ops\n");
		return st_fail(c);
	}
	c->stress_batch.count = 257;
	if (ioctl(fd, INT3_IOC_BATCH, &c->stress_batch) == 0) {
		fprintf(stderr, "[selftest] FAIL: batch accepted 257 ops\n");
		return st_fail(c);
	}
	bb.count = 0;
	if (ioctl(fd, INT3_IOC_BATCH, &bb) == 0) {
		fprintf(stderr, "[selftest] FAIL: batch accepted count 0\n");
		return st_fail(c);
	}
	bb.count = 3;
	bb._rsv = 1;
	if (ioctl(fd, INT3_IOC_BATCH, &bb) == 0) {
		fprintf(stderr, "[selftest] FAIL: batch accepted _rsv\n");
		return st_fail(c);
	}
	bb._rsv = 0;
	bb.flags = 9;
	if (ioctl(fd, INT3_IOC_BATCH, &bb) == 0) {
		fprintf(stderr, "[selftest] FAIL: batch accepted bad mode\n");
		return st_fail(c);
	}
	bb.flags = INT3_BATCH_READ;
	bo[0].flags = INT3_OP_WRITE;
	if (ioctl(fd, INT3_IOC_BATCH, &bb) == 0) {
		fprintf(stderr, "[selftest] FAIL: batch accepted write op in read mode\n");
		return st_fail(c);
	}
	bo[0].flags = INT3_OP_READ;
	bo[0].len = INT3_MAX_SINGLE + 1;
	if (ioctl(fd, INT3_IOC_BATCH, &bb) == 0) {
		fprintf(stderr, "[selftest] FAIL: batch accepted oversized op\n");
		return st_fail(c);
	}
	bo[0].len = 8;
	printf("[selftest] batch validation: PASS\n");
	return 0;
}
