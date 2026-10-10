#include "int3ctl.h"

int int3_open_device(void)
{
	int fd = open(INT3DRV_DEV, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "error: open(%s): %s\n", INT3DRV_DEV, strerror(errno));
		return -1;
	}
	return fd;
}

int int3_write_req(int pid, uint64_t addr, const uint8_t *buf, size_t len, unsigned long req,
		   const char *err, const char *ok_fmt)
{
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_rw rw = {
		.remote = addr,
		.local = (uint64_t)(uintptr_t)buf,
		.len = len,
	};
	if (ioctl(fd, req, &rw) < 0) {
		fprintf(stderr, "%s failed: %s\n", err, strerror(errno));
		close(fd);
		return 1;
	}
	printf(ok_fmt, len, addr);
	close(fd);
	return 0;
}

int int3_scan_print(int fd, unsigned long req, void *s, const uint64_t *matches,
		    uint32_t *count, const char *err)
{
	int rc = ioctl(fd, req, s);
	if (rc < 0 && errno != ENOSPC) {
		fprintf(stderr, "%s failed: %s\n", err, strerror(errno));
		return 1;
	}
	for (uint32_t i = 0; i < *count; i++)
		printf("match[%u]: 0x%" PRIx64 "\n", i, (uint64_t)matches[i]);
	printf("%u match(es)%s\n", *count, rc < 0 ? " (truncated)" : "");
	return 0;
}

bool parse_prot(const char *s, uint32_t *prot)
{
	uint32_t p = 0;

	for (; *s; s++) {
		if (*s == 'R' || *s == 'r')
			p |= PROT_READ;
		else if (*s == 'W' || *s == 'w')
			p |= PROT_WRITE;
		else if (*s == 'X' || *s == 'x')
			p |= PROT_EXEC;
		else
			return false;
	}
	*prot = p;
	return true;
}
