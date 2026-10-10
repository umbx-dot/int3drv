#include "int3_internal.h"

long int3_ioctl_chain(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_chain c;
	struct task_struct *t = NULL;
	u64 offs[INT3_MAX_CHAIN];
	u64 p = 0;
	int rc;
	int n;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&c, (void __user *)arg, sizeof(c)))
		return -EFAULT;
	if (c._rsv || !c.len || c.len > INT3_MAX_SINGLE || !c.local)
		return -EINVAL;
	if (c.depth > INT3_MAX_CHAIN)
		return -EINVAL;
	if (c.depth && !c.offsets)
		return -EINVAL;
	if (!int3_addr_ok(c.ptr_addr, sizeof(p)))
		return -EINVAL;
	if (c.depth && copy_from_user(offs, (const void __user *)(uintptr_t)c.offsets,
				      c.depth * sizeof(offs[0])))
		return -EFAULT;

	c.out_ptr = 0;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	n = access_process_vm(t, (unsigned long)c.ptr_addr, &p, (int)sizeof(p), 0);
	if (n != (int)sizeof(p)) {
		rc = n < 0 ? n : -EFAULT;
		goto out;
	}

	for (u32 i = 0; i < c.depth; i++) {
		u64 next;

		p += offs[i];
		if (!int3_addr_ok(p, sizeof(p))) {
			rc = -EINVAL;
			goto out;
		}
		n = access_process_vm(t, (unsigned long)p, &next, (int)sizeof(next), 0);
		if (n != (int)sizeof(next)) {
			rc = n < 0 ? n : -EFAULT;
			goto out;
		}
		p = next;
	}

	c.out_ptr = p;
	if (!int3_addr_ok(p, c.len)) {
		rc = -EINVAL;
		goto out;
	}

	return int3_read_finish(ctx, t, p, c.local, c.len,
				(u64)sizeof(p) * (1 + c.depth),
				(void __user *)arg, &c, sizeof(c));

out:
	put_task_struct(t);

	if (copy_to_user((void __user *)arg, &c, sizeof(c)))
		return -EFAULT;

	ctx->stats.ops_failed++;
	return rc;
}
