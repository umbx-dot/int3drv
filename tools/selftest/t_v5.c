#include "ctx.h"

int st_v5(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;

	if (c->ver < 5) {
		printf("[selftest] v5 ioctls skipped (driver v%u < 5)\n", c->ver);
		return 0;
	}

	uint64_t caps = 0;
	if (ioctl(fd, INT3_IOC_CAPS, &caps) < 0 || (caps & ~INT3_CAPS_ALL)) {
		fprintf(stderr, "[selftest] FAIL: caps got 0x%" PRIx64 "\n", caps);
		return st_fail(c);
	}
	if (c->ver >= 6 && (caps & INT3_CAPS_ALL) != INT3_CAPS_ALL) {
		fprintf(stderr, "[selftest] FAIL: caps missing bits: 0x%" PRIx64 "\n", caps);
		return st_fail(c);
	}
	printf("[selftest] caps: PASS (0x%" PRIx64 ")\n", caps);

	const char libc[] = "libc";
	struct int3_mod mod = {
		.name_ptr = (uint64_t)(uintptr_t)libc,
		.base = 0,
		.name_len = sizeof(libc) - 1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_MOD_BASE, &mod) < 0 || !mod.base) {
		fprintf(stderr, "[selftest] FAIL: modbase libc: %s\n", strerror(errno));
		return st_fail(c);
	}
	struct int3_vma mv = {.addr = mod.base, .vm_start = 0, .vm_end = 0, .vm_flags = 0};
	if (ioctl(fd, INT3_IOC_VMA, &mv) < 0 || mv.vm_start != mod.base) {
		fprintf(stderr, "[selftest] FAIL: modbase not a region start\n");
		return st_fail(c);
	}
	printf("[selftest] modbase: PASS (libc at 0x%" PRIx64 ")\n", (uint64_t)mod.base);
	mod.name_len = 0;
	if (ioctl(fd, INT3_IOC_MOD_BASE, &mod) == 0) {
		fprintf(stderr, "[selftest] FAIL: modbase accepted empty name\n");
		return st_fail(c);
	}
	mod.name_len = INT3_MAX_NAME + 1;
	if (ioctl(fd, INT3_IOC_MOD_BASE, &mod) == 0) {
		fprintf(stderr, "[selftest] FAIL: modbase accepted oversized name\n");
		return st_fail(c);
	}
	printf("[selftest] modbase validation: PASS\n");

	uint64_t cval = 0;
	struct int3_chain cc = {
		.ptr_addr = addrs[2],
		.local = (uint64_t)(uintptr_t)&cval,
		.out_ptr = 0,
		.offsets = 0,
		.len = sizeof(cval),
		.depth = 0,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_CHAIN, &cc) < 0 || (uint64_t)cc.out_ptr != addrs[1] ||
	    memcmp(&cval, "int3_fas", 8) != 0) {
		fprintf(stderr, "[selftest] FAIL: chain depth0 ptr=0x%" PRIx64 "\n", (uint64_t)cc.out_ptr);
		return st_fail(c);
	}
	printf("[selftest] chain depth0: PASS (0x%" PRIx64 ")\n", (uint64_t)cc.out_ptr);
	cc.depth = INT3_MAX_CHAIN + 1;
	if (ioctl(fd, INT3_IOC_CHAIN, &cc) == 0) {
		fprintf(stderr, "[selftest] FAIL: chain accepted depth %u\n", cc.depth);
		return st_fail(c);
	}
	cc.depth = 0;
	cc._rsv = 1;
	if (ioctl(fd, INT3_IOC_CHAIN, &cc) == 0) {
		fprintf(stderr, "[selftest] FAIL: chain accepted _rsv\n");
		return st_fail(c);
	}
	printf("[selftest] chain validation: PASS\n");

	uint64_t *hits = malloc(INT3_MAX_MATCHES * sizeof(*hits));
	if (!hits) {
		fprintf(stderr, "[selftest] FAIL: out of memory\n");
		return st_fail(c);
	}
	struct int3_vscan vs = {
		.remote_start = addrs[0],
		.remote_end = addrs[0] + 8,
		.match_ptr = (uint64_t)(uintptr_t)hits,
		.value = c->track_val,
		.match_cap = INT3_MAX_MATCHES,
		.match_count = 0,
		.width = 8,
		.op = INT3_VSCAN_EQ,
	};
	if (ioctl(fd, INT3_IOC_VSCAN, &vs) < 0 || vs.match_count != 1 ||
	    (uint64_t)hits[0] != addrs[0]) {
		fprintf(stderr, "[selftest] FAIL: vscan u64 eq count=%u\n", vs.match_count);
		free(hits);
		return st_fail(c);
	}
	vs.op = INT3_VSCAN_NE;
	if (ioctl(fd, INT3_IOC_VSCAN, &vs) == 0) {
		fprintf(stderr, "[selftest] FAIL: vscan ne matched identical cell\n");
		free(hits);
		return st_fail(c);
	}
	vs.op = INT3_VSCAN_EQ;
	vs.width = 3;
	if (ioctl(fd, INT3_IOC_VSCAN, &vs) == 0) {
		fprintf(stderr, "[selftest] FAIL: vscan accepted width 3\n");
		free(hits);
		return st_fail(c);
	}
	vs.width = 8;
	vs.op = 9;
	if (ioctl(fd, INT3_IOC_VSCAN, &vs) == 0) {
		fprintf(stderr, "[selftest] FAIL: vscan accepted bad op\n");
		free(hits);
		return st_fail(c);
	}
	printf("[selftest] vscan: PASS (match at 0x%" PRIx64 ")\n", (uint64_t)hits[0]);
	free(hits);

	struct int3_cksum ck = {
		.remote_start = addrs[1],
		.remote_end = addrs[1] + 64,
		.hash = 0,
	};
	if (ioctl(fd, INT3_IOC_CHECKSUM, &ck) < 0 || !ck.hash) {
		fprintf(stderr, "[selftest] FAIL: cksum: %s\n", strerror(errno));
		return st_fail(c);
	}
	uint64_t h1 = ck.hash;
	ck.hash = 0;
	if (ioctl(fd, INT3_IOC_CHECKSUM, &ck) < 0 || (uint64_t)ck.hash != h1) {
		fprintf(stderr, "[selftest] FAIL: cksum not deterministic\n");
		return st_fail(c);
	}
	printf("[selftest] cksum: PASS (0x%016" PRIx64 ")\n", h1);
	ck.remote_start = addrs[1] + 64;
	ck.remote_end = addrs[1];
	if (ioctl(fd, INT3_IOC_CHECKSUM, &ck) == 0) {
		fprintf(stderr, "[selftest] FAIL: cksum accepted empty range\n");
		return st_fail(c);
	}
	printf("[selftest] cksum validation: PASS\n");

	uint64_t saved = 0;
	struct int3_val sv = {.remote = addrs[0], .val = 0};
	if (ioctl(fd, INT3_IOC_READ_U64, &sv) < 0) {
		fprintf(stderr, "[selftest] FAIL: copy setup read\n");
		return st_fail(c);
	}
	saved = sv.val;
	struct int3_copy cp = {
		.src = addrs[1],
		.dst = addrs[0],
		.len = 8,
	};
	if (ioctl(fd, INT3_IOC_COPY, &cp) < 0) {
		fprintf(stderr, "[selftest] FAIL: copy: %s\n", strerror(errno));
		return st_fail(c);
	}
	sv.val = 0;
	if (ioctl(fd, INT3_IOC_READ_U64, &sv) < 0 || memcmp(&sv.val, "int3_fas", 8) != 0) {
		fprintf(stderr, "[selftest] FAIL: copy readback\n");
		return st_fail(c);
	}
	sv.val = saved;
	struct int3_rw restore = {
		.remote = addrs[0],
		.local = (uint64_t)(uintptr_t)&sv.val,
		.len = sizeof(sv.val),
	};
	if (ioctl(fd, INT3_IOC_WRITE, &restore) < 0) {
		fprintf(stderr, "[selftest] FAIL: copy restore\n");
		return st_fail(c);
	}
	printf("[selftest] copy: PASS\n");
	cp.len = 0;
	if (ioctl(fd, INT3_IOC_COPY, &cp) == 0) {
		fprintf(stderr, "[selftest] FAIL: copy accepted len 0\n");
		return st_fail(c);
	}
	printf("[selftest] copy validation: PASS\n");
	return 0;
}
