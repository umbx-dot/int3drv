#ifndef INT3DRV_H
#define INT3DRV_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#include <linux/types.h>
#include <sys/ioctl.h>
#endif

#define INT3DRV_VERSION 7u
#define INT3DRV_DEV "/dev/int3"

#define INT3_IOC_MAGIC 'I'

#define INT3_IOC_ATTACH       _IOW(INT3_IOC_MAGIC, 0x01, __s32)
#define INT3_IOC_DETACH       _IO(INT3_IOC_MAGIC, 0x02)
#define INT3_IOC_READ         _IOWR(INT3_IOC_MAGIC, 0x10, struct int3_rw)
#define INT3_IOC_WRITE        _IOW(INT3_IOC_MAGIC, 0x11, struct int3_rw)
#define INT3_IOC_FORCE_WRITE  _IOW(INT3_IOC_MAGIC, 0x12, struct int3_rw)
#define INT3_IOC_WRITE_VERIFY _IOW(INT3_IOC_MAGIC, 0x13, struct int3_rw)
#define INT3_IOC_BATCH        _IOWR(INT3_IOC_MAGIC, 0x20, struct int3_batch)
#define INT3_IOC_VERSION      _IOR(INT3_IOC_MAGIC, 0x30, __u32)
#define INT3_IOC_PID          _IOR(INT3_IOC_MAGIC, 0x31, __s32)
#define INT3_IOC_STATS        _IOR(INT3_IOC_MAGIC, 0x32, struct int3_stats)
#define INT3_IOC_PROBE        _IOWR(INT3_IOC_MAGIC, 0x33, struct int3_probe)
#define INT3_IOC_READ_U64     _IOWR(INT3_IOC_MAGIC, 0x34, struct int3_val)
#define INT3_IOC_WRITE_U64    _IOW(INT3_IOC_MAGIC, 0x35, struct int3_val)
#define INT3_IOC_READ_U32     _IOWR(INT3_IOC_MAGIC, 0x36, struct int3_val)
#define INT3_IOC_WRITE_U32    _IOW(INT3_IOC_MAGIC, 0x37, struct int3_val)
#define INT3_IOC_PATTERN_SCAN _IOWR(INT3_IOC_MAGIC, 0x38, struct int3_pattern)
#define INT3_IOC_VMA          _IOWR(INT3_IOC_MAGIC, 0x39, struct int3_vma)
#define INT3_IOC_VMA_NEXT     _IOWR(INT3_IOC_MAGIC, 0x3A, struct int3_vma_next)
#define INT3_IOC_DEREF        _IOWR(INT3_IOC_MAGIC, 0x3B, struct int3_deref)
#define INT3_IOC_SCAN_ALL     _IOWR(INT3_IOC_MAGIC, 0x3C, struct int3_scan_all)
#define INT3_IOC_MAPS_NEXT    _IOWR(INT3_IOC_MAGIC, 0x3D, struct int3_map)
#define INT3_IOC_PROC_NEXT    _IOWR(INT3_IOC_MAGIC, 0x3E, struct int3_proc)
#define INT3_IOC_MOD_BASE     _IOWR(INT3_IOC_MAGIC, 0x3F, struct int3_mod)
#define INT3_IOC_CHAIN        _IOWR(INT3_IOC_MAGIC, 0x40, struct int3_chain)
#define INT3_IOC_VSCAN        _IOWR(INT3_IOC_MAGIC, 0x41, struct int3_vscan)
#define INT3_IOC_CHECKSUM     _IOWR(INT3_IOC_MAGIC, 0x42, struct int3_cksum)
#define INT3_IOC_COPY         _IOWR(INT3_IOC_MAGIC, 0x43, struct int3_copy)
#define INT3_IOC_CAPS         _IOR(INT3_IOC_MAGIC, 0x44, __u64)
#define INT3_IOC_ALLOC        _IOWR(INT3_IOC_MAGIC, 0x45, struct int3_alloc)
#define INT3_IOC_FREE         _IOW(INT3_IOC_MAGIC, 0x46, struct int3_free)
#define INT3_IOC_THREAD       _IOWR(INT3_IOC_MAGIC, 0x47, struct int3_thread)

#define INT3_MAX_SINGLE (32u * 1024u * 1024u)
#define INT3_MAX_OPS    256u
#define INT3_MAX_TOTAL  (8u * 1024u * 1024u)
#define INT3_MAX_PATTERN 256u
#define INT3_MAX_MATCHES 8192u
#define INT3_MAX_NAME 256u
#define INT3_MAX_CHAIN 8u

#define INT3_VSCAN_EQ 0u
#define INT3_VSCAN_NE 1u
#define INT3_VSCAN_GT 2u
#define INT3_VSCAN_LT 3u

#define INT3_CAP_MODBASE  (1ull << 0)
#define INT3_CAP_CHAIN    (1ull << 1)
#define INT3_CAP_VSCAN    (1ull << 2)
#define INT3_CAP_CHECKSUM (1ull << 3)
#define INT3_CAP_COPY     (1ull << 4)
#define INT3_CAP_ALLOC    (1ull << 5)
#define INT3_CAP_THREAD   (1ull << 6)
#define INT3_CAPS_ALL     (INT3_CAP_MODBASE | INT3_CAP_CHAIN | INT3_CAP_VSCAN | \
			   INT3_CAP_CHECKSUM | INT3_CAP_COPY | INT3_CAP_ALLOC | \
			   INT3_CAP_THREAD)

#define INT3_BATCH_READ  0u
#define INT3_BATCH_WRITE 1u
#define INT3_BATCH_MIXED 2u

#define INT3_OP_READ  0u
#define INT3_OP_WRITE 1u
#define INT3_OP_FORCE 2u
#define INT3_OP_WRITE_VERIFY 3u

#define INT3_VM_READ    0x00000001ull
#define INT3_VM_WRITE   0x00000002ull
#define INT3_VM_EXEC    0x00000004ull
#define INT3_VM_SHARED  0x00000008ull

struct int3_rw {
	__u64 remote;
	__u64 local;
	__u64 len;
};

struct int3_op {
	__u64 remote;
	__u64 local;
	__u32 len;
	__u32 flags;
};

struct int3_batch {
	__u64 ops;
	__u32 count;
	__u32 flags;
	__s32 fail_idx;
	__u32 _rsv;
};

struct int3_stats {
	__u64 bytes_read;
	__u64 bytes_written;
	__u64 ops_completed;
	__u64 ops_failed;
};

struct int3_probe {
	__u64 remote;
	__u64 len;
	__u32 is_readable;
	__u32 is_writable;
};

struct int3_val {
	__u64 remote;
	__u64 val;
};

struct int3_pattern {
	__u64 remote_start;
	__u64 remote_end;
	__u64 match_addr;
	__u64 pattern_ptr;
	__u64 mask_ptr;
	__u32 pattern_len;
	__u32 _rsv;
};

struct int3_vma {
	__u64 addr;
	__u64 vm_start;
	__u64 vm_end;
	__u64 vm_flags;
};

struct int3_vma_next {
	__u64 cursor;
	__u64 vm_start;
	__u64 vm_end;
	__u64 vm_flags;
};

struct int3_deref {
	__u64 ptr_addr;
	__u64 disp;
	__u64 local;
	__u64 out_ptr;
	__u32 len;
	__u32 _rsv;
};

struct int3_scan_all {
	__u64 remote_start;
	__u64 remote_end;
	__u64 match_ptr;
	__u64 pattern_ptr;
	__u64 mask_ptr;
	__u32 match_cap;
	__u32 match_count;
	__u32 pattern_len;
	__u32 align;
	__u32 _rsv;
};

struct int3_map {
	__u64 cursor;
	__u64 vm_start;
	__u64 vm_end;
	__u64 vm_flags;
	__u64 path_ptr;
	__u32 path_cap;
	__u32 path_len;
};

struct int3_proc {
	__u32 cursor;
	__u32 pid;
	__u8 comm[16];
	__u32 _rsv;
};

struct int3_mod {
	__u64 name_ptr;
	__u64 base;
	__u32 name_len;
	__u32 _rsv;
};

struct int3_chain {
	__u64 ptr_addr;
	__u64 local;
	__u64 out_ptr;
	__u64 offsets;
	__u32 len;
	__u32 depth;
	__u64 _rsv;
};

struct int3_vscan {
	__u64 remote_start;
	__u64 remote_end;
	__u64 match_ptr;
	__u64 value;
	__u32 match_cap;
	__u32 match_count;
	__u32 width;
	__u32 op;
};

struct int3_cksum {
	__u64 remote_start;
	__u64 remote_end;
	__u64 hash;
};

struct int3_copy {
	__u64 src;
	__u64 dst;
	__u64 len;
};

struct int3_alloc {
	__u64 hint;
	__u64 addr;
	__u64 len;
	__u32 prot;
	__u32 _rsv;
};

struct int3_free {
	__u64 addr;
	__u64 len;
};

struct int3_thread {
	__u64 fn;
	__u64 arg;
	__u32 timeout_ms;
	__s32 tid;
	__u64 retval;
};

#endif
