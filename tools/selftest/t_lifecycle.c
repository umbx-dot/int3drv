#include "ctx.h"

int st_lifecycle(struct st_ctx *c)
{
	int fd = c->fd;
	struct int3_val *vp = &c->val;

	if (ioctl(fd, INT3_IOC_DETACH) < 0) {
		fprintf(stderr, "[selftest] FAIL: detach\n");
		return st_fail(c);
	}
	printf("[selftest] detach: PASS\n");

	vp->val = 0;
	if (ioctl(fd, INT3_IOC_READ_U64, vp) == 0) {
		fprintf(stderr, "[selftest] FAIL: op succeeded while detached\n");
		return st_fail(c);
	}
	int32_t bad_pid = 0;
	if (ioctl(fd, INT3_IOC_ATTACH, &bad_pid) == 0) {
		fprintf(stderr, "[selftest] FAIL: attach accepted pid 0\n");
		return st_fail(c);
	}
	bad_pid = -7;
	if (ioctl(fd, INT3_IOC_ATTACH, &bad_pid) == 0) {
		fprintf(stderr, "[selftest] FAIL: attach accepted negative pid\n");
		return st_fail(c);
	}
	bad_pid = 2147483647;
	if (ioctl(fd, INT3_IOC_ATTACH, &bad_pid) == 0 || errno != ESRCH) {
		fprintf(stderr, "[selftest] FAIL: attach dead pid errno=%d\n", errno);
		return st_fail(c);
	}
	printf("[selftest] attach lifecycle: PASS\n");
	return 0;
}
