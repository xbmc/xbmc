/* select.c
 * Select Loop
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"


#define DEBUG_CALLS

void check_timers();

struct thread {
	void (*read_func)(void *);
	void (*write_func)(void *);
	void (*error_func)(void *);
	void *data;
};

struct thread threads[FD_SETSIZE];

fd_set w_read;
fd_set w_write;
fd_set w_error;

fd_set x_read;
fd_set x_write;
fd_set x_error;

int w_max;

int timer_id = 0;

char *tmpurl = NULL;
int newurl = 0;

struct timer {
	struct timer *next;
	struct timer *prev;
	ttime interval;
	void (*func)(void *);
	void *data;
	int id;
};

struct list_head timers = {&timers, &timers};

ttime get_time()
{
#ifdef __XBOX__
	return (ttime)timeGetTime();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

long select_info(int type)
{
	int i = 0, j;
	struct cache_entry *ce;
	switch (type) {
		case CI_FILES:
			for (j = 0; j < FD_SETSIZE; j++)
				if (threads[j].read_func || threads[j].write_func || threads[j].error_func) i++;
			return i;
		case CI_TIMERS:
			foreach(ce, timers) i++;
			return i;
		default:
			internal("cache_info: bad request");
	}
	return 0;
}

struct bottom_half {
	struct bottom_half *next;
	struct bottom_half *prev;
	void (*fn)(void *);
	void *data;
};

struct list_head bottom_halves = { &bottom_halves, &bottom_halves };

int register_bottom_half(void (*fn)(void *), void *data)
{
	struct bottom_half *bh;
	foreach(bh, bottom_halves) if (bh->fn == fn && bh->data == data) return 0;
	if (!(bh = mem_alloc(sizeof(struct bottom_half)))) return -1;
	bh->fn = fn;
	bh->data = data;
	add_to_list(bottom_halves, bh);
	return 0;
}

void unregister_bottom_half(void (*fn)(void *), void *data)
{
	struct bottom_half *bh;
	retry:
	foreach(bh, bottom_halves) if (bh->fn == fn && bh->data == data) {
		del_from_list(bh);
		mem_free(bh);
		goto retry;
	}
}

void check_bottom_halves()
{
	struct bottom_half *bh;
	void (*fn)(void *);
	void *data;
	rep:
	if (list_empty(bottom_halves)) return;
	bh = bottom_halves.prev;
	fn = bh->fn;
	data = bh->data;
	del_from_list(bh);
	mem_free(bh);
#ifdef DEBUG_CALLS
	fprintf(stderr, "call: bh %p\n", fn);
#endif
	pr(fn(data)) {free_list(bottom_halves); return;};
#ifdef DEBUG_CALLS
	fprintf(stderr, "bh done\n");
#endif
	goto rep;
}

#define CHK_BH if (!list_empty(bottom_halves)) check_bottom_halves()
		
ttime last_time;

#if 0
void check_timers()
{
	ttime interval = get_time() - last_time;
	struct timer *t;
	foreach(t, timers) t->interval -= interval;
	/*ch:*/
	foreach(t, timers) if (t->interval <= 0) {
		/*struct timer *tt = t;
		del_from_list(tt);
		tt->func(tt->data);
		mem_free(tt);
		CHK_BH;
		goto ch;*/

		struct timer *tt;
#ifdef DEBUG_CALLS
		fprintf(stderr, "call: timer %p\n", t->func);
#endif
		pr(t->func(t->data)) {
			del_from_list((struct timer *)timers.next);
			return;
		}
#ifdef DEBUG_CALLS
		fprintf(stderr, "timer done\n");
#endif
		CHK_BH;
		tt = t->prev;
		del_from_list(t);
		mem_free(t);
		t = tt;
	} else break;
	last_time += interval;
}
#endif

void check_timers()
{
	ttime interval = get_time() - last_time;
	struct timer *t;
	foreach(t, timers) t->interval -= interval;
	ch:
	foreach(t, timers) if (t->interval <= 0) {
		struct timer *tt = t;
		del_from_list(tt);
		tt->func(tt->data);
		mem_free(tt);
		CHK_BH;
		goto ch;
	} else break;
	last_time += interval;
}

int install_timer(ttime t, void (*func)(void *), void *data)
{
	struct timer *tm, *tt;
	if (!(tm = mem_alloc(sizeof(struct timer)))) return -1;
	tm->interval = t;
	tm->func = func;
	tm->data = data;
	tm->id = timer_id++;
	foreach(tt, timers) if (tt->interval >= t) break;
	add_at_pos(tt->prev, tm);
	return tm->id;
}

void kill_timer(int id)
{
	struct timer *tm;
	int k = 0;
	foreach(tm, timers) if (tm->id == id) {
		struct timer *tt = tm;
		del_from_list(tm);
		tm = tm->prev;
		mem_free(tt);
		k++;
	}
	if (!k) internal("trying to kill nonexisting timer");
	if (k >= 2) internal("more timers with same id");
}

void *get_handler(int fd, int tp)
{
	if (fd < 0 || fd >= FD_SETSIZE) {
		internal("get_handler: handle %d >= FD_SETSIZE %d", fd, FD_SETSIZE);
		return NULL;
	}
	switch (tp) {
		case H_READ:	return threads[fd].read_func;
		case H_WRITE:	return threads[fd].write_func;
		case H_ERROR:	return threads[fd].error_func;
		case H_DATA:	return threads[fd].data;
	}
	internal("get_handler: bad type %d", tp);
	return NULL;
}

void set_handlers(int fd, void (*read_func)(void *), void (*write_func)(void *), void (*error_func)(void *), void *data)
{
	if (fd < 0 || fd >= FD_SETSIZE) {
		internal("set_handlers: handle %d >= FD_SETSIZE %d", fd, FD_SETSIZE);
		return;
	}
	threads[fd].read_func = read_func;
	threads[fd].write_func = write_func;
	threads[fd].error_func = error_func;
	threads[fd].data = data;
	if (read_func) FD_SET(fd, &w_read);
	else {
		FD_CLR(fd, &w_read);
		FD_CLR(fd, &x_read);
	}
	if (write_func) FD_SET(fd, &w_write);
	else {
		FD_CLR(fd, &w_write);
		FD_CLR(fd, &x_write);
	}
	if (error_func) FD_SET(fd, &w_error);
	else {
		FD_CLR(fd, &w_error);
		FD_CLR(fd, &x_error);
	}
	if (read_func || write_func || error_func) {
		if (fd >= w_max) w_max = fd + 1;
	} else if (fd == w_max - 1) {
		int i;
		for (i = fd - 1; i >= 0; i--)
			if (FD_ISSET(i, &w_read) || FD_ISSET(i, &w_write) ||
			    FD_ISSET(i, &w_error)) break;
		w_max = i + 1;
	}
}


#ifndef __XBOX__


#define NUM_SIGNALS	32

struct signal_handler {
	void (*fn)(void *);
	void *data;
	int critical;
};

int signal_mask[NUM_SIGNALS];
struct signal_handler signal_handlers[NUM_SIGNALS];

int critical_section = 0;

void check_for_select_race();

void got_signal(int sig)
{
	if (sig >= NUM_SIGNALS || sig < 0) {
		error("ERROR: bad signal number: %d", sig);
		return;
	}
	if (!signal_handlers[sig].fn) return;
	if (signal_handlers[sig].critical) {
		signal_handlers[sig].fn(signal_handlers[sig].data);
		return;
	}
	signal_mask[sig] = 1;
	check_for_select_race();
}

void install_signal_handler(int sig, void (*fn)(void *), void *data, int critical)
{
	struct sigaction sa;
	if (sig >= NUM_SIGNALS || sig < 0) {
		internal("bad signal number: %d", sig);
		return;
	}
	memset(&sa, 0, sizeof sa);
	if (!fn) sa.sa_handler = SIG_IGN;
	else sa.sa_handler = got_signal;
	sigfillset(&sa.sa_mask);
	/*sa.sa_flags = SA_RESTART;*/
	if (!fn) sigaction(sig, &sa, NULL);
	signal_handlers[sig].fn = fn;
	signal_handlers[sig].data = data;
	signal_handlers[sig].critical = critical;
	if (fn) sigaction(sig, &sa, NULL);
}

int pending_alarm = 0;

void alarm_handler(void *x)
{
	pending_alarm = 0;
	check_for_select_race();
}

void check_for_select_race()
{
	if (critical_section) {
#ifdef SIGALRM
		install_signal_handler(SIGALRM, alarm_handler, NULL, 1);
#endif
		pending_alarm = 1;
#ifdef HAVE_ALARM
		/*alarm(1);*/
#endif
	}
}

void uninstall_alarm()
{
	pending_alarm = 0;
#ifdef HAVE_ALARM
	alarm(0);
#endif
}

int check_signals()
{
	int i, r = 0;
	for (i = 0; i < NUM_SIGNALS; i++)
		if (signal_mask[i]) {
			signal_mask[i] = 0;
			if (signal_handlers[i].fn) {
#ifdef DEBUG_CALLS
				fprintf(stderr, "call: signal %d -> %p\n", i, signal_handlers[i].fn);
#endif
				pr(signal_handlers[i].fn(signal_handlers[i].data)) return 1;
#ifdef DEBUG_CALLS
				fprintf(stderr, "signal done\n");
#endif
			}
			CHK_BH;
			r = 1;
		}
	return r;
}

void sigchld(void *p)
{
#ifndef WNOHANG
	wait(NULL);
#else
	while ((int)waitpid(-1, NULL, WNOHANG) > 0) ;
#endif
}

void set_sigcld()
{
	install_signal_handler(SIGCHLD, sigchld, NULL, 1);
}

int terminate_loop = 0;


void select_loop(void (*init)())
{
	memset(signal_mask, 0, sizeof signal_mask);
	memset(signal_handlers, 0, sizeof signal_handlers);
	FD_ZERO(&w_read);
	FD_ZERO(&w_write);
	FD_ZERO(&w_error);
	w_max = 0;
	last_time = get_time();
#ifndef __XBOX__
	signal(SIGPIPE, SIG_IGN);
#endif
	init();
	CHK_BH;
	while (!terminate_loop) {
		int n, i;
		struct timeval tv;
		struct timeval *tm = NULL;
		check_signals();
		check_timers();
		check_timers();
		check_timers();
		check_timers();
//		if (!F) redraw_all_terminals();
		if (!list_empty(timers)) {
			ttime tt = ((struct timer *)&timers)->next->interval + 1;
			if (tt < 0) tt = 0;
			tv.tv_sec = tt / 1000;
			tv.tv_usec = (tt % 1000) * 1000;
			tm = &tv;
		}

		memcpy(&x_read, &w_read, sizeof(fd_set));
		memcpy(&x_write, &w_write, sizeof(fd_set));
		memcpy(&x_error, &w_error, sizeof(fd_set));
		/*rep_sel:*/
		if (terminate_loop) break;
		if (!w_max && list_empty(timers)) {
			/*internal("select_loop: no more events to wait for");*/
			break;
		}
		critical_section = 1;
		if (check_signals()) {
			critical_section = 0;
			continue;
		}
			/*{
				int i;
				printf("\nR:");
				for (i = 0; i < 256; i++) if (FD_ISSET(i, &x_read)) printf("%d,", i);
				printf("\nW:");
				for (i = 0; i < 256; i++) if (FD_ISSET(i, &x_write)) printf("%d,", i);
				printf("\nE:");
				for (i = 0; i < 256; i++) if (FD_ISSET(i, &x_error)) printf("%d,", i);
				fflush(stdout);
			}*/
#ifdef DEBUG_CALLS
		fprintf(stderr, "select\n");
#endif
		if ((n = select(w_max, &x_read, &x_write, &x_error, tm)) < 0) {
#ifdef DEBUG_CALLS
			fprintf(stderr, "select intr\n");
#endif
			critical_section = 0;
			uninstall_alarm();
			if (errno != EINTR) error("ERROR: select failed: %d", errno);
			continue;
		}
#ifdef DEBUG_CALLS
		fprintf(stderr, "select done\n");
#endif
		critical_section = 0;
		uninstall_alarm();
		check_signals();
		/*printf("sel: %d\n", n);*/
		check_timers();
		i = -1;
		while (n > 0 && ++i < w_max) {
			int k = 0;
			/*printf("C %d : %d,%d,%d\n",i,FD_ISSET(i, &w_read),FD_ISSET(i, &w_write),FD_ISSET(i, &w_error));
			printf("A %d : %d,%d,%d\n",i,FD_ISSET(i, &x_read),FD_ISSET(i, &x_write),FD_ISSET(i, &x_error));*/
			if (FD_ISSET(i, &x_read)) {
				if (threads[i].read_func) {
#ifdef DEBUG_CALLS
					fprintf(stderr, "call: read %d -> %p\n", i, threads[i].read_func);
#endif
					pr(threads[i].read_func(threads[i].data)) continue;
#ifdef DEBUG_CALLS
					fprintf(stderr, "read done\n");
#endif
					CHK_BH;
				}
				k = 1;
			}
			check_timers();
			if (FD_ISSET(i, &x_write)) {
				if (threads[i].write_func) {
#ifdef DEBUG_CALLS
					fprintf(stderr, "call: write %d -> %p\n", i, threads[i].write_func);
#endif
					pr(threads[i].write_func(threads[i].data)) continue;
#ifdef DEBUG_CALLS
					fprintf(stderr, "write done\n");
#endif
					CHK_BH;
				}
				k = 1;
			}
			check_timers();
			if (FD_ISSET(i, &x_error)) {
				if (threads[i].error_func) {
#ifdef DEBUG_CALLS
					fprintf(stderr, "call: error %d -> %p\n", i, threads[i].error_func);
#endif
					pr(threads[i].error_func(threads[i].data)) continue;
#ifdef DEBUG_CALLS
					fprintf(stderr, "error done\n");
#endif
					CHK_BH;
				}
				k = 1;
			}
			check_timers();
			n -= k;
		}
		nopr();
	}
#ifdef DEBUG_CALLS
	fprintf(stderr, "exit loop\n");
#endif
	nopr();
}

#endif /* __XBOX__ */


/*************** LIBRARY FUNC *****************/



#ifndef LINKSBOKS_STANDALONE

void poll_new_urls(struct graphics_device *dev)
{
	if(newurl)
	{
		struct terminal *term = (struct terminal *)dev->user_data;
		struct window *win = get_root_or_first_window(term);

		goto_url((struct session *)win->data, tmpurl);
		
		newurl = 0;
	}
}

void LinksBoks_GotoURL(char *url)
{
	if(newurl)
		return;

	tmpurl = realloc( tmpurl, strlen( url ) );
	newurl = 1;

	strcpy(tmpurl, url);
}

#endif