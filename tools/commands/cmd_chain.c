#include "int3ctl.h"

int cmd_chain(int pid, uint64_t ptr_addr, uint64_t len, uint32_t depth, const uint64_t *offs)
{
	if (!len || len > INT3_MAX_SINGLE) {
		fprintf(stderr, "invalid len: %" PRIu64 "\n", len);
		return 1;
	}
	if (depth > INT3_MAX_CHAIN) {
		fprintf(stderr, "invalid depth: %u\n", depth);
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
	struct int3_chain c = {
		.ptr_addr = ptr_addr,
		.local = (uint64_t)(uintptr_t)buf,
		.out_ptr = 0,
		.offsets = (uint64_t)(uintptr_t)offs,
		.len = (uint32_t)len,
		.depth = depth,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_CHAIN, &c) < 0) {
		fprintf(stderr, "chain failed: %s\n", strerror(errno));
		free(buf);
		close(fd);
		return 1;
	}
	printf("chain 0x%" PRIx64 " depth %u => 0x%" PRIx64 "\n",
	       ptr_addr, depth, (uint64_t)c.out_ptr);
	print_hexdump((uint64_t)c.out_ptr, buf, len);
	free(buf);
	close(fd);
	return 0;
}
