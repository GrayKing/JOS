// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display information about the stack", mon_backtrace }, 
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/
int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	//ccprintf("Hello!",0x10);
	//ccprintf("It's just a Sample.",0x12);
	//ccprintf("To see what would happen\n",0x14);
	//ccprintf("If I change the color of the text.\n",0x18);
	//ccprintf("Would it be ridiculars?\n",0x24);
	//ccprintf("Or not?\n",0x28);
	//ccprintf("I'm joke, don't beat me.\n",0x48);
	//ccprintf("Oh, it hurts , boy!\n",0x30);
	//ccprintf("I guess my battery would run out soon...\n",0x50);
	//ccprintf("Bye Bye!\n",0x70);
	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel sy0mbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
        volatile int * tmp_ebp ; 
        volatile int * pre_ebp ; 
        tmp_ebp = ( int * ) read_ebp() ;  

        ccprintf("Stack backtrace:\n",0x15);
	while ( ( tmp_ebp ) != 0 )  {
		ccprintf("  ebp %08x",0x12 ,(int)(tmp_ebp) );
                ccprintf("  eip %08x",0x24,*(tmp_ebp+1) );
		ccprintf("  args %08x %08x %08x %08x %08x\n", 25 ,
                *(tmp_ebp+2) , *(tmp_ebp+3) , *(tmp_ebp+4) , *(tmp_ebp+5) , *(tmp_ebp+6) );
		
                struct Eipdebuginfo eip_info ;
		debuginfo_eip( (*(tmp_ebp+1))-5 , &eip_info ) ; 
                ccprintf("%s:%d:",0x35,eip_info.eip_file, eip_info.eip_line);
                ccprintf(" %.*s+%d\n",0x36,eip_info.eip_fn_namelen, eip_info.eip_fn_name , *(tmp_ebp+1) - 5 - eip_info.eip_fn_addr);
	    tmp_ebp = ( int * ) ( *tmp_ebp ) ; 
	}
	
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the co00mmand
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
