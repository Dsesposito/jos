// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

static envid_t fork_v0(void);
static void dup_or_share(envid_t envid, void *addr, int perm);

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	int r;
	// cprintf("page fault at %x, eip %x\n", utf->utf_fault_va,
	// utf->utf_eip);
	if (!(utf->utf_err & FEC_WR))
		panic("[fork] pgfault received fault that wasn't a write\n");

	void *flt_addr = (void *) utf->utf_fault_va;
	if (!(uvpt[PGNUM(flt_addr)] & PTE_COW))
		panic("[fork] pgfault received a write fault for non-COW "
		      "page\n");

	if ((r = sys_page_alloc(0, UTEMP, PTE_U | PTE_P | PTE_W)))
		panic("[fork] pgfault:sys_page_alloc failed %x for addr: %x",
		      r,
		      flt_addr);

	memcpy(UTEMP, ROUNDDOWN(flt_addr, PGSIZE), PGSIZE);

	if ((r = sys_page_map(
	             0, UTEMP, 0, ROUNDDOWN(flt_addr, PGSIZE), PTE_U | PTE_P | PTE_W)))
		panic("[fork] pgfault:sys_page_map failed %x for addr: %x",
		      r,
		      flt_addr);

	if ((r = sys_page_unmap(0, UTEMP)))
		panic("[fork] pgfault:sys_page_unmap failed %x for addr: %x",
		      r,
		      UTEMP);
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
	void *va = (void *) (pn * PGSIZE);
	uint32_t perm = uvpt[pn] & PTE_SYSCALL;
	if (perm & PTE_SHARE) {
		r = sys_page_map(0, (void *) va, envid, (void *) va, perm);
		if (r < 0) {
			panic("duppage: sys_page_map failed for %x: %d\n", va, r);
		}
	} else if (perm & PTE_W || perm & PTE_COW) {
		// Writable
		// Mark COW in child
		if ((r = sys_page_map(0,
		                      (void *) va,
		                      envid,
		                      (void *) va,
		                      PTE_U | PTE_P | PTE_COW)))
			panic("duppage: sys_page_map failed for %x: %d\n", va, r);
		// Mark COW in parent (b/c it may have just been W)
		if ((r = sys_page_map(
		             0, (void *) va, 0, (void *) va, PTE_U | PTE_P | PTE_COW)))
			panic("duppage: sys_page_map failed for %x: %d\n", va, r);
	} else {
		// Read-only
		if ((r = sys_page_map(
		             0, (void *) va, envid, (void *) va, PTE_U | PTE_P)))
			panic("duppage: sys_page_map failed for %x: %d\n", va, r);
	}
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
	// return fork_v0();
	// panic("fork not implemented");
	int r;
	set_pgfault_handler(pgfault);


	envid_t envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e\n", envid);

	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	uintptr_t va;
	for (va = 0; va < USTACKTOP; va += PGSIZE) {
		if (!(uvpd[PDX(va)] & PTE_P) || !(uvpt[PGNUM(va)] & PTE_P)) {
			continue;
		}
		if (uvpd[PDX(va)] & PTE_U && uvpt[PGNUM(va)] & PTE_U) {
			duppage(envid, PGNUM(va));
		}
	}

	// Allocate a new user exception stack for the child
	sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE), PTE_U | PTE_P | PTE_W);

	extern void _pgfault_upcall(void);

	if ((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)))
		panic("[fork] sys_env_set_pgfault_upcall: %x", r);

	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}

envid_t
fork_v0(void)
{
	int ret;

	uint32_t pdeno, pteno;
	envid_t envid;
	uint32_t pgnum;
	uintptr_t *pgaddr;
	envid = sys_exofork();

	if (envid < 0)
		panic("sys_exofork: %e\n", envid);

	if (envid == 0) {
		// We're the child.
		// The copied value of the global variable 'thisenv'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
		if (!(uvpd[pdeno] & PTE_P))
			continue;
		// The PTE for page number N is stored in uvpt[N]
		for (pteno = 0; pteno < NPTENTRIES; pteno++) {
			pgaddr = PGADDR(pdeno, pteno, 0);
			pgnum = PGNUM(pgaddr);
			if (uvpt[pgnum] & PTE_P) {
				dup_or_share(envid,
				             pgaddr,
				             uvpt[pgnum] & PTE_SYSCALL);
			}
		}
	}
	// Also copy the stack we are currently running on.
	dup_or_share(envid, ROUNDDOWN(&ret, PGSIZE), PTE_P | PTE_U | PTE_W);

	// Start the child environment running
	if ((ret = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", ret);
	return envid;
}

static void
dup_or_share(envid_t envid, void *addr, int perm)
{
	int r;

	if (!(perm & PTE_W)) {  // SOLO LECTURA
		if ((r = sys_page_map(0, addr, envid, addr, perm)) <
		    0)  // MAPPEO DEL HIJO AL PADRE (UTILIZO LA MISMA DIRECCION
			// VIRTUAL)
			panic("duppage: sys_page_map failed for %x: %d\n", addr, r);
		return;
	}
	// LO QUE SIGUE ES PARA
	if ((r = sys_page_alloc(envid, addr, perm)) <
	    0)  // RESERVO UNA PAGINA EN EL HIJO CON DIRECCION VIRTUAL addr
		panic("sys_page_alloc: %e", r);

	//  // MAPPEO DEL HIJO AL PADRE (EN LA DIRECCION UTMP ??? COMO ES QUE
	//  FUNCIONA ESTO?)
	if ((r = sys_page_map(envid, addr, 0, UTEMP, perm)) < 0)
		panic("sys_page_map: %e", r);

	memmove(UTEMP, addr, PGSIZE);

	if ((r = sys_page_unmap(0, UTEMP)) < 0)
		panic("sys_page_unmap: %e", r);  // NO SE SI ESTA BIEN ESTO!!!!
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
