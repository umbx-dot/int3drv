#include "int3ctl.h"

int int3_attach(int fd, int pid)
{
	int32_t p = pid;
	return ioctl(fd, INT3_IOC_ATTACH, &p);
}

int int3_open_attached(int pid)
{
	int fd = int3_open_device();
	if (fd < 0)
		return -1;
	if (int3_attach(fd, pid) < 0) {
		fprintf(stderr, "attach(%d) failed: %s\n", pid, strerror(errno));
		close(fd);
		return -1;
	}
	return fd;
}

static int int3_rw_req(int fd, uint64_t addr, const void *buf, uint64_t len, unsigned long req)
{
	struct int3_rw rw = {
		.remote = addr,
		.local = (uint64_t)(uintptr_t)buf,
		.len = len,
	};
	return ioctl(fd, req, &rw);
}

int int3_read(int fd, uint64_t addr, void *buf, uint64_t len)
{
	return int3_rw_req(fd, addr, buf, len, INT3_IOC_READ);
}

int int3_write(int fd, uint64_t addr, const void *buf, uint64_t len)
{
	return int3_rw_req(fd, addr, buf, len, INT3_IOC_WRITE);
}

int int3_read_u64(int fd, uint64_t addr, uint64_t *val)
{
	struct int3_val v = {.remote = addr, .val = 0};
	if (ioctl(fd, INT3_IOC_READ_U64, &v) < 0)
		return -1;
	*val = v.val;
	return 0;
}

int int3_modbase(int fd, const char *name, uint64_t *base)
{
	size_t nlen = strlen(name);
	struct int3_mod m = {
		.name_ptr = (uint64_t)(uintptr_t)name,
		.base = 0,
		.name_len = (uint32_t)nlen,
		._rsv = 0,
	};
	if (!nlen || nlen > INT3_MAX_NAME)
		return -1;
	if (ioctl(fd, INT3_IOC_MOD_BASE, &m) < 0)
		return -1;
	*base = m.base;
	return 0;
}
