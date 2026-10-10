#include "ctx.h"

int st_scan(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;

	struct int3_pattern pat = {
		.remote_start = addrs[1],
		.remote_end = addrs[1] + 64,
		.match_addr = 0,
		.pattern_ptr = (uint64_t)(uintptr_t)ST_NEEDLE,
		.mask_ptr = 0,
		.pattern_len = sizeof(ST_NEEDLE) - 1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_PATTERN_SCAN, &pat) < 0 || pat.match_addr < addrs[1]) {
		fprintf(stderr, "[selftest] FAIL: pattern scan failed\n");
		return st_fail(c);
	}
	c->match_addr = pat.match_addr;
	printf("[selftest] pattern_scan: PASS (matched at 0x%" PRIx64 ")\n", (uint64_t)pat.match_addr);

	if (c->needle_vs != 0) {
		uint64_t cs = c->needle_vs >= 64 ? c->needle_vs - 64 : c->needle_vs;
		if (addrs[1] + 64 - cs < (256ull * 1024ull * 1024ull)) {
			struct int3_pattern hx = {
				.remote_start = cs,
				.remote_end = addrs[1] + 64,
				.match_addr = 0,
				.pattern_ptr = (uint64_t)(uintptr_t)ST_NEEDLE,
				.mask_ptr = 0,
				.pattern_len = sizeof(ST_NEEDLE) - 1,
				._rsv = 0,
			};
			if (ioctl(fd, INT3_IOC_PATTERN_SCAN, &hx) < 0 ||
			    (uint64_t)hx.match_addr != (uint64_t)pat.match_addr) {
				fprintf(stderr, "[selftest] FAIL: hole-crossing scan\n");
				return st_fail(c);
			}
			printf("[selftest] scan hole-crossing: PASS\n");
		} else {
			printf("[selftest] scan hole-crossing: SKIP (span too large)\n");
		}
	} else {
		printf("[selftest] scan hole-crossing: SKIP (needle homeless)\n");
	}
	return 0;
}
