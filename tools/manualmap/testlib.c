#include <string.h>

unsigned long map_flag = 0;

__attribute__((visibility("default"), constructor)) void map_init(void)
{
	map_flag = strlen("abcd") == 4 ? 0xC0FFEEul : 0ul;
}
