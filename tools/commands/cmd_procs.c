#include "int3ctl.h"

int cmd_procs(void)
{
	int fd = int3_open_device();
	if (fd < 0)
		return 1;
	uint32_t cursor = 0;
	uint32_t n = 0;
	for (;;) {
		struct int3_proc p = {
			.cursor = cursor,
			.pid = 0,
			.comm = {0},
			._rsv = 0,
		};
		if (ioctl(fd, INT3_IOC_PROC_NEXT, &p) < 0) {
			if (errno == ENOENT)
				break;
			fprintf(stderr, "proc_next failed: %s\n", strerror(errno));
			close(fd);
			return 1;
		}
		p.comm[sizeof(p.comm) - 1] = '\0';
		printf("%7u %s\n", p.pid, p.comm);
		if (p.pid <= cursor) {
			fprintf(stderr, "proc cursor did not advance\n");
			close(fd);
			return 1;
		}
		cursor = p.pid;
		if (++n > (1u << 22)) {
			fprintf(stderr, "too many processes\n");
			close(fd);
			return 1;
		}
	}
	close(fd);
	return 0;
}
