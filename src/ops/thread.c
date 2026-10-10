#include "int3_internal.h"

extern u8 int3_stub[];
extern u8 int3_stub_data[];
extern u8 int3_stub_end[];

#define STUB_FN 0
#define STUB_ARG 1
#define STUB_DONE 2
#define STUB_SEC 3
#define STUB_NSEC 4
#define STUB_RET 5
#define STUB_QWORDS 6

#define STUB_STACK_SIZE (16u * 1024u)
#define STUB_TOTAL_SIZE (PAGE_SIZE + STUB_STACK_SIZE)

static struct task_struct *int3_pick_thread(struct task_struct *leader)
{
	struct task_struct *best = NULL;
	struct task_struct *t;

	rcu_read_lock();
	for_each_thread(leader, t) {
		if (!t->mm || (t->flags & PF_EXITING))
			continue;
		if (READ_ONCE(t->__state) & TASK_UNINTERRUPTIBLE)
			continue;
		if (!(READ_ONCE(t->__state) & TASK_INTERRUPTIBLE))
			continue;
		get_task_struct(t);
		best = t;
		break;
	}
	rcu_read_unlock();
	if (best)
		return best;

	rcu_read_lock();
	for_each_thread(leader, t) {
		if (!t->mm || (t->flags & PF_EXITING) || t == leader)
			continue;
		if (READ_ONCE(t->__state) & TASK_UNINTERRUPTIBLE)
			continue;
		get_task_struct(t);
		best = t;
		break;
	}
	rcu_read_unlock();
	return best;
}

static int int3_wait_stop(struct task_struct *t, u32 timeout_ms)
{
	u32 waited = 0;

	while (!task_is_stopped_or_traced(t)) {
		if (signal_pending(current))
			return -ERESTARTSYS;
		if (waited >= timeout_ms)
			return -ETIMEDOUT;
		schedule_timeout_interruptible(1);
		waited += 4;
	}
	return 0;
}

static int int3_wait_done(struct task_struct *t, u64 done_addr, u32 timeout_ms)
{
	u32 waited = 0;
	u8 done = 0;

	(void)t;
	while (!done) {
		int n;

		if (signal_pending(current))
			return -ERESTARTSYS;
		if (waited >= timeout_ms)
			return -ETIMEDOUT;
		schedule_timeout_interruptible(1);
		waited += 4;
		n = access_process_vm(t, (unsigned long)done_addr, &done, 1, 0);
		if (n != 1)
			return n < 0 ? n : -EFAULT;
	}
	return 0;
}

long int3_ioctl_thread(struct int3_ctx *ctx, unsigned long arg)
{
	struct int3_thread th;
	struct task_struct *leader = NULL;
	struct task_struct *t = NULL;
	struct mm_struct *mm = NULL;
	struct pt_regs *regs;
	unsigned long blob = 0;
	unsigned long stack = 0;
	size_t blob_len;
	u64 slots[STUB_QWORDS];
	u64 data_addr;
	u64 stack_top;
	u64 done_addr;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&th, (void __user *)arg, sizeof(th)))
		return -EFAULT;
	if (!th.fn || !th.timeout_ms || th.timeout_ms > 60000)
		return -EINVAL;
	if (!int3_addr_ok(th.fn, 1))
		return -EINVAL;

	blob_len = (size_t)(int3_stub_end - int3_stub);
	if (!blob_len || blob_len > PAGE_SIZE)
		return -EIO;

	memset(slots, 0, sizeof(slots));

	rc = int3_resolve(ctx->pid, &leader);
	if (rc)
		return rc;

	t = int3_pick_thread(leader);
	put_task_struct(leader);
	leader = NULL;
	if (!t)
		return -ESRCH;

	mm = get_task_mm(t);
	if (!mm) {
		put_task_struct(t);
		return -ESRCH;
	}

	mmap_read_lock(mm);
	regs = task_pt_regs(t);
	{
		struct vm_area_struct *fv = vma_lookup(mm, (unsigned long)th.fn);
		if (!(fv && (fv->vm_flags & VM_EXEC))) {
			mmap_read_unlock(mm);
			mmput(mm);
			put_task_struct(t);
			return -EINVAL;
		}
	}
	mmap_read_unlock(mm);

	{
		struct kernel_siginfo info;
		clear_siginfo(&info);
		info.si_signo = SIGSTOP;
		info.si_code = SI_KERNEL;
		rc = send_sig_info(SIGSTOP, &info, t);
		if (rc < 0) {
			mmput(mm);
			put_task_struct(t);
			return rc;
		}
	}

	rc = int3_wait_stop(t, th.timeout_ms);
	if (rc) {
		struct kernel_siginfo info;
		clear_siginfo(&info);
		info.si_signo = SIGCONT;
		info.si_code = SI_KERNEL;
		send_sig_info(SIGCONT, &info, t);
		mmput(mm);
		put_task_struct(t);
		return rc;
	}

	rc = (int)int3_remote_mmap(t, 0, STUB_TOTAL_SIZE,
				  PROT_READ | PROT_WRITE | PROT_EXEC, &blob);
	if (rc)
		goto cont;
	stack = blob + PAGE_SIZE;

	slots[STUB_FN] = th.fn;
	slots[STUB_ARG] = th.arg;
	slots[STUB_SEC] = 10000;
	slots[STUB_NSEC] = 0;

	{
		size_t code_len = (size_t)(int3_stub_data - int3_stub);
		size_t data_len = (size_t)(int3_stub_end - int3_stub_data);
		size_t stage_len = code_len + sizeof(slots);
		u8 *stage;
		int n;

		if (data_len != sizeof(slots) || stage_len > PAGE_SIZE) {
			int3_remote_munmap(t, blob, STUB_TOTAL_SIZE);
			rc = -EIO;
			goto cont;
		}
		stage = kmalloc(stage_len, GFP_KERNEL);
		if (!stage) {
			int3_remote_munmap(t, blob, STUB_TOTAL_SIZE);
			rc = -ENOMEM;
			goto cont;
		}
		memcpy(stage, int3_stub, code_len);
		memcpy(stage + code_len, slots, sizeof(slots));
		data_addr = blob + code_len;
		n = access_process_vm(t, (unsigned long)blob, stage, (int)stage_len, FOLL_WRITE);
		kfree(stage);
		if (n != (int)stage_len) {
			int3_remote_munmap(t, blob, STUB_TOTAL_SIZE);
			rc = n < 0 ? n : -EFAULT;
			goto cont;
		}

		stack_top = stack + STUB_STACK_SIZE;
		done_addr = data_addr + STUB_DONE * 8;
		regs->ip = blob + 2;
		regs->sp = stack_top;
		th.tid = task_pid_nr(t);
	}
	{
		struct kernel_siginfo info;

		clear_siginfo(&info);
		info.si_signo = SIGCONT;
		info.si_code = SI_KERNEL;
		send_sig_info(SIGCONT, &info, t);
	}

	rc = int3_wait_done(t, done_addr, th.timeout_ms);
	if (rc) {
		if (copy_to_user((void __user *)arg, &th, sizeof(th)))
			rc = -EFAULT;
		ctx->stats.ops_failed++;
		goto out_mm;
	}

	{
		u64 ret = 0;
		int n = access_process_vm(t, (unsigned long)(done_addr +
					  (STUB_RET - STUB_DONE) * 8),
					  &ret, (int)sizeof(ret), 0);
		if (n != (int)sizeof(ret))
			rc = -EFAULT;
		else
			th.retval = ret;
		if (rc) {
			if (copy_to_user((void __user *)arg, &th, sizeof(th)))
				rc = -EFAULT;
			ctx->stats.ops_failed++;
			goto out_mm;
		}
	}

	if (copy_to_user((void __user *)arg, &th, sizeof(th)))
		rc = -EFAULT;
	else
		rc = 0;

	if (rc == 0) {
		ctx->stats.ops_completed++;
	} else {
		ctx->stats.ops_failed++;
	}

out_mm:
	mmput(mm);
	put_task_struct(t);
	return rc;

cont:
	{
		struct kernel_siginfo info;

		clear_siginfo(&info);
		info.si_signo = SIGCONT;
		info.si_code = SI_KERNEL;
		send_sig_info(SIGCONT, &info, t);
	}
	ctx->stats.ops_failed++;
	mmput(mm);
	put_task_struct(t);
	return rc;
}
