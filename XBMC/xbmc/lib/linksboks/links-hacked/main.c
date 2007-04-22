/* main.c
 * main()
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

int retval = RET_OK;

void unhandle_basic_signals(struct terminal *);

void sig_terminate(struct terminal *t)
{
	unhandle_basic_signals(t);
	terminate_loop = 1;
	retval = RET_SIGNAL;
}

void sig_intr(struct terminal *t)
{
	if (!t) {
		unhandle_basic_signals(t);
		terminate_loop = 1;
	} else {
		unhandle_basic_signals(t);
		exit_prog(t, NULL, NULL);
	}
}

void sig_ctrl_c(struct terminal *t)
{
	if (!is_blocked()) kbd_ctrl_c();
}

void sig_ign(void *x)
{
}

void sig_tstp(struct terminal *t)
{
#ifdef SIGSTOP
	int pid = getpid();
	if (!F) block_itrm(0);
#ifdef G
	else drv->block(NULL);
#endif
#if defined (SIGCONT) && defined(SIGTTOU)
	if (!fork()) {
		sleep(1);
		kill(pid, SIGCONT);
		exit(0);
	}
#endif
	raise(SIGSTOP);
#endif
}

void sig_cont(struct terminal *t)
{
	if (!F) {
		if (!unblock_itrm(0)) resize_terminal();
#ifdef G
	} else {
		drv->unblock(NULL);
#endif
	}
	/*else register_bottom_half(raise, SIGSTOP);*/
}

#ifdef BACKTRACE
static void sig_segv(struct terminal *t)
{
	/* Get some attention. */
	fprintf(stderr,"\a"); fflush(stderr); sleep(1);	fprintf(stderr,"\a\n");

	/* Rant. */
	fprintf(stderr, "Links crashed. That shouldn't happen. Please report this incident to\n");
	fprintf(stderr, "developers. Preferrably please include information about what probably\n");
	fprintf(stderr, "triggered this and the listout below. Note that it does NOT supercede the gdb\n");
	fprintf(stderr, "output, which is way more useful for developers. If you would like to help to\n");
	fprintf(stderr, "debug the problem you just uncovered, please keep the core you just got and\n");
	fprintf(stderr, "send the developers output of 'bt' command entered inside of gdb (which you run\n");
	fprintf(stderr, "as gdb links core). Thanks a lot for your cooperation!\n\n");

	/* Backtrace. */
	dump_backtrace(stderr, 1);

	/* TODO: Perhaps offer launching of gdb? Or trying to continue w/
	 * program execution? --pasky */

        /* ...And then there was core dump */
        {
		struct sigaction sa;

		memset(&sa, 0, sizeof sa);
		sa.sa_handler = SIG_DFL;
		sigfillset(&sa.sa_mask);
		sigaction(SIGSEGV, &sa, NULL);
        }
        fprintf(stderr, "\n\033[1m%s\033[0m\n", "Forcing core dump");
	fflush(stdout);
	fflush(stderr);
	raise(SIGSEGV);


        /* The fastest way OUT!
	abort();*/
}
#endif

void handle_basic_signals(struct terminal *term)
{
#ifndef __XBOX__
	install_signal_handler(SIGHUP, (void (*)(void *))sig_intr, term, 0);
	if (!F) install_signal_handler(SIGINT, (void (*)(void *))sig_ctrl_c, term, 0);
	/*install_signal_handler(SIGTERM, (void (*)(void *))sig_terminate, term, 0);*/
#ifdef SIGTSTP
	if (!F) install_signal_handler(SIGTSTP, (void (*)(void *))sig_tstp, term, 0);
#endif
#ifdef SIGTTIN
	if (!F) install_signal_handler(SIGTTIN, (void (*)(void *))sig_tstp, term, 0);
#endif
#ifdef SIGTTOU
	install_signal_handler(SIGTTOU, (void (*)(void *))sig_ign, term, 0);
#endif
#ifdef SIGCONT
	if (!F) install_signal_handler(SIGCONT, (void (*)(void *))sig_cont, term, 0);
#endif
#ifdef BACKTRACE
	install_signal_handler(SIGSEGV, (void (*)(void *))sig_segv, term, 1);
#endif
#endif /* __XBOX__ */
}

/*void handle_slave_signals(struct terminal *term)
{
	install_signal_handler(SIGHUP, (void (*)(void *))sig_terminate, term, 0);
	install_signal_handler(SIGINT, (void (*)(void *))sig_terminate, term, 0);
	install_signal_handler(SIGTERM, (void (*)(void *))sig_terminate, term, 0);
#ifdef SIGTSTP
	install_signal_handler(SIGTSTP, (void (*)(void *))sig_tstp, term, 0);
#endif
#ifdef SIGTTIN
	install_signal_handler(SIGTTIN, (void (*)(void *))sig_tstp, term, 0);
#endif
#ifdef SIGTTOU
	install_signal_handler(SIGTTOU, (void (*)(void *))sig_ign, term, 0);
#endif
#ifdef SIGCONT
	install_signal_handler(SIGCONT, (void (*)(void *))sig_cont, term, 0);
#endif
}*/

void unhandle_terminal_signals(struct terminal *term)
{
#ifndef __XBOX__
	install_signal_handler(SIGHUP, NULL, NULL, 0);
	if (!F) install_signal_handler(SIGINT, NULL, NULL, 0);
#ifdef SIGTSTP
	install_signal_handler(SIGTSTP, NULL, NULL, 0);
#endif
#ifdef SIGTTIN
	install_signal_handler(SIGTTIN, NULL, NULL, 0);
#endif
#ifdef SIGTTOU
	install_signal_handler(SIGTTOU, NULL, NULL, 0);
#endif
#ifdef SIGCONT
	install_signal_handler(SIGCONT, NULL, NULL, 0);
#endif
#ifdef BACKTRACE
	install_signal_handler(SIGSEGV, NULL, NULL, 0);
#endif
#endif /* __XBOX__ */
}

void unhandle_basic_signals(struct terminal *term)
{
#ifndef __XBOX__
	install_signal_handler(SIGHUP, NULL, NULL, 0);
	if (!F) install_signal_handler(SIGINT, NULL, NULL, 0);
	/*install_signal_handler(SIGTERM, NULL, NULL, 0);*/
#ifdef SIGTSTP
	install_signal_handler(SIGTSTP, NULL, NULL, 0);
#endif
#ifdef SIGTTIN
	install_signal_handler(SIGTTIN, NULL, NULL, 0);
#endif
#ifdef SIGTTOU
	install_signal_handler(SIGTTOU, NULL, NULL, 0);
#endif
#ifdef SIGCONT
	install_signal_handler(SIGCONT, NULL, NULL, 0);
#endif
#ifdef BACKTRACE
	install_signal_handler(SIGSEGV, NULL, NULL, 0);
#endif
#endif /* __XBOX__ */
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
		handle_basic_signals(term);	/* OK, this is race condition, but it must be so; GPM installs it's own buggy TSTP handler */
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

struct object_request *dump_obj;
int dump_pos;

void end_dump(struct object_request *r, void *p)
{
	struct cache_entry *ce;
	int oh;
	if (!r->state || (r->state == 1 && dmp != D_SOURCE)) return;
	if ((oh = get_output_handle()) == -1) return;
	ce = r->ce;
	if (dmp == D_SOURCE) {
		if (ce) {
			struct fragment *frag;
			nextfrag:
			foreach(frag, ce->frag) if (frag->offset <= dump_pos && frag->offset + frag->length > dump_pos) {
				int l = frag->length - (dump_pos - frag->offset);
				int w = hard_write(oh, frag->data + dump_pos - frag->offset, l);
				if (w != l) {
					detach_object_connection(r, dump_pos);
					if (w < 0) fprintf(stderr, "Error writing to stdout: %s.\n", strerror(errno));
					else fprintf(stderr, "Can't write to stdout.\n");
					retval = RET_ERROR;
					goto terminate;
				}
				dump_pos += w;
				detach_object_connection(r, dump_pos);
				goto nextfrag;
			}
		}
		if (r->state >= 0) return;
	} else if (ce) {
		/* !!! FIXME */
		/*struct document_options o;
		struct view_state *vs;
		struct f_data_c *fd;
		if (!(vs = create_vs())) goto terminate;
		if (!(vs = create_vs())) goto terminate;
		memset(&o, 0, sizeof(struct document_options));
		o.xp = 0;
		o.yp = 1;
		o.xw = 80;
		o.yw = 25;
		o.col = 0;
		o.cp = 0;
		ds2do(&dds, &o);
		o.plain = 0;
		o.frames = 0;
		memcpy(&o.default_fg, &default_fg, sizeof(struct rgb));
		memcpy(&o.default_bg, &default_bg, sizeof(struct rgb));
		memcpy(&o.default_link, &default_link, sizeof(struct rgb));
		memcpy(&o.default_vlink, &default_vlink, sizeof(struct rgb));
		o.framename = "";
		init_vs(vs, stat->ce->url);
		cached_format_html(vs, &fd, &o);
		dump_to_file(fd.f_data, oh);
		detach_formatted(&fd);
		destroy_vs(vs);*/

	}
	if (r->state != O_OK) {
		unsigned char *m = get_err_msg(r->stat.state);
		fprintf(stderr, "%s\n", get_english_translation(m));
		retval = RET_ERROR;
		goto terminate;
	}
	terminate:
	terminate_loop = 1;
}

int g_argc;
unsigned char **g_argv;

unsigned char *path_to_exe;

int init_b = 0;

void initialize_all_subsystems();
void initialize_all_subsystems_2();

void init()
{
	int uh;
	void *info;
	int len;
	unsigned char *u;

        initialize_all_subsystems();

/* OS/2 has some stupid bug and the pipe must be created before socket :-/ */
	if (c_pipe(terminal_pipe)) {
		error("ERROR: can't create pipe for internal communication");
		goto ttt;
	}
#ifdef __XBOX__
	ggr=1;
	dmp=0;
#else
	if (!(u = parse_options(g_argc - 1, g_argv + 1))) goto ttt;
        if (strstr(path_to_exe,"glinks")) ggr=1;
	if (ggr_drv[0] || ggr_mode[0]) ggr = 1;
	if (dmp) ggr = 0;
#endif
	if (!ggr && !no_connect && (uh = bind_to_af_unix()) != -1) {
		close(terminal_pipe[0]);
		close(terminal_pipe[1]);
		if (!(info = create_session_info(base_session, u, &len, NULL))) {
			close(uh);
			goto ttt;
		}
		initialize_all_subsystems_2();
                handle_trm(get_input_handle(), get_output_handle(), uh, uh, get_ctl_handle(), info, len);
                handle_basic_signals(NULL);	/* OK, this is race condition, but it must be so; GPM installs it's own buggy TSTP handler */
                mem_free(info);
		return;
	}
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
#ifndef __XBOX__
	u = parse_options(g_argc - 1, g_argv + 1);
#else
	u = "";
	ggr = 1;
	dmp = 0;
	strcpy( ggr_drv, "xbox" );
/*	ggr_mode = "";
	ggr_display = ""; */
#endif
        if (!u) {
		ttt:
		initialize_all_subsystems_2();
		tttt:
                terminate_loop = 1;
		retval = RET_SYNTAX;
		return;
	}
	if (!dmp) {
		if (ggr) {
#ifdef G
			unsigned char *r;

                        if(!no_connect &&
                           (uh = bind_to_af_unix()) != -1) {
                                unsigned char *wd  = get_cwd();
                                unsigned char *url = strlen(u)
                                        ? translate_url(u, wd)
                                        : u;

                                init_g_out(uh, url);

                                if(wd) mem_free(wd);
                                if(strlen(u)) mem_free(url);
                                return;
			}
                        /* Commented just because links crashes too often
                         and I want to see error messages in place

                        if(fork()>0) _exit(0);

                        */

                        if ((r = init_graphics(ggr_drv, ggr_mode, ggr_display))) {
				fprintf(stderr, "%s", r);
				mem_free(r);
				goto ttt;
			}
			handle_basic_signals(NULL);
			init_dither(drv->depth);
			F = 1;
#else
			fprintf(stderr, "Graphics not enabled when compiling\n");
			goto ttt;
#endif
		}
		initialize_all_subsystems_2();
                if (!((info = create_session_info(base_session, u, &len, NULL)) &&
                      gf_val(attach_terminal(get_input_handle(), get_output_handle(), get_ctl_handle(), info, len),
                             attach_g_terminal(info, len)) != -1)) {
                        retval = RET_FATAL;
                        terminate_loop = 1;
                }
	} else {
		unsigned char *uu, *wd;
		initialize_all_subsystems_2();
		close(terminal_pipe[0]);
		close(terminal_pipe[1]);
		if (!*u) {
			fprintf(stderr, "URL expected after %s\n.", dmp == D_DUMP ? "-dump" : "-source");
			goto tttt;
                }
		if (!(uu = translate_url(u, wd = get_cwd()))) uu = stracpy(u);
                request_object(NULL, uu, NULL, PRI_MAIN, NC_RELOAD, end_dump, NULL, &dump_obj);
		mem_free(uu);
		if (wd) mem_free(wd);
	}
}

/* Is called before gaphics driver init */
void initialize_all_subsystems()
{
#ifdef __XBOX__

	/* Initialize the WinSockX library.
	This should probably go elsewhere */
	if( XBNet_Init( 0 ) < 0 )
		exit( 1 );

	MountDevice( "\\??\\C:", "\\Device\\Harddisk0\\Partition2" );
	MountDevice( "\\??\\E:", "\\Device\\Harddisk0\\Partition1" );
	MountDevice( "\\??\\F:", "\\Device\\Harddisk0\\Partition6" );
	MountDevice( "\\??\\G:", "\\Device\\Harddisk0\\Partition7" );
	MountDevice( "\\??\\X:", "\\Device\\Harddisk0\\Partition3" );
	MountDevice( "\\??\\Y:", "\\Device\\Harddisk0\\Partition4" );
	MountDevice( "\\??\\Z:", "\\Device\\Harddisk0\\Partition5" );

/*
	// Set file I/O mode to binary
	// Else the downloads get corrupted
	_fmode = O_BINARY;
*/
#endif

	init_trans();
	set_sigcld();
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

void terminate_all_subsystems()
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
	release_object(&dump_obj);
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
}

#ifdef LINKSBOKS_STANDALONE
void __cdecl main()
{
	LD_LAUNCH_DASHBOARD LaunchData = { XLD_LAUNCH_DASHBOARD_MAIN_MENU };

	select_loop(init);
	terminate_all_subsystems();
	XLaunchNewImage( NULL, (LAUNCH_DATA*)&LaunchData );
}
#endif