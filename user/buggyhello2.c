// buggy hello world 2 -- pointed-to region extends into unmapped memory
// kernel should destroy user environment in response

#include <inc/lib.h>

const char *hello = "hello, world\n";

void
umain(int argc, char **argv)
{	
	// hello的虚拟地址 + 1024 * 1024 > data和text段的虚拟地址，而大于的这部分是没有映射的。
	sys_cputs(hello, 1024*1024);
}

