#include "int3ctl.h"

int cmd_read(int pid, uint64_t addr, uint64_t len)
{
	if (!len || len > INT3_MAX_SINGLE) {
		fprintf(stderr, "invalid len: %" PRIu64 "\n", len);
		return 1;
	}
	uint8_t *buf = malloc(len);
	if (!buf) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}
	int fd = int3_open_attached(pid);
	if (fd < 0) {
		free(buf);
		return 1;
	}
	struct int3_rw rw = {
		.remote = addr,
		.local = (uint64_t)(uintptr_t)buf,
		.len = len,
	};
	if (ioctl(fd, INT3_IOC_READ, &rw) < 0) {
		fprintf(stderr, "read failed: %s\n", strerror(errno));
		free(buf);
		close(fd);
		return 1;
	}
	print_hexdump(addr, buf, len);
	free(buf);
	close(fd);
	return 0;
}
