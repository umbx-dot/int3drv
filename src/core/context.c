#include "int3_internal.h"

int int3_core_open(struct inode *ino, struct file *f)
{
	struct int3_ctx *ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);

	(void)ino;
	if (!ctx)
		return -ENOMEM;
	mutex_init(&ctx->lock);
	ctx->pid = -1;
	f->private_data = ctx;
	return 0;
}

int int3_core_release(struct inode *ino, struct file *f)
{
	struct int3_ctx *ctx = f->private_data;

	(void)ino;
	if (ctx) {
		kfree(ctx->bounce_buf);
		kfree(ctx);
		f->private_data = NULL;
	}
	return 0;
}

void *int3_get_bounce(struct int3_ctx *ctx)
{
	if (!ctx->bounce_buf)
		ctx->bounce_buf = kmalloc(INT3_CHUNK, GFP_KERNEL);
	return ctx->bounce_buf;
}
