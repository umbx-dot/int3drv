#include "int3_internal.h"

struct int3_pat_arg {
	const u8 *pat;
	const u8 *mask;
	u32 len;
};

static int int3_pat_hit(const u8 *p, u64 o, void *arg)
{
	struct int3_pat_arg *a = arg;

	for (u32 j = 0; j < a->len; j++) {
		if ((p[o + j] & a->mask[j]) != (a->pat[j] & a->mask[j]))
			return 0;
	}
	return 1;
}

long int3_ioctl_scan_all(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_scan_all s;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	u8 pattern[INT3_MAX_PATTERN];
	u8 mask[INT3_MAX_PATTERN];
	u64 *matches = NULL;
	void *kbuf;
	struct int3_pat_arg pa;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&s, (void __user *)arg, sizeof(s)))
		return -EFAULT;
	if (s._rsv || !s.pattern_len || s.pattern_len > INT3_MAX_PATTERN)
		return -EINVAL;
	if (s.align != 1u && s.align != 2u && s.align != 4u && s.align != 8u)
		return -EINVAL;
	if (s.remote_start >= s.remote_end || !int3_addr_ok(s.remote_start, s.pattern_len))
		return -EINVAL;
	if (s.remote_end > INT3_ADDR_MAX)
		return -EINVAL;
	if (!s.pattern_ptr || !s.match_ptr || !s.match_cap || s.match_cap > INT3_MAX_MATCHES)
		return -EINVAL;

	if (copy_from_user(pattern, (const void __user *)(uintptr_t)s.pattern_ptr, s.pattern_len))
		return -EFAULT;
	if (s.mask_ptr) {
		if (copy_from_user(mask, (const void __user *)(uintptr_t)s.mask_ptr, s.pattern_len))
			return -EFAULT;
	} else {
		memset(mask, 0xFF, s.pattern_len);
	}

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

	pa.pat = pattern;
	pa.mask = mask;
	pa.len = s.pattern_len;

	rc = int3_scan_collect(t, mm, kbuf, s.remote_start, s.remote_end,
			       s.pattern_len, s.align, matches, s.match_cap,
			       &s.match_count, int3_pat_hit, &pa);
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
