#include "ctx.h"

int st_scanall(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;

	uint64_t *all = malloc(INT3_MAX_MATCHES * sizeof(*all));
	if (!all) {
		fprintf(stderr, "[selftest] FAIL: out of memory\n");
		return st_fail(c);
	}
	struct int3_scan_all sa = {
		.remote_start = addrs[1],
		.remote_end = addrs[1] + 64,
		.match_ptr = (uint64_t)(uintptr_t)all,
		.pattern_ptr = (uint64_t)(uintptr_t)ST_NEEDLE,
		.mask_ptr = 0,
		.match_cap = INT3_MAX_MATCHES,
		.match_count = 0,
		.pattern_len = sizeof(ST_NEEDLE) - 1,
		.align = 1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_SCAN_ALL, &sa) < 0 || sa.match_count < 1 ||
	    (uint64_t)all[0] != c->match_addr) {
		fprintf(stderr, "[selftest] FAIL: scan_all count=%u\n", sa.match_count);
		free(all);
		return st_fail(c);
	}
	printf("[selftest] scan_all: PASS (%u match(es))\n", sa.match_count);
	sa.align = 3;
	if (ioctl(fd, INT3_IOC_SCAN_ALL, &sa) == 0) {
		fprintf(stderr, "[selftest] FAIL: scan_all accepted bad align\n");
		free(all);
		return st_fail(c);
	}
	printf("[selftest] scan_all align reject: PASS\n");
	const char tone[] = "t";
	struct int3_scan_all stt = {
		.remote_start = addrs[1],
		.remote_end = addrs[1] + 64,
		.match_ptr = (uint64_t)(uintptr_t)all,
		.pattern_ptr = (uint64_t)(uintptr_t)tone,
		.mask_ptr = 0,
		.match_cap = 2,
		.match_count = 0,
		.pattern_len = sizeof(tone) - 1,
		.align = 1,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_SCAN_ALL, &stt) == 0 || errno != ENOSPC || stt.match_count != 2) {
		fprintf(stderr, "[selftest] FAIL: scan_all truncation count=%u errno=%d\n",
			stt.match_count, errno);
		free(all);
		return st_fail(c);
	}
	printf("[selftest] scan_all truncation: PASS\n");
	free(all);
	return 0;
}
