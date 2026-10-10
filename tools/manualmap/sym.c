#include "mm.h"

uint64_t mm_sym_off(const char *path, const char *name)
{
	int ffd = open(path, O_RDONLY | O_CLOEXEC);
	Elf64_Ehdr eh;
	Elf64_Shdr *sh = NULL;
	char *strtab = NULL;
	uint64_t val = 0;

	if (ffd < 0)
		return 0;
	if (mm_read_at(ffd, &eh, sizeof(eh), 0))
		goto out;
	if (memcmp(eh.e_ident, ELFMAG, SELFMAG) || eh.e_shentsize != sizeof(*sh) ||
	    !eh.e_shnum || eh.e_shstrndx >= eh.e_shnum)
		goto out;
	sh = malloc((size_t)eh.e_shnum * sizeof(*sh));
	if (!sh)
		goto out;
	if (mm_read_at(ffd, sh, (size_t)eh.e_shnum * sizeof(*sh), eh.e_shoff))
		goto out;
	for (uint16_t pass = 0; pass < 2 && !val; pass++) {
		uint32_t want = pass ? SHT_DYNSYM : SHT_SYMTAB;
		for (uint16_t i = 0; i < eh.e_shnum && !val; i++) {
			Elf64_Shdr *str;
			uint64_t count;

			if (sh[i].sh_type != want || !sh[i].sh_size || !sh[i].sh_link ||
			    sh[i].sh_link >= eh.e_shnum || sh[i].sh_entsize != sizeof(Elf64_Sym))
				continue;
			str = &sh[sh[i].sh_link];
			strtab = malloc(str->sh_size);
			if (!strtab)
				goto out;
			if (mm_read_at(ffd, strtab, str->sh_size, (off_t)str->sh_offset)) {
				free(strtab);
				strtab = NULL;
				goto out;
			}
			count = sh[i].sh_size / sizeof(Elf64_Sym);
			for (uint64_t k = 0; k < count; k++) {
				Elf64_Sym s;
				if (mm_read_at(ffd, &s, sizeof(s),
					       (off_t)(sh[i].sh_offset + k * sizeof(s))))
					break;
				if (!s.st_name || (uint64_t)s.st_name >= str->sh_size)
					continue;
				if (!strcmp(strtab + s.st_name, name)) {
					val = s.st_value;
					break;
				}
			}
			free(strtab);
			strtab = NULL;
		}
	}
out:
	free(strtab);
	free(sh);
	if (ffd >= 0)
		close(ffd);
	return val;
}
