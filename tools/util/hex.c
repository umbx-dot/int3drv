#include "int3ctl.h"

static inline int hex_nibble(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

bool parse_hex_bytes(const char *hex, uint8_t *out, size_t *out_len, size_t max_len)
{
	size_t slen = strlen(hex);
	if (slen % 2 != 0 || (slen / 2) > max_len)
		return false;
	size_t bytes = slen / 2;
	for (size_t i = 0; i < bytes; i++) {
		int hi = hex_nibble(hex[i * 2]);
		int lo = hex_nibble(hex[i * 2 + 1]);
		if (hi < 0 || lo < 0)
			return false;
		out[i] = (uint8_t)((hi << 4) | lo);
	}
	*out_len = bytes;
	return true;
}
