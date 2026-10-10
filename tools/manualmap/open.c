#include "mm.h"

int mm_open_elf(const char *path, int *ffd, Elf64_Ehdr *eh, Elf64_Phdr **ph, uint16_t *n,
		uint64_t *minv)
{
	int f = open(path, O_RDONLY | O_CLOEXEC);

	if (f < 0)
		return -1;
	if (mm_read_at(f, eh, sizeof(*eh), 0))
		goto fail;
	if (memcmp(eh->e_ident, ELFMAG, SELFMAG) || eh->e_machine != EM_X86_64 ||
	    eh->e_type != ET_DYN)
		goto fail;
	if (mm_phdrs(f, eh, ph, n))
		goto fail;
	*minv = (uint64_t)-1;
	for (uint16_t i = 0; i < *n; i++) {
		if ((*ph)[i].p_type != PT_LOAD || !(*ph)[i].p_memsz)
			continue;
		if ((*ph)[i].p_vaddr < *minv)
			*minv = (*ph)[i].p_vaddr;
	}
	*minv &= ~(uint64_t)4095;
	*ffd = f;
	return 0;
fail:
	close(f);
	return -1;
}
