#include "int3_internal.h"

long int3_ioctl_deref(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_deref d;
	struct task_struct *t = NULL;
	u64 ptr = 0;
	u64 target;
	int rc;
	int n;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&d, (void __user *)arg, sizeof(d)))
		return -EFAULT;
	if (d._rsv || !d.len || d.len > INT3_MAX_SINGLE || !d.local)
		return -EINVAL;
	if (!int3_addr_ok(d.ptr_addr, sizeof(ptr)))
		return -EINVAL;

	d.out_ptr = 0;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	n = access_process_vm(t, (unsigned long)d.ptr_addr, &ptr, (int)sizeof(ptr), 0);
	if (n != (int)sizeof(ptr)) {
		rc = n < 0 ? n : -EFAULT;
		goto out;
	}

	d.out_ptr = ptr;
	target = ptr + d.disp;
	if (!int3_addr_ok(target, d.len)) {
		rc = -EINVAL;
		goto out;
	}

	return int3_read_finish(ctx, t, target, d.local, d.len, (u64)sizeof(ptr),
				(void __user *)arg, &d, sizeof(d));

out:
	put_task_struct(t);

	if (copy_to_user((void __user *)arg, &d, sizeof(d)))
		return -EFAULT;

	ctx->stats.ops_failed++;
	return rc;
}
