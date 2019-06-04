#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include "ping.h"

#define MAXLINE 1024

int		daemon_proc;		/* set nonzero by daemon_init() */

/* Nonfatal error related to a system call.
 * Print a message and return. */

void err_ret(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error related to a system call.
 * Print a message and terminate. */

void err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Fatal error related to a system call.
 * Print a message, dump core, and terminate. */

void err_dump(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(LOG_ERR, fmt, ap);
	va_end(ap);
	abort();		/* dump core and terminate */
	exit(1);		/* shouldn't get here */
}

/* Nonfatal error unrelated to a system call.
 * Print a message and return. */

void err_msg(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error unrelated to a system call.
 * Print a message and terminate. */

void err_quit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Print a message and return to caller.
 * Caller specifies "errnoflag" and "level". */

void err_doit(int level, const char *fmt, va_list ap)
{
	int		errno_save, n;
	char	buf[MAXLINE] = {'\0'};

	errno_save = errno;		/* value caller might want printed */

	vsnprintf(buf, sizeof(buf), fmt, ap);	/* this is safe */

	n = strlen(buf);
	snprintf(buf+n, sizeof(buf)-n, ": %s\n", strerror(errno_save));

	if (daemon_proc) 
	{
		syslog(level, "%s", buf);
	}
	else
	{
		fflush(stdout);		/* in case stdout and stderr are the same */
		fputs(buf, stderr);
		fflush(stderr);
	}
	return;
}
