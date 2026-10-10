#include "mm.h"

uint64_t mm_gnu_hash(const char *s)
{
	uint64_t h = 5381;
	while (*s)
		h = h * 33 + (uint8_t)*s++;
	return h;
}
