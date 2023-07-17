#include "cpu.h"
#include "env.h"
#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/cpu.h>

void sched_halt(void);
void sched_yield();
// Choose a user environment to run and run it.
void do_timer() {
	if (--curenv->env_ticks == 0) {
		curenv->env_ticks = curenv->env_priority;
		sched_yield();
	}
}
void
sched_yield(void)
{
	struct Env *idle;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment. Make sure curenv is not null before
	// dereferencing it.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.
	envid_t curid = curenv ? (ENVX(curenv->env_id)) % NENV : 0;
	envid_t id2run = -1;
	int max_priority = 0;

	if (curenv && curenv->env_status == ENV_RUNNING) {
		id2run = curid;
		max_priority = envs[id2run].env_priority + 10; // CPU亲和性
	}

	for (uint32_t i = 0; i < NENV; ++i) {
		envid_t id = (curid + i) % NENV;
		if (envs[id].env_status == ENV_RUNNABLE) {
			if (id2run == -1) {
				id2run = id;
				max_priority = envs[id2run].env_priority;
			}else {
				int cur_prio = envs[id].env_priority;
				if (envs[id].env_cpunum == cpunum()) { // CPU亲和性
					cur_prio += 10;
				}
				if (cur_prio >= max_priority) {
					id2run = id;
					max_priority = cur_prio;
				}

			}
		
		}
	}

	if (id2run != -1) {
		// cprintf("cpunum: %d running %d(priority: %d)\n", 
		 	// cpunum(), id2run, envs[id2run].env_priority);
		env_run(&envs[id2run]);
	}

	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		// Uncomment the following line after completing exercise 13， 这样外部设备中断也能唤醒本cpu
		"sti\n"
		"1:\n"
		"hlt\n"
		"jmp 1b\n"
	: : "a" (thiscpu->cpu_ts.ts_esp0));
}

