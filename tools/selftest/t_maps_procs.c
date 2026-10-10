#include "ctx.h"

int st_maps_procs(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;

	if (c->ver >= 4) {
		char mpath[512];
		uint64_t mcur = 0;
		uint32_t mregions = 0;
		int saw_target = 0;
		int saw_file_text = 0;
		for (;;) {
			struct int3_map m = {
				.cursor = mcur,
				.vm_start = 0,
				.vm_end = 0,
				.vm_flags = 0,
				.path_ptr = (uint64_t)(uintptr_t)mpath,
				.path_cap = sizeof(mpath),
				.path_len = 0,
			};
			if (ioctl(fd, INT3_IOC_MAPS_NEXT, &m) < 0) {
				if (errno == ENOENT)
					break;
				fprintf(stderr, "[selftest] FAIL: maps_next: %s\n", strerror(errno));
				return st_fail(c);
			}
			mpath[sizeof(mpath) - 1] = '\0';
			if (m.vm_end <= mcur || m.vm_end <= m.vm_start || m.path_len == 0) {
				fprintf(stderr, "[selftest] FAIL: maps_next cursor/region\n");
				return st_fail(c);
			}
			if (addrs[0] >= m.vm_start && addrs[0] < m.vm_end)
				saw_target = 1;
			if ((m.vm_flags & INT3_VM_EXEC) && mpath[0] != '\0')
				saw_file_text = 1;
			mcur = (uint64_t)m.vm_end;
			if (++mregions > (1u << 20)) {
				fprintf(stderr, "[selftest] FAIL: maps_next too many regions\n");
				return st_fail(c);
			}
		}
		if (mregions == 0 || !saw_target || !saw_file_text) {
			fprintf(stderr, "[selftest] FAIL: maps_next walk regions=%u target=%d text=%d\n",
				mregions, saw_target, saw_file_text);
			return st_fail(c);
		}
		printf("[selftest] maps_next: PASS (%u regions)\n", mregions);
		struct int3_map badm = {
			.cursor = 0,
			.vm_start = 0,
			.vm_end = 0,
			.vm_flags = 0,
			.path_ptr = (uint64_t)(uintptr_t)mpath,
			.path_cap = 0,
			.path_len = 0,
		};
		if (ioctl(fd, INT3_IOC_MAPS_NEXT, &badm) == 0) {
			fprintf(stderr, "[selftest] FAIL: maps_next accepted cap 0\n");
			return st_fail(c);
		}
		printf("[selftest] maps_next validation: PASS\n");

		uint32_t pcur = 0;
		uint32_t nprocs = 0;
		int saw_child = 0;
		int saw_self = 0;
		pid_t self = getpid();
		for (;;) {
			struct int3_proc pr = {
				.cursor = pcur,
				.pid = 0,
				.comm = {0},
				._rsv = 0,
			};
			if (ioctl(fd, INT3_IOC_PROC_NEXT, &pr) < 0) {
				if (errno == ENOENT)
					break;
				fprintf(stderr, "[selftest] FAIL: proc_next: %s\n", strerror(errno));
				return st_fail(c);
			}
			pr.comm[sizeof(pr.comm) - 1] = '\0';
			if (pr.pid <= pcur) {
				fprintf(stderr, "[selftest] FAIL: proc_next cursor\n");
				return st_fail(c);
			}
			if ((pid_t)pr.pid == c->pid && strcmp((const char *)pr.comm, "int3ctl") == 0)
				saw_child = 1;
			if ((pid_t)pr.pid == self && strcmp((const char *)pr.comm, "int3ctl") == 0)
				saw_self = 1;
			pcur = pr.pid;
			if (++nprocs > (1u << 22)) {
				fprintf(stderr, "[selftest] FAIL: proc_next too many processes\n");
				return st_fail(c);
			}
		}
		if (nprocs == 0 || !saw_child || !saw_self) {
			fprintf(stderr, "[selftest] FAIL: proc_next walk n=%u child=%d self=%d\n",
				nprocs, saw_child, saw_self);
			return st_fail(c);
		}
		printf("[selftest] proc_next: PASS (%u processes)\n", nprocs);
		struct int3_proc badp = {
			.cursor = 0,
			.pid = 0,
			.comm = {0},
			._rsv = 1,
		};
		if (ioctl(fd, INT3_IOC_PROC_NEXT, &badp) == 0) {
			fprintf(stderr, "[selftest] FAIL: proc_next accepted _rsv\n");
			return st_fail(c);
		}
		printf("[selftest] proc_next validation: PASS\n");
	} else {
		printf("[selftest] maps_next/proc_next skipped (driver v%u < 4)\n", c->ver);
	}
	return 0;
}
