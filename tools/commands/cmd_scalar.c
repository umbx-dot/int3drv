#include "int3ctl.h"

int cmd_read64(int pid, uint64_t addr)
{
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_val v = {.remote = addr, .val = 0};
	if (ioctl(fd, INT3_IOC_READ_U64, &v) < 0) {
		fprintf(stderr, "read_u64 failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("0x%" PRIx64 " => 0x%016" PRIx64 " (%" PRIu64 ")\n", addr, (uint64_t)v.val, (uint64_t)v.val);
	close(fd);
	return 0;
}

int cmd_write64(int pid, uint64_t addr, uint64_t val)
{
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_val v = {.remote = addr, .val = val};
	if (ioctl(fd, INT3_IOC_WRITE_U64, &v) < 0) {
		fprintf(stderr, "write_u64 failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("wrote 0x%016" PRIx64 " to 0x%" PRIx64 "\n", val, addr);
	close(fd);
	return 0;
}
