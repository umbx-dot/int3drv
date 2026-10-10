#include "ctx.h"

int st_deref(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;

	uint64_t dval = 0;
	char dtext[13] = {0};
	struct int3_deref df = {
		.ptr_addr = addrs[2],
		.disp = 0,
		.local = (uint64_t)(uintptr_t)&dval,
		.out_ptr = 0,
		.len = sizeof(dval),
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_DEREF, &df) < 0 || (uint64_t)df.out_ptr != addrs[1] ||
	    memcmp(&dval, "int3_fas", 8) != 0) {
		fprintf(stderr, "[selftest] FAIL: deref got ptr=0x%" PRIx64 " val=0x%" PRIx64 "\n",
			(uint64_t)df.out_ptr, dval);
		return st_fail(c);
	}
	printf("[selftest] deref: PASS (0x%" PRIx64 " => 0x%" PRIx64 ")\n", addrs[2], dval);
	df.disp = 24;
	df.local = (uint64_t)(uintptr_t)dtext;
	df.out_ptr = 0;
	df.len = sizeof(dtext) - 1;
	if (ioctl(fd, INT3_IOC_DEREF, &df) < 0 || memcmp(dtext, "test_payload", 12) != 0) {
		fprintf(stderr, "[selftest] FAIL: deref chain read\n");
		return st_fail(c);
	}
	printf("[selftest] deref chain: PASS (\"%.12s\")\n", dtext);
	return 0;
}
