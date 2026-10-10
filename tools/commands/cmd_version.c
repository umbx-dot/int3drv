#include "int3ctl.h"

int cmd_version(void)
{
	int fd = int3_open_device();
	if (fd < 0)
		return 1;
	uint32_t ver = 0;
	if (ioctl(fd, INT3_IOC_VERSION, &ver) < 0) {
		fprintf(stderr, "ioctl(INT3_IOC_VERSION) failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("int3drv driver version: %u\n", ver);
	close(fd);
	return 0;
}
