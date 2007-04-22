#include "links.h"


/********************* OLD MAIN.C ********************/


int init_b = 0;
int linksboks_is_initialized = 0;
int terminate_loop = 0;


/* Is called before gaphics driver init */
void initialize_all_subsystems()
{
	init_trans();
	init_home();
	init_dns();
	init_cache();
    init_auth();
}

/* Is called sometimes after and sometimes before graphics driver init */
void initialize_all_subsystems_2()
{
    GF(init_dip());
	init_bfu();
	GF(init_imgcache());
	init_fcache();
	GF(init_grview());
}

void __LinksBoks_Terminate()
{

        af_unix_close();
	check_bottom_halves();
	abort_all_downloads();
#ifdef HAVE_SSL
	ssl_finish();
#endif
#ifdef HAVE_LUA
	if (init_b && !prepare_lua(NULL)) {
		lua_dostring(lua_state, "if quit_hook then quit_hook() end");
		finish_lua();
	}
#endif
	check_bottom_halves();
	destroy_all_terminals();
	check_bottom_halves();
	shutdown_bfu();
	GF(shutdown_dip());
	if (!F) free_all_itrms();
	abort_all_connections();

	free_all_caches();
	if (init_b) save_url_history();
#ifdef GLOBHIST
    if (init_b) finalize_global_history();
#endif
	free_history_lists();
	free_term_specs();
	free_types();
	free_auth();
	if (init_b) finalize_bookmarks();
	if (init_b) finalize_blocklist();
    finalize_options();
    free_conv_table();
	free_blacklist();
	if (init_b) cleanup_cookies();
	check_bottom_halves();
	end_config();
	free_strerror_buf();
	shutdown_trans();
	GF(shutdown_graphics());
	terminate_osdep();

	linksboks_is_initialized = 0;
}


int terminal_pipe[2];

int attach_terminal(int in, int out, int ctl, void *info, int len)
{
	struct terminal *term;
	fcntl(terminal_pipe[0], F_SETFL, O_NONBLOCK);
	fcntl(terminal_pipe[1], F_SETFL, O_NONBLOCK);
	handle_trm(in, out, out, terminal_pipe[1], ctl, info, len);
	mem_free(info);
	if ((term = init_term(terminal_pipe[0], out, win_func))) {
		return terminal_pipe[1];
        }
	close(terminal_pipe[0]);
	close(terminal_pipe[1]);
	return -1;
}

#ifdef G

int attach_g_terminal(void *info, int len)
{
	struct terminal *term;
        term = init_gfx_term(win_func, info, len);
	mem_free(info);
	return term ? 0 : -1;
}

#endif

int __LinksBoks_InitCore(unsigned char *homedir)
{
	links_home = homedir;
    initialize_all_subsystems();

	load_config();
	init_options();	

    /* Set interface language. More appropriate place?.. --karpov */
    set_language(language_index(options_get("interface_language")));

    init_b = 1;

    init_bookmarks();
	init_blocklist();
	create_initial_extensions();
#ifdef GLOBHIST
	read_global_history();
#endif
	load_url_history();
	init_cookies();
#ifdef HAVE_LUA
    init_lua();
#endif

	return 0;
}



/********************* OLD SELECT.C ********************/


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
	if(terminate_loop)
		return;
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
	if(terminate_loop)
		return -1;
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

int critical_section = 0;




/********************* OLD SELECT.C - NEW MAIN LOOP ********************/

int __LinksBoks_NewWindow()
{
	int uh;
	char *r;
	void *info;
	int len;

	if(!linksboks_is_initialized)
	{
		if (c_pipe(terminal_pipe))
			return 1;

		FD_ZERO(&w_read);
		FD_ZERO(&w_write);
		FD_ZERO(&w_error);
		w_max = 0;
		last_time = get_time();
		CHK_BH;

		if(!no_connect && (uh = bind_to_af_unix()) != -1) {
			unsigned char *wd  = get_cwd();
			unsigned char *url = "";

#ifdef USE_AF_UNIX
			init_g_out(uh, "");
#endif

			return 2;
		}


		if ((r = init_graphics("d3dx", "", ""))) {
			OutputDebugString(r);
			OutputDebugString("\n");
			initialize_all_subsystems_2();
			return 3;
		}

		init_dither(drv->depth);
		F = 1;

		initialize_all_subsystems_2();

		linksboks_is_initialized = 1;
	}

	if (!((info = create_session_info(base_session, "", &len, NULL)) &&
            gf_val(attach_terminal(get_input_handle(), get_output_handle(), get_ctl_handle(), info, len),
                    attach_g_terminal(info, len)) != -1)) {
            return 4;
    }

	terminate_loop = 0;
	return 0;
}

int __LinksBoks_FrameMove()
{
	int n, i;
	struct timeval tv;
	struct timeval *tm = NULL;

	if (terminate_loop) return 1;

	check_timers();
	check_timers();
	check_timers();
	check_timers();
	CHK_BH;
//		if (!F) redraw_all_terminals();

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	tm = &tv;

	memcpy(&x_read, &w_read, sizeof(fd_set));
	memcpy(&x_write, &w_write, sizeof(fd_set));
	memcpy(&x_error, &w_error, sizeof(fd_set));
	/*rep_sel:*/
	if (!w_max && list_empty(timers)) {
		/*internal("select_loop: no more events to wait for");*/
		return 0;
	}
	critical_section = 1;

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
	}
#ifdef DEBUG_CALLS
	fprintf(stderr, "select done\n");
#endif
	critical_section = 0;
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

	return 0;
}

