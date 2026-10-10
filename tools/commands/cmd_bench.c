#include "int3ctl.h"

int cmd_bench(int pid, uint64_t addr, uint32_t iterations)
{
	if (!iterations)
		iterations = 100000;
	int fd = int3_open_attached(pid);
	if (fd < 0)
		return 1;

	struct timespec t0, t1;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	struct int3_val val = {.remote = addr, .val = 0};
	for (uint32_t i = 0; i < iterations; i++) {
		if (ioctl(fd, INT3_IOC_READ_U64, &val) < 0) {
			fprintf(stderr, "scalar read failed at iter %u\n", i);
			close(fd);
			return 1;
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);
	double dur_s = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) * 1e-9;
	double ns_per_op = (dur_s / (double)iterations) * 1e9;
	printf("INT3_IOC_READ_U64: %u iterations in %.3f s (%.1f ns/op, %.2f Mops/s)\n",
	       iterations, dur_s, ns_per_op, ((double)iterations / dur_s) / 1e6);

	uint64_t batch_vals[256];
	struct int3_op ops[256];
	for (size_t i = 0; i < 256; i++) {
		ops[i].remote = addr;
		ops[i].local = (uint64_t)(uintptr_t)&batch_vals[i];
		ops[i].len = sizeof(uint64_t);
		ops[i].flags = INT3_OP_READ;
	}
	struct int3_batch b = {
		.ops = (uint64_t)(uintptr_t)ops,
		.count = 256,
		.flags = INT3_BATCH_READ,
		.fail_idx = -1,
		._rsv = 0,
	};
	uint32_t batch_iters = iterations / 256;
	if (!batch_iters)
		batch_iters = 1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (uint32_t i = 0; i < batch_iters; i++) {
		if (ioctl(fd, INT3_IOC_BATCH, &b) < 0) {
			fprintf(stderr, "batch read failed at iter %u\n", i);
			close(fd);
			return 1;
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);
	dur_s = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) * 1e-9;
	uint32_t total_ops = batch_iters * 256;
	ns_per_op = (dur_s / (double)total_ops) * 1e9;
	printf("INT3_IOC_BATCH (256x u64): %u ops in %.3f s (%.1f ns/op, %.2f Mops/s)\n",
	       total_ops, dur_s, ns_per_op, ((double)total_ops / dur_s) / 1e6);

	close(fd);
	return 0;
}
