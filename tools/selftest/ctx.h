#ifndef ST_CTX_H
#define ST_CTX_H

#include "int3ctl.h"

#define ST_NEEDLE "test_payload"

struct st_ctx {
	int fd;
	int sv[2];
	pid_t pid;
	uint64_t addrs[6];
	uint32_t ver;
	struct int3_val val;
	uint64_t hole_at;
	uint64_t needle_vs;
	uint64_t match_addr;
	uint64_t track_val;
	struct int3_op stress_ops[256];
	struct int3_batch stress_batch;
	uint64_t stress_val;
};

int st_fail(struct st_ctx *c);
int st_run(void);
unsigned long st_thread_fn(void *arg);

int st_scalar(struct st_ctx *c);
int st_batch(struct st_ctx *c);
int st_writes(struct st_ctx *c);
int st_validate(struct st_ctx *c);
int st_query(struct st_ctx *c);
int st_scan(struct st_ctx *c);
int st_deref(struct st_ctx *c);
int st_verify(struct st_ctx *c);
int st_scanall(struct st_ctx *c);
int st_maps_procs(struct st_ctx *c);
int st_v5(struct st_ctx *c);
int st_mmap(struct st_ctx *c);
int st_thread(struct st_ctx *c);
int st_lifecycle(struct st_ctx *c);

#endif
