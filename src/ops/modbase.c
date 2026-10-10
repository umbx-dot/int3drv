#include "int3_internal.h"

long int3_ioctl_modbase(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_mod m;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	char name[INT3_MAX_NAME + 1];
	char *kbuf;
	unsigned long cursor = 0;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&m, (void __user *)arg, sizeof(m)))
		return -EFAULT;
	if (m._rsv || !m.name_len || m.name_len > INT3_MAX_NAME || !m.name_ptr)
		return -EINVAL;
	if (copy_from_user(name, (const void __user *)(uintptr_t)m.name_ptr, m.name_len))
		return -EFAULT;
	if (memchr(name, '\0', m.name_len))
		return -EINVAL;
	name[m.name_len] = '\0';

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	mm = get_task_mm(t);
	if (!mm) {
		put_task_struct(t);
		return -ESRCH;
	}

	m.base = 0;
	rc = -ENOENT;

	kbuf = int3_get_bounce(ctx);
	if (!kbuf) {
		mmput(mm);
		put_task_struct(t);
		return -ENOMEM;
	}

	mmap_read_lock(mm);
	for (;;) {
		struct vm_area_struct *vma = find_vma(mm, cursor);
		struct file *f;
		char *p;

		if (!vma)
			break;
		cursor = vma->vm_end;
		f = vma->vm_file;
		if (!f)
			continue;
		p = d_path(&f->f_path, kbuf, INT3_CHUNK);
		if (IS_ERR(p))
			continue;
		if (strstr(p, name)) {
			m.base = (u64)vma->vm_start;
			rc = 0;
			break;
		}
		if (cursor == 0)
			break;
	}
	mmap_read_unlock(mm);
	mmput(mm);
	put_task_struct(t);

	if (copy_to_user((void __user *)arg, &m, sizeof(m)))
		return -EFAULT;

	return rc;
}
