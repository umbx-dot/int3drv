#include "int3_internal.h"

int int3_scan_skip(struct mm_struct *mm, u64 cur, u64 end, u64 *next)
{
	struct vm_area_struct *vma;
	u64 cand = 0;
	int found = 0;

	mmap_read_lock(mm);
	vma = vma_lookup(mm, (unsigned long)cur);
	if (vma) {
		if (!(vma->vm_flags & VM_READ))
			cand = (u64)vma->vm_end;
		else
			cand = ((cur & ~(PAGE_SIZE - 1ull)) + PAGE_SIZE);
		found = 1;
	} else {
		vma = find_vma(mm, (unsigned long)cur);
		if (vma && (u64)vma->vm_start < end) {
			cand = (u64)vma->vm_start;
			found = 1;
		}
	}
	mmap_read_unlock(mm);

	if (!found || cand <= cur || cand >= end)
		return 0;
	*next = cand;
	return 1;
}

int int3_scan_collect(struct task_struct *t, struct mm_struct *mm, void *kbuf,
		      u64 start, u64 end, u32 itemlen, u32 stride,
		      u64 *matches, u32 cap, u32 *count,
		      int (*hit)(const u8 *p, u64 o, void *arg), void *arg)
{
	u64 cur = start;
	u32 n = 0;

	*count = 0;

	while (cur < end) {
		u64 remain = end - cur;
		u32 chunk_sz = (u32)min_t(u64, remain, (u64)INT3_CHUNK);
		u64 base;
		u64 got_end;
		int r;

		if (signal_pending(current))
			return -ERESTARTSYS;

		if (chunk_sz < itemlen)
			break;

		r = access_process_vm(t, (unsigned long)cur, kbuf, (int)chunk_sz, 0);
		if (r <= 0) {
			u64 next = 0;

			if (!int3_scan_skip(mm, cur, end, &next))
				break;
			cur = next;
			continue;
		}

		n = (u32)r;
		base = (cur + (u64)(stride - 1u)) & ~(u64)(stride - 1u);
		got_end = cur + n;
		for (u64 i = base; i + itemlen <= got_end; i += stride) {
			u64 o = i - cur;
			const u8 *p = (const u8 *)kbuf;

			if ((o & 63u) == 0 && o + 64u < (u64)n)
				__builtin_prefetch(p + o + 64u, 0, 0);
			if (hit(p, o, arg)) {
				if (*count >= cap)
					return -ENOSPC;
				matches[(*count)++] = i;
			}
		}

		if (n <= itemlen) {
			cur += n;
		} else {
			u64 nxt = (cur + n - (itemlen - 1u)) & ~(u64)(stride - 1u);
			cur = (nxt <= cur || nxt > cur + n) ? cur + n : nxt;
		}

		cond_resched();
	}

	return *count ? 0 : -ENOENT;
}

long int3_ioctl_pattern(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_pattern pat;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	u8 pattern[INT3_MAX_PATTERN];
	u8 mask[INT3_MAX_PATTERN];
	void *kbuf;
	u64 cur;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&pat, (void __user *)arg, sizeof(pat)))
		return -EFAULT;
	if (pat._rsv || !pat.pattern_len || pat.pattern_len > INT3_MAX_PATTERN)
		return -EINVAL;
	if (pat.remote_start >= pat.remote_end || !int3_addr_ok(pat.remote_start, pat.pattern_len))
		return -EINVAL;
	if (pat.remote_end > INT3_ADDR_MAX)
		return -EINVAL;
	if (!pat.pattern_ptr)
		return -EINVAL;

	if (copy_from_user(pattern, (const void __user *)(uintptr_t)pat.pattern_ptr, pat.pattern_len))
		return -EFAULT;
	if (pat.mask_ptr) {
		if (copy_from_user(mask, (const void __user *)(uintptr_t)pat.mask_ptr, pat.pattern_len))
			return -EFAULT;
	} else {
		memset(mask, 0xFF, pat.pattern_len);
	}

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

	cur = pat.remote_start;
	pat.match_addr = 0;

	while (cur < pat.remote_end) {
		u64 remain = pat.remote_end - cur;
		u32 chunk_sz = (u32)min_t(u64, remain, (u64)INT3_CHUNK);
		int n;

		if (signal_pending(current)) {
			mmput(mm);
			put_task_struct(t);
			return -ERESTARTSYS;
		}

		if (chunk_sz < pat.pattern_len)
			break;

		n = access_process_vm(t, (unsigned long)cur, kbuf, (int)chunk_sz, 0);
		if (n <= 0) {
			u64 next = 0;

			if (!int3_scan_skip(mm, cur, pat.remote_end, &next))
				break;
			cur = next;
			continue;
		}

		if ((u32)n >= pat.pattern_len) {
			u32 limit = (u32)n - pat.pattern_len + 1u;
			const u8 *p = (const u8 *)kbuf;
			for (u32 i = 0; i < limit; i++) {
				if ((i & 63u) == 0 && i + 64u < limit)
					__builtin_prefetch(p + i + 64u, 0, 0);

				int match = 1;
				for (u32 j = 0; j < pat.pattern_len; j++) {
					if ((p[i + j] & mask[j]) != (pattern[j] & mask[j])) {
						match = 0;
						break;
					}
				}
				if (match) {
					pat.match_addr = cur + i;
					mmput(mm);
					put_task_struct(t);
					if (copy_to_user((void __user *)arg, &pat, sizeof(pat)))
						return -EFAULT;
					return 0;
				}
			}
		}

		if ((u32)n <= pat.pattern_len)
			cur += (u32)n;
		else
			cur += (u32)n - (pat.pattern_len - 1u);

		cond_resched();
	}

	mmput(mm);
	put_task_struct(t);
	if (copy_to_user((void __user *)arg, &pat, sizeof(pat)))
		return -EFAULT;
	return -ENOENT;
}
