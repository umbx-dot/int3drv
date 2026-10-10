#include "mm.h"

int mm_relocate(int fd, int ffd, uint64_t base, uint64_t minv, const Elf64_Phdr *ph,
		       uint16_t n, Elf64_Dyn *d)
{
	uint64_t rela = 0;
	uint64_t relasz = 0;
	uint64_t relent = 0;
	uint64_t strtab = 0;
	uint64_t symtab = 0;
	uint64_t syment = 0;
	uint64_t libc = 0;
	Elf64_Rela *r = NULL;
	struct int3_rw rw;
	uint64_t count;
	int rc = -1;

	for (Elf64_Dyn *e = d; e->d_tag; e++) {
		if (e->d_tag == DT_RELA)
			rela = e->d_un.d_ptr;
		else if (e->d_tag == DT_RELASZ)
			relasz = e->d_un.d_val;
		else if (e->d_tag == DT_RELAENT)
			relent = e->d_un.d_val;
		else if (e->d_tag == DT_STRTAB)
			strtab = e->d_un.d_ptr;
		else if (e->d_tag == DT_SYMTAB)
			symtab = e->d_un.d_ptr;
		else if (e->d_tag == DT_SYMENT)
			syment = e->d_un.d_val;
	}
	if (rela && relent != sizeof(*r))
		return -1;
	if (!rela)
		return 0;
	{
		int64_t reloff = mm_file_off(rela, ph, n);
		if (reloff < 0)
			return -1;
		r = malloc((size_t)relasz ? (size_t)relasz : sizeof(*r));
		if (!r)
			return -1;
		if (mm_read_at(ffd, r, (size_t)relasz, (off_t)reloff))
			goto out;
	}
	count = relasz / sizeof(*r);
	for (uint64_t i = 0; i < count; i++) {
		uint32_t type = ELF64_R_TYPE(r[i].r_info);
		uint64_t where = base + (r[i].r_offset - minv);
		uint64_t val = 0;

		if (type == R_X86_64_NONE) {
			continue;
		} else if (type == R_X86_64_RELATIVE) {
			val = base + (uint64_t)r[i].r_addend - minv;
			} else if (type == R_X86_64_GLOB_DAT || type == R_X86_64_JUMP_SLOT ||
				   type == R_X86_64_64 || type == R_X86_64_IRELATIVE) {
			uint32_t sym = ELF64_R_SYM(r[i].r_info);
			char name[256];
			Elf64_Sym s = {0};
			size_t nl;

			if (!strtab || !symtab || syment != sizeof(s))
				goto out;
			rw.remote = base + (symtab - minv) + (uint64_t)sym * sizeof(s);
			rw.local = (uint64_t)(uintptr_t)&s;
			rw.len = sizeof(s);
			if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
				goto out;
			if (!s.st_name)
				goto out;
			for (nl = 0; nl < sizeof(name); nl++) {
				uint8_t ch = 0;
				rw.remote = base + (strtab - minv) + s.st_name + nl;
				rw.local = (uint64_t)(uintptr_t)&ch;
				rw.len = 1;
				if (ioctl(fd, INT3_IOC_READ, &rw) < 0)
					goto out;
				name[nl] = (char)ch;
				if (!ch)
					break;
			}
			if (nl >= sizeof(name))
				goto out;
			if (s.st_shndx != SHN_UNDEF) {
				val = base + s.st_value;
			} else {
				val = 0;
				if (!libc && int3_modbase(fd, "libc", &libc) < 0)
					goto out;
				if (mm_lookup_remote(fd, libc, name, &val) < 0 &&
				    ELF64_ST_BIND(s.st_info) != STB_WEAK)
					goto out;
			}
			if (type == R_X86_64_IRELATIVE) {
				uint64_t ifunc[3] = {24, 0, 0};
				struct int3_alloc ja = {
					.hint = 0, .addr = 0, .len = sizeof(ifunc),
					.prot = PROT_READ | PROT_WRITE, ._rsv = 0,
				};
				struct int3_thread jt;

				if (s.st_shndx != SHN_UNDEF) {
					val = base + s.st_value;
				} else {
					val = 0;
					if (!libc && int3_modbase(fd, "libc", &libc))
						goto out;
					if (mm_lookup_remote(fd, libc, name, &val) &&
					    ELF64_ST_BIND(s.st_info) != STB_WEAK)
						goto out;
				}
				if (val) {
					if (ioctl(fd, INT3_IOC_ALLOC, &ja) < 0 || !ja.addr)
						goto out;
					rw.remote = ja.addr;
					rw.local = (uint64_t)(uintptr_t)ifunc;
					rw.len = sizeof(ifunc);
					if (ioctl(fd, INT3_IOC_WRITE, &rw) < 0)
						goto out;
					jt.fn = val;
					jt.arg = ja.addr;
					jt.timeout_ms = 5000;
					jt.tid = -1;
					jt.retval = 0;
					if (ioctl(fd, INT3_IOC_THREAD, &jt) < 0 || !jt.retval)
						goto out;
					val = jt.retval;
					{
						struct int3_free jf = {.addr = ja.addr, .len = sizeof(ifunc)};
						(void)ioctl(fd, INT3_IOC_FREE, &jf);
					}
				}
			}
			val += (uint64_t)r[i].r_addend;
		} else {
			goto out;
		}
		rw.remote = where;
		rw.local = (uint64_t)(uintptr_t)&val;
		rw.len = sizeof(val);
		if (ioctl(fd, INT3_IOC_WRITE, &rw) < 0)
			goto out;
	}
	rc = 0;
out:
	free(r);
	return rc;
}
