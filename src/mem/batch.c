#include "int3_internal.h"

long int3_ioctl_batch(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_batch b;
	struct int3_op *ops = NULL;
	struct task_struct *t = NULL;
	u8 stack_buf[INT3_STACK_BUF_SIZE];
	void *kbuf = NULL;
	u64 total = 0;
	u32 i;
	int rc = 0;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&b, (void __user *)arg, sizeof(b)))
		return -EFAULT;
	if (b._rsv)
		return -EINVAL;
	if (!b.count || b.count > INT3_MAX_OPS || !b.ops)
		return -EINVAL;
	if (b.flags != INT3_BATCH_READ && b.flags != INT3_BATCH_WRITE && b.flags != INT3_BATCH_MIXED)
		return -EINVAL;

	ops = kmalloc_array(b.count, sizeof(*ops), GFP_KERNEL);
	if (!ops)
		return -ENOMEM;
	if (copy_from_user(ops, (void __user *)(uintptr_t)b.ops, b.count * sizeof(*ops))) {
		rc = -EFAULT;
		goto out;
	}

	for (i = 0; i < b.count; i++) {
		if (b.flags == INT3_BATCH_READ &&
		    ops[i].flags != INT3_OP_READ) {
			rc = -EINVAL;
			goto out;
		}
		if (b.flags == INT3_BATCH_WRITE &&
		    ops[i].flags != 0 && ops[i].flags != INT3_OP_WRITE && ops[i].flags != INT3_OP_FORCE &&
		    ops[i].flags != INT3_OP_WRITE_VERIFY) {
			rc = -EINVAL;
			goto out;
		}
		if (b.flags == INT3_BATCH_MIXED &&
		    ops[i].flags != INT3_OP_READ &&
		    ops[i].flags != INT3_OP_WRITE &&
		    ops[i].flags != INT3_OP_FORCE &&
		    ops[i].flags != INT3_OP_WRITE_VERIFY) {
			rc = -EINVAL;
			goto out;
		}
		if (!ops[i].len || ops[i].len > INT3_MAX_SINGLE || !ops[i].local) {
			rc = -EINVAL;
			goto out;
		}
		if (!int3_addr_ok(ops[i].remote, ops[i].len)) {
			rc = -EINVAL;
			goto out;
		}
		total += ops[i].len;
		if (total > INT3_MAX_TOTAL) {
			rc = -EINVAL;
			goto out;
		}
	}

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		goto out;

	kbuf = int3_get_bounce(ctx);
	if (!kbuf) {
		rc = -ENOMEM;
		put_task_struct(t);
		t = NULL;
		goto out;
	}

	for (i = 0; i < b.count; i++) {
		void *buf = (ops[i].len <= sizeof(stack_buf)) ? stack_buf : kbuf;
		u32 cap = (buf == stack_buf) ? (u32)sizeof(stack_buf) : INT3_CHUNK;
		int is_write = 0;
		int is_force = 0;
		int is_verify = 0;

		if (signal_pending(current)) {
			rc = -ERESTARTSYS;
			b.fail_idx = (__s32)i;
			if (copy_to_user((void __user *)arg, &b, sizeof(b)))
				rc = -EFAULT;
			ctx->stats.ops_failed++;
			put_task_struct(t);
			kfree(ops);
			return rc;
		}

		if (b.flags == INT3_BATCH_MIXED) {
			is_write = (ops[i].flags == INT3_OP_WRITE || ops[i].flags == INT3_OP_FORCE ||
				    ops[i].flags == INT3_OP_WRITE_VERIFY);
			is_force = (ops[i].flags == INT3_OP_FORCE);
			is_verify = (ops[i].flags == INT3_OP_WRITE_VERIFY);
		} else if (b.flags == INT3_BATCH_WRITE) {
			is_write = 1;
			is_force = (ops[i].flags == INT3_OP_FORCE);
			is_verify = (ops[i].flags == INT3_OP_WRITE_VERIFY);
		} else {
			is_write = 0;
			is_force = 0;
			is_verify = 0;
		}

		if (is_verify)
			rc = int3_do_write_verify(t, ops[i].remote, (const void __user *)(uintptr_t)ops[i].local,
						  ops[i].len, buf, cap);
		else if (is_force)
			rc = int3_do_write_force(t, ops[i].remote, (const void __user *)(uintptr_t)ops[i].local, ops[i].len, buf);
		else if (is_write)
			rc = int3_do_write(t, ops[i].remote, (const void __user *)(uintptr_t)ops[i].local, ops[i].len, buf);
		else
			rc = int3_do_read(t, ops[i].remote, (void __user *)(uintptr_t)ops[i].local, ops[i].len, buf);

		if (rc) {
			b.fail_idx = (__s32)i;
			if (copy_to_user((void __user *)arg, &b, sizeof(b)))
				rc = -EFAULT;
			ctx->stats.ops_failed++;
			put_task_struct(t);
			kfree(ops);
			return rc;
		}

		if (is_write)
			ctx->stats.bytes_written += ops[i].len;
		else
			ctx->stats.bytes_read += ops[i].len;
		ctx->stats.ops_completed++;
	}

	put_task_struct(t);
	t = NULL;
	b.fail_idx = -1;
	if (copy_to_user((void __user *)arg, &b, sizeof(b)))
		rc = -EFAULT;
	else
		rc = 0;

out:
	if (t)
		put_task_struct(t);
	kfree(ops);
	return rc;
}
