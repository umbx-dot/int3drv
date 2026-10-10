#include "int3_internal.h"

long int3_ioctl_proc_next(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_proc p;
	struct task_struct *t;
	struct task_struct *best = NULL;
	char comm[16];
	__u32 best_pid = 0;
	__u32 pid;

	(void)ctx;

	if (copy_from_user(&p, (void __user *)arg, sizeof(p)))
		return -EFAULT;
	if (p._rsv)
		return -EINVAL;

	rcu_read_lock();
	for_each_process(t) {
		pid = (__u32)task_pid_nr(t);
		if (pid > p.cursor && (!best || pid < best_pid)) {
			if (best)
				put_task_struct(best);
			get_task_struct(t);
			best = t;
			best_pid = pid;
		}
	}
	rcu_read_unlock();
	if (!best)
		return -ENOENT;
	get_task_comm(comm, best);
	put_task_struct(best);
	memcpy(p.comm, comm, sizeof(p.comm));
	p.pid = best_pid;

	if (copy_to_user((void __user *)arg, &p, sizeof(p)))
		return -EFAULT;

	return 0;
}
