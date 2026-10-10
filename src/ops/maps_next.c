#include "int3_internal.h"

long int3_ioctl_maps_next(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_map m;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	struct vm_area_struct *vma;
	struct file *f;
	char *kbuf;
	char *p;
	size_t full;
	size_t n;
	int empty;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&m, (void __user *)arg, sizeof(m)))
		return -EFAULT;
	if (m.cursor != 0 && !int3_addr_ok(m.cursor, 1))
		return -EINVAL;
	if (!m.path_ptr || m.path_cap == 0 || m.path_cap > 4096)
		return -EINVAL;

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

	mmap_read_lock(mm);
	vma = find_vma(mm, (unsigned long)m.cursor);
	if (!vma) {
		mmap_read_unlock(mm);
		mmput(mm);
		put_task_struct(t);
		return -ENOENT;
	}
	m.vm_start = (u64)vma->vm_start;
	m.vm_end = (u64)vma->vm_end;
	m.vm_flags = int3_vma_flags(vma);
	f = vma->vm_file;
	empty = 1;
	if (f) {
		p = d_path(&f->f_path, kbuf, INT3_CHUNK);
		if (!IS_ERR(p)) {
			full = strlen(p) + 1;
			n = full;
			if (n > m.path_cap)
				n = m.path_cap;
			memmove(kbuf, p, n);
			kbuf[n - 1] = '\0';
			m.path_len = (__u32)n;
			empty = 0;
		}
	}
	if (empty) {
		kbuf[0] = '\0';
		m.path_len = 1;
		n = 1;
	}
	mmap_read_unlock(mm);
	mmput(mm);
	put_task_struct(t);

	if (copy_to_user((void __user *)(uintptr_t)m.path_ptr, kbuf, n))
		return -EFAULT;
	if (copy_to_user((void __user *)arg, &m, sizeof(m)))
		return -EFAULT;

	return 0;
}
