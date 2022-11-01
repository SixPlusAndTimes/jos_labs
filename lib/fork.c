// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800 // PTE第11位置1

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
	// 判断错误是否由writedeny产生，并且对应的PTE是PTE_COW
	//  (uvpd[PDX(addr)] & PTE_P)是为了先判断对应的页表存在
	// 疑问 ： FEC_WR什么时候设置，在trap()中不是设置了utf_err等于 trap_no吗？
	if ( !((err & FEC_WR) && (uvpd[PDX(addr)] & PTE_P) &&  (uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_COW))) {
		panic("fork.c:pgfault() : pagefault conditon wrong");
	}
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	// LAB 4: Your code here.
	// 分配一个物理页面，将它与PFTEMP建立映射
	if ((r = sys_page_alloc(0,PFTEMP,PTE_W | PTE_U | PTE_P) < 0)) {
		panic("fork.c:pgfault() : sys_page_alloc failed");
	}

	addr = ROUNDDOWN(addr, PGSIZE);
	// 将 发生页错误处的内容拷贝到PFTEMP处
	memmove((void *)PFTEMP,addr,PGSIZE);

	// 将页错误地址映射到物理地址上
	r = sys_page_map(0,(void *)PFTEMP,0,addr,PTE_W | PTE_U | PTE_P); // 注意两个envid都是0，源和目标都是本进程
	if (r < 0) {
		panic("fork.c:pgfault() :sysmap failed");
	}
	// 解除物理页与PFTEMP的映射
	r = sys_page_unmap(0,(void *) PFTEMP);
	if (r < 0) {
		panic("page_unmap error");
	}
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
	void * va = (void *)(pn * PGSIZE);
	if ( (uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) {
		// map the page copy-on-write into the address space of the child
		r = sys_page_map(0, va, envid, va, PTE_COW | PTE_P | PTE_U);
		if (r < 0)  {
			return r;
		}
		// remap the page copy-on-write in its own address space. 
		r = sys_page_map(0,va, 0,va, PTE_U | PTE_COW | PTE_P);
		if (r < 0) {
			return r;
		}
		// 此时父子进程的va都映射到同一物理内存，且权限都是只读！无论父子进程的那个会往va里写，都会出发缺页中断

	} else  {
		// 映射为可写
		r = sys_page_map(0,va,envid,va, PTE_P | PTE_U);
		if (r < 0) {
			return r;
		}
	}
	return 0;
	// panic("duppage not implemented");
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
	envid_t envid;
	int r;
	unsigned pn;
	set_pgfault_handler(pgfault);// 设置缺页处理函数,这样之后父子进程就都有_pgfault_handler了（在pgfault.c中定义），但是只有在父进程的struct env中存有_pgfault_upcall，因此在下面还要进一步处理
	envid = sys_exofork(); // 这里分配子进程的pgdir，并建立内核映射
	if (envid < 0) {
		panic("fork.c:fork() :sys_exofork: %e", envid);
	}
	if (envid == 0) {
		// we are children
		// fix "thisenv" in the child process
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	// parent job
	// 遍历 utext和ustacktop之间的虚拟内存页，调用duppage
	for (pn=PGNUM(UTEXT); pn<PGNUM(USTACKTOP); pn++){ 
		if ((uvpd[pn >> 10] & PTE_P) && (uvpt[pn] & PTE_P))
			if ((r =  duppage(envid, pn)) < 0)
				return r;
	}
	// 给子进程分配异常栈，这个栈不能是COW
	if ((r = sys_page_alloc(envid,(void *) (UXSTACKTOP - PGSIZE), (PTE_U | PTE_P | PTE_W))) < 0) {
		 panic("fork.c:fork() : sys_page_alloc failed");
	}
	extern void _pgfault_upcall(void); //缺页处理的汇编函数入口，它会调用pgfault
	// 因为Struct env
	if((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0){
			panic("fork.c:fork() : sys_set_pgfault_upcall:%e", r);
	}
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)//设置子进程可运行，在这一行之后，子进程才可能被调度执行！
            return r;

	// 父进程返回子进程envid
	return envid; 
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
