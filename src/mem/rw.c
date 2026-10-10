#include "int3_internal.h"

long int3_ioctl_rw(struct int3_ctx *ctx, unsigned long arg, int is_read, int is_force)
{
	struct int3_rw rw;
	struct task_struct *t = NULL;
	u8 stack_buf[INT3_STACK_BUF_SIZE];
	void *kbuf;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&rw, (void __user *)arg, sizeof(rw)))
		return -EFAULT;
	if (!rw.len || rw.len > INT3_MAX_SINGLE || !rw.local)
		return -EINVAL;
	if (!int3_addr_ok(rw.remote, rw.len))
		return -EINVAL;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	if (rw.len <= sizeof(stack_buf)) {
		kbuf = stack_buf;
	} else {
		kbuf = int3_get_bounce(ctx);
		if (!kbuf) {
			put_task_struct(t);
			return -ENOMEM;
		}
	}

	if (is_read)
		rc = int3_do_read(t, rw.remote, (void __user *)(uintptr_t)rw.local, rw.len, kbuf);
	else if (is_force)
		rc = int3_do_write_force(t, rw.remote, (const void __user *)(uintptr_t)rw.local, rw.len, kbuf);
	else
		rc = int3_do_write(t, rw.remote, (const void __user *)(uintptr_t)rw.local, rw.len, kbuf);

	put_task_struct(t);

	if (rc == 0) {
		if (is_read)
			ctx->stats.bytes_read += rw.len;
		else
			ctx->stats.bytes_written += rw.len;
		ctx->stats.ops_completed++;
	} else {
		ctx->stats.ops_failed++;
	}

	return rc;
}
