#include "int3ctl.h"

int cmd_maps(int pid)
{
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	char path[512];
	uint64_t cursor = 0;
	uint32_t n = 0;
	for (;;) {
		struct int3_map v = {
			.cursor = cursor,
			.vm_start = 0,
			.vm_end = 0,
			.vm_flags = 0,
			.path_ptr = (uint64_t)(uintptr_t)path,
			.path_cap = sizeof(path),
			.path_len = 0,
		};
		if (ioctl(fd, INT3_IOC_MAPS_NEXT, &v) < 0) {
			if (errno == ENOENT)
				break;
			fprintf(stderr, "maps_next failed: %s\n", strerror(errno));
			close(fd);
			return 1;
		}
		path[sizeof(path) - 1] = '\0';
		printf("%016" PRIx64 "-%016" PRIx64 " %c%c%c%c %s\n",
		       (uint64_t)v.vm_start, (uint64_t)v.vm_end,
		       (v.vm_flags & INT3_VM_READ) ? 'r' : '-',
		       (v.vm_flags & INT3_VM_WRITE) ? 'w' : '-',
		       (v.vm_flags & INT3_VM_EXEC) ? 'x' : '-',
		       (v.vm_flags & INT3_VM_SHARED) ? 's' : 'p',
		       path);
		if (v.vm_end <= cursor) {
			fprintf(stderr, "maps cursor did not advance\n");
			close(fd);
			return 1;
		}
		cursor = (uint64_t)v.vm_end;
		if (++n > (1u << 20)) {
			fprintf(stderr, "too many regions\n");
			close(fd);
			return 1;
		}
	}
	close(fd);
	return 0;
}
