/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TRAP_H
#define JOS_KERN_TRAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];
extern struct Pseudodesc idt_pd;

void trap_init(void);
void trap_init_percpu(void);
void print_regs(struct PushRegs *regs);
void print_trapframe(struct Trapframe *tf);
void page_fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);

// 在 trapentry.S中定义的入口函数，最后都会调用 trap()
void divide_handler();
void debug_handler();
void nmi_handler();    
void brkpt_handler(); 
void oflow_handler();
void bound_handler();
void illop_handler();
void device_handler();

void dblflt_handler();
void tss_handler();
void segnp_handler();
void stack_handler();
void gpflt_handler();
void pgflt_handler();

void fperr_handler();
void align_handler();
void mchk_handler();
void simderr_handler();
void syscall_handler();
#endif /* JOS_KERN_TRAP_H */
