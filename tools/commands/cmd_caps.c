#include "int3ctl.h"

int cmd_caps(void)
{
	int fd = int3_open_device();
	if (fd < 0)
		return 1;
	uint64_t caps = 0;
	if (ioctl(fd, INT3_IOC_CAPS, &caps) < 0) {
		fprintf(stderr, "ioctl(INT3_IOC_CAPS) failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("caps: 0x%016" PRIx64 " (modbase=%d chain=%d vscan=%d checksum=%d copy=%d alloc=%d thread=%d)\n",
	       caps,
	       !!(caps & INT3_CAP_MODBASE), !!(caps & INT3_CAP_CHAIN),
	       !!(caps & INT3_CAP_VSCAN), !!(caps & INT3_CAP_CHECKSUM),
	       !!(caps & INT3_CAP_COPY), !!(caps & INT3_CAP_ALLOC),
	       !!(caps & INT3_CAP_THREAD));
	close(fd);
	return 0;
}
