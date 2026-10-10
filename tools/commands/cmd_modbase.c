#include "int3ctl.h"

int cmd_modbase(int pid, const char *name)
{
	size_t nlen = strlen(name);
	if (!nlen || nlen > INT3_MAX_NAME) {
		fprintf(stderr, "invalid module name\n");
		return 1;
	}
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_mod m = {
		.name_ptr = (uint64_t)(uintptr_t)name,
		.base = 0,
		.name_len = (uint32_t)nlen,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_MOD_BASE, &m) < 0) {
		fprintf(stderr, "modbase lookup failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("%s base: 0x%" PRIx64 "\n", name, (uint64_t)m.base);
	close(fd);
	return 0;
}
