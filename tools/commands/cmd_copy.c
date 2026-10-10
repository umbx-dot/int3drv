#include "int3ctl.h"

int cmd_copy(int pid, uint64_t src, uint64_t dst, uint64_t len)
{
	if (!len || len > INT3_MAX_SINGLE) {
		fprintf(stderr, "invalid len: %" PRIu64 "\n", len);
		return 1;
	}
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_copy c = {
		.src = src,
		.dst = dst,
		.len = len,
	};
	if (ioctl(fd, INT3_IOC_COPY, &c) < 0) {
		fprintf(stderr, "copy failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("copied %" PRIu64 " bytes 0x%" PRIx64 " => 0x%" PRIx64 "\n", len, src, dst);
	close(fd);
	return 0;
}
