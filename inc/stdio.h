#ifndef JOS_INC_STDIO_H
#define JOS_INC_STDIO_H

#include <inc/stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

// lib/stdio.c
void	cputchar(int c);
int	getchar(void);
int	iscons(int fd);

// lib/printfmt.c
void	printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list);
void	cprintfmt( void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	cvprintfmt( void (*putch)(int, void*), void *putdat, const char *fmt, va_list);
int	snprintf(char *str, int size, const char *fmt, ...);
int	vsnprintf(char *str, int size, const char *fmt, va_list);

// lib/printf.c
int	cprintf(const char *fmt, ...);
int	vcprintf(const char *fmt, va_list);
int	ccprintf(const char *fmt, ...);
int	cvcprintf(const char *fmt, va_list);

// lib/fprintf.c
int	printf(const char *fmt, ...);
int	fprintf(int fd, const char *fmt, ...);

// lib/readline.c
char*	readline(const char *prompt);

#endif /* !JOS_INC_STDIO_H */
