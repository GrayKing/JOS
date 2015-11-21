#include <inc/lib.h>
#include <inc/elf.h>

#define UTEMP2USTACK(addr)	((void*) (addr) + (USTACKTOP - PGSIZE) - UTEMP)
#define UTEMP2			(UTEMP + PGSIZE)
#define UTEMP3			(UTEMP2 + PGSIZE)
#define CODE_MAP 		0xA0000000
#define CODE_SEG		0x00800000

// Helper functions for spawn.
static int prepare_stack(envid_t child, const char **argv, uintptr_t *init_esp);
static int map_segment(envid_t child, uintptr_t va, size_t memsz,
		       int fd, size_t filesz, off_t fileoffset, int perm , size_t *cnpages);

// Spawn a child process from a program image loaded from the file system.
// prog: the pathname of the program to run.
// argv: pointer to null-terminated array of pointers to strings,
// 	 which will be passed to the child as its command-line arguments.
// Returns child envid on success, < 0 on failure.
int
exec(const char *prog, const char **argv)
{
	unsigned char elf_buf[512];
	envid_t envid;
	envid = sys_getenvid();

	int fd, i, r;
	struct Elf *elf;
	struct Proghdr *ph;
	int perm;
	size_t code_npages = 0 ; 
	unsigned eip , esp ;

	if ((r = open(prog, O_RDONLY)) < 0)
		return r;
	fd = r;

		
	elf = (struct Elf*) elf_buf;
	if (readn(fd, elf_buf, sizeof(elf_buf)) != sizeof(elf_buf)
	    || elf->e_magic != ELF_MAGIC) {
		close(fd);
		cprintf("elf magic %08x want %08x\n", elf->e_magic, ELF_MAGIC);
		return -E_NOT_EXEC;
	}


	eip = elf->e_entry ; 
	
	ph = (struct Proghdr*) (elf_buf + elf->e_phoff);
	for (i = 0; i < elf->e_phnum; i++, ph++) {
		if (ph->p_type != ELF_PROG_LOAD)
			continue;
		perm = PTE_P | PTE_U;
		if (ph->p_flags & ELF_PROG_FLAG_WRITE)
			perm |= PTE_W;
		if ((r = map_segment(envid, ph->p_va, ph->p_memsz,
				     fd, ph->p_filesz, ph->p_offset, perm , &code_npages )) < 0)
			goto error;
	}
	fd = -1;

	if ((r = prepare_stack(envid, argv, &esp)) < 0)
		goto error;
	
	if ((r = sys_exec_support( eip , esp , (void*) UTEMP , (void*) CODE_MAP , code_npages )) < 0)
		panic("sys_exec_support: %e", r);

	return envid;

error:
	close(fd);
	sys_env_destroy( envid );
	return r;
}

// Set up the initial stack page for the new envid process with envid 'envid'
// using the arguments array pointed to by 'argv',
// which is a null-terminated array of pointers to null-terminated strings.
//
// On success, returns 0 and sets *init_esp
// to the initial stack pointer with which the envid should start.
// Returns < 0 on failure.
static int
prepare_stack(envid_t envid, const char **argv, uintptr_t *init_esp)
{
	size_t string_size;
	int argc, i, r;
	char *string_store;
	uintptr_t *argv_store;

	string_size = 0;
	for (argc = 0; argv[argc] != 0; argc++)
		string_size += strlen(argv[argc]) + 1;

	string_store = (char*) UTEMP + PGSIZE - string_size;
	argv_store = (uintptr_t*) (ROUNDDOWN(string_store, 4) - 4 * (argc + 1));

	if ((void*) (argv_store - 2) < (void*) UTEMP)
		return -E_NO_MEM;

	if ((r = sys_page_alloc(0, (void*) UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		return r;

	for (i = 0; i < argc; i++) {
		argv_store[i] = UTEMP2USTACK(string_store);
		strcpy(string_store, argv[i]);
		string_store += strlen(argv[i]) + 1;
	}
	argv_store[argc] = 0;
	assert(string_store == (char*)UTEMP + PGSIZE);

	argv_store[-1] = UTEMP2USTACK(argv_store);
	argv_store[-2] = argc;

	*init_esp = UTEMP2USTACK(&argv_store[-2]);

	return 0;

}

static int
map_segment(envid_t envid, uintptr_t va, size_t memsz,
	int fd, size_t filesz, off_t fileoffset, int perm , size_t *cnpages )
{
	int i, r;
	void *blk;


	if ((i = PGOFF(va))) {
		va -= i;
		memsz += i;
		filesz += i;
		fileoffset -= i;
	}


	for (i = 0; i < memsz; i += PGSIZE) {
		if (i >= filesz) {
			// allocate a blank page
			if ( ( 0x00800000 <= va + i ) && ( va + i < 0x10800000 ) ) 
			{
				if ((r=sys_page_alloc(envid,(void*)( va + i - CODE_SEG + CODE_MAP ),perm))<0) 
					return r ; 
				if ( va + i - CODE_SEG >= (*cnpages) * PGSIZE ) {
					*cnpages = ( va + i - CODE_SEG + PGSIZE - 1 ) / PGSIZE ; 
				} 
				continue ;
			}	
			if ((r = sys_page_alloc(envid, (void*) (va + i), perm)) < 0)
				return r;
		} else {
			// from file
			
			if ( ( 0x00800000 <= va + i ) && ( va + i < 0x10800000 ) ) 
			{
				if ((r = sys_page_alloc(0, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
					return r;
				if ((r = seek(fd, fileoffset + i)) < 0)
					return r;
				if ((r = readn(fd, UTEMP, MIN(PGSIZE, filesz-i))) < 0)
					return r;
				if ( va + i - CODE_SEG >= (*cnpages) * PGSIZE ) {
					*cnpages = ( va + i - CODE_SEG + PGSIZE - 1 ) / PGSIZE ; 
					if ((r=sys_page_alloc(envid,(void*)( va + i - CODE_SEG + CODE_MAP ),perm))<0) 
						return r ; 
				} 
				if ((r = sys_page_map(0, UTEMP, envid, (void*) (va + i - CODE_SEG +CODE_MAP ), perm)) < 0)
					panic("exec: sys_page_map data: %e", r);
				sys_page_unmap(0,UTEMP);
				continue ;
			}	
			if ((r = sys_page_alloc(0, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
				return r;
			if ((r = seek(fd, fileoffset + i)) < 0)
				return r;
			if ((r = readn(fd, UTEMP, MIN(PGSIZE, filesz-i))) < 0)
				return r;
			if ((r = sys_page_map(0, UTEMP, envid, (void*) (va + i), perm)) < 0)
				panic("exec: sys_page_map data: %e", r);
			sys_page_unmap(0, UTEMP);
		}
	}
	return 0;
}


int
execl(const char *prog, const char *arg0, ...)
{
	// We calculate argc by advancing the args until we hit NULL.
	// The contract of the function guarantees that the last
	// argument will always be NULL, and that none of the other
	// arguments will be NULL.
	int argc=0;
	va_list vl;
	va_start(vl, arg0);
	while(va_arg(vl, void *) != NULL)
		argc++;
	va_end(vl);

	// Now that we have the size of the args, do a second pass
	// and store the values in a VLA, which has the format of argv
	const char *argv[argc+2];
	argv[0] = arg0;
	argv[argc+1] = NULL;

	va_start(vl, arg0);
	unsigned i;
	for(i=0;i<argc;i++)
		argv[i+1] = va_arg(vl, const char *);
	va_end(vl);
	return exec(prog, argv);
}
