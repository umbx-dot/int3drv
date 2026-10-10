#include "int3_internal.h"

long int3_ioctl_copy(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_copy c;
	struct task_struct *t = NULL;
	void *kbuf;
	u64 off = 0;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&c, (void __user *)arg, sizeof(c)))
		return -EFAULT;
	if (!c.len || c.len > INT3_MAX_SINGLE)
		return -EINVAL;
	if (!int3_addr_ok(c.src, c.len) || !int3_addr_ok(c.dst, c.len))
		return -EINVAL;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	kbuf = int3_get_bounce(ctx);
	if (!kbuf) {
		put_task_struct(t);
		return -ENOMEM;
	}

	while (off < c.len) {
		u32 chunk = (u32)min_t(u64, c.len - off, (u64)INT3_CHUNK);
		int n;

		if (signal_pending(current)) {
			rc = -ERESTARTSYS;
			goto out;
		}
		n = access_process_vm(t, (unsigned long)(c.src + off), kbuf, (int)chunk, 0);
		if (n != (int)chunk) {
			rc = n < 0 ? n : -EFAULT;
			goto out;
		}
		n = access_process_vm(t, (unsigned long)(c.dst + off), kbuf, (int)chunk, FOLL_WRITE);
		if (n != (int)chunk) {
			rc = n < 0 ? n : -EFAULT;
			goto out;
		}
		off += chunk;
		cond_resched();
	}
	rc = 0;

out:
	put_task_struct(t);

	if (rc == 0) {
		ctx->stats.bytes_read += c.len;
		ctx->stats.bytes_written += c.len;
		ctx->stats.ops_completed++;
	} else {
		ctx->stats.ops_failed++;
	}

	return rc;
}
