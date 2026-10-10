#include "int3ctl.h"

int cmd_vma(int pid, uint64_t addr)
{
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_vma v = {.addr = addr, .vm_start = 0, .vm_end = 0, .vm_flags = 0};
	if (ioctl(fd, INT3_IOC_VMA, &v) < 0) {
		fprintf(stderr, "vma lookup failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("0x%" PRIx64 " in [%016" PRIx64 "-%016" PRIx64 "] flags: %c%c%c%c\n",
	       addr, (uint64_t)v.vm_start, (uint64_t)v.vm_end,
	       (v.vm_flags & INT3_VM_READ) ? 'r' : '-',
	       (v.vm_flags & INT3_VM_WRITE) ? 'w' : '-',
	       (v.vm_flags & INT3_VM_EXEC) ? 'x' : '-',
	       (v.vm_flags & INT3_VM_SHARED) ? 's' : 'p');
	close(fd);
	return 0;
}
