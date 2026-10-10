#include "mm.h"

int mm_mprotect(int fd, int pid, uint64_t addr, uint64_t len, uint32_t prot)
{
	return mm_mprotectv(fd, pid, &addr, &len, &prot, 1);
}
