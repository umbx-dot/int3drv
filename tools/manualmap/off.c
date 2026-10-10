#include "mm.h"

int64_t mm_file_off(uint64_t vaddr, const Elf64_Phdr *ph, uint16_t n)
{
	for (uint16_t i = 0; i < n; i++) {
		if (ph[i].p_type != PT_LOAD && ph[i].p_type != PT_DYNAMIC)
			continue;
		if (vaddr >= ph[i].p_vaddr && vaddr < ph[i].p_vaddr + ph[i].p_filesz)
			return (int64_t)(ph[i].p_offset + (vaddr - ph[i].p_vaddr));
	}
	return -1;
}
