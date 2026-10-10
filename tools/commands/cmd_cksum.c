#include "int3ctl.h"

int cmd_cksum(int pid, uint64_t start, uint64_t end)
{
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_cksum c = {
		.remote_start = start,
		.remote_end = end,
		.hash = 0,
	};
	if (ioctl(fd, INT3_IOC_CHECKSUM, &c) < 0) {
		fprintf(stderr, "checksum failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("cksum [0x%" PRIx64 "-0x%" PRIx64 "]: 0x%016" PRIx64 "\n",
	       start, end, (uint64_t)c.hash);
	close(fd);
	return 0;
}
