#include "int3ctl.h"

int cmd_vscan(int pid, uint64_t start, uint64_t end, uint32_t width, uint32_t op, uint64_t val)
{
	if (width != 1 && width != 2 && width != 4 && width != 8) {
		fprintf(stderr, "width must be 1, 2, 4 or 8\n");
		return 1;
	}
	if (op > INT3_VSCAN_LT) {
		fprintf(stderr, "op must be 0 (eq), 1 (ne), 2 (gt) or 3 (lt)\n");
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
	struct int3_vscan s = {
		.remote_start = start,
		.remote_end = end,
		.match_ptr = (uint64_t)(uintptr_t)matches,
		.value = val,
		.match_cap = INT3_MAX_MATCHES,
		.match_count = 0,
		.width = width,
		.op = op,
	};
	int rc = int3_scan_print(fd, INT3_IOC_VSCAN, &s, matches, &s.match_count, "value scan");
	free(matches);
	close(fd);
	return rc;
}
