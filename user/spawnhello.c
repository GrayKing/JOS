#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int r;
	if ( fork() == 0 ) 
	     if ( ( r = execl("hello","hello",0) ) < 0 ) 
		panic("exec(hello) failed: %e", r);	
	cprintf("hehe\n");
	if ( ( r = execl("hello","hello",0) ) < 0 ) 
		panic("exec(hello) failed: %e", r);	
}
