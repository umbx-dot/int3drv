#include "int3_internal.h"

long int3_ioctl_scalar(struct int3_ctx *ctx, unsigned long arg, unsigned int cmd)
{
	struct int3_val val;
	struct task_struct *t = NULL;
	u32 len;
	int is_write;
	int rc;

	if (ctx->pid <= 0)
		return -ENXIO;
	if (copy_from_user(&val, (void __user *)arg, sizeof(val)))
		return -EFAULT;

	len = (cmd == INT3_IOC_READ_U64 || cmd == INT3_IOC_WRITE_U64) ? 8u : 4u;
	is_write = (cmd == INT3_IOC_WRITE_U64 || cmd == INT3_IOC_WRITE_U32);

	if (!int3_addr_ok(val.remote, len))
		return -EINVAL;

	rc = int3_resolve(ctx->pid, &t);
	if (rc)
		return rc;

	if (is_write) {
		int n = access_process_vm(t, (unsigned long)val.remote, &val.val, (int)len, FOLL_WRITE);
		if (n != (int)len)
			rc = n < 0 ? n : -EFAULT;
		else
			rc = 0;
	} else {
		val.val = 0;
		int n = access_process_vm(t, (unsigned long)val.remote, &val.val, (int)len, 0);
		if (n != (int)len)
			rc = n < 0 ? n : -EFAULT;
		else if (copy_to_user((void __user *)arg, &val, sizeof(val)))
			rc = -EFAULT;
		else
			rc = 0;
	}

	put_task_struct(t);

	if (rc == 0) {
		if (is_write)
			ctx->stats.bytes_written += len;
		else
			ctx->stats.bytes_read += len;
		ctx->stats.ops_completed++;
	} else {
		ctx->stats.ops_failed++;
	}

	return rc;
}
