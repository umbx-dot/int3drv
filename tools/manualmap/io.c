#include "mm.h"

int mm_read_at(int ffd, void *buf, size_t len, off_t off)
{
	ssize_t r = pread(ffd, buf, len, off);
	return r == (ssize_t)len ? 0 : -1;
}
