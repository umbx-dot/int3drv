#include "int3_internal.h"

int int3_do_read(struct task_struct *task, u64 remote, void __user *local, u64 len, void *kbuf)
{
	u64 off = 0;

	while (off < len) {
		if (signal_pending(current))
			return -ERESTARTSYS;
		u32 c = (u32)min_t(u64, len - off, (u64)INT3_CHUNK);
		int n = access_process_vm(task, (unsigned long)(remote + off), kbuf, (int)c, 0);
		if (n != (int)c)
			return n < 0 ? n : -EFAULT;
		if (copy_to_user((char __user *)local + off, kbuf, c))
			return -EFAULT;
		off += c;
		cond_resched();
	}
	return 0;
}

long int3_read_finish(struct int3_ctx *ctx, struct task_struct *t, u64 target,
		      u64 local, u32 len, u64 stat_add,
		      void __user *uarg, const void *karg, size_t ksize)
{
	void *kbuf = int3_get_bounce(ctx);
	int rc;

	if (!kbuf)
		rc = -ENOMEM;
	else
		rc = int3_do_read(t, target, (void __user *)(uintptr_t)local, len, kbuf);

	put_task_struct(t);

	if (copy_to_user(uarg, karg, ksize))
		rc = -EFAULT;

	if (rc == 0) {
		ctx->stats.bytes_read += stat_add + len;
		ctx->stats.ops_completed++;
	} else {
		ctx->stats.ops_failed++;
	}

	return rc;
}
