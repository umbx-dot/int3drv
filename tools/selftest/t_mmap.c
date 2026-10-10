#include "ctx.h"

int st_mmap(struct st_ctx *c)
{
	int fd = c->fd;
	char self[1024];
	char lib[1152];
	ssize_t sl;
	char *slash;

	if (c->ver < 6) {
		printf("[selftest] alloc/manualmap skipped (driver v%u < 6)\n", c->ver);
		return 0;
	}

	struct int3_alloc a = {
		.hint = 0,
		.addr = 0,
		.len = 4096,
		.prot = PROT_READ | PROT_WRITE,
		._rsv = 0,
	};
	if (ioctl(fd, INT3_IOC_ALLOC, &a) < 0 || !a.addr) {
		fprintf(stderr, "[selftest] FAIL: alloc: %s\n", strerror(errno));
		return st_fail(c);
	}
	uint64_t pat = 0x0123456789ABCDEFull;
	struct int3_rw ww = {
		.remote = a.addr,
		.local = (uint64_t)(uintptr_t)&pat,
		.len = sizeof(pat),
	};
	if (ioctl(fd, INT3_IOC_WRITE, &ww) < 0) {
		fprintf(stderr, "[selftest] FAIL: alloc write: %s\n", strerror(errno));
		return st_fail(c);
	}
	uint64_t got = 0;
	struct int3_rw rr = {
		.remote = a.addr,
		.local = (uint64_t)(uintptr_t)&got,
		.len = sizeof(got),
	};
	if (ioctl(fd, INT3_IOC_READ, &rr) < 0 || got != pat) {
		fprintf(stderr, "[selftest] FAIL: alloc readback\n");
		return st_fail(c);
	}
	struct int3_free f = {.addr = a.addr, .len = 4096};
	if (ioctl(fd, INT3_IOC_FREE, &f) < 0) {
		fprintf(stderr, "[selftest] FAIL: free: %s\n", strerror(errno));
		return st_fail(c);
	}
	if (ioctl(fd, INT3_IOC_READ, &rr) == 0) {
		fprintf(stderr, "[selftest] FAIL: read after free succeeded\n");
		return st_fail(c);
	}
	printf("[selftest] alloc/free: PASS (0x%" PRIx64 ")\n", (uint64_t)a.addr);
	a.len = 0;
	if (ioctl(fd, INT3_IOC_ALLOC, &a) == 0) {
		fprintf(stderr, "[selftest] FAIL: alloc accepted len 0\n");
		return st_fail(c);
	}
	a.len = 4096;
	a.prot = 0;
	if (ioctl(fd, INT3_IOC_ALLOC, &a) == 0) {
		fprintf(stderr, "[selftest] FAIL: alloc accepted prot 0\n");
		return st_fail(c);
	}
	printf("[selftest] alloc validation: PASS\n");

	sl = readlink("/proc/self/exe", self, sizeof(self) - 1);
	if (sl <= 0) {
		fprintf(stderr, "[selftest] FAIL: readlink exe\n");
		return st_fail(c);
	}
	self[sl] = '\0';
	slash = strrchr(self, '/');
	if (!slash || (size_t)(slash - self) + 14 >= sizeof(lib)) {
		fprintf(stderr, "[selftest] FAIL: exe path too long\n");
		return st_fail(c);
	}
	memcpy(lib, self, (size_t)(slash - self + 1));
	strcpy(lib + (slash - self + 1), "libtestmap.so");

	uint64_t base = 0;
	uint64_t init = 0;
	uint64_t span = 0;
	if (mm_load_run(fd, (int)c->pid, lib, 5000, &base, &init, &span) < 0 || !base) {
		fprintf(stderr, "[selftest] FAIL: load %s\n", lib);
		return st_fail(c);
	}
	uint64_t flag_off = mm_sym_off(lib, "map_flag");
	if (!flag_off || !init || init < base || init >= base + span) {
		fprintf(stderr, "[selftest] FAIL: fixture syms flag=0x%" PRIx64 " init=0x%" PRIx64 "\n",
			flag_off, init);
		return st_fail(c);
	}
	uint64_t flag = 0;
	struct int3_rw fr = {
		.remote = base + flag_off,
		.local = (uint64_t)(uintptr_t)&flag,
		.len = sizeof(flag),
	};
	if (ioctl(fd, INT3_IOC_READ, &fr) < 0 || flag != 0xC0FFEEul) {
		fprintf(stderr, "[selftest] FAIL: init ran flag=0x%" PRIx64 "\n", flag);
		return st_fail(c);
	}
	printf("[selftest] manualmap: PASS (base 0x%" PRIx64 " init 0x%" PRIx64 ")\n", base, init);
	if (mm_mprotect(fd, (int)c->pid, base, 4096, PROT_READ | PROT_WRITE)) {
		fprintf(stderr, "[selftest] FAIL: stage mprotect RW\n");
		return st_fail(c);
	}
	uint64_t probe = 0x1122334455667788ull;
	struct int3_rw pw = {
		.remote = base,
		.local = (uint64_t)(uintptr_t)&probe,
		.len = sizeof(probe),
	};
	if (ioctl(fd, INT3_IOC_WRITE, &pw) < 0) {
		fprintf(stderr, "[selftest] FAIL: write to RW mapping: %s\n", strerror(errno));
		return st_fail(c);
	}
	if (mm_mprotect(fd, (int)c->pid, base, 4096, PROT_READ)) {
		fprintf(stderr, "[selftest] FAIL: stage mprotect RO\n");
		return st_fail(c);
	}
	struct int3_vma mv = {.addr = base, .vm_start = 0, .vm_end = 0, .vm_flags = 0};
	if (ioctl(fd, INT3_IOC_VMA, &mv) < 0 || !(mv.vm_flags & INT3_VM_READ) ||
	    (mv.vm_flags & (INT3_VM_WRITE | INT3_VM_EXEC))) {
		fprintf(stderr, "[selftest] FAIL: RO perms not applied\n");
		return st_fail(c);
	}
	if (ioctl(fd, INT3_IOC_WRITE, &pw) == 0) {
		fprintf(stderr, "[selftest] FAIL: write to RO mapping succeeded\n");
		return st_fail(c);
	}
	printf("[selftest] stage mprotect: PASS (RW→RO enforced)\n");
	struct int3_free fl = {.addr = base, .len = span};
	if (ioctl(fd, INT3_IOC_FREE, &fl) < 0) {
		fprintf(stderr, "[selftest] FAIL: mapped free: %s\n", strerror(errno));
		return st_fail(c);
	}
	uint64_t handle = 0;
	if (mm_load_libc(fd, (int)c->pid, lib, 5000, &handle) < 0 || !handle) {
		fprintf(stderr, "[selftest] FAIL: load_libc %s\n", lib);
		return st_fail(c);
	}
	uint64_t dbase = 0;
	uint64_t cursor = 0;
	char mpath[512];
	for (;;) {
		struct int3_map m = {
			.cursor = cursor,
			.vm_start = 0,
			.vm_end = 0,
			.vm_flags = 0,
			.path_ptr = (uint64_t)(uintptr_t)mpath,
			.path_cap = sizeof(mpath),
			.path_len = 0,
		};
		if (ioctl(fd, INT3_IOC_MAPS_NEXT, &m) < 0)
			break;
		mpath[sizeof(mpath) - 1] = '\0';
		if (strstr(mpath, "libtestmap.so") && (!dbase || m.vm_start < dbase))
			dbase = m.vm_start;
		if (m.vm_end <= cursor)
			break;
		cursor = m.vm_end;
	}
	if (!dbase) {
		fprintf(stderr, "[selftest] FAIL: dlopened lib not in maps\n");
		return st_fail(c);
	}
	uint64_t dflag = 0;
	struct int3_rw dr = {
		.remote = dbase + flag_off,
		.local = (uint64_t)(uintptr_t)&dflag,
		.len = sizeof(dflag),
	};
	if (ioctl(fd, INT3_IOC_READ, &dr) < 0 || dflag != 0xC0FFEEul) {
		fprintf(stderr, "[selftest] FAIL: dlopen flag=0x%" PRIx64 "\n", dflag);
		return st_fail(c);
	}
	printf("[selftest] load_libc: PASS (handle 0x%" PRIx64 " flag set)\n", handle);
	return 0;
}
