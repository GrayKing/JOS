/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>

###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.text;\
        .globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps;\
        .data;\
        .long name;\
	.long num;\
	

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
        .text;\
        .globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps;\
        .data;\
        .long name;\
        .long num;\

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
.data
.global vec;
.long vec;
vec:

TRAPHANDLER_NOEC(IDIVIDE    , 0)		// divide error
TRAPHANDLER_NOEC(IDEBUG     , 1)		// debug exception
TRAPHANDLER_NOEC(INMI       , 2)		// non-maskable interrupt
TRAPHANDLER_NOEC(IBRKPT     , 3)		// breakpoint
TRAPHANDLER_NOEC(IOFLOW     , 4)		// overflow
TRAPHANDLER_NOEC(IBOUND     , 5)		// bounds check
TRAPHANDLER_NOEC(IILLOP     , 6)		// illegal opcode
TRAPHANDLER_NOEC(IDEVICE    , 7)		// device not available
TRAPHANDLER(IDBLFLT    , 8)		// double fault
TRAPHANDLER(ITSS       ,10)		// invalid task switch segment
TRAPHANDLER(ISEGNP     ,11)		// segment not present
TRAPHANDLER(ISTACK     ,12)		// stack exception
TRAPHANDLER(IGPFLT     ,13)		// general protection fault
TRAPHANDLER(IPGFLT     ,14)		// page fault
TRAPHANDLER_NOEC(IFPERR     ,16)		// floating point error
TRAPHANDLER(IALIGN     ,17)		// aligment check
TRAPHANDLER_NOEC(IMCHK      ,18)		// machine check
TRAPHANDLER_NOEC(ISIMDERR   ,19)		// SIMD floating point error
TRAPHANDLER_NOEC(ISYSCALL   ,48)
TRAPHANDLER_NOEC(IRQ0,32)
TRAPHANDLER_NOEC(IRQ1,33)
TRAPHANDLER_NOEC(IRQ2,34)
TRAPHANDLER_NOEC(IRQ3,35)
TRAPHANDLER_NOEC(IRQ4,36)
TRAPHANDLER_NOEC(IRQ5,37)
TRAPHANDLER_NOEC(IRQ6,38)
TRAPHANDLER_NOEC(IRQ7,39)
TRAPHANDLER_NOEC(IRQ8,40)
TRAPHANDLER_NOEC(IRQ9,41)
TRAPHANDLER_NOEC(IRQ10,42)
TRAPHANDLER_NOEC(IRQ11,43)
TRAPHANDLER_NOEC(IRQ12,44)
TRAPHANDLER_NOEC(IRQ13,45)
TRAPHANDLER_NOEC(IRQ14,46)
TRAPHANDLER_NOEC(IRQ15,47)
.data
.long -1;
.global vec_end;
.long vec_end;
vec_end:
/*
 * Lab 3: Your code here for _alltraps
 */
.global _alltraps
.align 4
_alltraps:
	pushw  $0
	pushw  %es
 	pushw  $0
	pushw  %ds
	pushl  %eax
	pushl  %ecx
	pushl  %edx
	pushl  %ebx
	pushl  %esp
	pushl  %ebp
	pushl  %esi	
	pushl  %edi
        movw   $0x10,%ax
	movw   %ax,%es
	movw   %ax,%ds	
	pushl  %esp
	call   trap


