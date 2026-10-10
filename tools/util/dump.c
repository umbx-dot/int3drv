#include "int3ctl.h"

void print_hexdump(uint64_t base_addr, const uint8_t *data, size_t len)
{
	for (size_t i = 0; i < len; i += 16) {
		printf("%016" PRIx64 ": ", base_addr + i);
		for (size_t j = 0; j < 16; j++) {
			if (i + j < len)
				printf("%02x ", data[i + j]);
			else
				printf("   ");
		}
		printf(" |");
		for (size_t j = 0; j < 16; j++) {
			if (i + j < len) {
				uint8_t c = data[i + j];
				printf("%c", (c >= 32 && c <= 126) ? c : '.');
			}
		}
		printf("|\n");
	}
}
