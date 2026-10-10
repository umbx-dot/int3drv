#include "int3_internal.h"

static const struct file_operations int3_fops = {
	.owner = THIS_MODULE,
	.open = int3_core_open,
	.release = int3_core_release,
	.unlocked_ioctl = int3_core_ioctl,
	.compat_ioctl = int3_core_ioctl,
	.llseek = noop_llseek,
};

static struct miscdevice int3_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "int3",
	.fops = &int3_fops,
	.mode = 0666,
};

int int3_register_device(void)
{
	return misc_register(&int3_misc);
}

void int3_unregister_device(void)
{
	misc_deregister(&int3_misc);
}
