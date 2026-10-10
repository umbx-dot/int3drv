#include "int3ctl.h"

int cmd_deref(int pid, uint64_t ptr_addr, uint64_t disp, uint64_t len)
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
	struct int3_deref d = {
		.ptr_addr = ptr_addr,
		.disp = disp,
		.local = (uint64_t)(uintptr_t)buf,
		.out_ptr = 0,
		.len = (uint32_t)len,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_DEREF, &d) < 0) {
		fprintf(stderr, "deref failed: %s\n", strerror(errno));
		free(buf);
		close(fd);
		return 1;
	}
	printf("*(0x%" PRIx64 ") + 0x%" PRIx64 " => 0x%" PRIx64 "\n",
	       ptr_addr, disp, (uint64_t)d.out_ptr);
	print_hexdump((uint64_t)d.out_ptr + disp, buf, len);
	free(buf);
	close(fd);
	return 0;
}
