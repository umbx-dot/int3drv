#ifndef INT3_INTERNAL_H
#define INT3_INTERNAL_H

#include <linux/compiler.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "int3drv.h"

#define INT3_CHUNK (64u * 1024u)
#define INT3_STACK_BUF_SIZE 256u

#define INT3_ADDR_MIN 0x10000ull
#define INT3_ADDR_MAX 0x800000000000ull
#define INT3_ADDR_BAD 0xCCCCCCCCCCCCCCCCull

struct int3_ctx {
	struct mutex lock;
	pid_t pid;
	void *bounce_buf;
	struct int3_stats stats;
};

static_assert(sizeof(struct int3_rw) == 24, "int3_rw layout check");
static_assert(sizeof(struct int3_op) == 24, "int3_op layout check");
static_assert(sizeof(struct int3_batch) == 24, "int3_batch layout check");
static_assert(sizeof(struct int3_stats) == 32, "int3_stats layout check");
static_assert(sizeof(struct int3_probe) == 24, "int3_probe layout check");
static_assert(sizeof(struct int3_val) == 16, "int3_val layout check");
static_assert(sizeof(struct int3_pattern) == 48, "int3_pattern layout check");
static_assert(sizeof(struct int3_vma) == 32, "int3_vma layout check");
static_assert(sizeof(struct int3_vma_next) == 32, "int3_vma_next layout check");
static_assert(sizeof(struct int3_deref) == 40, "int3_deref layout check");
static_assert(sizeof(struct int3_scan_all) == 64, "int3_scan_all layout check");
static_assert(sizeof(struct int3_map) == 48, "int3_map layout check");
static_assert(sizeof(struct int3_proc) == 28, "int3_proc layout check");
static_assert(sizeof(struct int3_mod) == 24, "int3_mod layout check");
static_assert(sizeof(struct int3_chain) == 48, "int3_chain layout check");
static_assert(sizeof(struct int3_vscan) == 48, "int3_vscan layout check");
static_assert(sizeof(struct int3_cksum) == 24, "int3_cksum layout check");
static_assert(sizeof(struct int3_copy) == 24, "int3_copy layout check");
static_assert(sizeof(struct int3_alloc) == 32, "int3_alloc layout check");
static_assert(sizeof(struct int3_free) == 16, "int3_free layout check");
static_assert(sizeof(struct int3_thread) == 32, "int3_thread layout check");

int int3_register_device(void);
void int3_unregister_device(void);

int int3_core_open(struct inode *ino, struct file *f);
int int3_core_release(struct inode *ino, struct file *f);
long int3_core_ioctl(struct file *f, unsigned int cmd, unsigned long arg);
void *int3_get_bounce(struct int3_ctx *ctx);

int int3_resolve(pid_t pid, struct task_struct **t);
int int3_addr_ok(u64 addr, u64 len);

int int3_do_read(struct task_struct *task, u64 remote, void __user *local, u64 len, void *kbuf);
long int3_read_finish(struct int3_ctx *ctx, struct task_struct *t, u64 target,
		      u64 local, u32 len, u64 stat_add,
		      void __user *uarg, const void *karg, size_t ksize);
int int3_do_write(struct task_struct *task, u64 remote, const void __user *local, u64 len, void *kbuf);
int int3_do_write_force(struct task_struct *task, u64 remote, const void __user *local, u64 len, void *kbuf);
int int3_do_write_verify(struct task_struct *task, u64 remote, const void __user *local, u64 len, void *kbuf, u32 kcap);

u64 int3_vma_flags(struct vm_area_struct *vma);
int int3_scan_skip(struct mm_struct *mm, u64 cur, u64 end, u64 *next);
int int3_scan_collect(struct task_struct *t, struct mm_struct *mm, void *kbuf,
		      u64 start, u64 end, u32 itemlen, u32 stride,
		      u64 *matches, u32 cap, u32 *count,
		      int (*hit)(const u8 *p, u64 o, void *arg), void *arg);

long int3_ioctl_rw(struct int3_ctx *ctx, unsigned long arg, int is_read, int is_force);
long int3_ioctl_write_verify(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_batch(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_probe(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_scalar(struct int3_ctx *ctx, unsigned long arg, unsigned int cmd);
long int3_ioctl_pattern(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_scan_all(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_vma(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_vma_next(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_maps_next(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_proc_next(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_modbase(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_chain(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_vscan(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_cksum(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_copy(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_alloc(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_free(struct int3_ctx *ctx, unsigned long arg);
long int3_ioctl_thread(struct int3_ctx *ctx, unsigned long arg);
long int3_remote_mmap(struct task_struct *t, unsigned long hint, size_t len,
		      unsigned long prot, unsigned long *addr);
long int3_remote_munmap(struct task_struct *t, unsigned long addr, size_t len);
long int3_ioctl_deref(struct int3_ctx *ctx, unsigned long arg);

#endif
