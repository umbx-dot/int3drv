#include "int3ctl.h"

int cmd_manualmap(int pid, const char *path)
{
	int fd = int3_open_device();
	if (fd < 0)
		return 1;
	uint64_t base = 0;
	uint64_t init = 0;
	uint64_t span = 0;
	if (mm_load(fd, pid, path, &base, &init, &span)) {
		fprintf(stderr, "manualmap failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("%s mapped at 0x%" PRIx64 " init 0x%" PRIx64 " span %" PRIu64 "\n",
	       path, base, init, span);
	close(fd);
	return 0;
}

int cmd_load(int pid, const char *path, uint32_t timeout_ms)
{
	int fd = int3_open_device();
	if (fd < 0)
		return 1;
	uint64_t base = 0;
	uint64_t init = 0;
	uint64_t span = 0;
	if (mm_load_run(fd, pid, path, timeout_ms, &base, &init, &span)) {
		fprintf(stderr, "load failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("%s loaded at 0x%" PRIx64 " init 0x%" PRIx64 " ran\n", path, base, init);
	close(fd);
	return 0;
}

int cmd_thread(int pid, uint64_t fn, uint64_t arg, uint32_t timeout_ms){
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_thread t = {
		.fn = fn,
		.arg = arg,
		.timeout_ms = timeout_ms,
		.tid = -1,
		.retval = 0,
	};
	if (ioctl(fd, INT3_IOC_THREAD, &t) < 0) {
		fprintf(stderr, "thread failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("ran 0x%" PRIx64 " in tid %d\n", fn, t.tid);
	close(fd);
	return 0;
}

int cmd_alloc(int pid, uint64_t len, uint32_t prot)
{
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_alloc a = {
		.hint = 0,
		.addr = 0,
		.len = len,
		.prot = prot,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_ALLOC, &a) < 0 || !a.addr) {
		fprintf(stderr, "alloc failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("allocated %" PRIu64 " bytes at 0x%" PRIx64 "\n", len, (uint64_t)a.addr);
	close(fd);
	return 0;
}

int cmd_free(int pid, uint64_t addr, uint64_t len)
{
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;
	struct int3_free f = {
		.addr = addr,
		.len = len,
	};
	if (ioctl(fd, INT3_IOC_FREE, &f) < 0) {
		fprintf(stderr, "free failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("freed %" PRIu64 " bytes at 0x%" PRIx64 "\n", len, addr);
	close(fd);
	return 0;
}

int cmd_mprotect(int pid, uint64_t addr, uint64_t len, uint32_t prot)
{
	int fd = int3_open_device();
	if (fd < 0)
		return 1;
	if (mm_mprotect(fd, pid, addr, len, prot)) {
		fprintf(stderr, "mprotect failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("protected [0x%" PRIx64 "-0x%" PRIx64 "] prot 0x%x\n", addr, addr + len, prot);
	close(fd);
	return 0;
}

int cmd_load_libc(int pid, const char *path, uint32_t timeout_ms)
{
	int fd = int3_open_device();
	if (fd < 0)
		return 1;
	uint64_t handle = 0;
	if (mm_load_libc(fd, pid, path, timeout_ms, &handle)) {
		fprintf(stderr, "load_libc failed: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	printf("%s dlopened handle 0x%" PRIx64 "\n", path, handle);
	close(fd);
	return 0;
}
