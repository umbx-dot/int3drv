#include "ctx.h"

int st_validate(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;

	uint64_t rv = 0;
	struct int3_rw bad = {.remote = 0, .local = (uint64_t)(uintptr_t)&rv, .len = 0};
	if (ioctl(fd, INT3_IOC_READ, &bad) == 0) {
		fprintf(stderr, "[selftest] FAIL: rw accepted len 0\n");
		return st_fail(c);
	}
	bad.len = 8;
	if (ioctl(fd, INT3_IOC_READ, &bad) == 0) {
		fprintf(stderr, "[selftest] FAIL: rw accepted addr 0\n");
		return st_fail(c);
	}
	bad.remote = 0xCCCCCCCCCCCCCCCCull;
	if (ioctl(fd, INT3_IOC_READ, &bad) == 0) {
		fprintf(stderr, "[selftest] FAIL: rw accepted poison addr\n");
		return st_fail(c);
	}
	bad.remote = addrs[0];
	bad.len = (uint64_t)INT3_MAX_SINGLE + 1;
	if (ioctl(fd, INT3_IOC_READ, &bad) == 0) {
		fprintf(stderr, "[selftest] FAIL: rw accepted oversized len\n");
		return st_fail(c);
	}
	struct int3_val bv = {.remote = 0, .val = 0};
	if (ioctl(fd, INT3_IOC_READ_U64, &bv) == 0) {
		fprintf(stderr, "[selftest] FAIL: scalar accepted addr 0\n");
		return st_fail(c);
	}
	struct int3_deref bd = {
		.ptr_addr = addrs[2],
		.disp = 0,
		.local = (uint64_t)(uintptr_t)&rv,
		.out_ptr = 0,
		.len = 8,
		._rsv = 1,
	};
	if (ioctl(fd, INT3_IOC_DEREF, &bd) == 0) {
		fprintf(stderr, "[selftest] FAIL: deref accepted _rsv\n");
		return st_fail(c);
	}
	bd._rsv = 0;
	bd.local = 0;
	if (ioctl(fd, INT3_IOC_DEREF, &bd) == 0) {
		fprintf(stderr, "[selftest] FAIL: deref accepted null local\n");
		return st_fail(c);
	}
	bd.local = (uint64_t)(uintptr_t)&rv;
	bd.disp = (uint64_t)0 - addrs[1];
	if (ioctl(fd, INT3_IOC_DEREF, &bd) == 0) {
		fprintf(stderr, "[selftest] FAIL: deref accepted wrapped target\n");
		return st_fail(c);
	}
	struct int3_probe bp = {.remote = addrs[0], .len = 0, .is_readable = 0, .is_writable = 0};
	if (ioctl(fd, INT3_IOC_PROBE, &bp) == 0) {
		fprintf(stderr, "[selftest] FAIL: probe accepted len 0\n");
		return st_fail(c);
	}
	struct int3_vma bvm = {.addr = 0, .vm_start = 0, .vm_end = 0, .vm_flags = 0};
	if (ioctl(fd, INT3_IOC_VMA, &bvm) == 0) {
		fprintf(stderr, "[selftest] FAIL: vma accepted addr 0\n");
		return st_fail(c);
	}
	struct int3_pattern bpat = {
		.remote_start = addrs[1],
		.remote_end = addrs[1],
		.match_addr = 0,
		.pattern_ptr = (uint64_t)(uintptr_t)"x",
		.mask_ptr = 0,
		.pattern_len = 0,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_PATTERN_SCAN, &bpat) == 0) {
		fprintf(stderr, "[selftest] FAIL: scan accepted empty range/len\n");
		return st_fail(c);
	}
	printf("[selftest] arg validation: PASS\n");
	return 0;
}
