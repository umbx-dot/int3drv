#include "int3_internal.h"

static int int3_vscan_hit(u64 v, u64 want, u32 op)
{
	switch (op) {
	case INT3_VSCAN_NE:
		return v != want;
	case INT3_VSCAN_GT:
		return v > want;
	case INT3_VSCAN_LT:
		return v < want;
	default:
		return v == want;
	}
}

struct int3_val_arg {
	u64 value;
	u32 op;
	u32 width;
};

static int int3_val_hit(const u8 *p, u64 o, void *arg)
{
	struct int3_val_arg *a = arg;
	u64 v = 0;

	for (u32 j = 0; j < a->width; j++)
		v |= (u64)p[o + j] << (8u * j);
	return int3_vscan_hit(v, a->value, a->op);
}

long int3_ioctl_vscan(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_vscan s;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	u64 *matches = NULL;
	void *kbuf;
	struct int3_val_arg va;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&s, (void __user *)arg, sizeof(s)))
		return -EFAULT;
	if (s.width != 1u && s.width != 2u && s.width != 4u && s.width != 8u)
		return -EINVAL;
	if (s.op != INT3_VSCAN_EQ && s.op != INT3_VSCAN_NE &&
	    s.op != INT3_VSCAN_GT && s.op != INT3_VSCAN_LT)
		return -EINVAL;
	if (s.remote_start >= s.remote_end || !int3_addr_ok(s.remote_start, s.width))
		return -EINVAL;
	if (s.remote_end > INT3_ADDR_MAX)
		return -EINVAL;
	if (!s.match_ptr || !s.match_cap || s.match_cap > INT3_MAX_MATCHES)
		return -EINVAL;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	mm = get_task_mm(t);
	if (!mm) {
		put_task_struct(t);
		return -ESRCH;
	}

	matches = kmalloc_array(s.match_cap, sizeof(*matches), GFP_KERNEL);
	if (!matches) {
		mmput(mm);
		put_task_struct(t);
		return -ENOMEM;
	}

	kbuf = int3_get_bounce(ctx);
	if (!kbuf) {
		kfree(matches);
		mmput(mm);
		put_task_struct(t);
		return -ENOMEM;
	}

	va.value = s.value;
	va.op = s.op;
	va.width = s.width;

	rc = int3_scan_collect(t, mm, kbuf, s.remote_start, s.remote_end,
			       s.width, s.width, matches, s.match_cap,
			       &s.match_count, int3_val_hit, &va);
	if (rc == -ERESTARTSYS)
		goto out;

	if (s.match_count > 0 &&
	    copy_to_user((void __user *)(uintptr_t)s.match_ptr, matches, (size_t)s.match_count * sizeof(*matches))) {
		rc = -EFAULT;
		goto out;
	}
	if (copy_to_user((void __user *)arg, &s, sizeof(s))) {
		rc = -EFAULT;
		goto out;
	}

out:
	kfree(matches);
	mmput(mm);
	put_task_struct(t);
	return rc;
}
