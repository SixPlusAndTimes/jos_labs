// buggy program - causes an illegal software interrupt

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	asm volatile("int $14");	// page fault， 但是在用户台 执行int14会产生一般性保护异常错误，因为idt[T_GPFLT]的DPL被设置位0，不能在用户态下执行int 14
}

