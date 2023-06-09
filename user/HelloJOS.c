// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{	
	for (int i = 0; i< argc; i++) {
		cprintf("%dth arg = %s\n",i, argv[i]);
	}
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", thisenv->env_id);
}
