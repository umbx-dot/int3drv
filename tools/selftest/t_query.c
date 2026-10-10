#include "ctx.h"

int st_query(struct st_ctx *c)
{
	int fd = c->fd;
	uint64_t *addrs = c->addrs;

	struct int3_probe pr = {.remote = addrs[0], .len = 8, .is_readable = 0, .is_writable = 0};
	if (ioctl(fd, INT3_IOC_PROBE, &pr) < 0 || !pr.is_readable || !pr.is_writable) {
		fprintf(stderr, "[selftest] FAIL: probe returned readable=%u writable=%u\n",
		       pr.is_readable, pr.is_writable);
		return st_fail(c);
	}
	printf("[selftest] probe: PASS\n");

	struct int3_vma vm = {.addr = addrs[0], .vm_start = 0, .vm_end = 0, .vm_flags = 0};
	if (ioctl(fd, INT3_IOC_VMA, &vm) < 0 || vm.vm_start > addrs[0] || vm.vm_end <= addrs[0]) {
		fprintf(stderr, "[selftest] FAIL: vma lookup failed\n");
		return st_fail(c);
	}
	printf("[selftest] vma: PASS ([0x%" PRIx64 "-0x%" PRIx64 "])\n", (uint64_t)vm.vm_start, (uint64_t)vm.vm_end);

	struct int3_vma_next nx = {.cursor = 0, .vm_start = 0, .vm_end = 0, .vm_flags = 0};
	if (ioctl(fd, INT3_IOC_VMA_NEXT, &nx) < 0 || nx.vm_end <= nx.vm_start) {
		fprintf(stderr, "[selftest] FAIL: vma_next first region\n");
		return st_fail(c);
	}
	struct int3_vma_next nx2 = {.cursor = (uint64_t)nx.vm_end, .vm_start = 0, .vm_end = 0, .vm_flags = 0};
	if (ioctl(fd, INT3_IOC_VMA_NEXT, &nx2) < 0) {
		if (errno != ENOENT) {
			fprintf(stderr, "[selftest] FAIL: vma_next second region: %s\n", strerror(errno));
			return st_fail(c);
		}
	} else if (nx2.vm_start < (uint64_t)nx.vm_end) {
		fprintf(stderr, "[selftest] FAIL: vma_next did not advance\n");
		return st_fail(c);
	}
	printf("[selftest] vma_next: PASS ([0x%" PRIx64 "-0x%" PRIx64 "])\n",
	       (uint64_t)nx.vm_start, (uint64_t)nx.vm_end);

	uint64_t cursor = 0;
	uint64_t prev_end = 0;
	uint32_t regions = 0;
	for (;;) {
		struct int3_vma_next w = {
			.cursor = cursor,
			.vm_start = 0,
			.vm_end = 0,
			.vm_flags = 0,
		};
		if (ioctl(fd, INT3_IOC_VMA_NEXT, &w) < 0) {
			if (errno == ENOENT)
				break;
			fprintf(stderr, "[selftest] FAIL: vma walk: %s\n", strerror(errno));
			return st_fail(c);
		}
		if (w.vm_end <= w.vm_start || w.vm_start < prev_end) {
			fprintf(stderr, "[selftest] FAIL: vma walk not monotonic\n");
			return st_fail(c);
		}
		if (c->hole_at == 0 && w.vm_start > prev_end && w.vm_start - prev_end >= 0x10000 &&
		    w.vm_start > 0x20000)
			c->hole_at = w.vm_start - 0x1000;
		if (addrs[1] >= w.vm_start && addrs[1] < w.vm_end)
			c->needle_vs = w.vm_start;
		prev_end = w.vm_end;
		cursor = (uint64_t)w.vm_end;
		if (++regions > (1u << 20)) {
			fprintf(stderr, "[selftest] FAIL: vma walk runaway\n");
			return st_fail(c);
		}
	}
	if (regions == 0 || c->needle_vs == 0) {
		fprintf(stderr, "[selftest] FAIL: vma walk empty or needle homeless\n");
		return st_fail(c);
	}
	printf("[selftest] vma walk: PASS (%u regions)\n", regions);

	if (c->hole_at != 0) {
		struct int3_probe hp = {.remote = c->hole_at, .len = 0x1000, .is_readable = 0, .is_writable = 0};
		if (ioctl(fd, INT3_IOC_PROBE, &hp) < 0 || hp.is_readable || hp.is_writable) {
			fprintf(stderr, "[selftest] FAIL: probe hole readable=%u writable=%u\n",
				hp.is_readable, hp.is_writable);
			return st_fail(c);
		}
		printf("[selftest] probe hole: PASS\n");

		struct int3_vma hv = {.addr = c->hole_at, .vm_start = 0, .vm_end = 0, .vm_flags = 0};
		if (ioctl(fd, INT3_IOC_VMA, &hv) == 0) {
			fprintf(stderr, "[selftest] FAIL: vma hit inside hole\n");
			return st_fail(c);
		}
		printf("[selftest] vma hole: PASS\n");

		struct int3_pattern hs = {
			.remote_start = c->hole_at,
			.remote_end = c->hole_at + 0x1000,
			.match_addr = 0,
			.pattern_ptr = (uint64_t)(uintptr_t)ST_NEEDLE,
			.mask_ptr = 0,
			.pattern_len = sizeof(ST_NEEDLE) - 1,
			._rsv = 0,
		};
		if (ioctl(fd, INT3_IOC_PATTERN_SCAN, &hs) == 0) {
			fprintf(stderr, "[selftest] FAIL: scan matched inside hole\n");
			return st_fail(c);
		}
		printf("[selftest] scan hole: PASS\n");
	} else {
		printf("[selftest] hole tests: SKIP (no usable gap)\n");
	}
	return 0;
}
