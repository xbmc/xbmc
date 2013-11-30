#ifndef __BACKTRACE__H__
#define __BACKTRACE__H__

#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include <stdlib.h>

/* Since kernel version 2.2 the undocumented parameter to the signal handler has been declared
obsolete in adherence with POSIX.1b. A more correct way to retrieve additional information is
to use the SA_SIGINFO option when setting the handler */
#undef USE_SIGCONTEXT

#ifndef USE_SIGCONTEXT
/* get REG_EIP / REG_RIP from ucontext.h */
#define __USE_GNU
#include <ucontext.h>

	#ifndef EIP
	#define EIP     14
	#endif

	#if (defined (__x86_64__))
		#ifndef REG_RIP
		#define REG_RIP REG_INDEX(rip) /* seems to be 16 */
		#endif
	#endif

#endif

#ifndef USE_SIGCONTEXT
void bt_sighandler(int sig, siginfo_t *info, void *secret);
#else
void bt_sighandler(int sig, struct sigcontext ctx);
#endif

#endif // __BACKTRACE__H__
