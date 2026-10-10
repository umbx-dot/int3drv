#include "mm.h"

int mm_read_dyn(int ffd, const Elf64_Phdr *ph, uint16_t n, Elf64_Dyn **dyn)
{
	for (uint16_t i = 0; i < n; i++) {
		Elf64_Dyn *d;

		if (ph[i].p_type != PT_DYNAMIC || !ph[i].p_memsz)
			continue;
		d = malloc(ph[i].p_memsz);
		if (!d)
			return -1;
		if (mm_read_at(ffd, d, ph[i].p_memsz, (off_t)ph[i].p_offset)) {
			free(d);
			return -1;
		}
		*dyn = d;
		return 0;
	}
	*dyn = NULL;
	return 0;
}
