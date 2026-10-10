#include "int3ctl.h"

int cmd_scan_all(int pid, uint64_t start, uint64_t end, const char *pat_hex, const char *mask_hex,
		 uint32_t align)
{
	uint8_t pat[INT3_MAX_PATTERN];
	uint8_t mask[INT3_MAX_PATTERN];
	size_t pat_len = 0;
	size_t mask_len = 0;
	if (!parse_hex_bytes(pat_hex, pat, &pat_len, sizeof(pat)) || pat_len == 0) {
		fprintf(stderr, "invalid pattern hex\n");
		return 1;
	}
	if (mask_hex) {
		if (!parse_hex_bytes(mask_hex, mask, &mask_len, sizeof(mask)) || mask_len != pat_len) {
			fprintf(stderr, "mask length must match pattern length\n");
			return 1;
		}
	} else {
		memset(mask, 0xFF, pat_len);
	}
	if (align != 1 && align != 2 && align != 4 && align != 8) {
		fprintf(stderr, "align must be 1, 2, 4 or 8\n");
		return 1;
	}
	uint64_t *matches = malloc(INT3_MAX_MATCHES * sizeof(*matches));
	if (!matches) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}
	int fd = int3_open_attached(pid);
	if (fd < 0) {
		free(matches);
		return 1;
	}
	struct int3_scan_all s = {
		.remote_start = start,
		.remote_end = end,
		.match_ptr = (uint64_t)(uintptr_t)matches,
		.pattern_ptr = (uint64_t)(uintptr_t)pat,
		.mask_ptr = (uint64_t)(uintptr_t)mask,
		.match_cap = INT3_MAX_MATCHES,
		.match_count = 0,
		.pattern_len = (uint32_t)pat_len,
		.align = align,
		._rsv = 0,
	};
	int rc = int3_scan_print(fd, INT3_IOC_SCAN_ALL, &s, matches, &s.match_count, "pattern scan_all");
	free(matches);
	close(fd);
	return rc;
}
