#include "int3_internal.h"

long int3_ioctl_vma_next(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_vma_next v;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	struct vm_area_struct *vma;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&v, (void __user *)arg, sizeof(v)))
		return -EFAULT;
	if (v.cursor != 0 && !int3_addr_ok(v.cursor, 1))
		return -EINVAL;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	mm = get_task_mm(t);
	if (!mm) {
		put_task_struct(t);
		return -ESRCH;
	}

	mmap_read_lock(mm);
	vma = find_vma(mm, (unsigned long)v.cursor);
	if (vma) {
		v.vm_start = (u64)vma->vm_start;
		v.vm_end = (u64)vma->vm_end;
		v.vm_flags = int3_vma_flags(vma);
		rc = 0;
	} else {
		rc = -ENOENT;
	}
	mmap_read_unlock(mm);
	mmput(mm);
	put_task_struct(t);

	if (rc == 0 && copy_to_user((void __user *)arg, &v, sizeof(v)))
		return -EFAULT;

	return rc;
}
