#include "mm.h"

int mm_phdrs(int ffd, const Elf64_Ehdr *eh, Elf64_Phdr **out, uint16_t *out_n)
{
	if (eh->e_phentsize != sizeof(Elf64_Phdr) || eh->e_phnum == 0 || eh->e_phnum > 64)
		return -1;
	Elf64_Phdr *ph = malloc((size_t)eh->e_phnum * sizeof(*ph));
	if (!ph)
		return -1;
	if (mm_read_at(ffd, ph, (size_t)eh->e_phnum * sizeof(*ph), eh->e_phoff)) {
		free(ph);
		return -1;
	}
	*out = ph;
	*out_n = eh->e_phnum;
	return 0;
}
