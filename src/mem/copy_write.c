#include "int3_internal.h"

static int int3_do_write_flags(struct task_struct *task, u64 remote, const void __user *local, u64 len, void *kbuf, int flags)
{
	u64 off = 0;

	while (off < len) {
		if (signal_pending(current))
			return -ERESTARTSYS;
		u32 c = (u32)min_t(u64, len - off, (u64)INT3_CHUNK);
		if (copy_from_user(kbuf, (const char __user *)local + off, c))
			return -EFAULT;
		int n = access_process_vm(task, (unsigned long)(remote + off), kbuf, (int)c, flags);
		if (n != (int)c)
			return n < 0 ? n : -EFAULT;
		off += c;
		cond_resched();
	}
	return 0;
}

int int3_do_write(struct task_struct *task, u64 remote, const void __user *local, u64 len, void *kbuf)
{
	return int3_do_write_flags(task, remote, local, len, kbuf, FOLL_WRITE);
}

int int3_do_write_force(struct task_struct *task, u64 remote, const void __user *local, u64 len, void *kbuf)
{
	return int3_do_write_flags(task, remote, local, len, kbuf, FOLL_WRITE | FOLL_FORCE);
}
