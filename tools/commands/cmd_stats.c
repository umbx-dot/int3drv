#include "int3ctl.h"

int cmd_stats(void)
{
	int fd = int3_open_device();
	if (fd < 0)
		return 1;
	struct int3_stats s = {0};
	if (ioctl(fd, INT3_IOC_STATS, &s) < 0) {
		fprintf(stderr, "ioctl(INT3_IOC_STATS) failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("bytes_read:     %" PRIu64 "\n", (uint64_t)s.bytes_read);
	printf("bytes_written:  %" PRIu64 "\n", (uint64_t)s.bytes_written);
	printf("ops_completed:  %" PRIu64 "\n", (uint64_t)s.ops_completed);
	printf("ops_failed:     %" PRIu64 "\n", (uint64_t)s.ops_failed);
	close(fd);
	return 0;
}
