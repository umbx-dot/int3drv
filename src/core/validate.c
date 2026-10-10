#include "int3_internal.h"

int int3_addr_ok(u64 addr, u64 len)
{
	if (!len || addr < INT3_ADDR_MIN || addr == INT3_ADDR_BAD)
		return 0;
	if (addr >= INT3_ADDR_MAX)
		return 0;
	if (len > INT3_ADDR_MAX)
		return 0;
	if (addr + len < addr)
		return 0;
	if (addr + len > INT3_ADDR_MAX)
		return 0;
	return 1;
}
