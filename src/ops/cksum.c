#include "int3_internal.h"

long int3_ioctl_cksum(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_cksum c;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	void *kbuf;
	u64 cur;
	u64 hash = 14695981039346656037ull;
	u64 hashed = 0;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&c, (void __user *)arg, sizeof(c)))
		return -EFAULT;
	if (c.remote_start >= c.remote_end || !int3_addr_ok(c.remote_start, 1))
		return -EINVAL;
	if (c.remote_end > INT3_ADDR_MAX)
		return -EINVAL;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	mm = get_task_mm(t);
	if (!mm) {
		put_task_struct(t);
		return -ESRCH;
	}

	kbuf = int3_get_bounce(ctx);
	if (!kbuf) {
		mmput(mm);
		put_task_struct(t);
		return -ENOMEM;
	}

	cur = c.remote_start;
	c.hash = 0;

	while (cur < c.remote_end) {
		u64 remain = c.remote_end - cur;
		u32 chunk_sz = (u32)min_t(u64, remain, (u64)INT3_CHUNK);
		const u8 *p = (const u8 *)kbuf;
		int n;

		if (signal_pending(current)) {
			mmput(mm);
			put_task_struct(t);
			return -ERESTARTSYS;
		}

		n = access_process_vm(t, (unsigned long)cur, kbuf, (int)chunk_sz, 0);
		if (n <= 0) {
			u64 next = 0;

			if (!int3_scan_skip(mm, cur, c.remote_end, &next))
				break;
			cur = next;
			continue;
		}

		for (int i = 0; i < n; i++) {
			hash ^= p[i];
			hash *= 1099511628211ull;
		}
		hashed += (u64)n;
		cur += (u32)n;
		cond_resched();
	}

	mmput(mm);
	put_task_struct(t);

	if (!hashed)
		return -ENOENT;

	c.hash = hash;
	if (copy_to_user((void __user *)arg, &c, sizeof(c)))
		return -EFAULT;

	ctx->stats.bytes_read += hashed;
	ctx->stats.ops_completed++;
	return 0;
}
