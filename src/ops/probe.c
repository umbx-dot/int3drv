#include "int3_internal.h"

long int3_ioctl_probe(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_probe p;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	unsigned long cur, end;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&p, (void __user *)arg, sizeof(p)))
		return -EFAULT;
	if (!p.len || !int3_addr_ok(p.remote, p.len))
		return -EINVAL;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	mm = get_task_mm(t);
	if (!mm) {
		put_task_struct(t);
		return -ESRCH;
	}

	p.is_readable = 1;
	p.is_writable = 1;

	cur = (unsigned long)p.remote;
	end = (unsigned long)(p.remote + p.len);

	mmap_read_lock(mm);
	while (cur < end) {
		struct vm_area_struct *vma = vma_lookup(mm, cur);
		if (!vma) {
			p.is_readable = 0;
			p.is_writable = 0;
			break;
		}
		if (!(vma->vm_flags & VM_READ))
			p.is_readable = 0;
		if (!(vma->vm_flags & VM_WRITE))
			p.is_writable = 0;
		if (vma->vm_end <= cur) {
			p.is_readable = 0;
			p.is_writable = 0;
			break;
		}
		cur = vma->vm_end;
	}
	mmap_read_unlock(mm);

	mmput(mm);
	put_task_struct(t);

	if (copy_to_user((void __user *)arg, &p, sizeof(p)))
		return -EFAULT;
	return 0;
}
