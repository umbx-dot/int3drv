#include "int3ctl.h"

[[nodiscard]] static bool parse_pid(const char *str, int *out_pid)
{
	char *endptr = nullptr;
	errno = 0;
	long val = strtol(str, &endptr, 10);
	if (errno != 0 || endptr == str || *endptr != '\0' || val <= 0 || val > 4194304)
		return false;
	*out_pid = (int)val;
	return true;
}

[[nodiscard]] static bool parse_pid_msg(const char *str, int *out_pid)
{
	if (parse_pid(str, out_pid))
		return true;
	fprintf(stderr, "invalid pid: %s\n", str);
	return false;
}

static void print_usage(const char *prog)
{
	fprintf(stderr, "Usage: %s <command> [args...]\n", prog);
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "  version                               Query driver version\n");
	fprintf(stderr, "  stats                                 Query per-session transfer statistics\n");
	fprintf(stderr, "  probe <pid> <addr_hex> <len>          Probe address accessibility\n");
	fprintf(stderr, "  read <pid> <addr_hex> <len>           Hexdump memory from remote pid\n");
	fprintf(stderr, "  write <pid> <addr_hex> <hex_string>   Write raw hex bytes to remote pid\n");
	fprintf(stderr, "  write_verify <pid> <addr_hex> <hex>  Write bytes with in-kernel readback check\n");
	fprintf(stderr, "  force_write <pid> <addr_hex> <hex>    Force write to read-only/code pages\n");
	fprintf(stderr, "  read64 <pid> <addr_hex>               Read 64-bit integer\n");
	fprintf(stderr, "  write64 <pid> <addr_hex> <val_hex>    Write 64-bit integer\n");
	fprintf(stderr, "  scan <pid> <start_hex> <end_hex> <pattern_hex> [mask_hex] Fast in-kernel pattern scan\n");
	fprintf(stderr, "  scan_all <pid> <start_hex> <end_hex> <pattern_hex> [mask_hex] [align] Multi-match scan\n");
	fprintf(stderr, "  vma <pid> <addr_hex>                  Query kernel VMA region and memory flags\n");
	fprintf(stderr, "  modbase <pid> <name>                 Find base address of loaded module by name\n");
	fprintf(stderr, "  maps <pid>                            List all kernel VMA regions of pid\n");
	fprintf(stderr, "  procs                                 List all pids with command names\n");
	fprintf(stderr, "  caps                                  Show driver capability bits\n");
	fprintf(stderr, "  deref <pid> <ptr_hex> <disp_hex> <len> Follow remote pointer plus displacement\n");
	fprintf(stderr, "  chain <pid> <ptr_hex> <len> <off..>   Follow multi-level pointer chain\n");
	fprintf(stderr, "  cksum <pid> <start_hex> <end_hex>     Hash readable bytes in range (FNV-1a)\n");
	fprintf(stderr, "  copy <pid> <src_hex> <dst_hex> <len>  Remote-to-remote copy without round-trip\n");
	fprintf(stderr, "  alloc <pid> <len> [prot]              Allocate memory in remote process (prot=R|W|X letters)\n");
	fprintf(stderr, "  free <pid> <addr_hex> <len>           Release remote mapping\n");
	fprintf(stderr, "  mprotect <pid> <addr_hex> <len> [RWX] Change remote mapping protections\n");
	fprintf(stderr, "  manualmap <pid> <so_path>             Manual-map a .so into remote process\n");
	fprintf(stderr, "  load <pid> <so_path> [ms]            Manual-map a .so and run its init\n");
	fprintf(stderr, "  load_libc <pid> <so_path> [ms]       Load a .so via the target's own dlopen\n");
	fprintf(stderr, "  thread <pid> <fn_hex> <arg_hex> [ms]  Run fn(arg) on a hijacked thread\n");
	fprintf(stderr, "  vscan <pid> <s_hex> <e_hex> <w> <op> <val> Scan for integer value (w=1,2,4,8; op=0 eq,1 ne,2 gt,3 lt)\n");
	fprintf(stderr, "  bench <pid> <addr_hex> [iterations]   Benchmark single, scalar, and batch reads\n");
	fprintf(stderr, "  selftest                              Run unit and integration self-tests\n");
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		print_usage(argv[0]);
		return 1;
	}
	const char *cmd = argv[1];
	if (strcmp(cmd, "version") == 0)
		return cmd_version();
	if (strcmp(cmd, "stats") == 0)
		return cmd_stats();
	if (strcmp(cmd, "selftest") == 0)
		return cmd_selftest();

	if (strcmp(cmd, "probe") == 0 && argc == 5) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		uint64_t len = strtoull(argv[4], nullptr, 10);
		return cmd_probe(pid, addr, len);
	}
	if (strcmp(cmd, "read") == 0 && argc == 5) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		uint64_t len = strtoull(argv[4], nullptr, 10);
		return cmd_read(pid, addr, len);
	}
	if (strcmp(cmd, "write") == 0 && argc == 5) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		return cmd_write(pid, addr, argv[4]);
	}
	if (strcmp(cmd, "write_verify") == 0 && argc == 5) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		return cmd_write_verify(pid, addr, argv[4]);
	}
	if (strcmp(cmd, "force_write") == 0 && argc == 5) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		return cmd_force_write(pid, addr, argv[4]);
	}
	if (strcmp(cmd, "read64") == 0 && argc == 4) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		return cmd_read64(pid, addr);
	}
	if (strcmp(cmd, "write64") == 0 && argc == 5) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		uint64_t val = strtoull(argv[4], nullptr, 16);
		return cmd_write64(pid, addr, val);
	}
	if (strcmp(cmd, "scan") == 0 && (argc == 6 || argc == 7)) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t start = strtoull(argv[3], nullptr, 16);
		uint64_t end = strtoull(argv[4], nullptr, 16);
		const char *mask = (argc == 7) ? argv[6] : nullptr;
		return cmd_scan(pid, start, end, argv[5], mask);
	}
	if (strcmp(cmd, "scan_all") == 0 && (argc >= 6 && argc <= 8)) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t start = strtoull(argv[3], nullptr, 16);
		uint64_t end = strtoull(argv[4], nullptr, 16);
		const char *mask = (argc >= 7) ? argv[6] : nullptr;
		uint32_t align = (argc == 8) ? (uint32_t)strtoul(argv[7], nullptr, 10) : 1;
		return cmd_scan_all(pid, start, end, argv[5], mask, align);
	}
	if (strcmp(cmd, "maps") == 0 && argc == 3) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		return cmd_maps(pid);
	}
	if (strcmp(cmd, "procs") == 0 && argc == 2) {
		return cmd_procs();
	}
	if (strcmp(cmd, "caps") == 0 && argc == 2) {
		return cmd_caps();
	}
	if (strcmp(cmd, "modbase") == 0 && argc == 4) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		return cmd_modbase(pid, argv[3]);
	}
	if (strcmp(cmd, "deref") == 0 && argc == 6) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t ptr_addr = strtoull(argv[3], nullptr, 16);
		uint64_t disp = strtoull(argv[4], nullptr, 16);
		uint64_t len = strtoull(argv[5], nullptr, 10);
		return cmd_deref(pid, ptr_addr, disp, len);
	}
	if (strcmp(cmd, "chain") == 0 && argc >= 5) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t ptr_addr = strtoull(argv[3], nullptr, 16);
		uint64_t len = strtoull(argv[4], nullptr, 10);
		uint32_t depth = (uint32_t)(argc - 5);
		if (depth > INT3_MAX_CHAIN) {
			fprintf(stderr, "too many offsets (max %u)\n", INT3_MAX_CHAIN);
			return 1;
		}
		uint64_t offs[INT3_MAX_CHAIN] = {0};
		for (int i = 0; i + 5 < argc; i++)
			offs[i] = strtoull(argv[i + 5], nullptr, 16);
		return cmd_chain(pid, ptr_addr, len, depth, offs);
	}
	if (strcmp(cmd, "cksum") == 0 && argc == 5) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t start = strtoull(argv[3], nullptr, 16);
		uint64_t end = strtoull(argv[4], nullptr, 16);
		return cmd_cksum(pid, start, end);
	}
	if (strcmp(cmd, "copy") == 0 && argc == 6) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t src = strtoull(argv[3], nullptr, 16);
		uint64_t dst = strtoull(argv[4], nullptr, 16);
		uint64_t len = strtoull(argv[5], nullptr, 10);
		return cmd_copy(pid, src, dst, len);
	}
	if (strcmp(cmd, "vscan") == 0 && argc == 8) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t start = strtoull(argv[3], nullptr, 16);
		uint64_t end = strtoull(argv[4], nullptr, 16);
		uint32_t width = (uint32_t)strtoul(argv[5], nullptr, 10);
		uint32_t op = (uint32_t)strtoul(argv[6], nullptr, 10);
		uint64_t val = strtoull(argv[7], nullptr, 0);
		return cmd_vscan(pid, start, end, width, op, val);
	}
	if (strcmp(cmd, "alloc") == 0 && (argc == 4 || argc == 5)) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t len = strtoull(argv[3], nullptr, 10);
		uint32_t prot = PROT_READ | PROT_WRITE;
		if (argc == 5 && !parse_prot(argv[4], &prot)) {
			fprintf(stderr, "prot letters: R, W, X\n");
			return 1;
		}
		return cmd_alloc(pid, len, prot);
	}
	if (strcmp(cmd, "free") == 0 && argc == 5) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		uint64_t len = strtoull(argv[4], nullptr, 10);
		return cmd_free(pid, addr, len);
	}
	if (strcmp(cmd, "mprotect") == 0 && (argc == 5 || argc == 6)) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		uint64_t len = strtoull(argv[4], nullptr, 10);
		uint32_t prot = 0;
		if (argc == 6 && !parse_prot(argv[5], &prot)) {
			fprintf(stderr, "prot letters: R, W, X\n");
			return 1;
		}
		return cmd_mprotect(pid, addr, len, prot);
	}
	if (strcmp(cmd, "manualmap") == 0 && argc == 4) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		return cmd_manualmap(pid, argv[3]);
	}
	if (strcmp(cmd, "load") == 0 && (argc == 4 || argc == 5)) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint32_t ms = (argc == 5) ? (uint32_t)strtoul(argv[4], nullptr, 10) : 5000;
		return cmd_load(pid, argv[3], ms);
	}
	if (strcmp(cmd, "load_libc") == 0 && (argc == 4 || argc == 5)) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint32_t ms = (argc == 5) ? (uint32_t)strtoul(argv[4], nullptr, 10) : 5000;
		return cmd_load_libc(pid, argv[3], ms);
	}
	if (strcmp(cmd, "thread") == 0 && (argc == 5 || argc == 6)) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t fn = strtoull(argv[3], nullptr, 16);
		uint64_t arg = strtoull(argv[4], nullptr, 16);
		uint32_t ms = (argc == 6) ? (uint32_t)strtoul(argv[5], nullptr, 10) : 5000;
		return cmd_thread(pid, fn, arg, ms);
	}
	if (strcmp(cmd, "vma") == 0 && argc == 4) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		return cmd_vma(pid, addr);
	}
	if (strcmp(cmd, "bench") == 0 && (argc == 4 || argc == 5)) {
		int pid = 0;
		if (!parse_pid_msg(argv[2], &pid))
			return 1;
		uint64_t addr = strtoull(argv[3], nullptr, 16);
		uint32_t iters = (argc == 5) ? (uint32_t)strtoul(argv[4], nullptr, 10) : 100000;
		return cmd_bench(pid, addr, iters);
	}

	print_usage(argv[0]);
	return 1;
}
