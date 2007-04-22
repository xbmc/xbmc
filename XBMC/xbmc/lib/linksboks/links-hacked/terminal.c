/* terminal.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

#ifdef G

void t_redraw(struct graphics_device *, struct rect *);
void t_resize(struct graphics_device *);
void t_kbd(struct graphics_device *, int, int);
void t_mouse(struct graphics_device *, int, int, int);

#endif

int hard_write(int fd, unsigned char *p, int l)
{
	int w = 1;
	int t = 0;
	while (l > 0 && w) {
		if ((w = write(fd, p, l)) < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		t += w;
		p += w;
		l -= w;
	}
	return t;
}

int hard_read(int fd, unsigned char *p, int l)
{
	int r = 1;
	int t = 0;
	while (l > 0 && r) {
		if ((r = read(fd, p, l)) < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		/*{int ww;for(ww=0;ww<r;ww++)fprintf(stderr," %02x",(int)p[ww]);fflush(stderr);}*/
		t += r;
		p += r;
		l -= r;
	}
	return t;
}

unsigned char *get_cwd()
{
	int bufsize = 128;
	unsigned char *buf;
#ifdef __XBOX__
	return NULL;
#else
	while (1) {
		if (!(buf = mem_alloc(bufsize))) return NULL;
		if (getcwd(buf, bufsize)) return buf;
		mem_free(buf);
		if (errno == EINTR) continue;
		if (errno != ERANGE) return NULL;
		bufsize += 128;
	}
	return NULL;
#endif
}

void set_cwd(unsigned char *path)
{
//	if (path) while (chdir(path) && errno == EINTR) ;
}

struct list_head terminals = {&terminals, &terminals};

void alloc_term_screen(struct terminal *term, int x, int y)
{
	unsigned *s, *t;
	NO_GFX;
	if ((s = mem_realloc(term->screen, x * y * sizeof(unsigned)))) {
		if ((t = mem_realloc(term->last_screen, x * y * sizeof(unsigned)))) {
			memset(t, -1, x * y * sizeof(unsigned));
			term->x = x;
			term->y = y;
			term->last_screen = t;
			memset(s, 0, x * y * sizeof(unsigned));
			term->screen = s;
			term->dirty = 1;
		}
	}
}

void in_term(struct terminal *);
void destroy_terminal(struct terminal *);
void check_if_no_terminal();

void clear_terminal(struct terminal *term)
{
	NO_GFX;
	fill_area(term, 0, 0, term->x, term->y, ' ');
	set_cursor(term, 0, 0, 0, 0);
}

#define IF_ACTIVE(win,term) if(!(win)->type || (win)==get_root_window((term)))

void redraw_below_window(struct window *win)
{
	int tr;
	struct terminal *term = win->term;
	struct window *end = win;
	struct event ev = {EV_REDRAW, 0, 0, 0};
	NO_GFX;
	ev.x = term->x;
	ev.y = term->y;
	if (term->redrawing >= 2) return;
	tr = term->redrawing;
	win->term->redrawing = 2;
	for (win = term->windows.prev; win != end; win = win->prev) {
		IF_ACTIVE(win, term) win->handler(win, &ev, 0);
	}
	term->redrawing = tr;
}

void redraw_terminal_ev(struct terminal *term, int e)
{
	struct window *win;
	struct event ev = {0, 0, 0, 0};
	NO_GFX;
	ev.ev = e;
	ev.x = term->x;
	ev.y = term->y;
	clear_terminal(term);
	term->redrawing = 2;
	foreachback(win, term->windows)
		IF_ACTIVE(win, term) win->handler(win, &ev, 0);
	term->redrawing = 0;
}

void redraw_terminal(struct terminal *term)
{
	NO_GFX;
	redraw_terminal_ev(term, EV_REDRAW);
}

void redraw_terminal_all(struct terminal *term)
{
	NO_GFX;
	redraw_terminal_ev(term, EV_RESIZE);
}

void erase_screen(struct terminal *term)
{
	NO_GFX;
	if (!term->master || !is_blocked()) {
		if (term->master) want_draw();
		hard_write(term->fdout, "\033[2J\033[1;1H", 10);
		if (term->master) done_draw();
	}
}

void redraw_terminal_cls(struct terminal *term)
{
	NO_GFX;
	erase_screen(term);
	alloc_term_screen(term, term->x, term->y);
	redraw_terminal_all(term);
}

void cls_redraw_all_terminals()
{
	struct terminal *term;
	foreach(term, terminals) {
		if (!F) redraw_terminal_cls(term);
#ifdef G
		else {
			t_resize(term->dev);
		}
#endif
	}
}

#ifdef G

int do_rects_intersect(struct rect *r1, struct rect *r2)
{
	return (r1->x1 > r2->x1 ? r1->x1 : r2->x1) < (r1->x2 > r2->x2 ? r2->x2 : r1->x2) && (r1->y1 > r2->y1 ? r1->y1 : r2->y1) < (r1->y2 > r2->y2 ? r2->y2 : r1->y2);
}

void intersect_rect(struct rect *v, struct rect *r1, struct rect *r2)
{
	v->x1 = r1->x1 > r2->x1 ? r1->x1 : r2->x1;
	v->x2 = r1->x2 > r2->x2 ? r2->x2 : r1->x2;
	v->y1 = r1->y1 > r2->y1 ? r1->y1 : r2->y1;
	v->y2 = r1->y2 > r2->y2 ? r2->y2 : r1->y2;
}

void unite_rect(struct rect *v, struct rect *r1, struct rect *r2)
{
	if (!is_rect_valid(r1)) {
		if (v != r2) memcpy(v, r2, sizeof(struct rect));
		return;
	}
	if (!is_rect_valid(r2)) {
		if (v != r1) memcpy(v, r1, sizeof(struct rect));
		return;
	}
	v->x1 = r1->x1 < r2->x1 ? r1->x1 : r2->x1;
	v->x2 = r1->x2 < r2->x2 ? r2->x2 : r1->x2;
	v->y1 = r1->y1 < r2->y1 ? r1->y1 : r2->y1;
	v->y2 = r1->y2 < r2->y2 ? r2->y2 : r1->y2;
}

int is_rect_valid(struct rect *r1)
{
	return r1->x1 < r1->x2 && r1->y1 < r1->y2;
}

#define R_GR	8

struct rect_set *init_rect_set()
{
	struct rect_set *s;
	if (!(s = mem_calloc(sizeof(struct rect_set) + sizeof(struct rect) * R_GR))) return NULL;
	s->rl = R_GR;
	s->m = 0;
	return s;
}

void add_to_rect_set(struct rect_set **s, struct rect *r)
{
	struct rect_set *ss = *s;
	int i;
	if (!is_rect_valid(r)) return;
	for (i = 0; i < ss->rl; i++) if (!ss->r[i].x1 && !ss->r[i].x2 && !ss->r[i].y1 && !ss->r[i].y2) {
		x:
		memcpy(&ss->r[i], r, sizeof(struct rect));
		if (i >= ss->m) ss->m = i + 1;
		return;
	}
	if (!(ss = mem_realloc(ss, sizeof(struct rect_set) + sizeof(struct rect) * (ss->rl + R_GR)))) return;
	memset(&(*s = ss)->r[i = (ss->rl += R_GR) - R_GR], 0, sizeof(struct rect) * R_GR);
	goto x;
}

void exclude_rect_from_set(struct rect_set **s, struct rect *r)
{
	int i, a;
	struct rect *rr;
	do {
		a = 0;
		for (i = 0; i < (*s)->m; i++) if (do_rects_intersect(rr = &(*s)->r[i], r)) {
			struct rect r1, r2, r3, r4;
			r1.x1 = rr->x1;
			r1.x2 = rr->x2;
			r1.y1 = rr->y1;
			r1.y2 = r->y1;

			r2.x1 = rr->x1;
			r2.x2 = r->x1;
			r2.y1 = r->y1;
			r2.y2 = r->y2;

			r3.x1 = r->x2;
			r3.x2 = rr->x2;
			r3.y1 = r->y1;
			r3.y2 = r->y2;

			r4.x1 = rr->x1;
			r4.x2 = rr->x2;
			r4.y1 = r->y2;
			r4.y2 = rr->y2;

			intersect_rect(&r2, &r2, rr);
			intersect_rect(&r3, &r3, rr);
			rr->x1 = rr->x2 = rr->y1 = rr->y2 = 0;
#ifdef DEBUG
			if (is_rect_valid(&r1) && do_rects_intersect(&r1, r)) internal("bad intersection 1");
			if (is_rect_valid(&r2) && do_rects_intersect(&r2, r)) internal("bad intersection 2");
			if (is_rect_valid(&r3) && do_rects_intersect(&r3, r)) internal("bad intersection 3");
			if (is_rect_valid(&r4) && do_rects_intersect(&r4, r)) internal("bad intersection 4");
#endif
			add_to_rect_set(s, &r1);
			add_to_rect_set(s, &r2);
			add_to_rect_set(s, &r3);
			add_to_rect_set(s, &r4);
			a = 1;
		}
	} while (a);
}

/* memory address r must contain one struct rect
 * x1 is leftmost pixel that is still valid
 * x2 is leftmost pixel that isn't valid any more
 * y1, y2 analogically
 */
int restrict_clip_area(struct graphics_device *dev, struct rect *r, int x1, int y1, int x2, int y2)
{
	struct rect v, rr;
	rr.x1 = x1, rr.x2 = x2, rr.y1 = y1, rr.y2 = y2;
	if (r) memcpy(r, &dev->clip, sizeof(struct rect));
	intersect_rect(&v, &dev->clip, &rr);
	drv->set_clip_area(dev, &v);
	return is_rect_valid(&v);
}

#endif

void draw_to_window(struct window *win, void (*fn)(struct terminal *term, void *), void *data)
{
	struct terminal *term = win->term;
	struct window *end = (void *)&term->windows;

        /* Non-active tab ? */
        if (win->type && term->current_tab != get_tab_number(win))
                return;

        if (!F) {
		pr(fn(term, data));
                /*
                term = win->term;
		end = (void *)&term->windows;
                */
                if (win->prev == end || term->redrawing) return;
		term->redrawing = 1;
		{
			struct event ev = {EV_REDRAW, 0, 0, 0};
			ev.x = term->x;
			ev.y = term->y;
			for (win = win->prev; win != end; win = win->prev)
				IF_ACTIVE(win, term) win->handler(win, &ev, 0);
		}
		term->redrawing = 0;
#ifdef G
	} else {
		struct rect r1, *r;
		struct rect_set *s;
		int i, a;
		if (win->prev == end || !(s = init_rect_set())) {
			pr(fn(term, data));
			return;
		}
		intersect_rect(&r1, &win->pos, &term->dev->clip);
		add_to_rect_set(&s, &r1);
		for (win = win->prev; win != end; win = win->prev)
                        IF_ACTIVE(win, term) exclude_rect_from_set(&s, &win->pos);
		a = 0;
		memcpy(&r1, &term->dev->clip, sizeof(struct rect));
		for (i = 0; i < s->m; i++) if (is_rect_valid(r = &s->r[i])) {
			drv->set_clip_area(term->dev, r);
			pr(fn(term, data)) return;
			a = 1;
		}
		if (!a) {
			struct rect empty = { 0, 0, 0, 0 };
			drv->set_clip_area(term->dev, &empty);
			fn(term, data);
		}
		drv->set_clip_area(term->dev, &r1);
		mem_free(s);
#endif
	}
}

#ifdef G

void redraw_windows(struct terminal *term)
{
	struct terminal *t1;
	struct window *win;

	foreach(t1, terminals) if (t1 == term) goto ok;
	return;
	ok:
	foreach(win, term->windows)
	IF_ACTIVE(win, term) {
		struct event ev = { EV_REDRAW, 0, 0, 0 };

		ev.x = term->x;
		ev.y = term->y;
		drv->set_clip_area(term->dev, &win->redr);
		memset(&win->redr, 0, sizeof(struct rect));
		win->handler(win, &ev, 0);
	}
	drv->set_clip_area(term->dev, &term->dev->size);
}

void set_window_pos(struct window *win, int x1, int y1, int x2, int y2)
{
	struct terminal *term = win->term;
	struct rect r;
	NO_TEXT;
	r.x1 = x1, r.y1 = y1, r.x2 = x2, r.y2 = y2;
	if (is_rect_valid(&win->pos) && (x1 > win->pos.x1 || x2 < win->pos.x2 || y1 > win->pos.y1 || y2 < win->pos.y2) && term->redrawing < 2) {
		struct window *w;
		for (w = win->next; w != (void *)&win->term->windows; w = w->next)
                        IF_ACTIVE(w, term) unite_rect(&w->redr, &win->pos, &w->redr);
		register_bottom_half((void (*)(void *))redraw_windows, term);
	}
	memcpy(&win->pos, &r, sizeof(struct rect));
}

#endif

void add_window_at_pos(struct terminal *term, void (*handler)(struct window *, struct event *, int), void *data, struct window *at)
{
	struct event ev = {EV_INIT, 0, 0, 0};
	struct window *win;
	ev.x = term->x;
	ev.y = term->y;
	if (!(win = mem_calloc(sizeof(struct window)))) {
		mem_free(data);
		return;
	}
	win->handler = handler;
	win->data = data;
	win->term = term;
	win->xp = win->yp = 0;
        win->type = 0;
	add_at_pos(at, win);
	win->handler(win, &ev, 0);
}

void add_window(struct terminal *term, void (*handler)(struct window *, struct event *, int), void *data)
{
	add_window_at_pos(term, handler, data, (struct window *)&term->windows);
}

void delete_window(struct window *win)
{
	struct event ev = {EV_ABORT, 0, 0, 0};
	win->handler(win, &ev, 1);
	del_from_list(win);
	if (win->data) mem_free(win->data);
	if (!F) redraw_terminal(win->term);
#ifdef G
	else {
		struct window *w;
		for (w = win->next; w != (void *)&win->term->windows; w = w->next)
			IF_ACTIVE(win, win->term) unite_rect(&w->redr, &win->pos, &w->redr);
		register_bottom_half((void (*)(void *))redraw_windows, win->term);
	}
#endif
	mem_free(win);
}

void delete_window_ev(struct window *win, struct event *ev)
{
	struct window *w = win->next;
	if ((void *)w == &win->term->windows) w = NULL;
	delete_window(win);
	if (ev && w && w->next != w) w->handler(w, ev, 1);
}

void set_window_ptr(struct window *win, int x, int y)
{
	if (win->xp == x && win->yp == y) return;
	win->xp = x;
	win->yp = y;
#ifdef G
	if (F && win->prev != (void *)&win->term->windows) {
		memcpy(&win->prev->redr, &win->term->dev->size, sizeof(struct rect));
		register_bottom_half((void (*)(void *))redraw_windows, win->term);
	}
#endif
}

void get_parent_ptr(struct window *win, int *x, int *y)
{
	struct window *parent = win->next;

        if (parent->type)
                parent = get_root_window(win->term);

	if (parent) {
		*x = parent->xp;
		*y = parent->yp;
	} else {
		*x = 0;
		*y = 0;
        }
}

/* Number of tabs - just number of root windows in term->windows */
int number_of_tabs(struct terminal *term)
{
	int result = 0;
	struct window *win;

	foreach(win, term->windows)
		result += win->type;

        return result;
}

/* Number of tab */
int get_tab_number(struct window *window)
{
	struct terminal *term = window->term;
	struct window *win;
        int current = 0;
	int num = 0;

	foreachback(win, term->windows)
		if(win == window)
			num = current;
		else
			current += win->type;

	return num;
}

/* Get root window of a given tab */
struct window *get_tab_by_number(struct terminal *term, int num)
{
	struct window *win = NULL;

	foreachback(win,term->windows)
		if(win->type && !num)
			break;
		else
                        num -= win->type;

	return win;
}

/* Get root window */
struct window *get_root_window(struct terminal *term)
{
	return get_tab_by_number(term,term->current_tab);
}

/* Get first or root window */
struct window *get_root_or_first_window(struct terminal *term)
{
        struct window *win=term->windows.next;
        if(win->type)
		win=get_root_window(term);

        return win;
}

void switch_to_tab(struct terminal *term, int num)
{
        int number = number_of_tabs(term);
        int cycle  = options_get_bool("tabs_cycle");

        if(num >= number)
                num = cycle ? 0 : number-1;
        if(num < 0)
                num = cycle ? number-1 : 0;

	term->current_tab = num;

        if (!F)
		redraw_terminal_cls(term);
#ifdef G
	else {
		struct window *win=get_root_window(term);
		struct event ev = {EV_REDRAW, 0, 0, 0};

		drv->set_clip_area(term->dev, &term->dev->size);
		win->handler(win, &ev, 0);
        }
#endif
}

void close_tab(struct terminal *term)
{
        if(number_of_tabs(term) > 1){
                delete_window(get_root_window(term));
		switch_to_tab(term, term->current_tab -
			      (options_get_bool("tabs_close_switch_to_next") ? 0 : 1));
#ifdef G
		if(F && options_get_bool("tabs_show") &&
		   number_of_tabs(term) == 1 &&
		   !options_get_bool("tabs_show_if_single"))
			t_resize(term->dev);
#endif
	} else if(options_get_bool("tabs_close_last"))
                query_exit(get_root_window(term)->data);
 /*BOOK0000010x000bb*/}

struct ewd {
	void (*fn)(void *);
	void *data;
	int b;
};

void empty_window_handler(struct window *win, struct event *ev, int fwd)
{
	struct window *n;
	struct ewd *ewd = win->data;
	int x, y;
	void (*fn)(void *) = ewd->fn;
	void *data = ewd->data;
	if (ewd->b) return;
	switch (ev->ev) {
		case EV_INIT:
		case EV_RESIZE:
		case EV_REDRAW:
			get_parent_ptr(win, &x, &y);
			set_window_ptr(win, x, y);
			return;
		case EV_ABORT:
			fn(data);
			return;
	}
	ewd->b = 1;
	n = win->next;
	delete_window(win);
	fn(data);
	if (n->next != n) n->handler(n, ev, fwd);
}

void add_empty_window(struct terminal *term, void (*fn)(void *), void *data)
{
	struct ewd *ewd;
	if (!(ewd = mem_alloc(sizeof(struct ewd)))) return;
	ewd->fn = fn;
	ewd->data = data;
	ewd->b = 0;
	add_window(term, empty_window_handler, ewd);
}

void free_term_specs()
{
	free_list(term_specs);
}

struct list_head term_specs = {&term_specs, &term_specs};

struct term_spec dumb_term = { NULL, NULL, "", 0, 1, 0, 0, 0, 0 };

struct term_spec *get_term_spec(unsigned char *term)
{
	struct term_spec *t;
	NO_GFX;
	foreach(t, term_specs) if (!_stricmp(t->term, term)) return t;
	return &dumb_term;
}

struct term_spec *new_term_spec(unsigned char *term)
{
	struct term_spec *t;
	foreach(t, term_specs) if (!_stricmp(t->term, term)) return t;
	if (!(t = mem_alloc(sizeof(struct term_spec)))) return NULL;
	memcpy(t, &dumb_term, sizeof(struct term_spec));
	if (strlen(term) < MAX_TERM_LEN) strcpy(t->term, term);
	else memcpy(t->term, term, MAX_TERM_LEN - 1), t->term[MAX_TERM_LEN - 1] = 0;
	add_to_list(term_specs, t);
	sync_term_specs();
	return t;
}

void sync_term_specs()
{
	struct terminal *term;
	foreach(term, terminals) term->spec = get_term_spec(term->term);
}

struct terminal *init_term(int fdin, int fdout, void (*root_window)(struct window *, struct event *, int))
{
	static tcount terminal_count = 0;
	struct terminal *term;
	struct window *win;
	NO_GFX;
	if (!(term = mem_calloc(sizeof (struct terminal)))) {
		check_if_no_terminal();
		return NULL;
	}
	term->count = terminal_count++;
	term->fdin = fdin;
	term->fdout = fdout;
	term->master = term->fdout == get_output_handle();
	term->lcx = -1;
	term->lcy = -1;
	term->dirty = 1;
	term->blocked = -1;
	term->screen = DUMMY;
	term->last_screen = DUMMY;
	term->spec = &dumb_term;
	term->input_queue = DUMMY;
	init_list(term->windows);
	if (!(win = mem_calloc(sizeof(struct window)))) {
		mem_free(term);
		check_if_no_terminal();
		return NULL;
	}
	win->handler = root_window;
	win->term = term;
        win->type = 1;
	add_to_list(term->windows, win);
	/*alloc_term_screen(term, 80, 25);*/
	add_to_list(terminals, term);
	set_handlers(fdin, (void (*)(void *))in_term, NULL, (void (*)(void *))destroy_terminal, term);
	return term;
}

#ifdef G

struct term_spec gfx_term = { NULL, NULL, "", 0, 0, 0, 0, 0, 0 };

struct terminal *init_gfx_term(void (*root_window)(struct window *, struct event *, int), void *info, int len)
{
	static tcount terminal_count = 0;
	struct terminal *term;
	struct graphics_device *dev;
        struct window *win;
	unsigned char *cwd;
	NO_TEXT;
	if (!(term = mem_calloc(sizeof (struct terminal)))) {
		check_if_no_terminal();
		return NULL;
        }
	term->count = terminal_count++;
	term->fdin = -1;
	term->environment = ENV_G;

        if (!_stricmp(drv->name, "x")) term->environment |= ENV_XWIN;

        if (!(term->dev = dev = drv->init_device())) {
		mem_free(term);
		check_if_no_terminal();
		return NULL;
	}
	dev->user_data = term;
	term->master = 1;
	term->blocked = -1;
	term->x = dev->size.x2;
	term->y = dev->size.y2;
	term->spec = &gfx_term;
	if ((cwd = get_cwd())) {
		safe_strncpy(term->cwd, cwd, MAX_CWD_LEN);
		mem_free(cwd);
	}
	gfx_term.charset = get_cp_index("utf-8");
	if (gfx_term.charset == -1) gfx_term.charset = 0;
	init_list(term->windows);
	if (!(win = mem_calloc(sizeof (struct window)))) {
		drv->shutdown_device(dev);
		mem_free(term);
		check_if_no_terminal();
		return NULL;
	}
	win->handler = root_window;
	win->term = term;
	win->pos.x2 = dev->size.x2;
	win->pos.y2 = dev->size.y2;
        win->type = 1;
	add_to_list(term->windows, win);
	add_to_list(terminals, term);
	dev->redraw_handler = t_redraw;
	dev->resize_handler = t_resize;
	dev->keyboard_handler = t_kbd;
	dev->mouse_handler = t_mouse;
        {
		int *ptr;
		struct event ev = { EV_INIT, 0, 0, 0 };
		ev.x = dev->size.x2;
		ev.y = dev->size.y2;
		if ((ptr = mem_alloc(sizeof(int) + len))) {
			*ptr = len;
			memcpy(ptr + 1, info, len);
			ev.b = (long)ptr;
			root_window(win, &ev, 0);
			mem_free(ptr);
                }
        }
	return term;
}


void t_redraw(struct graphics_device *dev, struct rect *r)
{
	struct terminal *term = dev->user_data;
	struct window *win;
	/*debug("%d %d %d %d", r->x1, r->x2, r->y1, r->y2);*/
	/*fprintf(stderr, "t_redraw: %d,%d %d,%d\n", r->x1, r->y1, r->x2, r->y2);*/
	foreach(win, term->windows){
		IF_ACTIVE(win, term) unite_rect(&win->redr, r, &win->redr);
	}
	register_bottom_half((void (*)(void *))redraw_windows, term);
}

void t_resize(struct graphics_device *dev)
{
	struct terminal *term = dev->user_data;
	struct window *win;
	struct event ev = {EV_RESIZE, 0, 0, 0};
	term->x = ev.x = dev->size.x2;
	term->y = ev.y = dev->size.y2;
	drv->set_clip_area(dev, &dev->size);
	foreach(win, term->windows) {
		win->handler(win, &ev, 0);
	}
	drv->set_clip_area(dev, &dev->size);
}

void t_kbd(struct graphics_device *dev, int key, int flags)
{
	struct terminal *term = dev->user_data;
	struct event ev = {EV_KBD, 0, 0, 0};
	struct rect r = {0, 0, 0, 0};
        struct window *root = get_root_or_first_window(term);

	r.x2 = dev->size.x2, r.y2 = dev->size.y2;
	ev.x = key;
	ev.y = flags;
	if (upcase(key) == 'L' && flags == KBD_CTRL) {
		t_redraw(dev, &r);
		return;
	} else {
		drv->set_clip_area(dev, &r);
		if (list_empty(term->windows)) return;
		if (ev.x == KBD_CTRL_C || ev.x == KBD_CLOSE) root->handler(root, &ev, 0);
		else root->handler(root, &ev, 0);
        }
}

void t_mouse(struct graphics_device *dev, int x, int y, int b)
{
	struct terminal *term = dev->user_data;
	struct event ev = {EV_MOUSE, 0, 0, 0};
	struct rect r = {0, 0, 0, 0};
        struct window *root = get_root_or_first_window(term);

	r.x2 = dev->size.x2, r.y2 = dev->size.y2;
	ev.x = x, ev.y = y, ev.b = b;
	drv->set_clip_area(dev, &r);
	if (list_empty(term->windows)) return;
	root->handler(root, &ev, 0);
}

#endif

/* We need to send event to correct root window, not to first one --karpov */
void term_send_event(struct terminal *term, struct event *ev)
{
	struct window *first_win = term->windows.next;
	struct window *win = first_win->type
			     ? get_root_window(term)
			     : first_win;

	win->handler(win, ev, 0);
}

void in_term(struct terminal *term)
{
	struct event *ev;
	int r;
	unsigned char *iq;
	NO_GFX;
	if (!(iq = mem_realloc(term->input_queue, term->qlen + ALLOC_GR))) {
		destroy_terminal(term);
		return;
	}
	term->input_queue = iq;
	if ((r = read(term->fdin, iq + term->qlen, ALLOC_GR)) <= 0) {
		if (r == -1 && errno != ECONNRESET) error("ERROR: error %d on terminal: could not read event", errno);
		destroy_terminal(term);
		return;
	}
	term->qlen += r;
	test_queue:
	if (term->qlen < sizeof(struct event)) return;
	ev = (struct event *)iq;
	r = sizeof(struct event);
	if (ev->ev != EV_INIT && ev->ev != EV_RESIZE && ev->ev != EV_REDRAW && ev->ev != EV_KBD && ev->ev != EV_MOUSE && ev->ev != EV_ABORT) {
		error("ERROR: error on terminal: bad event %d", ev->ev);
		goto mm;
	}
	if (ev->ev == EV_INIT) {
		int init_len;
		if (term->qlen < sizeof(struct event) + MAX_TERM_LEN + MAX_CWD_LEN + 2 * sizeof(int)) return;
		init_len = *(int *)(iq + sizeof(struct event) + MAX_TERM_LEN + MAX_CWD_LEN + sizeof(int));
		if (term->qlen < sizeof(struct event) + MAX_TERM_LEN + MAX_CWD_LEN + 2 * sizeof(int) + init_len) return;
		memcpy(term->term, iq + sizeof(struct event), MAX_TERM_LEN);
		term->term[MAX_TERM_LEN - 1] = 0;
		memcpy(term->cwd, iq + sizeof(struct event) + MAX_TERM_LEN, MAX_CWD_LEN);
		term->cwd[MAX_CWD_LEN - 1] = 0;
		term->environment = *(int *)(iq + sizeof(struct event) + MAX_TERM_LEN + MAX_CWD_LEN);
		ev->b = (long)(iq + sizeof(struct event) + MAX_TERM_LEN + MAX_CWD_LEN + sizeof(int));
		r = sizeof(struct event) + MAX_TERM_LEN + MAX_CWD_LEN + 2 * sizeof(int) + init_len;
		sync_term_specs();
	}
	if (ev->ev == EV_REDRAW || ev->ev == EV_RESIZE || ev->ev == EV_INIT) {
		struct window *win;
		send_redraw:
		if (ev->x < 0 || ev->y < 0) {
			error("ERROR: bad terminal size: %d, %d", (int)ev->x, (int)ev->y);
			goto mm;
		}
		alloc_term_screen(term, ev->x, ev->y);
		clear_terminal(term);
		erase_screen(term);
		term->redrawing = 1;
		foreachback(win, term->windows)
			IF_ACTIVE(win, term) win->handler(win, ev, 0);
		term->redrawing = 0;
	}
	if (ev->ev == EV_KBD || ev->ev == EV_MOUSE) {
		if (ev->ev == EV_KBD && upcase(ev->x) == 'L' && ev->y == KBD_CTRL) {
			ev->ev = EV_REDRAW;
			ev->x = term->x;
			ev->y = term->y;
			goto send_redraw;
		}
		else if (ev->ev == EV_KBD && ev->x == KBD_CTRL_C) ((struct window *)&term->windows)->prev->handler(term->windows.prev, ev, 0);
		else term_send_event(term, ev);
	}
	if (ev->ev == EV_ABORT) destroy_terminal(term);
	/*redraw_screen(term);*/
	mm:
	if (term->qlen == r) term->qlen = 0;
	else memmove(iq, iq + r, term->qlen -= r);
	goto test_queue;
}

inline int getcompcode(int c)
{
	return (c<<1 | (c&4)>>2) & 7;
}

unsigned char frame_dumb[48] =	"   ||||++||++++++--|-+||++--|-+----++++++++     ";
unsigned char frame_vt100[48] =	"aaaxuuukkuxkjjjkmvwtqnttmlvwtqnvvwwmmllnnjla    ";
unsigned char frame_koi[48] = {
	144,145,146,129,135,178,180,167,
	166,181,161,168,174,173,172,131,
	132,137,136,134,128,138,175,176,
	171,165,187,184,177,160,190,185,
	186,182,183,170,169,162,164,189,
	188,133,130,141,140,142,143,139,
};
unsigned char frame_restrict[48] = {
	0, 0, 0, 0, 0, 179, 186, 186,
	205, 0, 0, 0, 0, 186, 205, 0,
	0, 0, 0, 0, 0, 0, 179, 186,
	0, 0, 0, 0, 0, 0, 0, 205,
	196, 205, 196, 186, 205, 205, 186, 186,
	179, 0, 0, 0, 0, 0, 0, 0,
};

#define PRINT_CHAR(p)									\
{											\
	unsigned ch = term->screen[p];							\
	unsigned char c = ch & 0xff;							\
	unsigned char A = ch >> 8 & 0x7f;						\
	if (s->mode == TERM_LINUX) {							\
		if (s->m11_hack) {							\
			if (ch >> 15 != mode) {						\
				if (!(mode = ch >> 15)) add_to_str(&a, &l, "\033[10m");	\
				else add_to_str(&a, &l, "\033[11m");			\
			}								\
		}									\
		if (s->restrict_852 && (ch >> 15) && c >= 176 && c < 224) {		\
			if (frame_restrict[c - 176]) c = frame_restrict[c - 176];	\
		}									\
	} else if (s->mode == TERM_VT100) {						\
		if (ch >> 15 != mode) {							\
			if (!(mode = ch >> 15)) add_to_str(&a, &l, "\x0f");		\
			else add_to_str(&a, &l, "\x0e");				\
		}									\
		if (mode && c >= 176 && c < 224) c = frame_vt100[c - 176];		\
	} else if (s->mode == TERM_KOI8 && (ch >> 15) && c >= 176 && c < 224) { c = frame_koi[c - 176];\
	} else if (s->mode == TERM_DUMB && (ch >> 15) && c >= 176 && c < 224) c = frame_dumb[c - 176];\
	if (!(A & 0100) && (A >> 3) == (A & 7)) A = (A & 070) | 7 * !(A & 020);		\
	if (A != attrib) {								\
		attrib = A;								\
		add_to_str(&a, &l, "\033[0");						\
		if (s->col || s->trans) {						\
			unsigned char m[4];						\
			m[0] = ';'; m[1] = '3'; m[3] = 0;				\
			m[2] = (attrib & 7) + '0';					\
                        add_to_str(&a, &l, m);                                          \
                        m[1] = '4';							\
			m[2] = ((attrib >> 3) & 7) + '0';				\
			if(!s->trans || m[2]-'0') add_to_str(&a, &l, m);		\
		} else if (getcompcode(attrib & 7) < getcompcode(attrib >> 3 & 7))	\
			add_to_str(&a, &l, ";7");					\
		if (attrib & 0100) add_to_str(&a, &l, ";1");				\
		add_to_str(&a, &l, "m");						\
	}										\
	if (c >= ' ' && c != 127/* && c != 155*/) add_chr_to_str(&a, &l, c);		\
	else if (!c || c == 1) add_chr_to_str(&a, &l, ' ');				\
	else add_chr_to_str(&a, &l, '.');						\
	cx++;										\
}											\

void redraw_all_terminals()
{
	struct terminal *term;
	foreach(term, terminals) redraw_screen(term);
}

void redraw_screen(struct terminal *term)
{
	int x, y, p = 0;
	int cx = -1, cy = -1;
	unsigned char *a;
	int attrib = -1;
	int mode = -1;
	int l = 0;
	struct term_spec *s;
	NO_GFX;
	if (!term->dirty || (term->master && is_blocked())) return;
	if (!(a = init_str())) return;
        /* FIXME: We need to move all terminal options to new options system */
	s = term->spec;
        s->trans = options_get_bool("transparency");
        for (y = 0; y < term->y; y++)
		for (x = 0; x < term->x; x++, p++) {
			if (y == term->y - 1 && x == term->x - 1) break;
			if (term->screen[p] == term->last_screen[p]) continue;
			if ((term->screen[p] & 0x3800) == (term->last_screen[p] & 0x3800) && ((term->screen[p] & 0xff) == 0 || (term->screen[p] & 0xff) == 1 || (term->screen[p] & 0xff) == ' ') && ((term->last_screen[p] & 0xff) == 0 || (term->last_screen[p] & 0xff) == 1 || (term->last_screen[p] & 0xff) == ' ')) continue;
			if (cx == x && cy == y) goto pc;/*PRINT_CHAR(p)*/
			else if (cy == y && x - cx < 10) {
				int i;
				for (i = x - cx; i >= 0; i--) PRINT_CHAR(p - i);
			} else {
				add_to_str(&a, &l, "\033[");
				add_num_to_str(&a, &l, y + 1);
				add_to_str(&a, &l, ";");
				add_num_to_str(&a, &l, x + 1);
				add_to_str(&a, &l, "H");
				cx = x; cy = y;
				pc:
				PRINT_CHAR(p);
			}
		}
	if (l) {
		if (s->col || s->trans) add_to_str(&a, &l, "\033[37;40m");
		add_to_str(&a, &l, "\033[0m");
		if (s->mode == TERM_LINUX && s->m11_hack) add_to_str(&a, &l, "\033[10m");
		if (s->mode == TERM_VT100) add_to_str(&a, &l, "\x0f");
	}
	if (l || term->cx != term->lcx || term->cy != term->lcy) {
		term->lcx = term->cx;
		term->lcy = term->cy;
		add_to_str(&a, &l, "\033[");
		add_num_to_str(&a, &l, term->cy + 1);
		add_to_str(&a, &l, ";");
		add_num_to_str(&a, &l, term->cx + 1);
		add_to_str(&a, &l, "H");
	}
	if (l && term->master) want_draw();
	hard_write(term->fdout, a, l);
	if (l && term->master) done_draw();
	mem_free(a);
	memcpy(term->last_screen, term->screen, term->x * term->y * sizeof(int));
	term->dirty = 0;
}

void destroy_terminal(struct terminal *term)
{
	while ((term->windows.next) != &term->windows) delete_window(term->windows.next);
	/*if (term->cwd) mem_free(term->cwd);*/
	del_from_list(term);
	if (term->blocked != -1) {
		close(term->blocked);
		set_handlers(term->blocked, NULL, NULL, NULL, NULL);
	}
	if (term->title) mem_free(term->title);
	if (!F) {
		mem_free(term->screen);
		mem_free(term->last_screen);
		set_handlers(term->fdin, NULL, NULL, NULL, NULL);
		mem_free(term->input_queue);
		close(term->fdin);
		if (term->fdout != 1) {
			if (term->fdout != term->fdin) close(term->fdout);
		} else {
#ifndef __XBOX__
			unhandle_terminal_signals(term);
#endif
			free_all_itrms();
#ifndef NO_FORK_ON_EXIT
			if (!list_empty(terminals)) {
				if (fork()) exit(0);
			}
#endif
		}
#ifdef G
	} else {
		drv->shutdown_device(term->dev);
#endif
	}
	mem_free(term);
	check_if_no_terminal();
}

void destroy_all_terminals()
{
	struct terminal *term;
	while ((void *)(term = terminals.next) != &terminals) destroy_terminal(term);
}

void check_if_no_terminal()
{
	if (!list_empty(terminals)) return;
	terminate_loop = 1;
}

void set_char(struct terminal *t, int x, int y, unsigned c)
{
	NO_GFX;
	t->dirty = 1;
	if (x >= 0 && x < t->x && y >= 0 && y < t->y) t->screen[x + t->x * y] = c;
}

unsigned get_char(struct terminal *t, int x, int y)
{
	NO_GFX;
	if (x >= t->x) x = t->x - 1;
	if (x < 0) x = 0;
	if (y >= t->y) y = t->y - 1;
	if (y < 0) y = 0;
	return t->screen[x + t->x * y];
}

void set_color(struct terminal *t, int x, int y, unsigned c)
{
	NO_GFX;
	t->dirty = 1;
	if (x >= 0 && x < t->x && y >= 0 && y < t->y) t->screen[x + t->x * y] = (t->screen[x + t->x * y] & 0x80ff) | (c & ~0x80ff);
}

void set_only_char(struct terminal *t, int x, int y, unsigned c)
{
	NO_GFX;
	t->dirty = 1;
	if (x >= 0 && x < t->x && y >= 0 && y < t->y) t->screen[x + t->x * y] = (t->screen[x + t->x * y] & ~0x80ff) | (c & 0x80ff);
}

void set_line(struct terminal *t, int x, int y, int l, chr *line)
{
	int i;
	NO_GFX;
	t->dirty = 1;
	for (i = x >= 0 ? 0 : -x; i < (x+l <= t->x ? l : t->x-x); i++)
		t->screen[x+i + t->x * y] = line[i];
}

void set_line_color(struct terminal *t, int x, int y, int l, unsigned c)
{
	int i;
	NO_GFX;
	t->dirty = 1;
	for (i = x >= 0 ? 0 : -x; i < (x+l <= t->x ? l : t->x-x); i++)
		t->screen[x+i + t->x * y] = (t->screen[x+i + t->x * y] & 0x80ff) | (c & ~0x80ff);
}

void fill_area(struct terminal *t, int x, int y, int xw, int yw, unsigned c)
{
	int i,j;
	NO_GFX;
	t->dirty = 1;
	for (j = y >= 0 ? 0 : -y; j < yw && y+j < t->y; j++)
		for (i = x >= 0 ? 0 : -x; i < xw && x+i < t->x; i++) 
			t->screen[x+i + t->x*(y+j)] = c;
}

int p1[] = { 218, 191, 192, 217, 179, 196 };
int p2[] = { 201, 187, 200, 188, 186, 205 };

void draw_frame(struct terminal *t, int x, int y, int xw, int yw, unsigned c, int w)
{
	int *p = w > 1 ? p2 : p1;
	NO_GFX;
	c |= ATTR_FRAME;
	set_char(t, x, y, c+p[0]);
	set_char(t, x+xw-1, y, c+p[1]);
	set_char(t, x, y+yw-1, c+p[2]);
	set_char(t, x+xw-1, y+yw-1, c+p[3]);
	fill_area(t, x, y+1, 1, yw-2, c+p[4]);
	fill_area(t, x+xw-1, y+1, 1, yw-2, c+p[4]);
	fill_area(t, x+1, y, xw-2, 1, c+p[5]);
	fill_area(t, x+1, y+yw-1, xw-2, 1, c+p[5]);
}

void print_text(struct terminal *t, int x, int y, int l, unsigned char *text, unsigned c)
{
	NO_GFX;
	for (; l-- && *text; text++, x++) set_char(t, x, y, *text + c);
}

void set_cursor(struct terminal *term, int x, int y, int altx, int alty)
{
	NO_GFX;
	term->dirty = 1;
	if (term->spec->block_cursor) x = altx, y = alty;
	if (x >= term->x) x = term->x - 1;
	if (y >= term->y) y = term->y - 1;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	term->cx = x;
	term->cy = y;
}

void exec_thread(unsigned char *path, int p)
{
#ifndef __XBOX__
#if defined(HAVE_SETPGID) && !defined(BEOS) && !defined(HAVE_BEGINTHREAD)
	if (path[0] == 2) setpgid(0, 0);
#endif
	exe(path + 1);
	close(p);
	if (path[1 + strlen(path + 1) + 1]) unlink(path + 1 + strlen(path + 1) + 1);
#endif /* __XBOX__ */
}

void close_handle(void *p)
{
	int h = (int)p;
	close(h);
	set_handlers(h, NULL, NULL, NULL, NULL);
}

void unblock_terminal(struct terminal *term)
{
	close_handle((void *)term->blocked);
	term->blocked = -1;
	if (!F) {
		set_handlers(term->fdin, (void (*)(void *))in_term, NULL, (void (*)(void *))destroy_terminal, term);
		unblock_itrm(term->fdin);
		redraw_terminal_cls(term);
#ifdef G
	} else {
		drv->unblock(term->dev);
#endif
	}
}

#ifndef __XBOX__ /* Reimplemented in linksboks.cpp */
void exec_on_terminal(struct terminal *term, unsigned char *path, unsigned char *delete, int fg)
{
	if (path && !*path) return;
	if (!path) path="";
#ifdef NO_FG_EXEC
	fg = 0;
#endif
	if (term->master) {
		if (!*path) {
			if (!F) dispatch_special(delete);
		} else {
			int blockh;
			unsigned char *param;
			if ((!F ? is_blocked() : term->blocked != -1) && fg) {
				unlink(delete);
				return;
			}
			if (!(param = mem_alloc(strlen(path) + strlen(delete) + 3))) return;
			param[0] = fg;
			strcpy(param + 1, path);
			strcpy(param + 1 + strlen(path) + 1, delete);
			if (fg == 1) {
				if (!F) block_itrm(term->fdin);
#ifdef G
				else if (drv->block(term->dev)) {
					mem_free(param);
					unlink(delete);
					return;
				}
#endif
			}
			if ((blockh = start_thread((void (*)(void *, int))exec_thread, param, strlen(path) + strlen(delete) + 3)) == -1) {
				if (fg == 1) {
					if (!F) unblock_itrm(term->fdin);
#ifdef G
					else drv->unblock(term->dev);
#endif
				}
				mem_free(param);
				return;
			}
			mem_free(param);
			if (fg == 1) {
				term->blocked = blockh;
				set_handlers(blockh, (void (*)(void *))unblock_terminal, NULL, (void (*)(void *))unblock_terminal, term);
				if (!F) set_handlers(term->fdin, NULL, NULL, (void (*)(void *))destroy_terminal, term);
				/*block_itrm(term->fdin);*/
			} else {
				set_handlers(blockh, close_handle, NULL, close_handle, (void *)blockh);
			}
		}
	} else {
		unsigned char *data;
		if ((data = mem_alloc(strlen(path) + strlen(delete) + 4))) {
			data[0] = 0;
			data[1] = fg;
			strcpy(data + 2, path);
			strcpy(data + 3 + strlen(path), delete);
			hard_write(term->fdout, data, strlen(path) + strlen(delete) + 4);
			mem_free(data);
		}
		/*char x = 0;
		hard_write(term->fdout, &x, 1);
		x = fg;
		hard_write(term->fdout, &x, 1);
		hard_write(term->fdout, path, strlen(path) + 1);
		hard_write(term->fdout, delete, strlen(delete) + 1);*/
	}
}
#endif /* #ifndef __XBOX__ */

void do_terminal_function(struct terminal *term, unsigned char code, unsigned char *data)
{
	unsigned char *x_data;
	NO_GFX;
	if (!(x_data = mem_alloc(strlen(data) + 2))) return;
	x_data[0] = code;
	strcpy(x_data + 1, data);
	exec_on_terminal(term, NULL, x_data, 0);
	mem_free(x_data);
}

void set_terminal_title(struct terminal *term, unsigned char *title)
{
	unsigned char *a;
	while ((a = strchr(title, 1))) memmove(a, a + 1, strlen(a + 1) + 1);
	if (term->title && !strcmp(title, term->title)) goto ret;
	if (term->title) mem_free(term->title);
	term->title = stracpy(title);
	if (!F) do_terminal_function(term, TERM_FN_TITLE, title);
#ifdef G
	else if (drv->set_title) drv->set_title(term->dev, title);
#endif
	ret:
	mem_free(title);
}
