/*
 gcc -rdynamic test.c -o test
 http://www.linuxjournal.com/article/6391
*/
#include "backtrace.h" 
#include "utils/log.h"


typedef struct { char name[10]; int id; char description[40]; } signal_def;

signal_def signal_data[] =
{
	{ "SIGHUP", SIGHUP, "Hangup (POSIX)" },
	{ "SIGINT", SIGINT, "Interrupt (ANSI)" },
	{ "SIGQUIT", SIGQUIT, "Quit (POSIX)" },
	{ "SIGILL", SIGILL, "Illegal instruction (ANSI)" },
	{ "SIGTRAP", SIGTRAP, "Trace trap (POSIX)" },
	{ "SIGABRT", SIGABRT, "Abort (ANSI)" },
	{ "SIGIOT", SIGIOT, "IOT trap (4.2 BSD)" },
	{ "SIGBUS", SIGBUS, "BUS error (4.2 BSD)" },
	{ "SIGFPE", SIGFPE, "Floating-point exception (ANSI)" },
	{ "SIGKILL", SIGKILL, "Kill, unblockable (POSIX)" },
	{ "SIGUSR1", SIGUSR1, "User-defined signal 1 (POSIX)" },
	{ "SIGSEGV", SIGSEGV, "Segmentation violation (ANSI)" },
	{ "SIGUSR2", SIGUSR2, "User-defined signal 2 (POSIX)" },
	{ "SIGPIPE", SIGPIPE, "Broken pipe (POSIX)" },
	{ "SIGALRM", SIGALRM, "Alarm clock (POSIX)" },
	{ "SIGTERM", SIGTERM, "Termination (ANSI)" },
	{ "SIGSTKFLT", SIGSTKFLT, "Stack fault" },
	{ "SIGCHLD", SIGCHLD, "Child status has changed (POSIX)" },
	{ "SIGCLD", SIGCLD, "Same as SIGCHLD (System V)" },
	{ "SIGCONT", SIGCONT, "Continue (POSIX)" },
	{ "SIGSTOP", SIGSTOP, "Stop, unblockable (POSIX)" },
	{ "SIGTSTP", SIGTSTP, "Keyboard stop (POSIX)" },
	{ "SIGTTIN", SIGTTIN, "Background read from tty (POSIX)" },
	{ "SIGTTOU", SIGTTOU, "Background write to tty (POSIX)" },
	{ "SIGURG", SIGURG, "Urgent condition on socket (4.2 BSD)" },
	{ "SIGXCPU", SIGXCPU, "CPU limit exceeded (4.2 BSD)" },
	{ "SIGXFSZ", SIGXFSZ, "File size limit exceeded (4.2 BSD)" },
	{ "SIGVTALRM", SIGVTALRM, "Virtual alarm clock (4.2 BSD)" },
	{ "SIGPROF", SIGPROF, "Profiling alarm clock (4.2 BSD)" },
	{ "SIGWINCH", SIGWINCH, "Window size change (4.3 BSD, Sun)" },
	{ "SIGIO", SIGIO, "I/O now possible (4.2 BSD)" },
	{ "SIGPOLL", SIGPOLL, "Pollable event occurred (System V)" },
	{ "SIGPWR", SIGPWR, "Power failure restart (System V)" },
	{ "SIGSYS", SIGSYS, "Bad system call" },
};


#ifndef USE_SIGCONTEXT
void bt_sighandler(int sig, siginfo_t *info, void *secret)
#else
void bt_sighandler(int sig, struct sigcontext ctx)
#endif
{

	void *trace[16];
	char **messages = (char **)NULL;
	int i, trace_size = 0;

	signal_def *d = NULL;
	for (i = 0; i < sizeof(signal_data) / sizeof(signal_def); i++)
		if (sig == signal_data[i].id)
			{ d = &signal_data[i]; break; }
	if (d)
    {

		CLog::Log(LOGERROR,"Got signal 0x%02X (%s): %s\n", sig, signal_data[i].name, signal_data[i].description);


    }
	else
    {
		CLog::Log(LOGERROR,"Got signal 0x%02X\n", sig);
    }

	#ifndef USE_SIGCONTEXT

		void *pnt = NULL;
		#if defined(__x86_64__)
			ucontext_t* uc = (ucontext_t*) secret;
			pnt = (void*) uc->uc_mcontext.gregs[REG_RIP] ;
		#elif defined(__hppa__)
			ucontext_t* uc = (ucontext_t*) secret;
			pnt = (void*) uc->uc_mcontext.sc_iaoq[0] & ~0×3UL ;
		#elif (defined (__ppc__)) || (defined (__powerpc__))
			ucontext_t* uc = (ucontext_t*) secret;
			pnt = (void*) uc->uc_mcontext.regs->nip ;
		#elif defined(__sparc__)
		struct sigcontext* sc = (struct sigcontext*) secret;
			#if __WORDSIZE == 64
				pnt = (void*) scp->sigc_regs.tpc ;
			#else
				pnt = (void*) scp->si_regs.pc ;
			#endif
		#elif defined(__i386__)
			ucontext_t* uc = (ucontext_t*) secret;
			pnt = (void*) uc->uc_mcontext.gregs[REG_EIP] ;
		#endif
	/* potentially correct for other archs:
	 * alpha: ucp->m_context.sc_pc
	 * arm: ucp->m_context.ctx.arm_pc
	 * ia64: ucp->m_context.sc_ip & ~0×3UL
	 * mips: ucp->m_context.sc_pc
	 * s390: ucp->m_context.sregs->regs.psw.addr
	 */

	if (sig == SIGSEGV)
		CLog::Log(LOGERROR,"Faulty address is %p, called from %p\n", info->si_addr, pnt);

	/* The first two entries in the stack frame chain when you
	 * get into the signal handler contain, respectively, a
	 * return address inside your signal handler and one inside
	 * sigaction() in libc. The stack frame of the last function
	 * called before the signal (which, in case of fault signals,
	 * also is the one that supposedly caused the problem) is lost.
	 */

	/* the third parameter to the signal handler points to an
	 * ucontext_t structure that contains the values of the CPU
	 * registers when the signal was raised.
	 */

	trace_size = backtrace(trace, 16);
	/* overwrite sigaction with caller's address */
	trace[1] = pnt;

	#else

	if (sig == SIGSEGV)
		CLog::Log(LOGERROR,"Faulty address is %p, called from %p\n",
			ctx.cr2, ctx.eip);

	/* An undocumented parameter of type sigcontext that is passed
	 * to the signal handler (see the UNDOCUMENTED section in man
	 * sigaction) and contains, among other things, the value of EIP
	 * when the signal was raised. Declared obsolete in adherence
	 * with POSIX.1b since kernel version 2.2
	 */

	trace_size = backtrace(trace, 16);
	/* overwrite sigaction with caller's address */
	trace[1] = (void *)ctx.eip;
	#endif

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	CLog::Log(LOGERROR,"[bt] Execution path:\n");
	for (i=1; i<trace_size; ++i)
		CLog::Log(LOGERROR,"[bt] %s\n", messages[i]);

	exit(0);
}
/*
int main()
{

	struct sigaction sa;

	sa.sa_sigaction = (void *)bt_sighandler;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);


}*/
