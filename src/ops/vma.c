#include "int3_internal.h"

u64 int3_vma_flags(struct vm_area_struct *vma)
{
	u64 f = 0;

	if (vma->vm_flags & VM_READ)
		f |= INT3_VM_READ;
	if (vma->vm_flags & VM_WRITE)
		f |= INT3_VM_WRITE;
	if (vma->vm_flags & VM_EXEC)
		f |= INT3_VM_EXEC;
	if (vma->vm_flags & VM_SHARED)
		f |= INT3_VM_SHARED;
	return f;
}

long int3_ioctl_vma(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_vma v;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	struct vm_area_struct *vma;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&v, (void __user *)arg, sizeof(v)))
		return -EFAULT;
	if (!int3_addr_ok(v.addr, 1))
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
	vma = vma_lookup(mm, (unsigned long)v.addr);
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
