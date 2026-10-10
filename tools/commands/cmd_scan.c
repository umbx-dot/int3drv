#include "int3ctl.h"

int cmd_scan(int pid, uint64_t start, uint64_t end, const char *pat_hex, const char *mask_hex)
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
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_pattern scan = {
		.remote_start = start,
		.remote_end = end,
		.match_addr = 0,
		.pattern_ptr = (uint64_t)(uintptr_t)pat,
		.mask_ptr = (uint64_t)(uintptr_t)mask,
		.pattern_len = (uint32_t)pat_len,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_PATTERN_SCAN, &scan) < 0) {
		fprintf(stderr, "pattern scan failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("pattern found at: 0x%" PRIx64 "\n", (uint64_t)scan.match_addr);
	close(fd);
	return 0;
}
