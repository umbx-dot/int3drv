#include "int3ctl.h"

int cmd_probe(int pid, uint64_t addr, uint64_t len)
{
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_probe pr = {
		.remote = addr,
		.len = len,
		.is_readable = 0,
		.is_writable = 0,
	};
	if (ioctl(fd, INT3_IOC_PROBE, &pr) < 0) {
		fprintf(stderr, "probe failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("addr 0x%" PRIx64 " len %" PRIu64 ": readable=%u writable=%u\n",
	       addr, len, pr.is_readable, pr.is_writable);
	close(fd);
	return 0;
}
