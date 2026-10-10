#include "int3_internal.h"

static int __init int3_module_init(void)
{
	int rc = int3_register_device();
	if (rc) {
		pr_err("int3: misc_register failed: %d\n", rc);
		return rc;
	}
	pr_info("int3: /dev/int3 ready (v%u)\n", INT3DRV_VERSION);
	return 0;
}

static void __exit int3_module_exit(void)
{
	int3_unregister_device();
	pr_info("int3: unloaded\n");
}

module_init(int3_module_init);
module_exit(int3_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("int3");
MODULE_DESCRIPTION("Fast batched remote-memory driver for int3");
MODULE_VERSION("7.0");
