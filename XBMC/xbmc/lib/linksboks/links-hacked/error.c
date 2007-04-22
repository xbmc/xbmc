/* error.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

#if DEBUGLEVEL >= 2
#define RED_ZONE	'R'
#define FREE_FILL	0xfe
#define REALLOC_FILL	0xfd
#define ALLOC_FILL	0xfc
#endif

#if DEBUGLEVEL < 0
#define FREE_FILL	0xfe
#endif

#if DEBUGLEVEL < 0
#define NO_IE
#endif

#ifdef RED_ZONE
#define RED_ZONE_INC	1
#else
#define RED_ZONE_INC	0
#endif

#if defined(__GNUC__) && __GNUC__ == 2 && __GNUC_MINOR__ <= 7 
void do_not_optimize_here(void *p)
{
	/* stop GCC optimization - avoid bugs in it */
}
#else
#define do_not_optimize_here(x) {}
#endif

#ifdef LEAK_DEBUG
long mem_amount = 0;
long last_mem_amount = -1;
#ifdef LEAK_DEBUG_LIST
struct list_head memory_list = { &memory_list, &memory_list };
#endif
#endif


void er(int b, unsigned char *m, va_list l)
{
#ifdef __XBOX__
	char buf[1024];
	vsprintf(buf, m, l);
	OutputDebugString(buf);
	OutputDebugString("\n");
#else
	if (b) fprintf(stderr, "%c", (char)7);
	vfprintf(stderr, m, l);
	fprintf(stderr, "\n");
	sleep(1);
#endif
}

void error(unsigned char *m, ...)
{
	va_list l;
	va_start(l, m);
	er(1, m, l);
	va_end(l);
}


void check_memory_leaks()
{
#if defined(NO_IE)
	return;
#else
#ifdef LEAK_DEBUG
	if (mem_amount) {
		error("\n\033[1mMemory leak by %ld bytes\033[0m\n", mem_amount);
#ifdef LEAK_DEBUG_LIST
		error("\nList of blocks: ");
		{
			int r = 0;
			struct alloc_header *ah;
			foreach (ah, memory_list) {
				error("%s%p:%d @ %s:%d", r ? ", ": "", (char *)ah + L_D_S, ah->size, ah->file, ah->line), r = 1;
				if (ah->comment) fprintf(stderr, ":\"%s\"", ah->comment);
			}
			error("\n");
		}
#endif
		//force_dump();
	}
#endif
#endif
}

static inline void force_dump()
{
	fprintf(stderr, "\n\033[1m%s\033[0m\n", "Forcing core dump");
	fflush(stdout);
	fflush(stderr);
	raise(SIGSEGV);
}


int errline;
unsigned char *errfile;

unsigned char errbuf[4096];

#ifdef __XBOX__
extern void xbox_DrawInternalError( unsigned char *m, unsigned char *errfile, int errline );
#endif

void int_error(unsigned char *m, ...)
{
#ifdef NO_IE
	return;
#else
	va_list l;
	va_start(l, m);
#ifdef __XBOX__
#ifdef LINKSBOKS_STANDALONE
	// Xbox: Draw a pretty error message on screen
	xbox_DrawInternalError(m, errfile, errline);
#endif
#else
	sprintf(errbuf, "\033[1mINTERNAL ERROR\033[0m at %s:%d: ", errfile, errline);
	strcat(errbuf, m);
	er(1, errbuf, l);
	force_dump();
#endif
	va_end(l);
#endif
}

void debug_msg(unsigned char *m, ...)
{
	va_list l;
	va_start(l, m);
	sprintf(errbuf, "DEBUG MESSAGE at %s:%d: ", errfile, errline);
	strcat(errbuf, m);
	er(0, errbuf, l);
	va_end(l);
}

#ifdef LEAK_DEBUG

void *debug_mem_alloc(unsigned char *file, int line, size_t size)
{
	void *p;
#ifdef LEAK_DEBUG
	struct alloc_header *ah;
#endif
	if (!size) return DUMMY;
#ifdef LEAK_DEBUG
	mem_amount += size;
	size += L_D_S;
#endif
	if (!(p = malloc(size + RED_ZONE_INC))) {
		error("ERROR: out of memory (malloc returned NULL)\n");
		exit(1);
		return NULL;
	}
#ifdef RED_ZONE
	*((char *)p + size + RED_ZONE_INC - 1) = RED_ZONE;
#endif
#ifdef LEAK_DEBUG
	ah = p;
	p = (char *)p + L_D_S;
	ah->size = size - L_D_S;
	ah->magic = ALLOC_MAGIC;
#ifdef LEAK_DEBUG_LIST
	ah->file = file;
	ah->line = line;
	ah->comment = NULL;
	add_to_list(memory_list, ah);
#endif
#endif
#ifdef ALLOC_FILL
	memset(p, ALLOC_FILL, size - L_D_S);
#endif
	return p;
}

void *debug_mem_calloc(unsigned char *file, int line, size_t size)
{
	void *p;
#ifdef LEAK_DEBUG
	struct alloc_header *ah;
#endif
	if (!size) return DUMMY;
#ifdef LEAK_DEBUG
	mem_amount += size;
	size += L_D_S;
#endif
	if (!(p = x_calloc(size + RED_ZONE_INC))) {
		error("ERROR: out of memory (calloc returned NULL)\n");
		exit(1);
		return NULL;
	}
#ifdef RED_ZONE
	*((char *)p + size + RED_ZONE_INC - 1) = RED_ZONE;
#endif
#ifdef LEAK_DEBUG
	ah = p;
	p = (char *)p + L_D_S;
	ah->size = size - L_D_S;
	ah->magic = ALLOC_MAGIC;
#ifdef LEAK_DEBUG_LIST
	ah->file = file;
	ah->line = line;
	ah->comment = NULL;
	add_to_list(memory_list, ah);
#endif
#endif
	return p;
}

void debug_mem_free(unsigned char *file, int line, void *p)
{
#ifdef LEAK_DEBUG
	struct alloc_header *ah;
#endif
	if (p == DUMMY) return;
	if (!p) {
		errfile = file, errline = line, int_error("mem_free(NULL)");
		return;
	}
#ifdef LEAK_DEBUG
	p = (char *)p - L_D_S;
	ah = p;
	if (ah->magic != ALLOC_MAGIC) {
		errfile = file, errline = line, int_error("mem_free: magic doesn't match: %08x", ah->magic);
		return;
	}
#ifdef FREE_FILL
	memset((char *)p + L_D_S, FREE_FILL, ah->size);
#endif
	ah->magic = ALLOC_FREE_MAGIC;
#ifdef LEAK_DEBUG_LIST
	del_from_list(ah);
	if (ah->comment) free(ah->comment);
#endif
	mem_amount -= ah->size;
#endif
#ifdef RED_ZONE
	if (*((char *)p + L_D_S + ah->size + RED_ZONE_INC - 1) != RED_ZONE) {
		errfile = file, errline = line, int_error("mem_free: red zone damaged: %02x (block allocated at %s:%d:%s)", *((unsigned char *)p + L_D_S + ah->size + RED_ZONE_INC - 1),
#ifdef LEAK_DEBUG_LIST
		ah->file, ah->line, ah->comment);
#else
		"-", 0, "-");
#endif
		return;
	}
#endif
	free(p);
}

void *debug_mem_realloc(unsigned char *file, int line, void *p, size_t size)
{
#ifdef LEAK_DEBUG
	struct alloc_header *ah;
#endif
	if (p == DUMMY) return debug_mem_alloc(file, line, size);
	if (!p) {
		errfile = file, errline = line, int_error("mem_realloc(NULL, %d)", size);
		return NULL;
	}
	if (!size) {
		debug_mem_free(file, line, p);
		return DUMMY;
	}
#ifdef LEAK_DEBUG
	p = (char *)p - L_D_S;
	ah = p;
	if (ah->magic != ALLOC_MAGIC) {
		errfile = file, errline = line, int_error("mem_realloc: magic doesn't match: %08x", ah->magic);
		return NULL;
	}
	ah->magic = ALLOC_REALLOC_MAGIC;
#ifdef REALLOC_FILL
	if (size < ah->size) memset((char *)p + L_D_S + size, REALLOC_FILL, ah->size - size);
#endif
#endif
#ifdef RED_ZONE
	if (*((char *)p + L_D_S + ah->size + RED_ZONE_INC - 1) != RED_ZONE) {
		errfile = file, errline = line, int_error("mem_realloc: red zone damaged: %02x (block allocated at %s:%d:%s)", *((unsigned char *)p + L_D_S + ah->size + RED_ZONE_INC - 1),
#ifdef LEAK_DEBUG_LIST
		ah->file, ah->line, ah->comment);
#else
		"-", 0, "-");
#endif
		return (char *)p + L_D_S;
	}
#endif
	if (!(p = realloc(p, size + L_D_S + RED_ZONE_INC))) {
		error("ERROR: out of memory (realloc returned NULL) - block allocated at %s:%d for size %d\n",
			file, line, size);
		//exit(1);
		return NULL;
	}
#ifdef RED_ZONE
	*((char *)p + size + L_D_S + RED_ZONE_INC - 1) = RED_ZONE;
#endif
#ifdef LEAK_DEBUG
	ah = p;
	mem_amount += size - ah->size;
	ah->size = size;
	ah->magic = ALLOC_MAGIC;
#ifdef LEAK_DEBUG_LIST
	ah->prev->next = ah;
	ah->next->prev = ah;
#endif
#endif
	return (char *)p + L_D_S;
}

void set_mem_comment(void *p, unsigned char *c, int l)
{
#ifdef LEAK_DEBUG_LIST
	struct alloc_header *ah = (struct alloc_header *)((char *)p - L_D_S);
	if (ah->comment) free(ah->comment);
	if ((ah->comment = malloc(l + 1))) memcpy(ah->comment, c, l), ah->comment[l] = 0;
#endif
}

unsigned char *get_mem_comment(void *p)
{
#ifdef LEAK_DEBUG_LIST
	/* perm je prase: return ((struct alloc_header*)((char*)((void*)((char*)p-sizeof(int))) - L_D_S))->comment;*/
	struct alloc_header *ah = (struct alloc_header *)((char *)p - L_D_S);
	if (!ah->comment) return "";
	else return ah->comment;
#else
	return "";
#endif
}


#endif

#ifdef OOPS

struct prot {
	struct prot *next;
	struct prot *prev;
	sigjmp_buf buf;
};

struct list_head prot = {&prot, &prot };

int handled = 0;

void fault(void *dummy)
{
	struct prot *p;
	/*fprintf(stderr, "FAULT: %d !\n", (int)(unsigned long)dummy);*/
	if (list_empty(prot)) exit(0);
	p = prot.next;
	del_from_list(p);
	longjmp(p->buf, 1);
}

sigjmp_buf *new_stack_frame()
{
	struct prot *new;
	if (!handled) {
		install_signal_handler(SIGSEGV, fault, (void *)SIGSEGV, 1);
		install_signal_handler(SIGBUS, fault, (void *)SIGBUS, 1);
		install_signal_handler(SIGFPE, fault, (void *)SIGFPE, 1);
		install_signal_handler(SIGILL, fault, (void *)SIGILL, 1);
		install_signal_handler(SIGABRT, fault, (void *)SIGABRT, 1);
		handled = 1;
	}
	if (!(new = mem_alloc(sizeof(struct prot)))) return NULL;
	add_to_list(prot, new);
	return &new->buf;
}

void xpr()
{
	if (!list_empty(prot)) {
		struct prot *next = prot.next;
		del_from_list(next);
		mem_free(next);
	}
}

void nopr()
{
	free_list(prot);
}

#endif

/* The backtrace corner. */

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

void
dump_backtrace(FILE *f, int trouble)
{
	/* If trouble is set, when we get here, we can be in various
	 * interesting situations like inside of the SIGSEGV handler etc. So be
	 * especially careful here.  Dynamic memory allocation may not work
	 * (corrupted stack). A lot of other things may not work too. So better
	 * don't do anything not 100% necessary. */

#ifdef HAVE_EXECINFO_H
	/* glibc way of doing this */

	void *stack[20];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace(stack, 20);

	if (trouble) {
		/* Let's hope fileno() is safe. */
		backtrace_symbols_fd(stack, size, fileno(f));
		/* Now out! */
		return;
	}

	strings = backtrace_symbols(stack, size);

	fprintf(f, "Obtained %d stack frames:\n", size);

	for (i = 0; i < size; i++)
		fprintf(f, "[%p] %s\n", stack[i], strings[i]);

	free(strings);

#endif
}
