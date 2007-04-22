/* session.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

struct list_head downloads = {&downloads, &downloads};

int are_there_downloads()
{
	int d = 0;
	struct download *down;
	foreach(down, downloads) if (!down->prog) d = 1;
	return d;
}

struct list_head sessions = {&sessions, &sessions};

struct strerror_val {
	struct strerror_val *next;
	struct strerror_val *prev;
	unsigned char msg[1];
};

struct list_head strerror_buf = { &strerror_buf, &strerror_buf };

void free_strerror_buf()
{
	free_list(strerror_buf);
}

unsigned char *get_err_msg(int state)
{
	unsigned char *e;
	struct strerror_val *s;
	if (state <= S_OKAY || state >= S_WAIT) {
		int i;
		for (i = 0; msg_dsc[i].msg; i++)
			if (msg_dsc[i].n == state) return msg_dsc[i].msg;
		unk:
		return TXT(T_UNKNOWN_ERROR);
	}
	if (!(e = strerror(-state)) || !*e) goto unk;
	foreach(s, strerror_buf) if (!strcmp(s->msg, e)) return s->msg;
	if (!(s = mem_alloc(sizeof(struct strerror_val) + strlen(e) + 1))) goto unk;
	strcpy(s->msg, e);
	add_to_list(strerror_buf, s);
	return s->msg;
}

void add_xnum_to_str(unsigned char **s, int *l, int n)
{
	unsigned char suff = 0;
	int d = -1;
	if (n >= 1000000) suff = 'M', d = (n / 100000) % 10, n /= 1000000;
	else if (n >= 1000) suff = 'k', d = (n / 100) % 10, n /= 1000;
	add_num_to_str(s, l, n);
	if (n < 10 && d != -1) add_chr_to_str(s, l, '.'), add_num_to_str(s, l, d);
	add_chr_to_str(s, l, ' ');
	if (suff) add_chr_to_str(s, l, suff);
	add_chr_to_str(s, l, 'B');
}

void add_time_to_str(unsigned char **s, int *l, ttime t)
{
	unsigned char q[64];
	t /= 1000;
	t &= 0xffffffff;
	if (t < 0) t = 0;
	if (t >= 86400) sprintf(q, "%dd ", (int)(t / 86400)), add_to_str(s, l, q);
	if (t >= 3600) t %= 86400, sprintf(q, "%d:%02d", (int)(t / 3600), (int)(t / 60 % 60)), add_to_str(s, l, q);
	else sprintf(q, "%d", (int)(t / 60)), add_to_str(s, l, q);
	sprintf(q, ":%02d", (int)(t % 60)), add_to_str(s, l, q);
}

unsigned char *get_stat_msg(struct status *stat, struct terminal *term)
{
	if (stat->state == S_TRANS && stat->prg->elapsed / 100) {
		unsigned char *m = init_str();
		int l = 0;
		add_to_str(&m, &l, _(TXT(T_RECEIVED), term));
		add_to_str(&m, &l, " ");
		add_xnum_to_str(&m, &l, stat->prg->pos);
		if (stat->prg->size >= 0)
			add_to_str(&m, &l, " "), add_to_str(&m, &l, _(TXT(T_OF), term)), add_to_str(&m, &l, " "), add_xnum_to_str(&m, &l, stat->prg->size);
		add_to_str(&m, &l, ", ");
		if (stat->prg->elapsed >= CURRENT_SPD_AFTER * SPD_DISP_TIME)
			add_to_str(&m, &l, _(TXT(T_AVG), term)), add_to_str(&m, &l, " ");
		add_xnum_to_str(&m, &l, stat->prg->loaded * 10 / (stat->prg->elapsed / 100));
		add_to_str(&m, &l, "/s");
		if (stat->prg->elapsed >= CURRENT_SPD_AFTER * SPD_DISP_TIME) 
			add_to_str(&m, &l, ", "), add_to_str(&m, &l, _(TXT(T_CUR), term)), add_to_str(&m, &l, " "),
			add_xnum_to_str(&m, &l, stat->prg->cur_loaded / (CURRENT_SPD_SEC * SPD_DISP_TIME / 1000)),
			add_to_str(&m, &l, "/s");
		return m;
	}
	return stracpy(_(get_err_msg(stat->state), term));
}

void change_screen_status(struct session *ses)
{
	struct status *stat = NULL;
	if (ses->rq) stat = &ses->rq->stat;
	else {
		struct f_data_c *fd = current_frame(ses);
		if (fd->rq) stat = &fd->rq->stat;
		if (stat&& stat->state == S_OKAY && fd->af) {
			struct additional_file *af;
			foreach(af, fd->af->af) {
				if (af->rq && af->rq->stat.state >= 0) stat = &af->rq->stat;
			}
		}
	}
	if (ses->st) mem_free(ses->st);

	/* default status se ukazuje, kdyz 
	 * 			a) by se jinak ukazovalo prazdno
	 * 			b) neni NULL a ukazovalo by se OK
	 */
	ses->st = NULL;
	if (stat) {
		if (stat->state == S_OKAY)ses->st = print_current_link(ses);
		if (!ses->st) ses->st = ses->default_status?stracpy(ses->default_status):get_stat_msg(stat, ses->term);
	} else ses->st = stracpy(ses->default_status);
}

void _print_screen_status(struct terminal *term, struct session *ses)
{
        unsigned char *mini=init_str();
        int l = 0;

        add_to_str(&mini,&l,"| ");
        if(options_get_bool("ministatus_visibility_connecting")){
                add_num_to_str(&mini,&l,connect_info(CI_CONNECTING));
                add_to_str(&mini,&l," ");
        }
        if(options_get_bool("ministatus_visibility_running")){
			add_num_to_str(&mini,&l,connect_info(CI_TRANSFER));
                        add_to_str(&mini,&l," ");
        }
        if(F && options_get_bool("ministatus_visibility_images")){
                add_to_str(&mini,&l,options_get_bool("html_images_display")?"I":" ");
                /*add_to_str(&mini,&l," ");*/
        }
        if(options_get_bool("ministatus_visibility_encoding")){
                int encoding=0;
                struct f_data_c *fd=current_frame(ses);
                struct object_request *rq=fd->rq;
                encoding=(rq && rq->ce && rq->ce->encoding_info);
                add_to_str(&mini,&l,encoding?"E":" ");
                /*add_to_str(&mini,&l," ");*/
        }
#ifdef HAVE_SSL
        if(options_get_bool("ministatus_visibility_ssl")){
                int ssl=0;
                struct f_data_c *fd=current_frame(ses);
                struct object_request *rq=fd->rq;
                ssl=(rq && rq->ce && rq->ce->ssl_info);
                add_to_str(&mini,&l,ssl?"S":" ");
                /*add_to_str(&mini,&l," ");*/
        }
#endif
        if(F && options_get_bool("ministatus_visibility_keyboard")){
                add_to_str(&mini,&l,options_get_bool("keyboard_navigation")?"K":" ");
                add_to_str(&mini,&l," ");
        }
        if(options_get_bool("ministatus_visibility_refresh")){
                add_to_str(&mini,&l,(ses->screen->refresh.timer>=0)?"R":" ");
                /*add_to_str(&mini,&l," ");*/
        }

        if (!F) {
                unsigned char *m = print_current_title(ses);
                int mini_width = strlen(mini);

                fill_area(term, 0, term->y - 1, term->x, 1, COLOR_STATUS);

                if (ses->st)
                        print_text(term, 0, term->y - 1, strlen(ses->st), ses->st, COLOR_STATUS);
                if (mini_width > 2) /* Any indicator enabled? */
                        print_text(term, term->x - mini_width - 1, term->y - 1, mini_width, mini, COLOR_STATUS);

                fill_area(term, 0, 0, term->x, 1, COLOR_TITLE_BG);

                if (m) {
			int p = term->x - 1 - strlen(m);
			if (p < 0) p = 0;
			print_text(term, p, 0, strlen(m), m, COLOR_TITLE);
			mem_free(m);
                }
                /* Draw textmode tabs, graphics are in view_gr.c */
                if(options_get_bool("tabs_show")) {
                        int number = number_of_tabs(term);
                        int tab_width = term->x/number;
                        int tab;
                        int msglen;
                        unsigned char *msg;
                        int selected_color = COLOR_STATUS_BG;
                        int normal_color = COLOR_STATUS;

                        if (number>1 ||
                            options_get_bool("tabs_show_if_single"))
                                for (tab = 0; tab < number; tab++){
                                        struct window *win = get_tab_by_number(term,tab);

                                        if(win->data &&
                                           current_frame(win->data) &&
                                           current_frame(win->data)->f_data &&
                                           current_frame(win->data)->f_data->title &&
                                           strlen(current_frame(win->data)->f_data->title))
                                                msg = current_frame(win->data)->f_data->title;
                                        else
                                                msg = "Untitled";

                                        msglen = strlen(msg);
                                        if(msglen > tab_width)
                                                msglen = tab_width - 1;

                                        fill_area(term,
                                                  tab*tab_width, term->y - 2,
                                                  tab_width, 1,
                                                  (tab == term->current_tab)
                                                  ? selected_color
                                                  : normal_color);
                                        print_text(term,
                                                   tab * tab_width, term->y - 2,
                                                   msglen, msg,
                                                   (tab == term->current_tab)
                                                   ? selected_color
                                                   : normal_color);
                                }
                }

#ifdef G
        } else {
                int y1,y2;
                struct rect old;
                int mini_width=0;

                mini_width=g_text_width(bfu_style_wb,mini);

                l=0;
                y1=term->y-G_BFU_FONT_SIZE;
                y2=term->y;

                restrict_clip_area(term->dev,&old,0,y1,term->x-mini_width,y2);

                if (ses->st)
                        g_print_text(drv, term->dev, 0, y1, bfu_style_bw_b, ses->st, &l);
		drv->fill_area(term->dev, l, y1, term->x-mini_width, y2, bfu_bg_color);
                drv->set_clip_area(term->dev, &old);
                l=0;
                g_print_text(drv, term->dev, term->x-mini_width, y1, bfu_style_bw, mini, &l);
#endif
        }
        mem_free(mini);
}

void print_screen_status(struct session *ses)
{
	unsigned char *m;

        if(ses->win->type && ses->win!=get_root_window(ses->term))
                return;

	if(!options_get_bool("hide_menus"))
        draw_to_window(ses->win, (void (*)(struct terminal *, void *))_print_screen_status, ses);
	if ((m = stracpy("Links"))) {
		if (ses->screen && ses->screen->f_data && ses->screen->f_data->title && ses->screen->f_data->title[0]) add_to_strn(&m, " - "), add_to_strn(&m, ses->screen->f_data->title);
		set_terminal_title(ses->term, m);
		/*mem_free(m); -- set_terminal_title frees it */
	}
        if(need_auth==1){
            need_auth=0;
            do_auth_dialog(ses);
        }
}

void print_error_dialog(struct session *ses, struct status *stat, unsigned char *title)
{
	unsigned char *t = get_err_msg(stat->state);
	unsigned char *u = stracpy(title);
	if (!t) return;
	msg_box(ses->term, getml(u, NULL), TXT(T_ERROR), AL_CENTER | AL_EXTD_TEXT, TXT(T_ERROR_LOADING), " ", u, ":\n\n", t, NULL, ses, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC/*, _("Retry"), NULL, 0 !!! FIXME: retry */);
}

static inline unsigned char hx(int a)
{
	return a >= 10 ? a + 'A' - 10 : a + '0';
}

static inline int unhx(unsigned char a)
{
	if (a >= '0' && a <= '9') return a - '0';
	if (a >= 'A' && a <= 'F') return a - 'A' + 10;
	if (a >= 'a' && a <= 'f') return a - 'a' + 10;
	return -1;
}

unsigned char *encode_url(unsigned char *url)
{
	unsigned char *u = init_str();
	int l = 0;

        for (; *url; url++) {
		if (is_safe_in_shell(*url) && *url != '+') add_chr_to_str(&u, &l, *url);
		else add_chr_to_str(&u, &l, '+'), add_chr_to_str(&u, &l, hx(*url >> 4)), add_chr_to_str(&u, &l, hx(*url & 0xf));
        }
        return u;
}

unsigned char *decode_url(unsigned char *url)
{
	unsigned char *u = init_str();
	int l = 0;
	for (; *url; url++) {
		if (*url != '+' || unhx(url[1]) == -1 || unhx(url[2]) == -1) add_chr_to_str(&u, &l, *url);
		else add_chr_to_str(&u, &l, (unhx(url[1]) << 4) + unhx(url[2])), url += 2;
        }
	return u;
}

int f_is_finished(struct f_data *f)
{
	struct additional_file *af;
	if (!f || f->rq->state >= 0) return 0;
	if (f->fd && f->fd->rq && f->fd->rq->state >= 0) return 0;
	if (f->af) foreach(af, f->af->af) if (af->rq->state >= 0) return 0;
	return 1;
}

static inline int f_need_reparse(struct f_data *f)
{
	struct additional_file *af;
	if (!f || f->rq->state >= 0) return 1;
	if (f->af) foreach(af, f->af->af) if (af->need_reparse) return 1;
	return 0;
}

struct f_data *format_html(struct f_data_c *fd, struct object_request *rq, unsigned char *url, struct document_options *opt, int *cch)
{
	struct f_data *f;
	pr(
	if (cch) *cch = 0;
	if (!rq->ce || !(f = init_formatted(opt))) goto nul;
	f->fd = fd;
	f->ses = fd->ses;
	f->time_to_get = -get_time();
	clone_object(rq, &f->rq);
	if (f->rq->ce) {
		unsigned char *start; unsigned char *end;
		int stl = -1;
		struct additional_file *af;

		if (fd->af) foreach(af, fd->af->af) af->need_reparse = 0;

		get_file(rq, &start, &end);
                if (jsint_get_source(fd, &start, &end)) f->uncacheable = 1;
                if (opt->plain == 2) {
			start = init_str();
			stl = 0;
			add_to_str(&start, &stl, "<img src=\"");
			add_to_str(&start, &stl, f->rq->ce->url);
			add_to_str(&start, &stl, "\">");
			end = start + stl;
		}
		really_format_html(f->rq->ce, start, end, f, fd != fd->ses->screen);
		if (stl != -1) mem_free(start);
		f->use_tag = f->rq->ce->count;
		if (f->af) foreach(af, f->af->af) if (af->rq->ce) af->use_tag = af->rq->ce->count;
	} else f->use_tag = 0;
	f->time_to_get += get_time();
	) nul:return NULL;
	return f;
}

void count_frames(struct f_data_c *fd, int *i)
{
	struct f_data_c *sub;
	if (!fd) return;
	if (fd->f_data) (*i)++;
	foreach(sub, fd->subframes) count_frames(sub, i);
}

long formatted_info(int type)
{
	int i = 0;
	struct session *ses;
	struct f_data *ce;
	switch (type) {
		case CI_FILES:
			foreach(ses, sessions)
				foreach(ce, ses->format_cache) i++;
			/* fall through */
		case CI_LOCKED:
			foreach(ses, sessions)
				count_frames(ses->screen, &i);
			return i;
		default:
			internal("formatted_info: bad request");
	}
	return 0;
}

void f_data_attach(struct f_data_c *fd, struct f_data *f)
{
	struct additional_file *af;
	f->rq->upcall = (void (*)(struct object_request *, void *))fd_loaded;
	f->rq->data = fd;
	if (f->af) foreach(af, f->af->af) {
		af->rq->upcall = (void (*)(struct object_request *, void *))fd_loaded;
		af->rq->data = fd;
        }
}

void detach_f_data(struct f_data **ff)
{
	struct f_data *f = *ff;
	if (!f) return;
	*ff = NULL;
	if (f->frame_desc_link) {
		destroy_formatted(f);
		return;
	}
	f->fd = NULL;
#ifdef G
	f->locked_on = NULL;
	free_list(f->image_refresh);
#endif
	if (f->uncacheable || !f_is_finished(f)) destroy_formatted(f);
	else add_to_list(f->ses->format_cache, f);
}

static inline int is_format_cache_entry_uptodate(struct f_data *f)
{
	struct cache_entry *ce = f->rq->ce;
	struct additional_file *af;
	if (!ce || ce->count != f->use_tag) return 0;
	if (f->af) foreach(af, f->af->af) {
		struct cache_entry *ce = af->rq->ce;
		if (af->need_reparse) if (!ce || ce->count != af->use_tag) return 0;
	}
	return 1;
}

int shrink_format_cache(int u)
{
	static int sc = 0;
	int scc;
	int r = 0;
	struct f_data *f;
	int c = 0;
	struct session *ses;
	foreach(ses, sessions) foreach(f, ses->format_cache) {
		if (u == SH_FREE_ALL || !is_format_cache_entry_uptodate(f)) {
			struct f_data *ff = f;
			f = f->prev;
			del_from_list(ff);
			destroy_formatted(ff);
			r |= ST_SOMETHING_FREED;
		} else c++;
	}
	if (u == SH_FREE_SOMETHING) c = options_get_int("cache_formatted_entries") + 1;
	if (c > options_get_int("cache_formatted_entries")) {
		a:
		scc = sc++;
		foreach (ses, sessions) if (!scc--) {
			foreachback(f, ses->format_cache) {
				struct f_data *ff = f;
				f = f->next;
				del_from_list(ff);
				destroy_formatted(ff);
				r |= ST_SOMETHING_FREED;
				if (--c <= options_get_int("cache_formatted_entries")) goto ret;
			}
			goto q;
		}
		sc = 0;
		goto a;
		q:;
	}
	ret:
	return r | (!c) * ST_CACHE_EMPTY;
}

void init_fcache()
{
	register_cache_upcall(shrink_format_cache, "format");
}

struct f_data *cached_format_html(struct f_data_c *fd, struct object_request *rq, unsigned char *url, struct document_options *opt, int *cch)
{
	struct f_data *f;
	/*if (F) opt->tables = 0;*/

        if (fd->marginwidth != -1) {
		int marg = (fd->marginwidth + G_HTML_MARGIN - 1) / G_HTML_MARGIN;
		if (marg >= 0 && marg < 9) opt->margin = marg;
	}
	if (opt->plain == 2) opt->margin = 0, opt->display_images = 1;
	pr(
           if (!jsint_get_source(fd, NULL, NULL))
               foreach(f, fd->ses->format_cache)
                   if (!strcmp(f->rq->url, url) && !compare_opt(&f->opt, opt)) {
                       if (!is_format_cache_entry_uptodate(f)) {
                           struct f_data *ff = f;
                           f = f->prev;
                           del_from_list(ff);
                           destroy_formatted(ff);
                           continue;
                       }
                       del_from_list(f);
                       f->fd = fd;
                       if (cch) *cch = 1;
                       f_data_attach(fd, f);
                       xpr();
                       return f;
                   }
          );
        f = format_html(fd, rq, url, opt, cch);
	if (f) f->fd = fd;
	shrink_memory(SH_CHECK_QUOTA);
	return f;
}

struct f_data_c *create_f_data_c(struct session *, struct f_data_c *);
void reinit_f_data_c(struct f_data_c *);
struct location *new_location();

void create_new_frames(struct f_data_c *fd, struct frameset_desc *fs, struct document_options *o)
{
	struct location *loc;
	struct frame_desc *frm;
	int c_loc;
	int i;
	int x, y;
	int xp, yp;

	i = 0;
#ifdef JS
	if (fd->onload_frameset_code)mem_free(fd->onload_frameset_code);
	fd->onload_frameset_code=stracpy(fs->onload_code);
#endif
	foreach(loc, fd->loc->subframes) i++;
	if (i != fs->n) {
		while (!list_empty(fd->loc->subframes)) destroy_location(fd->loc->subframes.next);
		c_loc = 1;
	} else {
		c_loc = 0;
		loc = fd->loc->subframes.next;
	}

	yp = fd->yp;
	frm = &fs->f[0];
	for (y = 0; y < fs->y; y++) {
		xp = fd->xp;
		for (x = 0; x < fs->x; x++) {
			struct f_data_c *nfdc;
			if (!(nfdc = create_f_data_c(fd->ses, fd))) return;
			if (c_loc) {
				loc = new_location();
				add_to_list(*(struct list_head *)fd->loc->subframes.prev, loc);
				loc->parent = fd->loc;
				loc->name = stracpy(frm->name);
				loc->url = stracpy(frm->url);
			}
			nfdc->xp = xp; nfdc->yp = yp;
			nfdc->xw = frm->xw;
			nfdc->yw = frm->yw;
			nfdc->loc = loc;
			nfdc->vs = loc->vs;
			if (frm->marginwidth != -1) nfdc->marginwidth = frm->marginwidth;
			else nfdc->marginwidth = fd->marginwidth;
			if (frm->marginheight != -1) nfdc->marginheight = frm->marginheight;
			else nfdc->marginheight = fd->marginheight;
			/*debug("frame: %d %d, %d %d\n", nfdc->xp, nfdc->yp, nfdc->xw, nfdc->yw);*/
			add_to_list(*(struct list_head *)fd->subframes.prev, nfdc);
			if (frm->subframe) {
				create_new_frames(nfdc, frm->subframe, o);
				/*nfdc->f_data = init_formatted(&fd->f_data->opt);*/
				nfdc->f_data = init_formatted(o);
				nfdc->f_data->frame_desc = copy_frameset_desc(frm->subframe);
				nfdc->f_data->frame_desc_link = 1;
			} else {
				if (fd->depth < HTML_MAX_FRAME_DEPTH && loc->url && *loc->url)
					request_object(fd->ses->term, loc->url, fd->loc->url, PRI_FRAME, NC_CACHE, (void (*)(struct object_request *, void *))fd_loaded, nfdc, &nfdc->rq);
			}
			xp += frm->xw + gf_val(1, 0);
			frm++;
			if (!c_loc) loc = loc->next;
		}
		yp += (frm - 1)->yw + gf_val(1, 0);
	}
}

void make_document_options(struct document_options *o)
{
        o->assume_cp = options_get_cp("html_assume_codepage");
        o->hard_assume = options_get_bool("html_hard_codepage");

        o->tables = options_get_bool("html_tables");
        o->images = options_get_bool("html_images");
        o->display_images = options_get_bool("html_images_display");
        o->image_scale = options_get_int("html_images_scale");

        o->font_size = options_get_int("html_font_size");
        o->margin = options_get_int("html_margin");
        o->table_order = options_get_bool("html_table_order");
        o->num_links = options_get_bool("html_links_numbered");

        /* Default colors depends on current mode (graphics/text) */
        if (!F) {
                o->default_fg=options_get_rgb("default_color_fg");
                o->default_bg=options_get_rgb("default_color_bg");
                o->default_link=options_get_rgb("default_color_link");
                o->default_vlink=options_get_rgb("default_color_vlink");
#ifdef G
        } else {
                o->default_fg=options_get_rgb("default_color_fg_g");
                o->default_bg=options_get_rgb("default_color_bg_g");
                o->default_link=options_get_rgb("default_color_link_g");
                o->default_vlink=options_get_rgb("default_color_vlink_g");
#endif
        }
}

void html_interpret(struct f_data_c *fd)
{
	int i;
	int oxw; int oyw; int oxp; int oyp;
	struct f_data_c *sf;
	int cch;
	/*int first = !fd->f_data;*/
	struct document_options o;

        if (!fd ||
            !fd->loc)
                goto d;

        if (fd->f_data) {
		oxw = fd->f_data->opt.xw;
		oyw = fd->f_data->opt.yw;
		oxp = fd->f_data->opt.xp;
		oyp = fd->f_data->opt.yp;
	} else {
		oxw = oyw = oxp = oyp = -1;
	}

        make_document_options(&o);

#ifdef JS
	o.js_enable = options_get_bool("js_enable");
#endif
#ifdef G
	o.aspect_on=aspect_on;
	o.bfu_aspect=bfu_aspect;
#else
	o.aspect_on=0;
	o.bfu_aspect=0;
#endif
	o.plain = fd->vs->plain;
	o.xp = fd->xp;
	o.yp = fd->yp;
	o.xw = fd->xw;
	o.yw = fd->yw;
	if (fd->ses->term->spec) {
		o.col = fd->ses->term->spec->col;
		o.cp = fd->ses->term->spec->charset;
	} else {
		o.col = 3;
		o.cp = 0;
	}

        o.framename = fd->loc->name;
        if (!o.framename)
                o.framename = NULL;

        detach_f_data(&fd->f_data);

        fd->f_data = cached_format_html(fd, fd->rq, fd->rq->url, &o, &cch);
        if (!fd->f_data)
                goto d;

	/* erase frames if changed */
	i = 0;
	foreach(sf, fd->subframes) i++;

        if (i != (fd->f_data->frame_desc ? fd->f_data->frame_desc->n : 0) &&
            (f_is_finished(fd->f_data) || !f_need_reparse(fd->f_data))) {
        rd:
                        foreach(sf, fd->subframes)
                                reinit_f_data_c(sf);
                        free_list(fd->subframes);

                        /* create new frames */
                        if (fd->f_data->frame_desc)
                                create_new_frames(fd, fd->f_data->frame_desc, &fd->f_data->opt);
        } else {
                if (fd->f_data->frame_desc && f_is_finished(fd->f_data)) {
                        if (fd->f_data->opt.xw != oxw ||
                            fd->f_data->opt.yw != oyw ||
                            fd->f_data->opt.xp != oxp ||
			    fd->f_data->opt.yp != oyp) goto rd;
		}
	}

	d:;
	/*draw_fd(fd);*/
}

/* just added */

#ifdef HAVE_LUA
unsigned char *pre_format_html_hook(struct session *ses, unsigned char *url, unsigned char *html, int *len)
{
	lua_State *L = lua_state;
	unsigned char *s = NULL;
	int err;

	lua_getglobal(L, "pre_format_html_hook");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return NULL;
	}

	lua_pushstring(L, url);
	lua_pushlstring(L, html, *len);

	if (prepare_lua(ses)) return NULL;
	err = lua_call(L, 2, 1);
	finish_lua();
        if (err) return NULL;

        if (lua_isstring(L, -1)) {
                *len = lua_strlen(L, -1);
                s = memacpy((unsigned char *) lua_tostring(L, -1), *len);
        }
        else if (!lua_isnil(L, -1)) alert_lua_error("pre_format_html_hook must return a string or nil");
        lua_pop(L, 1);
        return s;
}
#endif


/* end just added */

void html_interpret_recursive(struct f_data_c *f)
{
	struct f_data_c *fd;
	if (f->rq) html_interpret(f);
	foreach(fd, f->subframes) html_interpret_recursive(fd);
}



/* You get a struct_additionl_file. never mem_free it. When you stop
 * using it, just forget the pointer.
 */
struct additional_file *request_additional_file(struct f_data *f, unsigned char *url_)
{
	struct additional_file *af;
	unsigned char *u, *url;
	url = stracpy(url_);
	if ((u = extract_position(url))) mem_free(u);
	if (!f->af) {
		if (!(f->af = f->fd->af)) {
			if (!(f->af = f->fd->af = mem_calloc(sizeof(struct additional_files)))) {
				mem_free(url);
				return NULL;
			}
			f->af->refcount = 1;
			init_list(f->af->af);
		}
		f->af->refcount++;
	}
	foreach (af, f->af->af) if (!strcmp(af->url, url)) {
		mem_free(url);
		return af;
	}
	if (!(af = mem_alloc(sizeof(struct additional_file) + strlen(url) + 1))) return NULL;
	af->use_tag = 0;
	strcpy(af->url, url);
	request_object(f->ses->term, url, f->rq->url, PRI_IMG, NC_CACHE, f->rq->upcall, f->rq->data, &af->rq);
	af->need_reparse = 0;
	add_to_list(f->af->af, af);
	mem_free(url);
	return af;
}

#ifdef G

void image_timer(struct f_data_c *fd)
{
	struct image_refresh *ir;
	struct list_head new;
	init_list(new);
	fd->image_timer = -1;
	if (!fd->f_data) return;
	foreach (ir, fd->f_data->image_refresh) {
		if (ir->t > G_IMG_REFRESH) ir->t -= G_IMG_REFRESH;
		else {
			struct image_refresh *irr = ir->prev;
			del_from_list(ir);
			add_to_list(new, ir);
			ir = irr;
		}
	}
	foreach (ir, new) {
		/*fprintf(stderr, "DRAW: %p\n", ir->img);*/
		draw_one_object(fd, ir->img);
	}
	free_list(new);
	if (fd->image_timer == -1 && !list_empty(fd->f_data->image_refresh)) fd->image_timer = install_timer(G_IMG_REFRESH, (void (*)(void *))image_timer, fd);
}

void refresh_image(struct f_data_c *fd, struct g_object *img, ttime tm)
{
	struct image_refresh *ir;
	if (!fd->f_data) return;
	foreach (ir, fd->f_data->image_refresh) if (ir->img == img) {
		if (ir->t > tm) ir->t = tm;
		return;
	}
	if (!(ir = mem_alloc(sizeof(struct image_refresh)))) return;
	ir->img = img;
	ir->t = tm;
	add_to_list(fd->f_data->image_refresh, ir);
	if (fd->image_timer == -1) fd->image_timer = install_timer(G_IMG_REFRESH, (void (*)(void *))image_timer, fd);
}

#endif

void reinit_f_data_c(struct f_data_c *fd)
{
	struct additional_file *af;
	struct f_data_c *fd1;
#ifdef JS
        jsint_destroy(fd);
#endif
        foreach(fd1, fd->subframes) {
		if (fd->ses->wtd_target_base == fd1) fd->ses->wtd_target_base = NULL;
		reinit_f_data_c(fd1);
		if (fd->ses->wtd_target_base == fd1) fd->ses->wtd_target_base = fd;
		if (fd->ses->defered_target_base == fd1) fd->ses->defered_target_base = fd;
	}
	free_list(fd->subframes);

#ifdef JS
	if (fd->onload_frameset_code)mem_free(fd->onload_frameset_code),fd->onload_frameset_code=NULL;
#endif
	fd->loc = NULL;
	if (fd->f_data && fd->f_data->rq) fd->f_data->rq->upcall = NULL;
	if (fd->f_data && fd->f_data->af) foreach(af, fd->f_data->af->af) af->rq->upcall = NULL;
	if (fd->af) foreach(af, fd->af->af) af->rq->upcall = NULL;
	detach_f_data(&fd->f_data);
	if (fd->link_bg) mem_free(fd->link_bg), fd->link_bg = NULL;
	fd->link_bg_n = 0;
	if (fd->goto_position) mem_free(fd->goto_position), fd->goto_position = NULL;
	release_object(&fd->rq);
	free_additional_files(&fd->af);
	fd->next_update = get_time();
	fd->done = 0;
	fd->parsed_done = 0;
	if (fd->image_timer != -1) kill_timer(fd->image_timer), fd->image_timer = -1;
        fd_refresh_kill(fd);

}

struct f_data_c *create_f_data_c(struct session *ses, struct f_data_c *parent)
{
	static long id = 1;
	struct f_data_c *fd;
	if (!(fd = mem_calloc(sizeof(struct f_data_c)))) return NULL;
	fd->parent = parent;
	fd->ses = ses;
	fd->depth = parent ? parent->depth + 1 : 1;
	init_list(fd->subframes);
	fd->next_update = get_time();
	fd->done = 0;
	fd->parsed_done = 0;
	fd->script_t = 0;
	fd->id = id++;
	fd->marginwidth = fd->marginheight = -1;
	fd->image_timer = -1;
        fd->selection=NULL;

	fd->refresh.timer = -1;
	fd->refresh.url = NULL;
	fd->refresh.state = 0;

	return fd;
}

int plain_type(struct session *ses, struct object_request *rq, unsigned char **p)
{
	struct cache_entry *ce;
	unsigned char *ct;
	int r = 0;
	if (p) *p = NULL;
	if (!rq || !(ce = rq->ce)) {
		r = 1;
                goto f;
	}
	if (!(ct = get_content_type(ce->head, ce->url))) goto f;
	if (!_stricmp(ct, "text/html")) goto ff;
	r = 1;
	if (!_stricmp(ct, "text/plain")) goto ff;
	r = 2;
	/* !!! FIXME: tady by se mel dat test, zda to ten obrazek umi */
	if (F && !casecmp(ct, "image/", 6)) goto ff;
	r = -1;

	ff:
	if (!p) mem_free(ct);
	else *p = ct;
	f:
	return r;
}

int get_file(struct object_request *o, unsigned char **start, unsigned char **end)
{
	struct cache_entry *ce;
	struct fragment *fr;
	*start = *end = NULL;
	if (!o || !o->ce) return 1;
	ce = o->ce;
	defrag_entry(ce);
	fr = ce->frag.next;
	if ((void *)fr == &ce->frag || fr->offset || !fr->length) return 1;
	else *start = fr->data, *end = fr->data + fr->length;
	return 0;
}


static int __frame_and_all_subframes_loaded(struct f_data_c *fd)
{
	struct f_data_c *f;
	int loaded=fd->done||fd->rq==NULL;

	if (loaded)		/* this frame is loaded */
		foreach(f,fd->subframes)
		{
			loaded=__frame_and_all_subframes_loaded(f);
			if (!loaded)break;
		}
	return loaded;
}

void fd_loaded(struct object_request *rq, struct f_data_c *fd)
{
	unsigned char *ct = NULL;
	if (fd->done) {
		if (f_is_finished(fd->f_data)) goto priint;
		else fd->done = 0, fd->parsed_done = 1;
	}
        if (fd->vs->plain == -1 && rq->state != O_WAITING) {
		fd->vs->plain = plain_type(fd->ses, fd->rq, NULL);
        }

	if ((fd->rq->state < 0) && ( f_is_finished(fd->f_data) || !fd->f_data )) {
#ifdef GLOBHIST
		{       /* Add page to global history */
			/* We want to add titles in utf-8 */
			struct conv_table *convert_table =
				get_translation_table(fd->ses->term->spec->charset,
						      get_cp_index("utf-8"));
			unsigned char *title =
				(fd->f_data && fd->f_data->title)
                                ? convert_string(convert_table,
                                                 fd->f_data->title,
                                                 strlen(fd->f_data->title),
                                                 NULL)
                                : stracpy("Untitled");

                        add_global_history_item(cur_loc(fd->ses)->url,
                                                title,
                                                time(NULL));
                        mem_free(title);
		}
#endif

                if (!fd->parsed_done) {
#ifdef HAVE_LUA
                        if (f_is_finished(fd->f_data)){
                                struct session *ses=fd->ses;
                                struct view_state *vs = cur_loc(ses)->vs;
                                struct cache_entry *ce;
                                struct fragment *fr;
                                unsigned char *s;
                                int len;
                                if (!get_cache_entry(fd->loc->url, &ce)) {
                                        defrag_entry(ce);
                                        fr = ce->frag.next;
                                        len = fr->length;
                                        if ((s = pre_format_html_hook(ses, ce->url, fr->data, &len))) {
                                                add_fragment(ce, 0, s, len);
                                                truncate_entry(ce, len, 1);
                                                mem_free(s);
                                        }
                                }
                        }
#endif


                        html_interpret(fd);
                }
		draw_fd(fd);
		/* it may happen that html_interpret requests load of additional file */
		if (!f_is_finished(fd->f_data)) goto more_data;
		fn:
		if (fd->f_data->are_there_scripts) {
#ifdef JS
                    jsint_scan_script_tags(fd);
#endif
                    if (!f_is_finished(fd->f_data)) goto more_data;
		}
		fd->done = 1;
		fd->parsed_done = 0;
#ifdef JS
		jsint_run_queue(fd);
#endif
	} else more_data: if (get_time() >= fd->next_update) {
		ttime t;
		int first = !fd->f_data;
		if (!fd->parsed_done) {
			html_interpret(fd);
			if (fd->rq->state < 0 && !f_need_reparse(fd->f_data)) fd->parsed_done = 1;
		}
		draw_fd(fd);
		if (f_is_finished(fd->f_data)) goto fn;
		t = fd->f_data ? ((fd->parsed_done ? 0 : fd->f_data->time_to_get * DISPLAY_TIME) + fd->f_data->time_to_draw * IMG_DISPLAY_TIME) : 0;
		if (t < DISPLAY_TIME_MIN) t = DISPLAY_TIME_MIN;
		if (first && t > DISPLAY_TIME_MAX_FIRST) t = DISPLAY_TIME_MAX_FIRST;
		fd->next_update = get_time() + t;
	} else {
		change_screen_status(fd->ses);
		print_screen_status(fd->ses);

        }
	priint:
	/* process onload handler of a frameset */
#ifdef JS
	{
		int all_loaded;

		/* go to parent and test if all subframes are loaded, if yes, call onload handler */

		if (!fd->parent) goto hell;	/* this frame has no parent, skip */
		if (!fd->parent->onload_frameset_code)goto hell;	/* no onload handler, skip all this */
		all_loaded=__frame_and_all_subframes_loaded(fd->parent);
		if (!all_loaded) goto hell;
		/* parent has all subframes loaded */
		jsint_execute_code(fd->parent,fd->parent->onload_frameset_code,strlen(fd->parent->onload_frameset_code),-1,-1,-1);
		mem_free(fd->parent->onload_frameset_code), fd->parent->onload_frameset_code=NULL;
	hell:;
	}
#endif

        if (rq && (rq->state == O_FAILED || rq->state == O_INCOMPLETE) && (fd->rq == rq || fd->ses->rq == rq)) print_error_dialog(fd->ses, &rq->stat, rq->url);

        if(ct)
                mem_free(ct);
}

struct location *new_location()
{
	struct location *loc;
	if (!(loc = mem_calloc(sizeof(struct location)))) return NULL;
	init_list(loc->subframes);
	loc->vs = create_vs();
	return loc;
}

static inline struct location *alloc_ses_location(struct session *ses)
{
	struct location *loc = new_location();

        if (!loc) return NULL;

        if (list_empty(ses->history))
		add_to_list(ses->history, loc);
	else
		add_at_pos(cur_loc(ses)->prev, loc);
	ses->current_loc = loc;

        return loc;
}

void subst_location(struct f_data_c *fd, struct location *old, struct location *new)
{
	struct f_data_c *f;
	foreach(f, fd->subframes) subst_location(f, old, new);
	if (fd->loc == old) fd->loc = new;
}

struct location *copy_sublocations(struct session *ses, struct location *d, struct location *s, struct location *x)
{
	struct location *dl, *sl, *y, *z;
	d->name = stracpy(s->name);
	if (s == x) return d;
	d->url = stracpy(s->url);
	d->prev_url = stracpy(s->prev_url);
	destroy_vs(d->vs);
	d->vs = s->vs; s->vs->refcount++;
	subst_location(ses->screen, s, d);
	y = NULL;
	foreach(sl, s->subframes) {
		if ((dl = new_location())) {
			add_to_list(*(struct list_head *)d->subframes.prev, dl);
			dl->parent = d;
			z = copy_sublocations(ses, dl, sl, x);
			if (z && y) internal("copy_sublocations: crossed references");
			if (z) y = z;
		}
	}
	return y;
}

struct location *copy_location(struct session *ses, struct location *loc)
{
	struct location *l2, *l1, *nl;
	l1 = cur_loc(ses);
	if (!(l2 = alloc_ses_location(ses))) return NULL;
	if (!(nl = copy_sublocations(ses, l2, l1, loc))) internal("copy_location: sublocation not found");
	return nl;
}

struct f_data_c *new_main_location(struct session *ses)
{
	struct location *loc;
	if (!(loc = alloc_ses_location(ses))) return NULL;
	reinit_f_data_c(ses->screen);
	ses->screen->loc = loc;
	ses->screen->vs = loc->vs;
	return ses->screen;
}

struct f_data_c *copy_location_and_replace_frame(struct session *ses, struct f_data_c *fd)
{
	struct location *loc;
	if (!(loc = copy_location(ses, fd->loc))) return NULL;
	reinit_f_data_c(fd);
	fd->loc = loc;
	fd->vs = loc->vs;
	return fd;
}

struct f_data_c *find_frame(struct session *ses, unsigned char *target, struct f_data_c *base)
{
	struct f_data_c *f, *ff;
	if (!target || !*target) return NULL;
	if (!base) base = ses->screen;
	if (!_stricmp(target, "_top") || !_stricmp(target, "_blank"))
		/* "_blank" sucks. Do not automatically open new windows */
		return ses->screen;
	if (!_stricmp(target, "_self")) return base;
	if (!_stricmp(target, "_parent")) {
		for (ff = base->parent; ff && !ff->rq; ff = ff->parent) ;
		return ff ? ff : ses->screen;
	}
	f = ses->screen;
	if (f->loc && f->loc->name && !_stricmp(f->loc->name, target)) return f;
	d:
	foreach(ff, f->subframes) if (ff->loc && ff->loc->name && !_stricmp(ff->loc->name, target)) return ff;
	if (!list_empty(f->subframes)) {
		f = f->subframes.next;
		goto d;
	}
	u:
	if (!f->parent) return NULL;
	if (f->next == (void *)&f->parent->subframes) {
		f = f->parent;
		goto u;
	}
	f = f->next;
	goto d;
}

void destroy_location(struct location *loc)
{
	while (loc->subframes.next != &loc->subframes) destroy_location(loc->subframes.next);
	del_from_list(loc);
	if (loc->name) mem_free(loc->name);
	if (loc->url) mem_free(loc->url);
	if (loc->prev_url) mem_free(loc->prev_url);
	destroy_vs(loc->vs);
	mem_free(loc);
}

void ses_go_forward(struct session *ses, int plain)
{
	struct f_data_c *fd;
	if (ses->search_word) mem_free(ses->search_word), ses->search_word = NULL;
	if ((fd = find_frame(ses, ses->wtd_target, ses->wtd_target_base))) fd = copy_location_and_replace_frame(ses, fd);
	else fd = new_main_location(ses);
	if (!fd) return;
	if (ses->history.next != ses->history.prev) {
		struct location *l;
		while((l = cur_loc(ses)->prev) != (void *) &ses->history) destroy_location(l);
	}
	fd->vs->plain = plain;
	ses->wtd = NULL;
	fd->rq = ses->rq; ses->rq = NULL;
	fd->goto_position = ses->goto_position; ses->goto_position = NULL;
	fd->loc->url = stracpy(fd->rq->url);
	fd->loc->prev_url = stracpy(fd->rq->prev_url);
	fd->rq->upcall = (void (*)(struct object_request *, void *))fd_loaded;
	fd->rq->data = fd;
	ses->locked_link = 0;
	fd->rq->upcall(fd->rq, fd);
}

void ses_go_backward(struct session *ses)
{
	if (ses->search_word) mem_free(ses->search_word), ses->search_word = NULL;
	reinit_f_data_c(ses->screen);

        ses->locked_link = 0;
	ses->screen->loc = cur_loc(ses);
	ses->screen->vs = ses->screen->loc->vs;
	ses->wtd = NULL;
	ses->screen->rq = ses->rq; ses->rq = NULL;
	ses->screen->rq->upcall = (void (*)(struct object_request *, void *))fd_loaded;
	ses->screen->rq->data = ses->screen;
	ses->screen->rq->upcall(ses->screen->rq, ses->screen);

}

void tp_cancel(struct session *ses)
{
	release_object(&ses->tq);
}

void download_data(struct status *stat, struct download *down);

unsigned char *get_temp_name(unsigned char *url);

static void continue_download_do(struct terminal *, int, void *, int);

struct codw_hop {
	struct session *ses;
	unsigned char *real_file;
	unsigned char *file;
};

static void continue_download(struct session *ses, unsigned char *file)
{
	struct codw_hop *codw_hop;
	unsigned char *url = ses->tq->url;

	if (!url) return;

	if (ses->tq_prog) {
		file = get_temp_name(url);
		if (!file) {
			tp_cancel(ses);
			return;
		}
	}

	codw_hop = mem_calloc(sizeof(struct codw_hop));
	if (!codw_hop) return; /* XXX: Something for mem_free()...? --pasky */
	codw_hop->ses = ses;
	codw_hop->file = file;

	kill_downloads_to_file(file);

	create_download_file(ses->term, file, &codw_hop->real_file,
			!!ses->tq_prog, 0, continue_download_do, codw_hop);
}

static void continue_download_do(struct terminal *term, int fd, void *data, int resume)
{
	struct codw_hop *codw_hop = data;
	struct download *down = NULL;
	unsigned char *url = codw_hop->ses->tq->url;

	if (!codw_hop->real_file) goto cancel;

	down = mem_calloc(sizeof(struct download));
	if (!down) goto cancel;

	down->url = stracpy(url);
	if (!down->url) goto cancel;

	down->file = codw_hop->real_file;

	down->stat.end = (void (*)(struct status *, void *)) download_data;
	down->stat.data = down;
	down->last_pos = 0;
	down->handle = fd;
	down->ses = codw_hop->ses;

	if (codw_hop->ses->tq_prog) {
		down->prog = subst_file(codw_hop->ses->tq_prog, codw_hop->file);
		mem_free(codw_hop->file);
		mem_free(codw_hop->ses->tq_prog);
		codw_hop->ses->tq_prog = NULL;
	}

	down->prog_flags = codw_hop->ses->tq_prog_flags;
	add_to_list(downloads, down);

	release_object_get_stat(&codw_hop->ses->tq, &down->stat, PRI_DOWNLOAD);
	display_download(codw_hop->ses->term, down, codw_hop->ses);
	mem_free(codw_hop);
	return;

cancel:
	tp_cancel(codw_hop->ses);
	if (codw_hop->ses->tq_prog && codw_hop->file) mem_free(codw_hop->file);
	if (down) {
		if (down->url) mem_free(down->url);
		mem_free(down);
	}
	mem_free(codw_hop);
}


void tp_save(struct session *ses)
{
	if (ses->tq_prog) mem_free(ses->tq_prog), ses->tq_prog = NULL;
	query_file(ses, ses->tq->url, continue_download, tp_cancel);
}

void tp_open(struct session *ses)
{
	continue_download(ses, "");
}

int ses_abort_1st_state_loading(struct session *ses)
{
	int r = !!ses->rq;
	release_object(&ses->rq);
	ses->wtd = NULL;
	if (ses->wtd_target) mem_free(ses->wtd_target), ses->wtd_target = NULL;
	ses->wtd_target_base = NULL;
	if (ses->goto_position) mem_free(ses->goto_position), ses->goto_position = NULL;
	change_screen_status(ses);
	print_screen_status(ses);
	return r;
}

void tp_display(struct session *ses)
{
	ses_abort_1st_state_loading(ses);
	ses->rq = ses->tq;
	ses->tq = NULL;
	ses_go_forward(ses, 1);
}

int prog_sel_save(struct dialog_data *dlg, struct dialog_item_data* idata)
{
	struct session *ses=(struct session *)(dlg->dlg->udata2);

	if (ses->tq_prog) mem_free(ses->tq_prog), ses->tq_prog = NULL;
	query_file(ses, ses->tq->url, continue_download, tp_cancel);

	cancel_dialog(dlg,idata);
	return 0;
}

int prog_sel_display(struct dialog_data *dlg, struct dialog_item_data* idata)
{
	struct session *ses=(struct session *)(dlg->dlg->udata2);

	ses_abort_1st_state_loading(ses);
	ses->rq = ses->tq;
	ses->tq = NULL;
	ses_go_forward(ses, 1);

	cancel_dialog(dlg,idata);
	return 0;
}

int prog_sel_cancel(struct dialog_data *dlg, struct dialog_item_data* idata)
{
	struct session *ses=(struct session *)(dlg->dlg->udata2);

	release_object(&ses->tq);
	cancel_dialog(dlg,idata);
	return 0;
}


int prog_sel_open(struct dialog_data *dlg, struct dialog_item_data* idata)
{
	struct assoc *a=(struct assoc*)idata->item->udata;
	struct session *ses=(struct session *)(dlg->dlg->udata2);

	if (!a) internal("This should not happen.\n");
	ses->tq_prog = stracpy(a->prog), ses->tq_prog_flags = a->block;
	tp_open(ses);
	cancel_dialog(dlg,idata);
	return 0;
}


void vysad_dvere(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
/*	struct session *ses=(struct session *)(dlg->dlg->udata2);*/
	unsigned char *ct=(unsigned char *)(dlg->dlg->udata);
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	unsigned char *txt;
	int l=0;

	txt=init_str();
	
        /* brainovi to tady pada !!!
        add_to_str(&txt,&l,ses->tq->url);
	add_to_str(&txt,&l," ");
	add_to_str(&txt,&l,_(TXT(T_HAS_TYPE),term));
        */
        add_to_str(&txt,&l,_(TXT(T_CONTEN_TYPE_IS),term));
        add_to_str(&txt,&l," ");
	add_to_str(&txt,&l,ct);
	add_to_str(&txt,&l,".\n");
	if (!anonymous)
		add_to_str(&txt,&l,_(TXT(T_DO_YOU_WANT_TO_OPEN_SAVE_OR_DISPLAY_THIS_FILE),term));
	else
		add_to_str(&txt,&l,_(TXT(T_DO_YOU_WANT_TO_OPEN_OR_DISPLAY_THIS_FILE),term));

	max_text_width(term, txt, &max, AL_CENTER);
	min_text_width(term, txt, &min, AL_CENTER);
	max_buttons_width(term, dlg->items , dlg->n, &max);
	min_buttons_width(term, dlg->items , dlg->n, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;
        dlg_format_text(dlg, NULL, txt, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_CENTER);
	y += gf_val(1, 1 * G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items , dlg->n, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, txt, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_CENTER);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items , dlg->n, dlg->x + DIALOG_LB, &y, w, &rw, AL_CENTER);
	mem_free(txt);
}

void vysad_okno(struct session *ses, unsigned char *ct, struct assoc *a, int n)
{
	int i;
	struct dialog *d;
	struct memory_list *ml;

	if (!(d = mem_alloc(sizeof(struct dialog) + (n+3+(!anonymous)) * sizeof(struct dialog_item)))) return;
	memset(d, 0, sizeof(struct dialog) + (n+3+(!anonymous)) * sizeof(struct dialog_item));
	d->title = TXT(T_WHAT_TO_DO);
	d->fn = vysad_dvere;
	d->udata = ct;
	d->udata2=ses;
	ml=getml(d,a,ct,NULL);

	for (i=0;i<n;i++)
	{
		unsigned char *bla=stracpy(_(TXT(T_OPEN_WITH),ses->term));
		add_to_strn(&bla," ");
		add_to_strn(&bla,_(a[i].label,ses->term));
		
		d->items[i].type = D_BUTTON;
		d->items[i].fn = prog_sel_open;
		d->items[i].udata = a+i;
		d->items[i].text = bla;
		add_to_ml(&ml,bla,NULL);
	}
	if (!anonymous)
	{
		d->items[i].type = D_BUTTON;
		d->items[i].fn = prog_sel_save;
		d->items[i].text = TXT(T_SAVE);
		i++;
	}
	d->items[i].type = D_BUTTON;
	d->items[i].fn = prog_sel_display;
	d->items[i].text = TXT(T_DISPLAY);
	i++;
	d->items[i].type = D_BUTTON;
	d->items[i].fn = prog_sel_cancel;
	d->items[i].gid = B_ESC;
	d->items[i].text = TXT(T_CANCEL);
	d->items[i+1].type = D_END;
 	do_dialog(ses->term, d, ml);
}


/* odalokovava a */
void type_query(struct session *ses, unsigned char *ct, struct assoc *a, int n)
{
	unsigned char *m1;
	unsigned char *m2;
	if (ses->tq_prog) mem_free(ses->tq_prog), ses->tq_prog = NULL;

	if (n>1){vysad_okno(ses,ct,a,n);return;}
	
	if (a) ses->tq_prog = stracpy(a[0].prog), ses->tq_prog_flags = a[0].block;
	if (a && !a[0].ask) {
		tp_open(ses);
		if (n)mem_free(a);
		if (ct)mem_free(ct);
		return;
	}
	m1 = stracpy(ct);
	if (!a) {
		if (!anonymous) msg_box(ses->term, getml(m1, NULL), TXT(T_UNKNOWN_TYPE), AL_CENTER | AL_EXTD_TEXT, TXT(T_CONTEN_TYPE_IS), " ", m1, ".\n", TXT(T_DO_YOU_WANT_TO_SAVE_OR_DISLPAY_THIS_FILE), NULL, ses, 3, TXT(T_SAVE), tp_save, B_ENTER, TXT(T_DISPLAY), tp_display, 0, TXT(T_CANCEL), tp_cancel, B_ESC);
		else msg_box(ses->term, getml(m1, NULL), TXT(T_UNKNOWN_TYPE), AL_CENTER | AL_EXTD_TEXT, TXT(T_CONTEN_TYPE_IS), " ", m1, ".\n", TXT(T_DO_YOU_WANT_TO_SAVE_OR_DISLPAY_THIS_FILE), NULL, ses, 2, TXT(T_DISPLAY), tp_display, B_ENTER, TXT(T_CANCEL), tp_cancel, B_ESC);
	} else {
		m2 = stracpy(a[0].label ? a[0].label : (unsigned char *)"");
		if (!anonymous) msg_box(ses->term, getml(m1, m2, NULL), TXT(T_WHAT_TO_DO), AL_CENTER | AL_EXTD_TEXT, TXT(T_CONTEN_TYPE_IS), " ", m1, ".\n", TXT(T_DO_YOU_WANT_TO_OPEN_FILE_WITH), " ", m2, ", ", TXT(T_SAVE_IT_OR_DISPLAY_IT), NULL, ses, 4, TXT(T_OPEN), tp_open, B_ENTER, TXT(T_SAVE), tp_save, 0, TXT(T_DISPLAY), tp_display, 0, TXT(T_CANCEL), tp_cancel, B_ESC);
		else msg_box(ses->term, getml(m1, m2, NULL), TXT(T_WHAT_TO_DO), AL_CENTER | AL_EXTD_TEXT, TXT(T_CONTEN_TYPE_IS), " ", m1, ".\n", TXT(T_DO_YOU_WANT_TO_OPEN_FILE_WITH), " ", m2, ", ", TXT(T_SAVE_IT_OR_DISPLAY_IT), NULL, ses, 3, TXT(T_OPEN), tp_open, B_ENTER, TXT(T_DISPLAY), tp_display, 0, TXT(T_CANCEL), tp_cancel, B_ESC);
	}
	if (n)mem_free(a);
	if (ct)mem_free(ct);
}

void ses_go_to_2nd_state(struct session *ses)
{
	struct assoc *a;
	int n;
	unsigned char *ct = NULL;
	int r = plain_type(ses, ses->rq, &ct);
	if (r == 0 || r == 1 || r == 2 || r == 3) goto go;
	if (!(a = get_type_assoc(ses->term, ct, &n)) && strlen(ct) >= 4 && !casecmp(ct, "text/", 4)) {
		r = 1;
		goto go;
	}
	if (ses->tq) {
		ses_abort_1st_state_loading(ses);
		if (n)mem_free(a);
		return;
	}
	(ses->tq = ses->rq)->upcall = NULL;
	ses->rq = NULL;
	ses_abort_1st_state_loading(ses);
	type_query(ses, ct, a, n);
	return;
	go:
	ses_go_forward(ses, r);
	if (ct) mem_free(ct);
}

void ses_go_back_to_2nd_state(struct session *ses)
{
	ses_go_backward(ses);
}

void ses_finished_1st_state(struct object_request *rq, struct session *ses)
{
	switch (rq->state) {
		case O_WAITING:
			change_screen_status(ses);
			print_screen_status(ses);
			break;
		case O_FAILED:
			print_error_dialog(ses, &rq->stat, rq->url);
			ses_abort_1st_state_loading(ses);
			break;
		case O_LOADING:
		case O_INCOMPLETE:
		case O_OK:
			ses->wtd(ses);
			break;
	}
}

void ses_destroy_defered_jump(struct session *ses)
{
	if (ses->defered_url) mem_free(ses->defered_url), ses->defered_url = NULL;
	if (ses->defered_target) mem_free(ses->defered_target), ses->defered_target = NULL;
	ses->defered_target_base = NULL;
}

#ifdef JS
/* test if there're any running scripts */
static inline int __any_running_scripts(struct f_data_c *fd)
{
	if (!fd->js)return 0;
	return (fd->js->active)||(!list_empty(fd->js->queue));
}
#endif

/* if from_goto_dialog is 1, set prev_url to NULL */
/* new_tab>0 means reauest for new tab */
void goto_url_f(struct session *ses, void (*state2)(struct session *), unsigned char *url, unsigned char *target, struct f_data_c *df, int data, int defer, int from_goto_dialog, int new_tab)
{
	unsigned char *u, *pos;
	unsigned char *prev_url;
	void (*fn)(struct session *, unsigned char *);

        /* This code need to be cleaned up??? */
        if(new_tab) {
                int base = 0;

                /* Just a hack. --karpov */
                open_in_new_tab(ses->term, "", url);
                return;
        }

        if (!state2) state2 = ses_go_to_2nd_state;
	ses_destroy_defered_jump(ses);
        /*
        if(ses->screen)
                fd_refresh_kill(ses->screen);
        */
	if ((fn = get_external_protocol_function(url))) {
		fn(ses, url);
		return;
        }
	ses->reloadlevel = NC_PR_NO_CACHE;
	if (!(u = translate_url(url, ses->term->cwd))) {
		struct status stat = { NULL, NULL, NULL, NULL, S_BAD_URL, PRI_CANCEL, 0, NULL, NULL };
		print_error_dialog(ses, &stat, url);
		return;
        }
#ifdef JS
	if (defer && __any_running_scripts(ses->screen) ) {
		ses->defered_url = u;
		ses->defered_target = stracpy(target);
		ses->defered_target_base = df;
		ses->defered_data=data;
		return;
	}
#endif
	pos = extract_position(u);
        if (ses->wtd == state2 &&
            !strcmp(ses->rq->orig_url, u) &&
            !xstrcmp(ses->wtd_target, target) &&
            ses->wtd_target_base == df) {
		mem_free(u);
		if (ses->goto_position) mem_free(ses->goto_position);
		ses->goto_position = pos;
		return;
	}
	ses_abort_1st_state_loading(ses);
	ses->wtd = state2;
	ses->wtd_target = stracpy(target);
	ses->wtd_target_base = df;
	if (ses->goto_position) mem_free(ses->goto_position);
	ses->goto_position = pos;
	if (ses->default_status){mem_free(ses->default_status);ses->default_status=NULL;}	/* smazeme default status, aby neopruzoval na jinych strankach */
	if (!from_goto_dialog&&df&&(df->rq))prev_url=df->rq->url;
	else prev_url=NULL;   /* previous page is empty - this probably never happens, but for sure */
	request_object(ses->term, u, prev_url, PRI_MAIN, NC_CACHE, (void (*)(struct object_request *, void *))ses_finished_1st_state, ses, &ses->rq);
	mem_free(u);
}

/* this doesn't send rederer */
void goto_url(struct session *ses, unsigned char *url)
{
	goto_url_f(ses, NULL, url, NULL, NULL, -1, 0, 1, 0);
}

/* this one sends referer */
void goto_url_not_from_dialog(struct session *ses, unsigned char *url)
{
	goto_url_f(ses, NULL, url, NULL, NULL, -1, 0, 0, 0);
}

void ses_imgmap(struct session *ses)
{
	unsigned char *start, *end;
	struct memory_list *ml;
	struct menu_item *menu;
	if (ses->rq->state != O_OK && ses->rq->state != O_INCOMPLETE) return;
	get_file(ses->rq, &start, &end);
	if (get_image_map(ses->rq->ce && ses->rq->ce->head ? ses->rq->ce->head : (unsigned char *)"", start, end, ses->goto_position, &menu, &ml, ses->imgmap_href_base, ses->imgmap_target_base, ses->term->spec->charset, options_get_cp("html_assume_codepage"), options_get_bool("html_hard_codepage"), 0)) {
		ses_abort_1st_state_loading(ses);
		return;
	}
	add_empty_window(ses->term, (void (*)(void *))freeml, ml);
	do_menu(ses->term, menu, ses);
	ses_abort_1st_state_loading(ses);
}

void goto_imgmap(struct session *ses, unsigned char *url, unsigned char *href, unsigned char *target)
{
	if (ses->imgmap_href_base) mem_free(ses->imgmap_href_base);
	ses->imgmap_href_base = href;
	if (ses->imgmap_target_base) mem_free(ses->imgmap_target_base);
	ses->imgmap_target_base = target;
	goto_url_f(ses, ses_imgmap, url, NULL, NULL, -1, 0, 0, 0);
}

void map_selected(struct terminal *term, struct link_def *ld, struct session *ses)
{
	int x = 0;
	if (ld->onclick) {
		struct f_data_c *fd = current_frame(ses);
#ifdef JS
                jsint_execute_code(fd, ld->onclick, strlen(ld->onclick), -1, -1, -1);
#endif
                x = 1;
	}
	goto_url_f(ses, NULL, ld->link, ld->target, current_frame(ses), -1, x, 0, 0);
}

void go_forward(struct session *ses)
{
	struct location *loc;
	if (list_empty(ses->history) || ses->history.next == ses->history.prev)
		return;
	loc = cur_loc(ses)->prev;
	if (loc == (void *) &ses->history) return;
        	
	ses->reloadlevel = NC_CACHE;
	ses_destroy_defered_jump(ses);
        /*
        if(ses->screen)
                fd_refresh_kill(ses->screen);
        */
	if (ses_abort_1st_state_loading(ses)) {		/* ?? does not hurt */
		change_screen_status(ses);
		print_screen_status(ses);
		return;
	}
	
	ses->current_loc = loc;
	ses->wtd = ses_go_back_to_2nd_state;
	if (ses->default_status){mem_free(ses->default_status);ses->default_status=NULL;}	/* smazeme default status, aby neopruzoval na jinych strankach */
	request_object(ses->term, loc->url, loc->prev_url, PRI_MAIN, NC_ALWAYS_CACHE, (void (*)(struct object_request *, void *))ses_finished_1st_state, ses, &ses->rq);
}

void go_back(struct session *ses, struct location *loc)
{
        ses->reloadlevel = NC_PR_NO_CACHE;
	ses_destroy_defered_jump(ses);
        /*
        if(ses->screen)
                fd_refresh_kill(ses->screen);
        */
	if (ses_abort_1st_state_loading(ses)) {
		change_screen_status(ses);
		print_screen_status(ses);
		return;
	}
	if (ses->history.next == &ses->history || ses->history.next == ses->history.prev)
		return;

        {
            struct location *l = cur_loc(ses);
            if (loc == l) return;
	    if (!loc) {
	       if (l->next == (void *)&ses->history) return;
	       loc = (struct location *)l->next;
	    }
	    ses->current_loc = loc;
	}	

        ses->wtd = ses_go_back_to_2nd_state;
	if (ses->default_status){mem_free(ses->default_status);ses->default_status=NULL;}	/* smazeme default status, aby neopruzoval na jinych strankach */
	request_object(ses->term, loc->url, loc->prev_url, PRI_MAIN, NC_ALWAYS_CACHE, (void (*)(struct object_request *, void *))ses_finished_1st_state, ses, &ses->rq);
}

void reload_frame(struct f_data_c *fd, int no_cache)
{
	unsigned char *u;
	if (!list_empty(fd->subframes)) {
		struct f_data_c *fdd;
		foreach(fdd, fd->subframes) {
			reload_frame(fdd, no_cache);
		}
		return;
	}
	if (!fd->f_data || !f_is_finished(fd->f_data)) return;
	u = stracpy(fd->rq->url);
	release_object(&fd->rq);
	release_object(&fd->f_data->rq);
	request_object(fd->ses->term, u, NULL, PRI_MAIN, no_cache, (void (*)(struct object_request *, void *))fd_loaded, fd, &fd->rq);
        clone_object(fd->rq, &fd->f_data->rq);
	fd->done = 0;
	fd->parsed_done = 0;
	mem_free(u);
#ifdef JS
        jsint_destroy(fd);
#endif
}

void reload(struct session *ses, int no_cache)
{
	ses_destroy_defered_jump(ses);
        /*
        if(ses->screen)
                fd_refresh_kill(ses->screen);
        */
	if (no_cache == -1) no_cache = ++ses->reloadlevel;
	else ses->reloadlevel = no_cache;
	reload_frame(ses->screen, no_cache);
	/*request_object(ses->term, cur_loc(ses)->url, cur_loc(ses)->prev_url, PRI_MAIN, no_cache, (void (*)(struct object_request *, void *))ses_finished_1st_state, ses, &ses->rq);*/
}

void set_doc_view(struct session *ses)
{
	if(options_get_bool("hide_menus")) {
		ses->screen->xp = 0;
		ses->screen->yp = 0;
		ses->screen->xw = ses->term->x;
		ses->screen->yw = ses->term->y;
		return;
	}
	ses->screen->xp = 0;
        /* One line skip at top */
        ses->screen->yp = gf_val(1, G_BFU_FONT_SIZE);
	ses->screen->xw = ses->term->x;
        /* One more line skip at bottom */
        ses->screen->yw = ses->term->y - gf_val(2, 2 * G_BFU_FONT_SIZE);
        /* And one more for tabbar if needed */
        if(options_get_bool("tabs_show") &&
           (options_get_bool("tabs_show_if_single") ||
            number_of_tabs(ses->term)>1))
                ses->screen->yw -= gf_val(1, G_BFU_FONT_SIZE);
}

struct session *create_basic_session(struct window *win)
{
        struct session *ses = mem_calloc(sizeof(struct session));
	static int session_id = 1;

        if(ses){
                init_list(ses->history);
		ses->term = win->term;
		ses->win = win;
		ses->id = session_id++;
		ses->screen = create_f_data_c(ses, NULL);
                set_doc_view(ses);
		init_list(ses->format_cache);
		add_to_list(sessions, ses);
        }

        return ses;
}

struct session *create_session(struct window *win)
{
	struct terminal *term = win->term;
	struct session *ses = create_basic_session(win);

        /* Print 'Welcome to Links' message */
        if (first_use) {
		first_use = 0;
		msg_box(term, NULL, TXT(T_WELCOME), AL_CENTER | AL_EXTD_TEXT, TXT(T_WELCOME_TO_LINKS), "\n\n", TXT(T_BASIC_HELP), NULL, NULL, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
	}
	return ses;
}

/*
 Session info format:
 int base_section;
 int url_length;
 char url[url_length];
 struct terminal *term;

 negative base_session means that we request new tab instead of new window
 */

void *create_session_info(int cp, unsigned char *url, int *ll, struct terminal *term)
{
	int l = strlen(url);
	int *i;
	*ll = 2 * sizeof(int) + l + sizeof(struct terminal*);
	if (!(i = mem_alloc(2 * sizeof(int) + l + sizeof(struct terminal*)))) return NULL;
	i[0] = cp;
	i[1] = l;
	memcpy(i + 2, url, l);
        *((struct terminal **)((char *) i + 2 * sizeof(int) + l)) = term;
        return i;
}

int read_session_info(struct session *ses, void *data, int len)
{
	unsigned char *h;
	int cpfrom, sz;
	struct session *s;
	if (len < 2 * sizeof(int)) return -1;
	cpfrom = *(int *)data;
	sz = *((int *)data + 1);
        /* Uncomment this to open current location in new window */
#if 0
        foreach(s, sessions) if (s->id == cpfrom) {
		if (!list_empty(s->history)) {
 /*BOOK0018010x000bb*/			struct location *loc = cur_loc(s);
			if (loc->url) goto_url(ses, loc->url);
		}
		return 0;
        }
#endif
        if (sz) {
		char *u, *uu;
		if (len < 2 * sizeof(int) + sz) return 0;
		if ((u = mem_alloc(sz + 1))) {
			memcpy(u, (int *)data + 2, sz);
			u[sz] = 0;
			uu = decode_url(u);
                        goto_url(ses, uu);
			mem_free(u);
			mem_free(uu);
                }
        } else if ((h = options_get("homepage")) && *h) goto_url(ses, h);
        return 0;
}

void destroy_session(struct session *ses)
{
	struct download *d;
	foreach(d, downloads) if (d->ses == ses && d->prog) {
		d = d->prev;
		abort_download(d->next);
	}
	ses_abort_1st_state_loading(ses);
        /*
        if(ses->screen)
                fd_refresh_kill(ses->screen);
        */
        reinit_f_data_c(ses->screen);
	mem_free(ses->screen);
	while (!list_empty(ses->format_cache)) {
		struct f_data *f = ses->format_cache.next;
		del_from_list(f);
		destroy_formatted(f);
	}
	while (!list_empty(ses->history)) destroy_location(ses->history.next);
	if (ses->st) mem_free(ses->st);
	if (ses->default_status)mem_free(ses->default_status);
	if (ses->dn_url) mem_free(ses->dn_url);
	if (ses->search_word) mem_free(ses->search_word);
	if (ses->last_search_word) mem_free(ses->last_search_word);
	if (ses->imgmap_href_base) mem_free(ses->imgmap_href_base);
	if (ses->imgmap_target_base) mem_free(ses->imgmap_target_base);

	release_object(&ses->tq);
	if (ses->tq_prog) mem_free(ses->tq_prog);

	ses_destroy_defered_jump(ses);

        del_from_list(ses);
}

void win_func(struct window *win, struct event *ev, int fw)
{
	struct session *ses = win->data;
	switch (ev->ev) {
		case EV_ABORT:
			destroy_session(ses);
			break;
		case EV_INIT:
			if (!(ses = win->data = create_session(win)) || read_session_info(ses, (char *)ev->b + sizeof(int), *(int *)ev->b)) {
				destroy_terminal(win->term);
				return;
			}
		case EV_RESIZE:
			GF(set_window_pos(win, 0, 0, ev->x, ev->y));
			set_doc_view(ses);
			html_interpret_recursive(ses->screen);
			draw_fd(ses->screen);
			break;
		case EV_REDRAW:
                        draw_formatted(ses);
			break;
		case EV_MOUSE:
#ifdef G
                        if(F) set_window_ptr(win, ev->x, ev->y);
#endif
			/* fall through ... */
		case EV_KBD:
			send_event(ses, ev);
			break;
		default:
			error("ERROR: unknown event");
	}
}

/* 
  Gets the url being viewed by this session. Writes it into str.
  A maximum of str_size bytes (including null) will be written.
*/  
unsigned char *get_current_url(struct session *ses, unsigned char *str, size_t str_size) {
	unsigned char *here, *end_of_url;
	size_t url_len = 0;

	/* Not looking at anything */
	if (list_empty(ses->history))
		return NULL;

	here = cur_loc(ses)->url;

	/* Find the length of the url */
    if ((end_of_url = strchr(here, POST_CHAR))) {
		url_len = (size_t)(end_of_url - (unsigned char *)here);
	} else {
		url_len = strlen(here);
	}

	/* Ensure that the url size is not greater than str_size */ 
	if (url_len >= str_size)
			url_len = str_size - 1;

	strncpy(str, here, url_len);

	/* Ensure null termination */
	str[url_len] = '\0';
	
	return str;
}


/* 
  Gets the title of the page being viewed by this session. Writes it into str.
  A maximum of str_size bytes (including null) will be written.
*/  
unsigned char *get_current_title(struct session *ses, unsigned char *str, size_t str_size) {
	struct f_data_c *fd;
	fd = (struct f_data_c *)current_frame(ses);

	/* Ensure that the title is defined */
	if (!fd || !fd->f_data)
		return NULL;

	return safe_strncpy(str, fd->f_data->title, str_size);
}

BOOL get_links(struct session *ses, struct link **links, int *ncurrent, int *nlinks) {
  struct f_data_c *fd;
  fd = (struct f_data_c *)current_frame(ses);
  if (!fd || !fd->vs || !fd->f_data)
    return FALSE;
  if (links) *links = fd->f_data->links;
  if (ncurrent) *ncurrent = fd->vs->current_link;
  if (nlinks) *nlinks = fd->f_data->nlinks;
  return TRUE;
}

struct link *get_current_link(struct session *ses) {
  struct f_data_c *fd;
  
  fd = (struct f_data_c *)current_frame(ses);
  if (!fd || !fd->vs || !fd->f_data)
    return NULL;
  if (fd->vs->current_link == -1 || fd->vs->current_link >= fd->f_data->nlinks) 
		return NULL;
  return &fd->f_data->links[fd->vs->current_link];
}

/* 
  Gets the url of the link currently selected. Writes it into str.
  A maximum of str_size bytes (including null) will be written.
*/  
unsigned char *get_current_link_url(struct session *ses, unsigned char *str, size_t str_size) {
	struct f_data_c *fd;
    struct link *l;
	
	fd = (struct f_data_c *)current_frame(ses);
	/* What the hell is an 'fd'? */
	if (!fd)
		return NULL;
	
	/* Nothing selected? */
    if (fd->vs->current_link == -1 || fd->vs->current_link >= fd->f_data->nlinks) 
		return NULL;

    l = &fd->f_data->links[fd->vs->current_link];
	/* Only write a link */
    if (l->type != L_LINK )
		return NULL;
	
	return safe_strncpy(str, l->where, str_size);
}

/* We need to abort only our connections */

void stop_recursively(struct f_data_c *fd)
{
        if (fd->rq) stop_object_connection(fd->rq);
        if (fd->af) {
                struct additional_file *af;
                foreach(af, fd->af->af) {
                        if (af->rq) stop_object_connection(af->rq);
                }
        }
}

/* Stop our own and our subframes' connections */

void stop_button_pressed(struct session *ses)
{
        /* abort_all_connections(); */

        ses->reloadlevel = NC_PR_NO_CACHE;
	ses_destroy_defered_jump(ses);

        if(ses->screen)
                fd_refresh_kill(ses->screen);

        if (ses_abort_1st_state_loading(ses)) {
		change_screen_status(ses);
		print_screen_status(ses);
		return;
        }

        stop_recursively(current_frame(ses));
#ifdef JS
        jsint_kill_recursively(current_frame(ses));
#endif
}

void fd_refresh_kill(struct f_data_c *fd)
{
        if(!fd)
                internal("refresh_kill for non-existing f_data_c!");

        if(fd->refresh.timer>=0){
                /* fprintf(stderr, "timer %d killed\n", fd->refresh.timer);*/
                kill_timer(fd->refresh.timer);
        }
        if(fd->refresh.url)
                mem_free(fd->refresh.url);
	fd->refresh.url = NULL;
	fd->refresh.timer = -1;
	fd->refresh.state = 0;
}

static void fd_refresh_handler(void *data)
{
	struct f_data_c *fd = data;
        /* We need to use our own copy of url as goto_url calls fd_refresh_kill */
        unsigned char *url = stracpy(fd->refresh.url);

        if(!fd) internal("non-existent fd in refresh_handler!");

        /* fprintf(stderr, "timer %d expired!\n", fd->refresh.timer);*/

        if(fd->refresh.url)
                mem_free(fd->refresh.url);
        fd->refresh.timer = -1;
        fd->refresh.url = NULL;

        /* Workaround for refreshing current page */
	if(fd->loc &&
           fd->loc->url &&
           !strcmp(fd->loc->url, url))
		reload(fd->ses, -1);
	else {
		fd->refresh.state = 1;
		goto_url(fd->ses, url);
	}

        mem_free(url);
}

void fd_refresh_install(struct f_data_c *fd, unsigned char *url, ttime time)
{
        if(fd && url && *url &&
           fd->refresh.timer < 0 &&
	   !fd->refresh.state){ /* No refresh for this session */
		fd->refresh.url = stracpy(url);
                fd->refresh.timer = install_timer(time, fd_refresh_handler, fd);
                /* fprintf(stderr, "timer %d installed, %d msec, url= %s\n", fd->refresh.timer, time, url);*/
        }
}

