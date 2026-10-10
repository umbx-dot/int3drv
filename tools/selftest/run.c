#include "ctx.h"

#include <sys/mman.h>

unsigned long st_thread_fn(void *arg)
{
	*(volatile unsigned long *)arg = 0x1BADB002ul;
	return 0xFEEDFACEul;
}

int st_fail(struct st_ctx *c)
{
	kill(c->pid, SIGKILL);
	waitpid(c->pid, nullptr, 0);
	close(c->sv[0]);
	close(c->fd);
	return 1;
}

static int st_layout(void)
{
	printf("[selftest] verifying struct layouts and constants...\n");
	if (sizeof(struct int3_rw) != 24 || sizeof(struct int3_op) != 24 ||
	    sizeof(struct int3_batch) != 24 || sizeof(struct int3_stats) != 32 ||
	    sizeof(struct int3_probe) != 24 || sizeof(struct int3_val) != 16 ||
	    sizeof(struct int3_pattern) != 48 || sizeof(struct int3_vma) != 32 ||
	    sizeof(struct int3_vma_next) != 32 || sizeof(struct int3_deref) != 40 ||
	    sizeof(struct int3_scan_all) != 64 || sizeof(struct int3_map) != 48 ||
	    sizeof(struct int3_proc) != 28 || sizeof(struct int3_mod) != 24 ||
	    sizeof(struct int3_chain) != 48 || sizeof(struct int3_vscan) != 48 ||
	    sizeof(struct int3_cksum) != 24 || sizeof(struct int3_copy) != 24 ||
	    sizeof(struct int3_alloc) != 32 || sizeof(struct int3_free) != 16 ||
	    sizeof(struct int3_thread) != 32) {
		fprintf(stderr, "[selftest] FAIL: struct alignment / size error\n");
		return 1;
	}
	printf("[selftest] struct layouts verified.\n");
	return 0;
}

static int st_spawn(struct st_ctx *c)
{
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, c->sv) < 0) {
		close(c->fd);
		return 1;
	}

	c->pid = fork();
	if (c->pid == 0) {
		close(c->sv[0]);
		volatile uint64_t target_secret = 0x1122334455667788ull;
		char target_buf[64] = "int3_fast_memory_driver_test_payload";
		uint64_t buf_ptr = (uint64_t)(uintptr_t)target_buf;
		volatile unsigned long thread_flag = 0;
		uint8_t *ro_page = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (ro_page == MAP_FAILED)
			_exit(1);
		memcpy(ro_page, "RODATA_PAGE_MARKER", 18);
		if (mprotect(ro_page, 4096, PROT_READ) < 0)
			_exit(1);
		uint64_t addrs[6] = {
			(uint64_t)(uintptr_t)&target_secret,
			(uint64_t)(uintptr_t)target_buf,
			(uint64_t)(uintptr_t)&buf_ptr,
			(uint64_t)(uintptr_t)ro_page,
			(uint64_t)(uintptr_t)&thread_flag,
			(uint64_t)(uintptr_t)st_thread_fn
		};
		if (write(c->sv[1], addrs, sizeof(addrs)) != sizeof(addrs))
			_exit(1);
		char dummy = 0;
		if (read(c->sv[1], &dummy, 1) != 1)
			_exit(1);
		if (read(c->sv[1], &dummy, 1) != 1)
			_exit(1);
		close(c->sv[1]);
		_exit(0);
	}

	close(c->sv[1]);
	if (read(c->sv[0], c->addrs, sizeof(c->addrs)) != sizeof(c->addrs)) {
		kill(c->pid, SIGKILL);
		waitpid(c->pid, nullptr, 0);
		close(c->sv[0]);
		close(c->fd);
		return 1;
	}

	int32_t attach_pid = (int32_t)c->pid;
	if (ioctl(c->fd, INT3_IOC_ATTACH, &attach_pid) < 0) {
		fprintf(stderr, "[selftest] FAIL: attach(%d): %s\n", c->pid, strerror(errno));
		return st_fail(c);
	}

	int32_t cur_pid = -1;
	if (ioctl(c->fd, INT3_IOC_PID, &cur_pid) < 0 || cur_pid != attach_pid) {
		fprintf(stderr, "[selftest] FAIL: get_pid returned %d, expected %d\n", cur_pid, attach_pid);
		return st_fail(c);
	}
	return 0;
}

int st_run(void)
{
	struct st_ctx ctx = {0};

	signal(SIGPIPE, SIG_IGN);
	if (st_layout())
		return 1;

	ctx.fd = open(INT3DRV_DEV, O_RDWR | O_CLOEXEC);
	if (ctx.fd < 0) {
		printf("[selftest] %s not accessible (%s). Skipping live driver test.\n",
		       INT3DRV_DEV, strerror(errno));
		printf("[selftest] To run full live test, load int3drv.ko with 'sudo insmod int3drv.ko'.\n");
		return 0;
	}

	if (ioctl(ctx.fd, INT3_IOC_VERSION, &ctx.ver) < 0 || ctx.ver < 1) {
		fprintf(stderr, "[selftest] FAIL: INT3_IOC_VERSION\n");
		close(ctx.fd);
		return 1;
	}
	printf("[selftest] driver version: %u\n", ctx.ver);

	if (st_spawn(&ctx))
		return 1;

	ctx.val.remote = ctx.addrs[0];
	ctx.val.val = 0;

	if (st_scalar(&ctx) || st_batch(&ctx) || st_writes(&ctx) ||
	    st_validate(&ctx) || st_query(&ctx) || st_scan(&ctx) ||
	    st_deref(&ctx) || st_verify(&ctx) || st_scanall(&ctx) ||
	    st_maps_procs(&ctx) || st_v5(&ctx) || st_mmap(&ctx) || st_thread(&ctx) ||
	    st_lifecycle(&ctx))
		return 1;

	char fin = 'x';
	if (write(ctx.sv[0], &fin, 1) != 1) {
		kill(ctx.pid, SIGKILL);
	}
	close(ctx.sv[0]);
	kill(ctx.pid, SIGKILL);
	waitpid(ctx.pid, nullptr, 0);
	close(ctx.fd);
	printf("[selftest] ALL TESTS PASSED.\n");
	return 0;
}
