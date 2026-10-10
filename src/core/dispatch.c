#include "int3_internal.h"

static long int3_ioctl_attach(struct int3_ctx *ctx, unsigned long arg)
{
	__s32 pid;
	struct task_struct *t = NULL;
	int rc;

	if (copy_from_user(&pid, (void __user *)arg, sizeof(pid)))
		return -EFAULT;
	if (pid <= 0)
		return -EINVAL;
	rc = int3_resolve((pid_t)pid, &t);
	if (rc)
		return rc;
	put_task_struct(t);
	ctx->pid = (pid_t)pid;
	return 0;
}

long int3_core_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct int3_ctx *ctx = f->private_data;
	long rc;

	if (_IOC_TYPE(cmd) != INT3_IOC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case INT3_IOC_VERSION: {
		__u32 v = INT3DRV_VERSION;
		if (copy_to_user((void __user *)arg, &v, sizeof(v)))
			return -EFAULT;
		return 0;
	}
	case INT3_IOC_CAPS: {
		__u64 caps = INT3_CAPS_ALL;
		if (copy_to_user((void __user *)arg, &caps, sizeof(caps)))
			return -EFAULT;
		return 0;
	}
	case INT3_IOC_PID: {
		__s32 p;
		if (!ctx)
			return -ENOTTY;
		if (mutex_lock_interruptible(&ctx->lock))
			return -ERESTARTSYS;
		p = (__s32)ctx->pid;
		mutex_unlock(&ctx->lock);
		if (copy_to_user((void __user *)arg, &p, sizeof(p)))
			return -EFAULT;
		return 0;
	}
	default:
		break;
	}

	if (!ctx)
		return -ENOTTY;

	if (mutex_lock_interruptible(&ctx->lock))
		return -ERESTARTSYS;

	switch (cmd) {
	case INT3_IOC_ATTACH:
		rc = int3_ioctl_attach(ctx, arg);
		break;
	case INT3_IOC_DETACH:
		ctx->pid = -1;
		rc = 0;
		break;
	case INT3_IOC_READ:
		rc = int3_ioctl_rw(ctx, arg, 1, 0);
		break;
	case INT3_IOC_WRITE:
		rc = int3_ioctl_rw(ctx, arg, 0, 0);
		break;
	case INT3_IOC_FORCE_WRITE:
		rc = int3_ioctl_rw(ctx, arg, 0, 1);
		break;
	case INT3_IOC_WRITE_VERIFY:
		rc = int3_ioctl_write_verify(ctx, arg);
		break;
	case INT3_IOC_BATCH:
		rc = int3_ioctl_batch(ctx, arg);
		break;
	case INT3_IOC_STATS:
		if (copy_to_user((void __user *)arg, &ctx->stats, sizeof(ctx->stats)))
			rc = -EFAULT;
		else
			rc = 0;
		break;
	case INT3_IOC_PROBE:
		rc = int3_ioctl_probe(ctx, arg);
		break;
	case INT3_IOC_READ_U64:
	case INT3_IOC_WRITE_U64:
	case INT3_IOC_READ_U32:
	case INT3_IOC_WRITE_U32:
		rc = int3_ioctl_scalar(ctx, arg, cmd);
		break;
	case INT3_IOC_PATTERN_SCAN:
		rc = int3_ioctl_pattern(ctx, arg);
		break;
	case INT3_IOC_SCAN_ALL:
		rc = int3_ioctl_scan_all(ctx, arg);
		break;
	case INT3_IOC_VMA:
		rc = int3_ioctl_vma(ctx, arg);
		break;
	case INT3_IOC_VMA_NEXT:
		rc = int3_ioctl_vma_next(ctx, arg);
		break;
	case INT3_IOC_MAPS_NEXT:
		rc = int3_ioctl_maps_next(ctx, arg);
		break;
	case INT3_IOC_PROC_NEXT:
		rc = int3_ioctl_proc_next(ctx, arg);
		break;
	case INT3_IOC_MOD_BASE:
		rc = int3_ioctl_modbase(ctx, arg);
		break;
	case INT3_IOC_CHAIN:
		rc = int3_ioctl_chain(ctx, arg);
		break;
	case INT3_IOC_VSCAN:
		rc = int3_ioctl_vscan(ctx, arg);
		break;
	case INT3_IOC_CHECKSUM:
		rc = int3_ioctl_cksum(ctx, arg);
		break;
	case INT3_IOC_COPY:
		rc = int3_ioctl_copy(ctx, arg);
		break;
	case INT3_IOC_ALLOC:
		rc = int3_ioctl_alloc(ctx, arg);
		break;
	case INT3_IOC_FREE:
		rc = int3_ioctl_free(ctx, arg);
		break;
	case INT3_IOC_THREAD:
		rc = int3_ioctl_thread(ctx, arg);
		break;
	case INT3_IOC_DEREF:
		rc = int3_ioctl_deref(ctx, arg);
		break;
	default:
		rc = -ENOTTY;
		break;
	}
	mutex_unlock(&ctx->lock);
	return rc;
}
