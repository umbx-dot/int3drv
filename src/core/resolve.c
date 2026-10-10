#include "int3_internal.h"

int int3_resolve(pid_t pid, struct task_struct **t)
{
	struct pid *ps;
	struct task_struct *task;
	struct mm_struct *mm;

	if (pid <= 0)
		return -EINVAL;
	ps = find_get_pid(pid);
	if (!ps)
		return -ESRCH;
	task = get_pid_task(ps, PIDTYPE_PID);
	put_pid(ps);
	if (!task)
		return -ESRCH;
	mm = get_task_mm(task);
	if (!mm) {
		put_task_struct(task);
		return -ESRCH;
	}
	mmput(mm);
	*t = task;
	return 0;
}
