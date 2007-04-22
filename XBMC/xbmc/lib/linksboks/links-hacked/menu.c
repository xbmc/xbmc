/* menu.c
 * (c) 2002 Mikulas Patocka, Petr 'Brain' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

/*static inline struct session *get_term_session(struct terminal *term)
{
	if ((void *)term->windows.prev == &term->windows) {
		internal("terminal has no windows");
		return NULL;
	}
	return ((struct window *)term->windows.prev)->data;
}*/

void menu_about(struct terminal *term, void *d, struct session *ses)
{
        /*
        msg_box(term, NULL, TXT(T_ABOUT), AL_CENTER, TXT(T_LINKS__LYNX_LIKE), NULL, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
        */

        open_in_new_tab(term,NULL,"internal:about");
}

void menu_keys(struct terminal *term, void *d, struct session *ses)
{
	msg_box(term, NULL, TXT(T_KEYS), AL_LEFT | AL_MONO, TXT(T_KEYS_DESC), NULL, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
}

void menu_copying(struct terminal *term, void *d, struct session *ses)
{
        /*
        msg_box(term, NULL, TXT(T_COPYING), AL_CENTER, TXT(T_COPYING_DESC), NULL, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
        */
        open_in_new_tab(term,NULL,"internal:license");
}

/*
void menu_manual(struct terminal *term, void *d, struct session *ses)
{
	goto_url(ses, LINKS_MANUAL_URL);
}

void menu_homepage(struct terminal *term, void *d, struct session *ses)
{
	goto_url(ses, LINKS_HOMEPAGE_URL);
}

void menu_calibration(struct terminal *term, void *d, struct session *ses)
{
	goto_url(ses, LINKS_CALIBRATION_URL);
}
*/

void menu_for_frame(struct terminal *term, void (*f)(struct session *, struct f_data_c *, int), struct session *ses)
{
	do_for_frame(ses, f, 0);
}

void menu_goto_url(struct terminal *term, void *d, struct session *ses)
{
	dialog_goto_url(ses, "");
}

void menu_save_url_as(struct terminal *term, void *d, struct session *ses)
{
	dialog_save_url(ses);
}

void menu_go_back(struct terminal *term, void *d, struct session *ses)
{
	go_back(ses, NULL);
}

void menu_go_forward(struct terminal *term, void *d, struct session *ses)
{
	go_forward(ses);
}


void menu_reload(struct terminal *term, void *d, struct session *ses)
{
	reload(ses, -1);
}

void menu_stop(struct terminal *term, void *d, struct session *ses)
{
	stop_button_pressed(ses);
}

void really_exit_prog(struct session *ses)
{
        register_bottom_half((void (*)(void *))destroy_terminal, ses->term);
}

void dont_exit_prog(struct session *ses)
{
	ses->exit_query = 0;
}

void query_exit(struct session *ses)
{
	unsigned char *message=TXT(T_DO_YOU_REALLY_WANT_TO_CLOSE_WINDOW);

	if(ses->term->next == ses->term->prev)
		message = are_there_downloads()
			? TXT(T_DO_YOU_REALLY_WANT_TO_EXIT_LINKS_AND_TERMINATE_ALL_DOWNLOADS)
			: TXT(T_DO_YOU_REALLY_WANT_TO_EXIT_LINKS);

	ses->exit_query = 1;
	msg_box(ses->term,
                NULL, TXT(T_EXIT_LINKS), AL_CENTER,
                message,
		ses, 2, TXT(T_YES), (void (*)(void *))really_exit_prog, B_ENTER, TXT(T_NO), dont_exit_prog, B_ESC);
}

void menu_close_tab(struct terminal *term, void *d, struct session *ses)
{
        close_tab(term);
}

void exit_prog(struct terminal *term, void *d, struct session *ses)
{
	if (!ses) {
		register_bottom_half((void (*)(void *))destroy_terminal, term);
		return;
	}
	if (!ses->exit_query && (!d || (term->next == term->prev && are_there_downloads()))) {
		query_exit(ses);
		return;
	}
	really_exit_prog(ses);
}

struct refresh {
	struct terminal *term;
	struct window *win;
	struct session *ses;
	void (*fn)(struct terminal *term, void *d, struct session *ses);
	void *data;
	int timer;
};

void refresh(struct refresh *r)
{
	struct refresh rr;
	r->timer = -1;
	memcpy(&rr, r, sizeof(struct refresh));
	rr.fn(rr.term, rr.data, rr.ses);
	delete_window(r->win);
}

void end_refresh(struct refresh *r)
{
	if (r->timer != -1) kill_timer(r->timer);
	mem_free(r);
}

void refresh_abort(struct dialog_data *dlg)
{
	end_refresh(dlg->dlg->udata2);
}

void cache_inf(struct terminal *term, void *d, struct session *ses)
{
	unsigned char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9, *a10, *a11, *a12, *a13, *a14, *a15, *a16;
#ifdef G
	unsigned char *b14, *b15, *b16, *b17;
	unsigned char *c14, *c15, *c16, *c17;
#endif
	int l = 0;
	struct refresh *r;
	if (!(r = mem_alloc(sizeof(struct refresh)))) return;
	r->term = term;
	r->win = NULL;
	r->ses = ses;
	r->fn = cache_inf;
	r->data = d;
	r->timer = -1;
	l = 0;
	l = 0, a1 = init_str(); add_to_str(&a1, &l, ": "); add_num_to_str(&a1, &l, select_info(CI_FILES));add_to_str(&a1, &l, " ");
	l = 0, a2 = init_str(); add_to_str(&a2, &l, ", "); add_num_to_str(&a2, &l, select_info(CI_TIMERS));add_to_str(&a2, &l, " ");
	l = 0, a3 = init_str(); add_to_str(&a3, &l, ".\n");

	l = 0, a4 = init_str(); add_to_str(&a4, &l, ": "); add_num_to_str(&a4, &l, connect_info(CI_FILES));add_to_str(&a4, &l, " ");
	l = 0, a5 = init_str(); add_to_str(&a5, &l, ", "); add_num_to_str(&a5, &l, connect_info(CI_CONNECTING));add_to_str(&a5, &l, " ");
	l = 0, a6 = init_str(); add_to_str(&a6, &l, ", "); add_num_to_str(&a6, &l, connect_info(CI_TRANSFER));add_to_str(&a6, &l, " ");
	l = 0, a7 = init_str(); add_to_str(&a7, &l, ", "); add_num_to_str(&a7, &l, connect_info(CI_KEEP));add_to_str(&a7, &l, " ");
	l = 0, a8 = init_str(); add_to_str(&a8, &l, ".\n");

	l = 0, a9 = init_str(); add_to_str(&a9, &l, ": "); add_num_to_str(&a9, &l, cache_info(CI_BYTES));add_to_str(&a9, &l, " ");
	l = 0, a10 = init_str(); add_to_str(&a10, &l, ", "); add_num_to_str(&a10, &l, cache_info(CI_FILES));add_to_str(&a10, &l, " ");
	l = 0, a11 = init_str(); add_to_str(&a11, &l, ", "); add_num_to_str(&a11, &l, cache_info(CI_LOCKED));add_to_str(&a11, &l, " ");
	l = 0, a12 = init_str(); add_to_str(&a12, &l, ", "); add_num_to_str(&a12, &l, cache_info(CI_LOADING));add_to_str(&a12, &l, " ");
	l = 0, a13 = init_str(); add_to_str(&a13, &l, ".\n");

#ifdef G
	if (F) {
		l = 0, b14 = init_str();
		add_to_str(&b14, &l, ", ");
		add_num_to_str(&b14, &l, imgcache_info(CI_BYTES));
		add_to_str(&b14, &l, " ");
		
		l = 0, b15 = init_str(); 
		add_to_str(&b15, &l, ", ");
		add_num_to_str(&b15, &l, imgcache_info(CI_FILES));
		add_to_str(&b15, &l, " ");
		
		l = 0, b16 = init_str(); 
		add_to_str(&b16, &l, ", ");
		add_num_to_str(&b16, &l, imgcache_info(CI_LOCKED));
		add_to_str(&b16, &l, " ");
		
		l = 0, b17 = init_str();
		add_to_str(&b17, &l, ".\n");
		
		l = 0, c14 = init_str();
		add_to_str(&c14, &l, ", ");
		add_num_to_str(&c14, &l, font_cache.bytes);
		add_to_str(&c14, &l, " ");
		
		l = 0, c15 = init_str(); 
		add_to_str(&c15, &l, ", ");
		add_num_to_str(&c15, &l, font_cache.max_bytes);
		add_to_str(&c15, &l, " ");
		
		l = 0, c16 = init_str(); 
		add_to_str(&c16, &l, ", ");
		add_num_to_str(&c16, &l, font_cache.items);
		add_to_str(&c16, &l, " ");
		
		l = 0, c17 = init_str();
		add_to_str(&c17, &l, ".\n");
	}
#endif

	l = 0, a14 = init_str(); add_to_str(&a14, &l, ": "); add_num_to_str(&a14, &l, formatted_info(CI_FILES));add_to_str(&a14, &l, " ");
	l = 0, a15 = init_str(); add_to_str(&a15, &l, ", "); add_num_to_str(&a15, &l, formatted_info(CI_LOCKED));add_to_str(&a15, &l, " ");
	l = 0, a16 = init_str(); add_to_str(&a16, &l, ".");

	if (!F) msg_box(term, getml(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, NULL), TXT(T_RESOURCES), AL_LEFT | AL_EXTD_TEXT, TXT(T_RESOURCES), a1, TXT(T_HANDLES), a2, TXT(T_TIMERS), a3, TXT(T_CONNECTIONS), a4, TXT(T_cONNECTIONS), a5, TXT(T_CONNECTING), a6, TXT(T_tRANSFERRING), a7, TXT(T_KEEPALIVE), a8, TXT(T_MEMORY_CACHE), a9, TXT(T_BYTES), a10, TXT(T_FILES), a11, TXT(T_LOCKED), a12, TXT(T_LOADING), a13, TXT(T_FORMATTED_DOCUMENT_CACHE), a14, TXT(T_DOCUMENTS), a15, TXT(T_LOCKED), a16, NULL, r, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
#ifdef G
	else msg_box(term, getml(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11,
				a12, a13, b14, b15, b16, b17, a14, a15, a16,
				c14, c15, c16, c17, NULL), TXT(T_RESOURCES),
			AL_LEFT | AL_EXTD_TEXT, TXT(T_RESOURCES), a1,
			TXT(T_HANDLES), a2, TXT(T_TIMERS), a3,
			TXT(T_CONNECTIONS), a4, TXT(T_cONNECTIONS), a5,
			TXT(T_CONNECTING), a6, TXT(T_tRANSFERRING), a7,
			TXT(T_KEEPALIVE), a8, TXT(T_MEMORY_CACHE), a9,
			TXT(T_BYTES), a10, TXT(T_FILES), a11, TXT(T_LOCKED),
			a12, TXT(T_LOADING), a13, TXT(T_IMAGE_CACHE), b14,
			TXT(T_BYTES), b15, TXT(T_IMAGES), b16, TXT(T_LOCKED),
			b17, TXT(T_FONT_CACHE), c14, TXT(T_BYTES), c15,
			TXT(T_BYTES_MAX), c16, TXT(T_LETTERS), c17,
			TXT(T_FORMATTED_DOCUMENT_CACHE), a14, TXT(T_DOCUMENTS), a15, TXT(T_LOCKED), a16, NULL, r, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
#endif
	r->win = term->windows.next;
	((struct dialog_data *)r->win->data)->dlg->abort = refresh_abort;
	r->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r);
}

#ifdef DEBUG

void list_cache(struct terminal *term, void *d, struct session *ses)
{
	unsigned char *a;
	int l = 0;
	struct refresh *r;
	struct cache_entry *ce, *cache;
	if (!(a = init_str())) return;
	if (!(r = mem_alloc(sizeof(struct refresh)))) {
		mem_free(a);
		return;
	}
	r->term = term;
	r->win = NULL;
	r->ses = ses;
	r->fn = list_cache;
	r->data = d;
	r->timer = -1;
	cache = (struct cache_entry *)cache_info(CI_LIST);
	add_to_str(&a, &l, ":");
	foreach(ce, *cache) {
		add_to_str(&a, &l, "\n");
		add_to_str(&a, &l, ce->url);
	}
	msg_box(term, getml(a, NULL), TXT(T_CACHE_INFO), AL_LEFT | AL_EXTD_TEXT, TXT(T_CACHE_CONTENT), a, NULL, r, 1, TXT(T_OK), end_refresh, B_ENTER | B_ESC);
	r->win = term->windows.next;
	r->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r);
	/* !!! the refresh here is buggy */
}

#endif

#ifdef LEAK_DEBUG

void memory_cld(struct terminal *term, void *d)
{
	last_mem_amount = mem_amount;
}

#define MSG_BUF	2000
#define MSG_W	100

void memory_info(struct terminal *term, void *d, struct session *ses)
{
	char message[MSG_BUF];
	char *p;
	struct refresh *r;
	if (!(r = mem_alloc(sizeof(struct refresh)))) return;
	r->term = term;
	r->win = NULL;
	r->ses = ses;
	r->fn = memory_info;
	r->data = d;
	r->timer = -1;
	p = message;
	p += sprintf(p, "%ld %s", mem_amount, _(TXT(T_MEMORY_ALLOCATED), term));
	if (last_mem_amount != -1) p += sprintf(p, ", %s %ld, %s %ld", _(TXT(T_LAST), term), last_mem_amount, _(TXT(T_DIFFERENCE), term), mem_amount - last_mem_amount);
	p += sprintf(p, ".");
#if 0 && defined(MAX_LIST_SIZE)
	if (last_mem_amount != -1) {
		long i, j;
		int l = 0;
		for (i = 0; i < MAX_LIST_SIZE; i++) if (memory_list[i].p && memory_list[i].p != last_memory_list[i].p) {
			for (j = 0; j < MAX_LIST_SIZE; j++) if (last_memory_list[j].p == memory_list[i].p) goto b;
			if (!l) p += sprintf(p, "\n%s: ", _(TXT(T_NEW_ADDRESSES), term)), l = 1;
			else p += sprintf(p, ", ");
			p += sprintf(p, "#%p of %d at %s:%d", memory_list[i].p, (int)memory_list[i].size, memory_list[i].file, memory_list[i].line);
			if (p - message >= MSG_BUF - MSG_W) {
				p += sprintf(p, "..");
				break;
			}
			b:;
		}
		if (!l) p += sprintf(p, "\n%s", _(TXT(T_NO_NEW_ADDRESSES), term));
		p += sprintf(p, ".");
	}
#endif
	if (!(p = stracpy(message))) {
		mem_free(r);
		return;
	}
	msg_box(term, getml(p, NULL), TXT(T_MEMORY_INFO), AL_CENTER, p, r, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
	r->win = term->windows.next;
	((struct dialog_data *)r->win->data)->dlg->abort = refresh_abort;
	r->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r);
}

#endif

void flush_caches(struct terminal *term, void *d, void *e)
{
	shrink_memory(SH_FREE_ALL);
}

/* jde v historii o psteps polozek dozadu */
void go_backwards(struct terminal *term, void *psteps, struct session *ses)
{
        go_back(ses, (struct location *)psteps);
}

struct menu_item no_hist_menu[] = {
	{ TXT(T_NO_HISTORY), "", M_BAR, NULL, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

void history_menu(struct terminal *term, void *ddd, struct session *ses)
{
	struct location *l;
	struct menu_item *mi = NULL;

        foreach(l, ses->history) {
		if(!mi && !(mi = new_menu(3))) return;
                if (l==cur_loc(ses)) {
		     unsigned char *m = mem_alloc(strlen(l->url)+3+1);
		     sprintf(m, "=> %s", l->url);
		     add_to_menu(&mi, m, "", "", MENU_FUNC go_backwards, l, 0);
                } else
		     add_to_menu(&mi, stracpy(l->url), "", "", MENU_FUNC go_backwards, l, 0);
	}

        if (list_empty(ses->history)) do_menu(term, no_hist_menu, ses);
        else do_menu(term, mi, ses);
}

struct menu_item no_downloads_menu[] = {
	{ TXT(T_NO_DOWNLOADS), "", M_BAR, NULL, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

void downloads_menu(struct terminal *term, void *ddd, struct session *ses)
{
	struct download *d;
	struct menu_item *mi = NULL;
	int n = 0;
	foreachback(d, downloads) {
		unsigned char *u;
		if (!mi) if (!(mi = new_menu(3))) return;
		u = stracpy(d->url);
		if (strchr(u, POST_CHAR)) *strchr(u, POST_CHAR) = 0;
		add_to_menu(&mi, u, "", "", MENU_FUNC display_download, d, 0);
		n++;
	}
	if (!n) do_menu(term, no_downloads_menu, ses);
	else do_menu(term, mi, ses);
}

void menu_doc_info(struct terminal *term, void *ddd, struct session *ses)
{
	state_msg(ses);
}

void menu_toggle(struct terminal *term, void *ddd, struct session *ses)
{
	toggle(ses, ses->screen, 0);
}

void display_codepage(struct terminal *term, void *pcp, struct session *ses)
{
	int cp = (int)pcp;
	struct term_spec *t = new_term_spec(term->term);
	if (t) t->charset = cp;
	cls_redraw_all_terminals();
}

/*
void assumed_codepage(struct terminal *term, void *pcp, struct session *ses)
{
	int cp = (int)pcp;
	ses->ds.assume_cp = cp;
	redraw_terminal_cls(term);
}
*/

void charset_list(struct terminal *term, void *xxx, struct session *ses)
{
	int i, sel;
	unsigned char *n;
	struct menu_item *mi;
	if (!(mi = new_menu(1))) return;
	for (i = 0; (n = get_cp_name(i)); i++) {
		if (is_cp_special(i)) continue;
		add_to_menu(&mi, get_cp_name(i), "", "", MENU_FUNC display_codepage, (void *)i, 0);
	}
	sel = ses->term->spec->charset;
	if (sel < 0) sel = 0;
	do_menu_selected(term, mi, ses, sel);
}

void set_val(struct terminal *term, void *ip, int *d)
{
	*d = (int)ip;
}

void charset_sel_list(struct terminal *term, struct session *ses, int *ptr)
{
	int i, sel;
	unsigned char *n;
	struct menu_item *mi;
	if (!(mi = new_menu(1))) return;
	for (i = 0; (n = get_cp_name(i)); i++) {
		add_to_menu(&mi, get_cp_name(i), "", "", MENU_FUNC set_val, (void *)i, 0);
	}
	sel = *ptr;
	if (sel < 0) sel = 0;
	do_menu_selected(term, mi, ptr, sel);
}

void terminal_optionS_OKAY(void *p)
{
	cls_redraw_all_terminals();
}

unsigned char *td_labels[] = { TXT(T_NO_FRAMES), TXT(T_VT_100_FRAMES), TXT(T_LINUX_OR_OS2_FRAMES), TXT(T_KOI8R_FRAMES), TXT(T_USE_11M), TXT(T_RESTRICT_FRAMES_IN_CP850_852), TXT(T_BLOCK_CURSOR), TXT(T_COLOR), NULL };

void terminal_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	struct term_spec *ts = new_term_spec(term->term);
	if (!ts) return;
	if (!(d = mem_alloc(sizeof(struct dialog) + 11 * sizeof(struct dialog_item)))) return;
	memset(d, 0, sizeof(struct dialog) + 11 * sizeof(struct dialog_item));
	d->title = TXT(T_TERMINAL_OPTIONS);
	d->fn = checkbox_list_fn;
	d->udata = td_labels;
	d->refresh = (void (*)(void *))terminal_optionS_OKAY;
	d->items[0].type = D_CHECKBOX;
	d->items[0].gid = 1;
	d->items[0].gnum = TERM_DUMB;
	d->items[0].dlen = sizeof(int);
	d->items[0].data = (void *)&ts->mode;
	d->items[1].type = D_CHECKBOX;
	d->items[1].gid = 1;
	d->items[1].gnum = TERM_VT100;
	d->items[1].dlen = sizeof(int);
	d->items[1].data = (void *)&ts->mode;
	d->items[2].type = D_CHECKBOX;
	d->items[2].gid = 1;
	d->items[2].gnum = TERM_LINUX;
	d->items[2].dlen = sizeof(int);
	d->items[2].data = (void *)&ts->mode;
	d->items[3].type = D_CHECKBOX;
	d->items[3].gid = 1;
	d->items[3].gnum = TERM_KOI8;
	d->items[3].dlen = sizeof(int);
	d->items[3].data = (void *)&ts->mode;
	d->items[4].type = D_CHECKBOX;
	d->items[4].gid = 0;
	d->items[4].dlen = sizeof(int);
	d->items[4].data = (void *)&ts->m11_hack;
	d->items[5].type = D_CHECKBOX;
	d->items[5].gid = 0;
	d->items[5].dlen = sizeof(int);
	d->items[5].data = (void *)&ts->restrict_852;
	d->items[6].type = D_CHECKBOX;
	d->items[6].gid = 0;
	d->items[6].dlen = sizeof(int);
	d->items[6].data = (void *)&ts->block_cursor;
	d->items[7].type = D_CHECKBOX;
	d->items[7].gid = 0;
	d->items[7].dlen = sizeof(int);
	d->items[7].data = (void *)&ts->col;
	d->items[8].type = D_BUTTON;
	d->items[8].gid = B_ENTER;
	d->items[8].fn = ok_dialog;
	d->items[8].text = TXT(T_OK);
	d->items[9].type = D_BUTTON;
	d->items[9].gid = B_ESC;
	d->items[9].fn = cancel_dialog;
	d->items[9].text = TXT(T_CANCEL);
	d->items[10].type = D_END;
 	do_dialog(term, d, getml(d, NULL));
}
 /*BOOK0000010x000bb*/
void menu_shell(struct terminal *term, void *xxx, void *yyy)
{
	unsigned char *sh;
	if (!(sh = GETSHELL)) sh = DEFAULT_SHELL;
	exec_on_terminal(term, sh, "", 1);
}

void menu_kill_background_connections(struct terminal *term, void *xxx, void *yyy)
{
	abort_background_connections();
}
void menu_kill_all_connections(struct terminal *term, void *xxx, void *yyy)
{
	abort_all_connections();
}

void menu_set_language(struct terminal *term, void *pcp, struct session *ses)
{
	set_language((int)pcp);
	cls_redraw_all_terminals();
}

void menu_language_list(struct terminal *term, void *xxx, struct session *ses)
{
	int i, sel;
	unsigned char *n;
	struct menu_item *mi;
	if (!(mi = new_menu(1))) return;
	for (i = 0; i < n_languages(); i++) {
		n = language_name(i);
		add_to_menu(&mi, n, "", "", MENU_FUNC menu_set_language, (void *)i, 0);
	}
	sel = current_language;
	do_menu_selected(term, mi, ses, sel);
}

unsigned char *resize_texts[] = { TXT(T_COLUMNS), TXT(T_ROWS) };

unsigned char x_str[4];
unsigned char y_str[4];

void do_resize_terminal(struct terminal *term)
{
	unsigned char str[8];
	strcpy(str, x_str);
	strcat(str, ",");
	strcat(str, y_str);
	do_terminal_function(term, TERM_FN_RESIZE, str);
}

void dlg_resize_terminal(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	int x = term->x > 999 ? 999 : term->x;
	int y = term->y > 999 ? 999 : term->y;
	sprintf(x_str, "%d", x);
	sprintf(y_str, "%d", y);
	if (!(d = mem_alloc(sizeof(struct dialog) + 5 * sizeof(struct dialog_item)))) return;
	memset(d, 0, sizeof(struct dialog) + 5 * sizeof(struct dialog_item));
	d->title = TXT(T_RESIZE_TERMINAL);
	d->fn = group_fn;
	d->udata = resize_texts;
	d->refresh = (void (*)(void *))do_resize_terminal;
	d->refresh_data = term;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = 4;
	d->items[0].data = x_str;
	d->items[0].fn = check_number;
	d->items[0].gid = 1;
	d->items[0].gnum = 999;
	d->items[1].type = D_FIELD;
	d->items[1].dlen = 4;
	d->items[1].data = y_str;
	d->items[1].fn = check_number;
	d->items[1].gid = 1;
	d->items[1].gnum = 999;
	d->items[2].type = D_BUTTON;
	d->items[2].gid = B_ENTER;
	d->items[2].fn = ok_dialog;
	d->items[2].text = TXT(T_OK);
	d->items[3].type = D_BUTTON;
	d->items[3].gid = B_ESC;
	d->items[3].fn = cancel_dialog;
	d->items[3].text = TXT(T_CANCEL);
	d->items[4].type = D_END;
	do_dialog(term, d, getml(d, NULL));

}

struct menu_item file_menu11[] = {
	{ TXT(T_GOTO_URL), "g", TXT(T_HK_GOTO_URL), MENU_FUNC menu_goto_url, (void *)0, 0, 0 },
	{ TXT(T_GO_BACK), "z", TXT(T_HK_GO_BACK), MENU_FUNC menu_go_back, (void *)0, 0, 0 },
        { TXT(T_GO_FORWARD), "`", TXT(T_HK_GO_FORWARD), MENU_FUNC menu_go_forward, (void *)0, 0, 0 },
	{ TXT(T_HISTORY), ">", TXT(T_HK_HISTORY), MENU_FUNC history_menu, (void *)0, 1, 0 },
	{ TXT(T_RELOAD), "Ctrl-R", TXT(T_HK_RELOAD), MENU_FUNC menu_reload, (void *)0, 0, 0 },
        { TXT(T_STOP), "a", TXT(T_HK_STOP), MENU_FUNC menu_stop, (void *)0, 0, 0 },
};

struct menu_item file_menu12[] = {
        { TXT(T_BOOKMARKS), "s", TXT(T_HK_BOOKMARKS), MENU_FUNC menu_bookmark_manager, (void *)0, 0, 0 },
};

struct menu_item file_menu21[] = {
	{ "", "", M_BAR, NULL, NULL, 0, 0 },
	{ TXT(T_SAVE_AS), "", TXT(T_HK_SAVE_AS), MENU_FUNC save_as, (void *)0, 0, 0 },
	{ TXT(T_SAVE_URL_AS), "", TXT(T_HK_SAVE_URL_AS), MENU_FUNC menu_save_url_as, (void *)0, 0, 0 },
	{ TXT(T_SAVE_FORMATTED_DOCUMENT), "", TXT(T_HK_SAVE_FORMATTED_DOCUMENT), MENU_FUNC menu_save_formatted, (void *)0, 0, 0 },
        { TXT(T_COPY_URL_LOCATION), "", TXT(T_HK_COPY_URL_LOCATION), MENU_FUNC send_copy_url_location, (void *)0, 0, 0 },
};

#ifdef G
struct menu_item file_menu211[] = {
	{ "", "", M_BAR, NULL, NULL, 0, 0 },
	{ TXT(T_SAVE_AS), "", TXT(T_HK_SAVE_AS), MENU_FUNC save_as, (void *)0, 0, 0 },
	{ TXT(T_SAVE_URL_AS), "", TXT(T_HK_SAVE_URL_AS), MENU_FUNC menu_save_url_as, (void *)0, 0, 0 },
        { TXT(T_COPY_URL_LOCATION), "", TXT(T_HK_COPY_URL_LOCATION), MENU_FUNC send_copy_url_location, (void *)0, 0, 0 },
};
#endif

struct menu_item file_menu22[] = {
	{ "", "", M_BAR, NULL, NULL, 0, 0} ,
	{ TXT(T_KILL_BACKGROUND_CONNECTIONS), "", TXT(T_HK_KILL_BACKGROUND_CONNECTIONS), MENU_FUNC menu_kill_background_connections, (void *)0, 0, 0 },
	{ TXT(T_KILL_ALL_CONNECTIONS), "", TXT(T_HK_KILL_ALL_CONNECTIONS), MENU_FUNC menu_kill_all_connections, (void *)0, 0, 0 },
	{ TXT(T_FLUSH_ALL_CACHES), "", TXT(T_HK_FLUSH_ALL_CACHES), MENU_FUNC flush_caches, (void *)0, 0, 0 },
	{ TXT(T_RESOURCE_INFO), "", TXT(T_HK_RESOURCE_INFO), MENU_FUNC cache_inf, (void *)0, 0, 0 },
#if 0
	TXT(T_CACHE_INFO), "", TXT(T_HK_CACHE_INFO), MENU_FUNC list_cache, (void *)0, 0, 0,
#endif
#ifdef LEAK_DEBUG
	{ TXT(T_MEMORY_INFO), "", TXT(T_HK_MEMORY_INFO), MENU_FUNC memory_info, (void *)0, 0, 0 },
#endif
	{ "", "", M_BAR, NULL, NULL, 0, 0 },
};

struct menu_item file_menu3[] = {
	{ "", "", M_BAR, NULL, NULL, 0, 0 },
        { TXT(T_CLOSE_TAB), "c", TXT(T_HK_CLOSE_TAB), MENU_FUNC menu_close_tab, (void *)0, 0, 0 },
        { TXT(T_EXIT), "q", TXT(T_HK_EXIT), MENU_FUNC exit_prog, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

void do_file_menu(struct terminal *term, void *xxx, struct session *ses)
{
	int x;
	int o;
	struct menu_item *file_menu, *e, *f;
	if (!(file_menu = mem_alloc(sizeof(file_menu11) + sizeof(file_menu12) + sizeof(file_menu21) + sizeof(file_menu22) + sizeof(file_menu3) + 3 * sizeof(struct menu_item)))) return;
	e = file_menu;

        memcpy(e, file_menu11, sizeof(file_menu11));
        e += sizeof(file_menu11) / sizeof(struct menu_item);

        if (!anonymous) {
		memcpy(e, file_menu12, sizeof(file_menu12));
		e += sizeof(file_menu12) / sizeof(struct menu_item);
	}
	if ((o = can_open_in_new(term))) {
		e->text = TXT(T_NEW_WINDOW);
		e->rtext = o - 1 ? ">" : "";
		e->hotkey = TXT(T_HK_NEW_WINDOW);
		e->func = MENU_FUNC open_in_new_window;
		e->data = send_open_new_xterm;
		e->in_m = o - 1;
		e->free_i = 0;
		e++;
	}
	if (!anonymous) {
		if (!F) {
			memcpy(e, file_menu21, sizeof(file_menu21));
			e += sizeof(file_menu21) / sizeof(struct menu_item);
#ifdef G
		} else {
			memcpy(e, file_menu211, sizeof(file_menu211));
			e += sizeof(file_menu211) / sizeof(struct menu_item);
#endif
		}
	}
	memcpy(e, file_menu22, sizeof(file_menu22));
	e += sizeof(file_menu22) / sizeof(struct menu_item);
	/*"", "", M_BAR, NULL, NULL, 0, 0,
	TXT(T_OS_SHELL), "", TXT(T_HK_OS_SHELL), MENU_FUNC menu_shell, NULL, 0, 0,*/
	x = 1;
	if (!anonymous && can_open_os_shell(term->environment)) {
		e->text = TXT(T_OS_SHELL);
		e->rtext = "";
		e->hotkey = TXT(T_HK_OS_SHELL);
		e->func = MENU_FUNC menu_shell;
		e->data = NULL;
		e->in_m = 0;
		e->free_i = 0;
		e++;
		x = 0;
	}
	if (can_resize_window(term->environment)) {
		e->text = TXT(T_RESIZE_TERMINAL);
		e->rtext = "";
		e->hotkey = TXT(T_HK_RESIZE_TERMINAL);
		e->func = MENU_FUNC dlg_resize_terminal;
		e->data = NULL;
		e->in_m = 0;
		e->free_i = 0;
		e++;
		x = 0;
	}
        memcpy(e, file_menu3 + x, sizeof(file_menu3) - x * sizeof(struct menu_item));
	e += sizeof(file_menu3) / sizeof(struct menu_item);
        for (f = file_menu; f < e; f++) f->free_i = 1;
	do_menu(term, file_menu, ses);
}

struct menu_item view_menu[] = {
	{ TXT(T_SEARCH), "/", TXT(T_HK_SEARCH), MENU_FUNC menu_for_frame, (void *)search_dlg, 0, 0 },
	{ TXT(T_SEARCH_BACK), "?", TXT(T_HK_SEARCH_BACK), MENU_FUNC menu_for_frame, (void *)search_back_dlg, 0, 0 },
	{ TXT(T_FIND_NEXT), "n", TXT(T_HK_FIND_NEXT), MENU_FUNC menu_for_frame, (void *)find_next, 0, 0 },
	{ TXT(T_FIND_PREVIOUS), "N", TXT(T_HK_FIND_PREVIOUS), MENU_FUNC menu_for_frame, (void *)find_next_back, 0, 0 },
	{ "", "", M_BAR, NULL, NULL, 0, 0 },
	{ TXT(T_TOGGLE_HTML_PLAIN), "\\", TXT(T_HK_TOGGLE_HTML_PLAIN), MENU_FUNC menu_toggle, NULL, 0, 0 },
	{ TXT(T_DOCUMENT_INFO), "=", TXT(T_HK_DOCUMENT_INFO), MENU_FUNC menu_doc_info, NULL, 0, 0 },
	{ TXT(T_FRAME_AT_FULL_SCREEN), "f", TXT(T_HK_FRAME_AT_FULL_SCREEN), MENU_FUNC menu_for_frame, (void *)set_frame, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item view_menu_anon[] = {
	{ TXT(T_SEARCH), "/", TXT(T_HK_SEARCH), MENU_FUNC menu_for_frame, (void *)search_dlg, 0, 0 },
	{ TXT(T_SEARCH_BACK), "?", TXT(T_HK_SEARCH_BACK), MENU_FUNC menu_for_frame, (void *)search_back_dlg, 0, 0 },
	{ TXT(T_FIND_NEXT), "n", TXT(T_HK_FIND_NEXT), MENU_FUNC menu_for_frame, (void *)find_next, 0, 0 },
	{ TXT(T_FIND_PREVIOUS), "N", TXT(T_HK_FIND_PREVIOUS), MENU_FUNC menu_for_frame, (void *)find_next_back, 0, 0 },
	{ "", "", M_BAR, NULL, NULL, 0, 0 },
	{ TXT(T_TOGGLE_HTML_PLAIN), "\\", TXT(T_HK_TOGGLE_HTML_PLAIN), MENU_FUNC menu_toggle, NULL, 0, 0 },
	{ TXT(T_DOCUMENT_INFO), "=", TXT(T_HK_DOCUMENT_INFO), MENU_FUNC menu_doc_info, NULL, 0, 0 },
	{ TXT(T_FRAME_AT_FULL_SCREEN), "f", TXT(T_HK_FRAME_AT_FULL_SCREEN), MENU_FUNC menu_for_frame, (void *)NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item help_menu[] = {
	{ TXT(T_ABOUT), "", TXT(T_HK_ABOUT), MENU_FUNC menu_about, (void *)0, 0, 0 },
	{ TXT(T_KEYS), "", TXT(T_HK_KEYS), MENU_FUNC menu_keys, (void *)0, 0, 0 },
	{ TXT(T_COPYING), "", TXT(T_HK_COPYING), MENU_FUNC menu_copying, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item setup_menu[] = {
	{ TXT(T_LANGUAGE), ">", TXT(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TXT(T_CHARACTER_SET), ">", TXT(T_HK_CHARACTER_SET), MENU_FUNC charset_list, (void *)1, 1, 0 },
	{ TXT(T_TERMINAL_OPTIONS), "", TXT(T_HK_TERMINAL_OPTIONS), MENU_FUNC terminal_options, NULL, 0, 0 },

        { TXT(T_OPTIONS_MANAGER),"", TXT(T_HK_OPTIONS_MANAGER), MENU_FUNC menu_options_manager, 0, 0},

	{ TXT(T_ASSOCIATIONS), "", TXT(T_HK_ASSOCIATIONS), MENU_FUNC menu_assoc_manager, NULL, 0, 0 },
	{ TXT(T_FILE_EXTENSIONS), "", TXT(T_HK_FILE_EXTENSIONS), MENU_FUNC menu_ext_manager, NULL, 0, 0 },
	{ "", "", M_BAR, NULL, NULL, 0, 0 },
	{ TXT(T_SAVE_OPTIONS), "", TXT(T_HK_SAVE_OPTIONS), MENU_FUNC write_config, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item setup_menu_anon[] = {
	{ TXT(T_LANGUAGE), ">", TXT(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TXT(T_CHARACTER_SET), ">", TXT(T_HK_CHARACTER_SET), MENU_FUNC charset_list, (void *)1, 1, 0 },
	{ TXT(T_TERMINAL_OPTIONS), "", TXT(T_HK_TERMINAL_OPTIONS), MENU_FUNC terminal_options, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

#ifdef G

struct menu_item setup_menu_g[] = {
	{ TXT(T_LANGUAGE), ">", TXT(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
        { TXT(T_OPTIONS_MANAGER),"", TXT(T_HK_OPTIONS_MANAGER), MENU_FUNC menu_options_manager, 0, 0},
        { TXT(T_FONTLIST_MANAGER),"", TXT(T_HK_FONTLIST_MANAGER), MENU_FUNC menu_fontlist_manager, 0, 0},

	{ TXT(T_ASSOCIATIONS), "", TXT(T_HK_ASSOCIATIONS), MENU_FUNC menu_assoc_manager, NULL, 0, 0 },
	{ TXT(T_FILE_EXTENSIONS), "", TXT(T_HK_FILE_EXTENSIONS), MENU_FUNC menu_ext_manager, NULL, 0, 0 },
	{ TXT(T_BLOCKLIST_MANAGER), "", TXT(T_HK_BLOCKLIST_MANAGER), MENU_FUNC menu_blocklist_manager, NULL, 0, 0 },
	{ "", "", M_BAR, NULL, NULL, 0, 0 },
	{ TXT(T_SAVE_OPTIONS), "", TXT(T_HK_SAVE_OPTIONS), MENU_FUNC write_config, NULL, 0, 0 },
        { NULL, NULL, 0, NULL, NULL, 0, 0 }
};

struct menu_item setup_menu_anon_g[] = {
	{ TXT(T_LANGUAGE), ">", TXT(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

#endif

void do_view_menu(struct terminal *term, void *xxx, struct session *ses)
{
	if (!anonymous) do_menu(term, view_menu, ses);
	else do_menu(term, view_menu_anon, ses);
}

void do_setup_menu(struct terminal *term, void *xxx, struct session *ses)
{
#ifdef G
	if (F) if (!anonymous) do_menu(term, setup_menu_g, ses);
	else do_menu(term, setup_menu_anon_g, ses);
	else
#endif
	if (!anonymous) do_menu(term, setup_menu, ses);
	else do_menu(term, setup_menu_anon, ses);
}

struct menu_item main_menu[] = {
	{ TXT(T_FILE), "", TXT(T_HK_FILE), MENU_FUNC do_file_menu, NULL, 1, 1 },
	{ TXT(T_VIEW), "", TXT(T_HK_VIEW), MENU_FUNC do_view_menu, NULL, 1, 1 },
	{ TXT(T_LINK), "", TXT(T_HK_LINK), MENU_FUNC link_menu, NULL, 1, 1 },
	{ TXT(T_DOWNLOADS), "", TXT(T_HK_DOWNLOADS), MENU_FUNC downloads_menu, NULL, 1, 1 },
	{ TXT(T_SETUP), "", TXT(T_HK_SETUP), MENU_FUNC do_setup_menu, NULL, 1, 1 },
	{ TXT(T_HELP), "", TXT(T_HK_HELP), MENU_FUNC do_menu, help_menu, 1, 1 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

/* lame technology rulez ! */

void activate_bfu_technology(struct session *ses, int item)
{
	struct terminal *term = ses->term;
	if(!options_get_bool("hide_menus"))
        do_mainmenu(term, main_menu, ses, item);
}

struct menu_item background_menu[] = {
	{ TXT(T_GO_BACK), "z", TXT(T_HK_GO_BACK), MENU_FUNC menu_go_back, (void *)0, 0, 0 },
	{ TXT(T_HISTORY), ">", TXT(T_HK_HISTORY), MENU_FUNC history_menu, (void *)0, 1, 0 },
        { TXT(T_GO_FORWARD), "`", TXT(T_HK_GO_FORWARD), MENU_FUNC menu_go_forward, (void *)0, 0, 0 },
	{ TXT(T_RELOAD), "Ctrl-R", TXT(T_HK_RELOAD), MENU_FUNC menu_reload, (void *)0, 0, 0 },
	{ TXT(T_STOP), "a", TXT(T_HK_STOP), MENU_FUNC menu_stop, (void *)0, 0, 0 },
	{ "", "", M_BAR, NULL, NULL, 0, 0 },
	{ TXT(T_SAVE_AS), "", TXT(T_HK_SAVE_AS), MENU_FUNC save_as, (void *)0, 0, 0 },
        { TXT(T_COPY_URL_LOCATION), "", TXT(T_HK_COPY_URL_LOCATION), MENU_FUNC send_copy_url_location, (void *)0, 0, 0 },
        { TXT(T_CLOSE_TAB), "c", TXT(T_HK_CLOSE_TAB), MENU_FUNC menu_close_tab, (void *)0, 0, 0 },
        { NULL, NULL, 0, NULL, NULL, 0, 0 }
};

void do_background_menu(struct terminal *term, void *xxx, struct session *ses)
{
	do_menu(term, background_menu, ses);
}

struct history goto_url_history = { 0, { &goto_url_history.items, &goto_url_history.items } };

unsigned char *get_url_with_hook(struct session *ses, unsigned char *url)
{
#ifndef HAVE_LUA
        return stracpy(url);
#else
	lua_State *L = lua_state;
	int err;

	lua_getglobal(L, "goto_url_hook");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
                return stracpy(url);
		return;
	}

	lua_pushstring(L, url);
	if (list_empty(ses->history)) lua_pushnil(L);
	else lua_pushstring(L, cur_loc(ses)->url);

	if (prepare_lua(ses)) return;
	err = lua_call(L, 2, 1);
	finish_lua();
	if (err) return;

        if (lua_isstring(L, -1)) {
            char *lua_url=(unsigned char *) lua_tostring(L, -1);
            char *encoded_url = lua_url;
            return stracpy(encoded_url);
        }
	else if (!lua_isnil(L, -1)) alert_lua_error("goto_url_hook must return a string or nil");
	lua_pop(L, 1);
#endif
        return stracpy("");
}

void dialog_save_url(struct session *ses)
{
	input_field(ses->term, NULL, TXT(T_SAVE_URL), TXT(T_ENTER_URL), TXT(T_OK), TXT(T_CANCEL), ses, &goto_url_history, MAX_INPUT_URL_LEN, "", 0, 0, NULL, (void (*)(void *, unsigned char *)) save_url, NULL);
}

struct history file_history = { 0, { &file_history.items, &file_history.items } };

void query_file(struct session *ses, unsigned char *url, void (*std)(struct session *, unsigned char *), void (*cancel)(struct session *))
{
	unsigned char *file, *def;
	int dfl = 0;
	int l;
	get_filename_from_url(url, &file, &l);
	def = init_str();
        if (options_get("network_download_directory"))
                add_to_str(&def, &dfl, options_get("network_download_directory"));
	if (*def && !dir_sep(def[strlen(def) - 1])) add_chr_to_str(&def, &dfl, '/');
	add_bytes_to_str(&def, &dfl, file, l);
	input_field(ses->term, NULL, TXT(T_DOWNLOAD), TXT(T_SAVE_TO_FILE), TXT(T_OK), TXT(T_CANCEL), ses, &file_history, MAX_INPUT_URL_LEN, def, 0, 0, NULL, (void (*)(void *, unsigned char *))std, (void (*)(void *))cancel);
	mem_free(def);
}

struct history search_history = { 0, { &search_history.items, &search_history.items } };

void search_back_dlg(struct session *ses, struct f_data_c *f, int a)
{
	input_field(ses->term, NULL, TXT(T_SEARCH_BACK), TXT(T_SEARCH_FOR_TEXT), TXT(T_OK), TXT(T_CANCEL), ses, &search_history, MAX_INPUT_URL_LEN, "", 0, 0, NULL, (void (*)(void *, unsigned char *)) search_for_back, NULL);
}

void search_dlg(struct session *ses, struct f_data_c *f, int a)
{
	input_field(ses->term, NULL, TXT(T_SEARCH), TXT(T_SEARCH_FOR_TEXT), TXT(T_OK), TXT(T_CANCEL), ses, &search_history, MAX_INPUT_URL_LEN, "", 0, 0, NULL, (void (*)(void *, unsigned char *)) search_for, NULL);
}

#ifdef HAVE_LUA

struct history lua_console_history = { 0, &lua_console_history.items, &lua_console_history.items };

void dialog_lua_console(struct session *ses)
{
	input_field(ses->term, NULL, TXT(T_LUA_CONSOLE), TXT(T_ENTER_EXPRESSION), TXT(T_OK), TXT(T_CANCEL), ses, &lua_console_history, MAX_INPUT_LUA_LEN, "", 0, 0, NULL, (void (*)(void *, unsigned char *)) lua_console, NULL);
}

#endif

struct history print_history = { 0, &print_history.items, &print_history.items };

void dialog_print(struct session *ses)
{
	input_field(ses->term, NULL, TXT(T_PRINT_TO_FILE), TXT(T_SAVE_TO_FILE), TXT(T_OK), TXT(T_CANCEL), ses, &print_history, MAX_STR_LEN, "", 0, 0, NULL, (void (*)(void *, unsigned char *)) print_to_file, NULL);
}

void free_history_lists()
{
	free_list(goto_url_history.items);
	free_list(file_history.items);
	free_list(search_history.items);
#ifdef JS
	free_list(js_get_string_history.items);   /* is in jsint.c */
#endif
#ifdef HAVE_LUA
	free_list(lua_console_history.items);
#endif
        free_list(print_history.items);
}

/* 'Smart' goto url dialog */

#define LL gf_val(1, G_BFU_FONT_SIZE)

int dialog_goto_url_cancel(struct dialog_data *dlg, struct dialog_item_data *di)
{
	void (*fn)(void *) = di->item->udata;
	void *data = dlg->dlg->udata2;
	if (fn) fn(data);
	cancel_dialog(dlg, di);
	return 0;
}

int dialog_goto_url_ok(struct dialog_data *dlg, struct dialog_item_data *di)
{
	void (*fn)(void *, unsigned char *) = di->item->udata;
        struct session *ses=(struct session*)dlg->dlg->udata2;
        int open_in=*(int*)(dlg->items[1].cdata);
        unsigned char *text=(dlg->items->cdata);
        unsigned char *url;

        if (check_dialog(dlg)) return 1;
        /* add_to_history strips leading and trailing spaces */
        add_to_history(dlg->dlg->items->history, text);
        url=get_url_with_hook(ses,text);

        if(open_in==0)
                goto_url(ses,url);
        else {
                struct open_in_new *oi, *oin=get_open_in_new(dlg->win->term->environment);

                for(oi=oin;--open_in;oi++);
                oi->fn(ses->term,path_to_exe,url);
                mem_free(oin);
        }

        ok_dialog(dlg, di);

        mem_free(url);
        return 0;
}

void dialog_goto_url_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	max_text_width(term, dlg->dlg->udata, &max, AL_LEFT);
	min_text_width(term, dlg->dlg->udata, &min, AL_LEFT);
	max_buttons_width(term, dlg->items + 1, 2, &max);
	min_buttons_width(term, dlg->items + 1, 2, &min);
	if (max < dlg->dlg->items->dlen) max = dlg->dlg->items->dlen;
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	rw = w;
	dlg_format_text(dlg, NULL, dlg->dlg->udata, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, NULL, dlg->items, 0, &y, w, &rw, AL_LEFT);
	y += LL;
	dlg_format_buttons(dlg, NULL, dlg->items + 1, 3, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_text(dlg, term, dlg->dlg->udata, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, dlg->items, dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += LL;
	dlg_format_buttons(dlg, term, dlg->items + 1, 3, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

int dialog_goto_url_menu(struct dialog_data *dlg, struct dialog_item_data *di)
{
        struct session *ses=dlg->dlg->udata2;
        int *ptr=(int *)di->cdata;

        int i, sel;
	unsigned char *n;
	struct menu_item *mi;

        struct open_in_new *oi, *oin=get_open_in_new(dlg->win->term->environment);

        if (!(mi = new_menu(1))) return;

        i=0;
        add_to_menu(&mi, TXT(T_CURRENT_WINDOW), "", "", MENU_FUNC set_val, (void*)i++, 0);

        if (oin) {
                for (oi = oin; oi->text; oi++)
                        add_to_menu(&mi, oi->text, "", oi->hk, MENU_FUNC set_val, (void*)i++, 0);

                mem_free(oin);
        }
        sel = *ptr;
	if (sel < 0) sel = 0;
	do_menu_selected(dlg->win->term, mi, ptr, sel);
}

void dialog_goto_url(struct session *ses, char *url)
{
        struct memory_list *ml=NULL;
        struct dialog *dlg;
	unsigned char *field;
        int l=MAX_INPUT_URL_LEN;

        int *open_in = mem_alloc(sizeof(int));
        *open_in=0;

        if (!(dlg = mem_alloc(sizeof(struct dialog) + 5 * sizeof(struct dialog_item) + l)))
		return;
	memset(dlg, 0, sizeof(struct dialog) + 5 * sizeof(struct dialog_item) + l);
	*(field = (unsigned char *)dlg + sizeof(struct dialog) + 5 * sizeof(struct dialog_item)) = 0;
	if (url) {
                if (strlen(url) + 1 > l) memcpy(field, url, l - 1);
                else strcpy(field, url);
	}
	dlg->title = TXT(T_GOTO_URL);
	dlg->fn = dialog_goto_url_fn;
	dlg->udata = TXT(T_ENTER_URL);
	dlg->udata2 = ses;

        dlg->items[0].type = D_FIELD;
	dlg->items[0].gid = 0;
	dlg->items[0].gnum = 0;
	dlg->items[0].fn = NULL;
	dlg->items[0].history = &goto_url_history;
	dlg->items[0].dlen = l;
	dlg->items[0].data = field;

	dlg->items[1].type = D_BUTTON;
	dlg->items[1].gid = 0;
	dlg->items[1].fn = dialog_goto_url_menu;
        dlg->items[1].text = TXT(T_OPEN_IN);
	dlg->items[1].data = (unsigned char *) open_in;
        dlg->items[1].dlen = sizeof(int);

        dlg->items[2].type = D_BUTTON;
	dlg->items[2].gid = B_ENTER;
	dlg->items[2].fn = dialog_goto_url_ok;
	dlg->items[2].dlen = 0;
	dlg->items[2].text = TXT(T_OK);
        dlg->items[2].udata = NULL;

        dlg->items[3].type = D_BUTTON;
	dlg->items[3].gid = B_ESC;
	dlg->items[3].fn = dialog_goto_url_cancel;
	dlg->items[3].dlen = 0;
	dlg->items[3].text = TXT(T_CANCEL);
	dlg->items[3].udata = NULL;

        dlg->items[4].type = D_END;

        add_to_ml(&ml, dlg, open_in, NULL);
	do_dialog(ses->term, dlg, ml);
}
