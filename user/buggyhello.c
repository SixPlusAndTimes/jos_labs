// buggy hello world -- unmapped pointer passed to kernel
// kernel should destroy user environment in response

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	// 虚拟地址1， 不会映射到用户区域！
	sys_cputs((char*)1, 1);
}

