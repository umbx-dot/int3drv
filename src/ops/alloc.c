#include "int3_internal.h"

struct int3_mm_job {
	struct mm_struct *mm;
	struct completion done;
	unsigned long hint;
	unsigned long addr;
	size_t len;
	unsigned long prot;
	long ret;
	int is_free;
};

static int int3_mm_worker(void *data)
{
	struct int3_mm_job *job = data;

	kthread_use_mm(job->mm);
	if (job->is_free) {
		job->ret = vm_munmap(job->addr, job->len);
	} else {
		unsigned long a = vm_mmap(NULL, job->hint, job->len, job->prot,
					   MAP_PRIVATE | MAP_ANONYMOUS, 0);
		if (IS_ERR_VALUE(a)) {
			job->ret = (long)a;
		} else {
			job->addr = a;
			job->ret = 0;
		}
	}
	kthread_unuse_mm(job->mm);
	complete(&job->done);
	return 0;
}

static long int3_run_in_mm(struct task_struct *t, struct int3_mm_job *job)
{
	struct task_struct *k;
	struct mm_struct *mm = get_task_mm(t);

	if (!mm)
		return -ESRCH;
	job->mm = mm;
	init_completion(&job->done);
	k = kthread_run(int3_mm_worker, job, "int3alloc");
	if (IS_ERR(k)) {
		mmput(mm);
		return PTR_ERR(k);
	}
	wait_for_completion(&job->done);
	kthread_stop(k);
	mmput(mm);
	return job->ret;
}

long int3_remote_mmap(struct task_struct *t, unsigned long hint, size_t len,
		      unsigned long prot, unsigned long *addr)
{
	struct int3_mm_job job = {0};
	long rc;

	job.hint = hint;
	job.len = len;
	job.prot = prot;
	rc = int3_run_in_mm(t, &job);
	if (rc)
		return rc;
	*addr = job.addr;
	return 0;
}

long int3_remote_munmap(struct task_struct *t, unsigned long addr, size_t len)
{
	struct int3_mm_job job = {0};

	job.is_free = 1;
	job.addr = addr;
	job.len = len;
	return int3_run_in_mm(t, &job);
}

long int3_ioctl_alloc(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_alloc a;
	struct task_struct *t = NULL;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;
	if (a._rsv || !a.len || a.len > INT3_MAX_SINGLE)
		return -EINVAL;
	if (!a.prot || (a.prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC)))
		return -EINVAL;
	if (a.hint && (!int3_addr_ok(a.hint, 1) || a.hint + a.len < a.hint))
		return -EINVAL;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	{
		unsigned long addr = 0;

		rc = (int)int3_remote_mmap(t, (unsigned long)a.hint, (size_t)a.len, a.prot, &addr);
		put_task_struct(t);
		if (rc)
			return rc;
		if (!int3_addr_ok(addr, a.len))
			return -EFAULT;
		a.addr = addr;
	}
	if (copy_to_user((void __user *)arg, &a, sizeof(a)))
		return -EFAULT;

	ctx->stats.ops_completed++;
	return 0;
}

long int3_ioctl_free(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_free f;
	struct task_struct *t = NULL;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&f, (void __user *)arg, sizeof(f)))
		return -EFAULT;
	if (!f.len || f.len > INT3_MAX_SINGLE || !int3_addr_ok(f.addr, f.len))
		return -EINVAL;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	rc = (int)int3_remote_munmap(t, (unsigned long)f.addr, (size_t)f.len);
	put_task_struct(t);
	if (rc)
		return rc;

	ctx->stats.ops_completed++;
	return 0;
}
