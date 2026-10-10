#include "int3_internal.h"

int int3_do_write_verify(struct task_struct *task, u64 remote, const void __user *local, u64 len, void *kbuf, u32 kcap)
{
	u8 *w = kbuf;
	u8 *r = (u8 *)kbuf + kcap / 2u;
	u32 half = kcap / 2u;
	u64 off = 0;

	if (kcap < 2u)
		return -EINVAL;

	while (off < len) {
		if (signal_pending(current))
			return -ERESTARTSYS;
		u32 c = (u32)min_t(u64, len - off, (u64)half);
		int n = 0;

		if (copy_from_user(w, (const char __user *)local + off, c))
			return -EFAULT;
		n = access_process_vm(task, (unsigned long)(remote + off), w, (int)c, FOLL_WRITE);
		if (n != (int)c)
			return n < 0 ? n : -EFAULT;
		n = access_process_vm(task, (unsigned long)(remote + off), r, (int)c, 0);
		if (n != (int)c)
			return n < 0 ? n : -EFAULT;
		if (memcmp(w, r, c) != 0)
			return -EIO;
		off += c;
		cond_resched();
	}
	return 0;
}

long int3_ioctl_write_verify(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_rw rw;
	struct task_struct *t = NULL;
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

	kbuf = int3_get_bounce(ctx);
	if (!kbuf) {
		put_task_struct(t);
		return -ENOMEM;
	}

	rc = int3_do_write_verify(t, rw.remote, (const void __user *)(uintptr_t)rw.local, rw.len, kbuf, INT3_CHUNK);

	put_task_struct(t);

	if (rc == 0) {
		ctx->stats.bytes_written += rw.len;
		ctx->stats.ops_completed++;
	} else {
		ctx->stats.ops_failed++;
	}

	return rc;
}
