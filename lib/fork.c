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
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	panic("pgfault not implemented");
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
	panic("duppage not implemented");
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
	return fork_v0();
	// panic("fork not implemented");
}

envid_t
fork_v0(void)
{
	int ret;

	uint32_t pdeno, pteno;
	envid_t envid;
	uint32_t pgnum;
	uintptr_t* pgaddr;
	envid = sys_exofork();
	if (envid < 0) {
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
				dup_or_share(envid,pgaddr,uvpt[pgnum] & PTE_SYSCALL);
			}
		}
	}
	// Also copy the stack we are currently running on.
	dup_or_share(envid,ROUNDDOWN(&ret, PGSIZE),PTE_P | PTE_U | PTE_W);

	// Start the child environment running
	if ((ret = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", ret);
	return envid;
}

static void
dup_or_share(envid_t envid, void *addr, int perm)
{
	int r;
	if ((r = sys_page_alloc(envid, addr, perm)) < 0)
		panic("sys_page_alloc: %e", r);

	if (perm & PTE_W) {
		if ((r = sys_page_map(0, addr, 0, addr, perm)) < 0)
			panic("sys_page_map: %e", r);

		
		if ((r = sys_page_map(0, addr, envid, addr, perm)) < 0) // hint : mapping and dup page
			panic("sys_page_map: %e", r);				
	}else {
		if ((r = sys_page_map(0, addr, envid, addr, perm))<0)
			panic("duppage: sys_page_map failed for %x: %d\n", addr, r);
	}

	memmove(UTEMP, addr, PGSIZE);
	if ((r = sys_page_unmap(0, UTEMP)) < 0)
		panic("sys_page_unmap: %e", r);

}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
