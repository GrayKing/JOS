// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	//cprintf(" pgfault : fault va is  %08x\n", ( uint32_t ) addr ) ; 
	//cprintf(" pgfault : fault eip is  %08x\n", ( uint32_t ) utf->utf_eip ) ; 
	pde_t * pde_ptr = ( pde_t * ) ( UVPT + ( PDX(UVPT) << 12 ) + ( PDX(addr) << 2 ) ) ; 
	pte_t * pte_ptr = ( pte_t * ) ( UVPT + ( PDX(addr) << 12 ) + ( PTX(addr) << 2 ) ) ; 
	if (!( ( * pde_ptr ) & PTE_P ) ) 
		panic(" in inc/fork.c <pgfault> : Page Directory Entry doesn't exsist!\n");
	if (!( ( * pte_ptr ) & PTE_P ) ) 
		panic(" in inc/fork.c <pgfault> : Page Table Entry doesn't exsist!\n");
	if (!( ( ( *pte_ptr ) & PTE_W ) || ( ( *pte_ptr ) & PTE_COW ) ) )
		panic(" in inc/fork.c <pgfault> : Invalid page!\n");
	
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	envid_t envid = sys_getenvid() ;
	//cprintf(" pgfault : %d\n %08x\n",envid , PFTEMP); 
	sys_page_alloc( envid , ( void * ) PFTEMP , (( * pte_ptr ) & ( PTE_SYSCALL ^ PTE_COW )) | PTE_W);  
	//cprintf(" Did it reach here and made an alloc call?\n");
	memmove( ( void * ) PFTEMP , ( void * ) ROUNDDOWN( addr , PGSIZE ) , PGSIZE ) ; 
	sys_page_map( envid , ( void * ) PFTEMP , envid , ( void * ) ROUNDDOWN( addr , PGSIZE ) ,  (( * pte_ptr ) & ( PTE_SYSCALL ^ PTE_COW )) | PTE_W );  
	sys_page_unmap( envid , PFTEMP ) ; 
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	envid_t curenvid = sys_getenvid() ; 
	uintptr_t addr = pn * PGSIZE ;
	//cprintf(" duppage : now envid is : %d\n", sys_getenvid() ) ;
	//cprintf(" duppage : page address is %08x %d %d\n" , addr,PDX(addr),PTX(addr) ) ;
	pde_t * pde_ptr = ( pde_t * ) ( ( ( uint32_t ) uvpd ) + ( PDX(addr) << 2 ) ) ; 
	pte_t * pte_ptr = ( pte_t * ) ( ( ( uint32_t ) uvpt ) + ( PDX(addr) << 12 ) + ( PTX(addr) << 2 ) ) ; 
	
	//cprintf(" duppage : pte_ptr's address is : %08x\n" , ( uint32_t) pte_ptr ) ;
	//cprintf(" duppage : pte_ptr's value is : %08x\n" , ( * pte_ptr ) ) ;
	if (!( ( * pde_ptr ) & PTE_P ) ) 
		panic(" in inc/fork.c <duppage> : Page Directory Entry doesn't exsist!\n");
	if (!( ( * pte_ptr ) & PTE_P ) ) 
		panic(" in inc/fork.c <duppage> : Page Table Entry doesn't exsist!\n");
	int perm = (*pte_ptr) & PTE_SYSCALL ; 
	if ( perm & PTE_W ) {
		if ( sys_page_map( curenvid , ( void * ) addr , 
                                   envid    , ( void * ) addr , 
                                   ( perm ^ PTE_W ) | PTE_COW ) < 0 ) 
			panic(" in inc/fork.c <duppage> : Unable to map the page! %e\n");
		if ( sys_page_map( curenvid , ( void * ) addr , 
				   curenvid , ( void * ) addr , 
				   ( perm ^ PTE_W ) | PTE_COW ) < 0 ) 
			panic(" in inc/fork.c <duppage> : Unable to map the page!\n");
		return 0 ; 	
	}  
	if ( perm & PTE_COW ) {
		if ( sys_page_map( curenvid , ( void * ) addr , 
                                   envid    , ( void * ) addr , 
                                   perm ) < 0 ) 
			panic(" in inc/fork.c <duppage> : Unable to map the page!\n");
		return 0 ;
	}
	if ( sys_page_map( curenvid , ( void * ) addr , 
		           envid    , ( void * ) addr , 
			   perm ) < 0 ) 
		panic(" in inc/fork.c <duppage> : Unable to map the page!\n");
	//panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// First , set handler
	set_pgfault_handler(pgfault); 
	// Second , fork a child
	envid_t childid = sys_exofork() ;
	if ( childid < 0 ) 	
		panic(" in inc/fork.c <fork> : Unable to fork a child!\n");
	if ( childid == 0 ) {
		thisenv = ( ( struct Env * ) envs ) + ENVX(sys_getenvid());
		return 0 ;
	} 	
	//cprintf(" fork : now envid is : %d\n", sys_getenvid() ) ;
	// Third , move pages 
	uint32_t pdx = 0 , ptx = 0 ; 
	pde_t * pde_ptr ;
	pte_t * pte_ptr ; 
	for ( pdx = 0 ; pdx < PDX(envs) ; pdx ++ ) {
		pde_ptr = ( pde_t * ) ( ( ( uint32_t ) uvpd )  + ( pdx << 2 ) ) ; 
		if ( !( ( *pde_ptr ) & PTE_P ) ) continue ;
		for ( ptx = 0 ; ptx < NPTENTRIES ; ptx ++ ) {	 
			pte_ptr = ( pte_t * ) ( ( ( uint32_t ) uvpt ) + ( pdx << 12 ) + ( ptx << 2 ) ) ; 
			if ( ! ( ( * pte_ptr ) & PTE_P ) )  continue ; 
			if ( ( pdx != PDX(UXSTACKTOP-PGSIZE) ) ||
			     ( ptx != PTX(UXSTACKTOP-PGSIZE) ) ) {
				//cprintf(" fork : pdx : %d , ptx : %d\n" , pdx , ptx ) ; 
				//cprintf(" fork : pte_ptr's address is : %08x\n" , ( uint32_t) pte_ptr ) ;
				//cprintf(" fork : pte_ptr's value is : %08x\n" , (* pte_ptr ) ) ;
				duppage( childid , ( pdx << 10 ) + ptx ) ; 
				continue; 
			}
			sys_page_alloc( childid , (void*)( UXSTACKTOP - PGSIZE ) , PTE_U | PTE_W | PTE_P ) ; 
		}						
	}
	sys_env_set_status( childid , ENV_RUNNABLE ) ;
	return childid ;
	//panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
