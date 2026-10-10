#include "int3ctl.h"

int cmd_write_verify(int pid, uint64_t addr, const char *hex_str)
{
	uint8_t buf[4096];
	size_t len = 0;
	if (!parse_hex_bytes(hex_str, buf, &len, sizeof(buf)) || len == 0) {
		fprintf(stderr, "invalid hex string\n");
		return 1;
	}
	return int3_write_req(pid, addr, buf, len, INT3_IOC_WRITE_VERIFY, "write_verify",
			      "verified %zu bytes at 0x%" PRIx64 "\n");
}
