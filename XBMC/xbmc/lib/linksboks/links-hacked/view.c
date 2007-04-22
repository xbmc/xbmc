/* view.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

struct view_state *create_vs()
{
	struct view_state *vs;
	if (!(vs = mem_calloc(sizeof(struct view_state)))) return NULL;
	vs->refcount = 1;
	vs->current_link = -1;
	vs->frame_pos = -1;
	vs->plain = -1;
	vs->form_info = DUMMY;
	vs->form_info_len = 0;
	return vs;
}

void destroy_vs(struct view_state *vs)
{
	int i;
	if (--vs->refcount) {
		if (vs->refcount < 0) internal("destroy_vs: view_state refcount underflow");
		return;
	}
	for (i = 0; i < vs->form_info_len; i++) if (vs->form_info[i].value) mem_free(vs->form_info[i].value);
	mem_free(vs->form_info);
	mem_free(vs);
}

void copy_vs(struct view_state *dst, struct view_state *src)
{
	memcpy(dst, src, sizeof(struct view_state));
	if ((dst->form_info = mem_alloc(src->form_info_len * sizeof(struct form_state)))) {
		int i;
		memcpy(dst->form_info, src->form_info, src->form_info_len * sizeof(struct form_state));
		for (i = 0; i < src->form_info_len; i++) if (src->form_info[i].value) dst->form_info[i].value = stracpy(src->form_info[i].value);
	}
}

#ifdef JS
void create_js_event_spec(struct js_event_spec **j)
{
	if (*j) return;
	*j = mem_calloc(sizeof(struct js_event_spec));
}

void free_js_event_spec(struct js_event_spec *j)
{
	if (!j) return;
	if (j->move_code) mem_free(j->move_code);
	if (j->over_code) mem_free(j->over_code);
	if (j->out_code) mem_free(j->out_code);
	if (j->down_code) mem_free(j->down_code);
	if (j->up_code) mem_free(j->up_code);
	if (j->click_code) mem_free(j->click_code);
	if (j->dbl_code) mem_free(j->dbl_code);
	if (j->blur_code) mem_free(j->blur_code);
	if (j->focus_code) mem_free(j->focus_code);
	if (j->change_code) mem_free(j->change_code);
	if (j->keypress_code) mem_free(j->keypress_code);
	if (j->keyup_code) mem_free(j->keyup_code);
	if (j->keydown_code) mem_free(j->keydown_code);
	mem_free(j);
}

int compare_js_event_spec(struct js_event_spec *j1, struct js_event_spec *j2)
{
	if (!j1 && !j2) return 0;
	if (!j1 || !j2) return 1;
	return
		xstrcmp(j1->move_code, j2->move_code) ||
		xstrcmp(j1->over_code, j2->over_code) ||
		xstrcmp(j1->out_code, j2->out_code) ||
		xstrcmp(j1->down_code, j2->down_code) ||
		xstrcmp(j1->up_code, j2->up_code) ||
		xstrcmp(j1->click_code, j2->click_code) ||
		xstrcmp(j1->dbl_code, j2->dbl_code) ||
		xstrcmp(j1->blur_code, j2->blur_code) ||
		xstrcmp(j1->focus_code, j2->focus_code) ||
		xstrcmp(j1->change_code, j2->change_code) ||
		xstrcmp(j1->keypress_code, j2->keypress_code) ||
		xstrcmp(j1->keydown_code, j2->keydown_code) ||
		xstrcmp(j1->keyup_code, j2->keyup_code); 
}

void copy_js_event_spec(struct js_event_spec **target, struct js_event_spec *source)
{
	struct js_event_spec *t;
	*target = NULL;
	if (!source) return;
	create_js_event_spec(target);
	if (!((t = *target))) return;
	copy_string(&t->move_code, source->move_code);
	copy_string(&t->over_code, source->over_code);
	copy_string(&t->out_code, source->out_code);
	copy_string(&t->down_code, source->down_code);
	copy_string(&t->up_code, source->up_code);
	copy_string(&t->click_code, source->click_code);
	copy_string(&t->dbl_code, source->dbl_code);
	copy_string(&t->blur_code, source->blur_code);
	copy_string(&t->focus_code, source->focus_code);
	copy_string(&t->change_code, source->change_code);
	copy_string(&t->keypress_code, source->keypress_code);
	copy_string(&t->keyup_code, source->keyup_code);
	copy_string(&t->keydown_code, source->keydown_code);
}

void add_event_desc(unsigned char **str, int *l, unsigned char *fn, unsigned char *desc)
{
	if (!fn) return;
	if (*l) add_to_str(str, l, ", ");
	add_to_str(str, l, desc);
	add_to_str(str, l, ":");
	add_to_str(str, l, fn);
}

unsigned char *print_js_event_spec(struct js_event_spec *j)
{
	unsigned char *str = init_str();
	int l = 0;
	if (!j) return str;
	add_event_desc(&str, &l, j->click_code, "onclick");
	add_event_desc(&str, &l, j->dbl_code, "ondblclick");
	add_event_desc(&str, &l, j->down_code, "onmousedown");
	add_event_desc(&str, &l, j->up_code, "onmouseup");
	add_event_desc(&str, &l, j->over_code, "onmouseover");
	add_event_desc(&str, &l, j->out_code, "onmouseout");
	add_event_desc(&str, &l, j->move_code, "onmousemove");
	add_event_desc(&str, &l, j->focus_code, "onfocus");
	add_event_desc(&str, &l, j->blur_code, "onblur");
	add_event_desc(&str, &l, j->change_code, "onchange");
	add_event_desc(&str, &l, j->keypress_code, "onkeypress");
	add_event_desc(&str, &l, j->keyup_code, "onkeyup");
	add_event_desc(&str, &l, j->keydown_code, "onkeydown");
	return str;
}

#endif

static inline int c_in_view(struct f_data_c *);
void set_pos_x(struct f_data_c *, struct link *);
void set_pos_y(struct f_data_c *, struct link *);
void find_link(struct f_data_c *, int, int);
void next_frame(struct session *, int);

void check_vs(struct f_data_c *f)
{
	struct view_state *vs = f->vs;
	if (f->f_data->frame_desc) {
		struct f_data_c *ff;
		int n = 0;
		foreach(ff, f->subframes) n++;
		if (vs->frame_pos < 0) vs->frame_pos = 0;
		if (vs->frame_pos >= n) vs->frame_pos = n - 1;
		return;
	}
	if (!F) {
		if (vs->current_link >= f->f_data->nlinks) vs->current_link = f->f_data->nlinks - 1;
		if (vs->current_link != -1 && !c_in_view(f)) {
			set_pos_x(f, &f->f_data->links[f->vs->current_link]);
			set_pos_y(f, &f->f_data->links[f->vs->current_link]);
		}
		if (vs->current_link == -1) find_link(f, 1, 0);
	}
}

void set_link(struct f_data_c *f)
{
	if (c_in_view(f)) return;
	find_link(f, 1, 0);
}

/* This function returns y pos of requested anchor */
int find_tag(struct f_data *f, unsigned char *name)
{
	struct tag *tag;
        foreach(tag, f->tags)
                if (!stricmp(tag->name, name))
                        return tag->y;

        /* 'Everything is a link' technology */

        if (!options_get_bool("search_everything_is_a_link"))
            return -1;
        /* TODO: look for first occurence of a name in page and return its y */

        return -1;
}

int comp_links(struct link *l1, struct link *l2)
{
	return l1->num - l2->num;
}

void sort_links(struct f_data *f)
{
	int i;
	if (F) return;
	if (f->nlinks) qsort(f->links, f->nlinks, sizeof(struct link), (void *)comp_links);
	if (!(f->lines1 = mem_alloc(f->y * sizeof(struct link *)))) return;
	if (!(f->lines2 = mem_alloc(f->y * sizeof(struct link *)))) {
		mem_free(f->lines1);
		return;
	}
	memset(f->lines1, 0, f->y * sizeof(struct link *));
	memset(f->lines2, 0, f->y * sizeof(struct link *));
	for (i = 0; i < f->nlinks; i++) {
		int p, q, j;
		struct link *link = &f->links[i];
		if (!link->n) {
			if (link->where) mem_free(link->where);
			if (link->target) mem_free(link->target);
			if (link->where_img) mem_free(link->where_img);
			if (link->img_alt) mem_free(link->img_alt);
			if (link->pos) mem_free(link->pos);
#ifdef JS
			free_js_event_spec(link->js_event);
#endif
                        memmove(link, link + 1, (f->nlinks - i - 1) * sizeof(struct link));
			f->nlinks --;
			i--;
			continue;
		}
		p = link->pos[0].y;
		q = link->pos[link->n - 1].y;
		if (p > q) j = p, p = q, q = j;
		for (j = p; j <= q; j++) {
			if (j >= f->y) {
				internal("link out of screen");
				continue;
			}
			f->lines2[j] = &f->links[i];
			if (!f->lines1[j]) f->lines1[j] = &f->links[i];
		}
	}
}

unsigned char *utf8_add(unsigned char *t, int i)
{
	if (!F) return t + i;
#ifdef G
	while (i-- && *t) FWD_UTF_8(t);
#endif
	return t;
}

int utf8_diff(unsigned char *t2, unsigned char *t1)
{
	if (!F) return t2 - t1;
#ifdef G
	{
		int i = 0;
		while (t2 > t1) {
			FWD_UTF_8(t1);
			i++;
		}
		return i;
	}
#endif
}

struct line_info *format_text(unsigned char *text, int width, int wrap)
{
	struct line_info *ln = DUMMY;
	int lnn = 0;
	unsigned char *b = text;
	int sk, ps = 0;
	while (*text) {
		unsigned char *s;
		if (*text == '\n') {
			sk = 1;
			put:
			if (!(lnn & (ALLOC_GR-1))) {
				struct line_info *_ln;
				if (!(_ln = mem_realloc(ln, (lnn + ALLOC_GR) * sizeof(struct line_info)))) {
					mem_free(ln);
					return NULL;
				}
				ln = _ln;
			}
			ln[lnn].st = b;
			ln[lnn++].en = text;
			b = text += sk;
			continue;
		}
		if (!wrap || utf8_diff(text, b) < width) {
			if (!F) text++;
#ifdef G
			else FWD_UTF_8(text);
#endif
			continue;
		}
		for (s = text; s >= b; s--) if (*s == ' ') {
			text = s;
			sk = 1;
			goto put;
		}
		sk = 0;
		goto put;
	}
	if (ps < 2) {
		ps++;
		sk = 0;
		goto put;
	}
	ln[lnn - 1].st = ln[lnn - 1].en = NULL;
	return ln;
}

int _area_cursor(struct form_control *form, struct form_state *fs)
{
	struct line_info *ln;
	int q = 0;
	if ((ln = format_text(fs->value, form->cols, form->wrap))) {
		int x, y;
		for (y = 0; ln[y].st; y++) if (fs->value + fs->state >= ln[y].st && fs->value + fs->state < ln[y].en + (ln[y+1].st != ln[y].en)) {
			x = utf8_diff(fs->value + fs->state, ln[y].st);
			if (form->wrap && x == form->cols) x--;
			if (x >= form->cols + fs->vpos) fs->vpos = x - form->cols + 1;
			if (x < fs->vpos) fs->vpos = x;
			if (y >= form->rows + fs->vypos) fs->vypos = y - form->rows + 1;
			if (y < fs->vypos) fs->vypos = y;
			x -= fs->vpos;
			y -= fs->vypos;
			q = y * form->cols + x;
			break;
		}
		mem_free(ln);
	}
	return q;
}

void draw_link(struct terminal *t, struct f_data_c *scr, int l)
{
	struct link *link = &scr->f_data->links[l];
	int xp = scr->xp;
	int yp = scr->yp;
	int xw = scr->xw;
	int yw = scr->yw;
	int vx, vy;
	struct view_state *vs = scr->vs;
	int f = 0;
	vx = vs->view_posx;
	vy = vs->view_pos;
	if (scr->link_bg) {
		internal("link background not empty");
		mem_free(scr->link_bg);
	}
	if (l == -1) return;
	switch (link->type) {
		int i;
		int q;
		case L_LINK:
		case L_CHECKBOX:
		case L_BUTTON:
		case L_SELECT:
		case L_FIELD:
		case L_AREA:
			q = 0;
			if (link->type == L_FIELD) {
				struct form_state *fs = find_form_state(scr, link->form);
				if (fs) q = fs->state - fs->vpos;
				/*else internal("link has no form control");*/
			} else if (link->type == L_AREA) {
				struct form_state *fs = find_form_state(scr, link->form);
				if (fs) q = _area_cursor(link->form, fs);
				/*else internal("link has no form control");*/
			}
			if (!(scr->link_bg = mem_alloc(link->n * sizeof(struct link_bg)))) return;
			scr->link_bg_n = link->n;
			for (i = 0; i < link->n; i++) {
				int x = link->pos[i].x + xp - vx;
				int y = link->pos[i].y + yp - vy;
				if (x >= xp && y >= yp && x < xp+xw && y < yp+yw) {
					unsigned co;
					co = get_char(t, x, y);
					if (scr->link_bg) scr->link_bg[i].x = x,
							  scr->link_bg[i].y = y,
							  scr->link_bg[i].c = co;
					if (!f || (link->type == L_CHECKBOX && i == 1) || (link->type == L_BUTTON && i == 2) || ((link->type == L_FIELD || link->type == L_AREA) && i == q)) {
						int xx = x, yy = y;
						if (link->type != L_FIELD && link->type != L_AREA) {
							if (((co >> 8) & 0x38) != (link->sel_color & 0x38)) xx = xp + xw - 1, yy = yp + yw - 1;
                                                }
						set_cursor(t, x, y, xx, yy);
                                                set_window_ptr(scr->ses->win, x, y);
						f = 1;
                                        }
					set_color(t, x, y, /*((link->sel_color << 3) | (co >> 11 & 7)) << 8*/ link->sel_color << 8);
				} else scr->link_bg[i].x = scr->link_bg[i].y = scr->link_bg[i].c = -1;
			}
			break;
		default: internal("bad link type");
	}
}

void free_link(struct f_data_c *scr)
{
	if (scr->link_bg) {
		mem_free(scr->link_bg);
		scr->link_bg = NULL;
	}
	scr->link_bg_n = 0;
}

void clear_link(struct terminal *t, struct f_data_c *scr)
{
	if (scr->link_bg) {
		int i;
		for (i = scr->link_bg_n - 1; i >= 0; i--)
			set_char(t, scr->link_bg[i].x, scr->link_bg[i].y, scr->link_bg[i].c);
		free_link(scr);
	}
}

int get_range(struct f_data *f, int y, int yw, int l, struct search **s1, struct search **s2)
{
	int i;
	*s1 = *s2 = NULL;
	for (i = y < 0 ? 0 : y; i < y + yw && i < f->y; i++) {
		if (f->slines1[i] && (!*s1 || f->slines1[i] < *s1)) *s1 = f->slines1[i];
		if (f->slines2[i] && (!*s2 || f->slines2[i] > *s2)) *s2 = f->slines2[i];
	}
	if (!*s1 || !*s2) return -1;
	*s1 -= l;
	if (*s1 < f->search) *s1 = f->search;
	if (*s2 + l > f->search + f->nsearch) *s2 = f->search + f->nsearch - l;
	if (*s1 > *s2) *s1 = *s2 = NULL;
	if (!*s1 || !*s2) return -1;
	return 0;
}

int is_in_range(struct f_data *f, int y, int yw, unsigned char *txt, int *min, int *max)
{
	int found = 0;
	int l = strlen(txt);
	struct search *s1, *s2;
	if (min || max) *min = MAXINT, *max = 0;
	if (get_range(f, y, yw, l, &s1, &s2)) return 0;
	for (; s1 <= s2; s1++) {
		int i;
		if (srch_cmp(s1->c, txt[0])) {
			unable_to_handle_kernel_paging_request___oops:
			continue;
		}
		for (i = 1; i < l; i++) if (srch_cmp(s1[i].c, txt[i])) goto unable_to_handle_kernel_paging_request___oops;
		if (s1[i].y < y || s1[i].y >= y + yw) continue;
		if (!min && !max) return 1;
		found = 1;
		for (i = 0; i < l; i++) if (s1[i].n) {
			if (s1[i].x < *min) *min = s1[i].x;
			if (s1[i].x + s1[i].n > *max) *max = s1[i].x + s1[i].n;
		}
	}
	return found;
}

void get_searched(struct f_data_c *scr, struct point **pt, int *pl)
{
	int xp = scr->xp;
	int yp = scr->yp;
	int xw = scr->xw;
	int yw = scr->yw;
	int vx = scr->vs->view_posx;
	int vy = scr->vs->view_pos;
	struct search *s1, *s2;
	int l;
	unsigned char c;
	struct point *points = DUMMY;
	int len = 0;
	unsigned char *w = scr->ses->search_word;
	if (!w || !*w) return;
	get_search_data(scr->f_data);
	l = strlen(w);
	c = w[0];
	if (get_range(scr->f_data, scr->vs->view_pos, scr->yw, l, &s1, &s2)) goto ret;
	for (; s1 <= s2; s1++) {
		int i, j;
		if (srch_cmp(s1->c, c)) {
			c:continue;
		}
		for (i = 1; i < l; i++) if (srch_cmp(s1[i].c, w[i])) goto c;
		for (i = 0; i < l; i++) for (j = 0; j < s1[i].n; j++) {
			int x = s1[i].x + j + xp - vx;
			int y = s1[i].y + yp - vy;
			if (x >= xp && y >= yp && x < xp + xw && y < yp + yw) {
				/*unsigned co;
				co = get_char(t, x, y);
				co = ((co >> 3) & 0x0700) | ((co << 3) & 0x3800);
				set_color(t, x, y, co);*/
				if (!(len & (ALLOC_GR-1))) {
					struct point *npt;
					if (!(npt = mem_realloc(points, sizeof(struct point) * (len + ALLOC_GR)))) continue;
					points = npt;
				}
				points[len].x = s1[i].x + j;
				points[len++].y = s1[i].y;
			}
		}
	}
	ret:
	*pt = points;
	*pl = len;
}

void draw_searched(struct terminal *t, struct f_data_c *scr)
{
	int xp = scr->xp;
	int yp = scr->yp;
	int vx = scr->vs->view_posx;
	int vy = scr->vs->view_pos;
	struct point *pt;
	int len, i;
	if (!scr->ses->search_word || !scr->ses->search_word[0]) return;
	get_searched(scr, &pt, &len);
	for (i = 0; i < len; i++) {
		int x = pt[i].x + xp - vx, y = pt[i].y + yp - vy;
		unsigned co;
		co = get_char(t, x, y);
		co = ((co >> 3) & 0x0700) | ((co << 3) & 0x3800);
		set_color(t, x, y, co);
	}
	mem_free(pt);
}

void draw_current_link(struct terminal *t, struct f_data_c *scr)
{
	draw_link(t, scr, scr->vs->current_link);
	draw_searched(t, scr);
}

struct link *get_first_link(struct f_data_c *f)
{
	int i;
	struct link *l = f->f_data->links + f->f_data->nlinks;
	for (i = f->vs->view_pos; i < f->vs->view_pos + f->yw; i++)
		if (i >= 0 && i < f->f_data->y && f->f_data->lines1[i] && f->f_data->lines1[i] < l)
			l = f->f_data->lines1[i];
	if (l == f->f_data->links + f->f_data->nlinks) l = NULL;
	return l;
}

struct link *get_last_link(struct f_data_c *f)
{
	int i;
	struct link *l = NULL;
	for (i = f->vs->view_pos; i < f->vs->view_pos + f->yw; i++)
		if (i >= 0 && i < f->f_data->y && f->f_data->lines2[i] > l)
			l = f->f_data->lines2[i];
	return l;
}

void fixup_select_state(struct form_control *fc, struct form_state *fs)
{
	int i;
	if (fs->state >= 0 && fs->state < fc->nvalues && !strcmp(fc->values[fs->state], fs->value)) return;
	for (i = 0; i < fc->nvalues; i++) {
		if (!strcmp(fc->values[i], fs->value)) {
			fs->state = i;
			return;
		}
	}
	fs->state = 0;
	if (fs->value) mem_free(fs->value);
	if (fc->nvalues) fs->value = stracpy(fc->values[0]);
	else fs->value = stracpy("");
}

void init_ctrl(struct form_control *form, struct form_state *fs)
{
	if (fs->value) mem_free(fs->value), fs->value = NULL;
	switch (form->type) {
		case FC_TEXT:
		case FC_PASSWORD:
		case FC_TEXTAREA:
			fs->value = stracpy(form->default_value);
			fs->state = strlen(form->default_value);
			fs->vpos = 0;
			break;
		case FC_FILE:
			fs->value = stracpy("");
			fs->state = 0;
			fs->vpos = 0;
			break;
		case FC_CHECKBOX:
		case FC_RADIO:
			fs->state = form->default_state;
			break;
		case FC_SELECT:
			fs->value = stracpy(form->default_value);
			fs->state = form->default_state;
			fixup_select_state(form, fs);
			break;
	}
}

struct form_state *find_form_state(struct f_data_c *f, struct form_control *form)
{
	struct view_state *vs = f->vs;
	struct form_state *fs;
	int n = form->g_ctrl_num;
	if (n < vs->form_info_len) fs = &vs->form_info[n];
	else {
		if (!(fs = mem_realloc(vs->form_info, (n + 1) * sizeof(struct form_state))))
			return NULL;
		vs->form_info = fs;
		memset(fs + vs->form_info_len, 0, (n + 1 - vs->form_info_len) * sizeof(struct form_state));
		vs->form_info_len = n + 1;
		fs = &vs->form_info[n];
	}
//	if (fs->form_num == form->form_num && fs->ctrl_num == form->ctrl_num && fs->g_ctrl_num == form->g_ctrl_num && /*fs->position == form->position &&*/ fs->type == form->type) return fs;
	if (fs->type == form->type) return fs;
	if (fs->value) mem_free(fs->value);
	memset(fs, 0, sizeof(struct form_state));
	fs->form_num = form->form_num;
	fs->ctrl_num = form->ctrl_num;
	fs->g_ctrl_num = form->g_ctrl_num;
	fs->position = form->position;
	fs->type = form->type;
	init_ctrl(form, fs);
	return fs;
}

void draw_form_entry(struct terminal *t, struct f_data_c *f, struct link *l)
{
	int xp = f->xp;
	int yp = f->yp;
	int xw = f->xw;
	int yw = f->yw;
	struct view_state *vs = f->vs;
	int vx = vs->view_posx;
	int vy = vs->view_pos;
	struct form_state *fs;
	struct form_control *form = l->form;
	int i, x, y;
	if (!form) {
		internal("link %d has no form", (int)(l - f->f_data->links));
		return;
	}
	if (!(fs = find_form_state(f, form))) return;
	switch (form->type) {
		unsigned char *s;
		struct line_info *ln, *lnx;
		int sl;
		case FC_TEXT:
		case FC_PASSWORD:
		case FC_FILE:
			if (fs->state >= fs->vpos + form->size) fs->vpos = fs->state - form->size + 1;
			if (fs->state < fs->vpos) fs->vpos = fs->state;
			if (!l->n) break;
			x = l->pos[0].x + xp - vx; y = l->pos[0].y + yp - vy;
			for (i = 0; i < form->size; i++, x++)
				if (x >= xp && y >= yp && x < xp+xw && y < yp+yw) {
					if (fs->value && i >= -fs->vpos && i < strlen(fs->value) - fs->vpos) set_only_char(t, x, y, form->type != FC_PASSWORD ? fs->value[i + fs->vpos] : '*');
					else set_only_char(t, x, y, '_');
				}
			break;
		case FC_TEXTAREA:
			if (!l->n) break;
			x = l->pos[0].x + xp - vx; y = l->pos[0].y + yp - vy;
			_area_cursor(form, fs);
			if (!(lnx = format_text(fs->value, form->cols, form->wrap))) break;
			ln = lnx;
			sl = fs->vypos;
			while (ln->st && sl) sl--, ln++;
			for (; ln->st && y < l->pos[0].y + yp - vy + form->rows; ln++, y++) {
				for (i = 0; i < form->cols; i++) {
					if (x+i >= xp && y >= yp && x+i < xp+xw && y < yp+yw) {
						if (fs->value && i >= -fs->vpos && i + fs->vpos < ln->en - ln->st) set_only_char(t, x+i, y, ln->st[i + fs->vpos]);
						else set_only_char(t, x+i, y, '_');
					}
				}
			}
			for (; y < l->pos[0].y + yp - vy + form->rows; y++) {
				for (i = 0; i < form->cols; i++) {
					if (x+i >= xp && y >= yp && x+i < xp+xw && y < yp+yw)
						set_only_char(t, x+i, y, '_');
				}
			}
			
			mem_free(lnx);
			break;
		case FC_CHECKBOX:
			if (l->n < 2) break;
			x = l->pos[1].x + xp - vx;
			y = l->pos[1].y + yp - vy;
			if (x >= xp && y >= yp && x < xp+xw && y < yp+yw)
				set_only_char(t, x, y, fs->state ? 'X' : ' ');
			break;
		case FC_RADIO:
			if (l->n < 2) break;
			x = l->pos[1].x + xp - vx;
			y = l->pos[1].y + yp - vy;
			if (x >= xp && y >= yp && x < xp+xw && y < yp+yw)
				set_only_char(t, x, y, fs->state ? 'X' : ' ');
			break;
		case FC_SELECT:
			fixup_select_state(form, fs);
			s = fs->state < form->nvalues ? form->labels[fs->state] : (unsigned char *)"";
			sl = s ? strlen(s) : 0;
			for (i = 0; i < l->n; i++) {
				x = l->pos[i].x + xp - vx;
				y = l->pos[i].y + yp - vy;
				if (x >= xp && y >= yp && x < xp+xw && y < yp+yw)
					set_only_char(t, x, y, i < sl ? s[i] : '_');
			}
			break;
		case FC_SUBMIT:
		case FC_IMAGE:
		case FC_RESET:
		case FC_HIDDEN:
		case FC_BUTTON:
			break;
	}
}

struct xdfe {
	struct f_data_c *f;
	struct link *l;
};

void y_draw_form_entry(struct terminal *t, struct xdfe *x)
{
	draw_form_entry(t, x->f, x->l);
}

void x_draw_form_entry(struct session *ses, struct f_data_c *f, struct link *l)
{
	struct xdfe x;
	x.f = f, x.l = l;
	draw_to_window(ses->win, (void (*)(struct terminal *, void *))y_draw_form_entry, &x);
}

void draw_forms(struct terminal *t, struct f_data_c *f)
{
	struct link *l1 = get_first_link(f);
	struct link *l2 = get_last_link(f);
	if (!l1 || !l2) {
		if (l1 || l2) internal("get_first_link == %p, get_last_link == %p", l1, l2);
		return;
	}
	do {
		if (l1->type != L_LINK) draw_form_entry(t, f, l1);
	} while (l1++ < l2);
}

void draw_frame_lines(struct terminal *t, struct frameset_desc *fsd, int xp, int yp)
{
	int i, j;
	int x, y;
	if (!fsd) return;
	y = yp - 1;
	for (j = 0; j < fsd->y; j++) {
		int wwy = fsd->f[j * fsd->x].yw;
		x = xp - 1;
		for (i = 0; i < fsd->x; i++) {
			int wwx = fsd->f[i].xw;
			if (i) fill_area(t, x, y + 1, 1, wwy, 179 | ATTR_FRAME);
			if (j) fill_area(t, x + 1, y, wwx, 1, 196 | ATTR_FRAME);
			if (i && j) set_char(t, x, y, 197 | ATTR_FRAME);
			/*if (fsd->f[j * fsd->x + i].subframe) {
				draw_frame_lines(t, fsd->f[j * fsd->x + i].subframe, x + 1, y + 1);
			}*/
			x += wwx + 1;
		}
		y += wwy + 1;
	}
}

void draw_doc(struct terminal *t, struct f_data_c *scr)
{
	int active = scr->active;
	int y;
	int xp = scr->xp;
	int yp = scr->yp;
	int xw = scr->xw;
	int yw = scr->yw;
	struct view_state *vs;
	int vx, vy;
	if (!scr->vs || !scr->f_data) {
		if (!F) {
			if (active) {
				if (!scr->parent) set_cursor(t, 0, 0, 0, 0);
				else set_cursor(t, xp, yp, xp, yp);
			}
			fill_area(t, xp, yp, xw, yw, ' ');
#ifdef G
		} else {
			struct rgb color = options_get_rgb("default_color_bg_g");
			drv->fill_area(t->dev, xp, yp, xp + xw, yp + yw, dip_get_color_sRGB((color.r*256+color.g)*256+color.b));
#endif
		}
		if (active) set_window_ptr(scr->ses->win, xp, yp);
		return;
	}
	if (active) {
		if (!F) set_cursor(t, xp + xw - 1, yp + yw - 1, xp + xw - 1, yp + yw - 1);
		if (!F) set_window_ptr(scr->ses->win, xp, yp);
	}
	check_vs(scr);
	if (scr->f_data->frame_desc) {
		struct f_data_c *f;
		int n;
	 	if (!F) {
			fill_area(t, xp, yp, xw, yw, scr->f_data->y ? scr->f_data->bg : ' ');
			draw_frame_lines(t, scr->f_data->frame_desc, xp, yp);
		}
		n = 0;
		foreach(f, scr->subframes) {
			f->active = active && n++ == scr->vs->frame_pos;
			draw_doc(t, f);
		}
		return;
	}
	vs = scr->vs;
	if (scr->goto_position && (vy = find_tag(scr->f_data, scr->goto_position)) != -1) {
		if (vy > scr->f_data->y) vy = scr->f_data->y - 1;
		if (vy < 0) vy = 0;
		vs->view_pos = vy;
                if (!F) set_link(scr);
		mem_free(scr->goto_position);	/* !!! FIXME: opravit goto_position */
		scr->goto_position = NULL;
	}
	if (!F) {
		vx = vs->view_posx;
		vy = vs->view_pos;
		if (scr->xl == vx && scr->yl == vy && scr->xl != -1 && !scr->ses->search_word) {
			clear_link(t, scr);
			draw_forms(t, scr);
			if (active) draw_current_link(t, scr);
			return;
		}
		free_link(scr);
		scr->xl = vx;
		scr->yl = vy;
		fill_area(t, xp, yp, xw, yw, scr->f_data->y ? scr->f_data->bg : ' ');
		if (!scr->f_data->y) return;
		while (vs->view_pos >= scr->f_data->y) vs->view_pos -= yw;
		if (vs->view_pos < 0) vs->view_pos = 0;
		if (vy != vs->view_pos) vy = vs->view_pos, check_vs(scr);
		for (y = vy <= 0 ? 0 : vy; y < (-vy + scr->f_data->y <= yw ? scr->f_data->y : yw + vy); y++) {
			int st = vx <= 0 ? 0 : vx;
			int en = -vx + scr->f_data->data[y].l <= xw ? scr->f_data->data[y].l : xw + vx;
			set_line(t, xp + st - vx, yp + y - vy, en - st, &scr->f_data->data[y].d[st]);
		}
		draw_forms(t, scr);
		if (active) draw_current_link(t, scr);
		if (scr->ses->search_word) scr->xl = scr->yl = -1;
#ifdef G
	} else {
		draw_graphical_doc(t, scr, active);
#endif
	}
}

void clr_xl(struct f_data_c *fd)
{
	struct f_data_c *fdd;
	fd->xl = fd->yl = -1;
	foreach(fdd, fd->subframes) clr_xl(fdd);
}

void draw_doc_c(struct terminal *t, struct f_data_c *scr)
{
	clr_xl(scr);
#ifdef G
	if (F) if (scr == scr->ses->screen) draw_title(scr);
#endif
	draw_doc(t, scr);
}

void draw_formatted(struct session *ses)
{
	/*clr_xl(ses->screen);*/
	ses->screen->active = 1;
	draw_to_window(ses->win, (void (*)(struct terminal *, void *))draw_doc_c, ses->screen);
	change_screen_status(ses);
	print_screen_status(ses);
}

int is_active_frame(struct session *ses, struct f_data_c *f);

void draw_fd(struct f_data_c *f)
{
	if (f->f_data) f->f_data->time_to_draw = -get_time();
	f->active = is_active_frame(f->ses, f);
	draw_to_window(f->ses->win, (void (*)(struct terminal *, void *))draw_doc_c, f);
	change_screen_status(f->ses);
	print_screen_status(f->ses);
	if (f->f_data) f->f_data->time_to_draw += get_time();
}

void draw_fd_nrd(struct f_data_c *f)
{
	f->active = is_active_frame(f->ses, f);
	draw_to_window(f->ses->win, (void (*)(struct terminal *, void *))draw_doc, f);
	change_screen_status(f->ses);
	print_screen_status(f->ses);
}

#define D_BUF	65536

extern unsigned char frame_dumb[];

int dump_to_file(struct f_data *fd, int h)
{
	int x, y;
	unsigned char *buf;
	int bptr = 0;
	if (!(buf = mem_alloc(D_BUF))) return -1;
	for (y = 0; y < fd->y; y++) for (x = 0; x <= fd->data[y].l; x++) {
		int c;
		if (x == fd->data[y].l) c = '\n';
		else {
			if (((c = fd->data[y].d[x]) & 0xff) == 1) c += ' ' - 1;
			if ((c >> 15) && (c & 0xff) >= 176 && (c & 0xff) < 224) c = frame_dumb[(c & 0xff) - 176];
		}
		buf[bptr++] = c;
		if (bptr >= D_BUF) {
			if (hard_write(h, buf, bptr) != bptr) goto fail;
			bptr = 0;
		}
	}
	if (hard_write(h, buf, bptr) != bptr) {
		fail:
		mem_free(buf);
		return -1;
	}
	mem_free(buf);
	return 0;
}

int in_viewx(struct f_data_c *f, struct link *l)
{
	int i;
	for (i = 0; i < l->n; i++) {
		if (l->pos[i].x >= f->vs->view_posx && l->pos[i].x < f->vs->view_posx + f->xw)
			return 1;
	}
	return 0;
}

int in_viewy(struct f_data_c *f, struct link *l)
{
	int i;
	for (i = 0; i < l->n; i++) {
		if (l->pos[i].y >= f->vs->view_pos && l->pos[i].y < f->vs->view_pos + f->yw)
		return 1;
	}
	return 0;
}

int in_view(struct f_data_c *f, struct link *l)
{
	return in_viewy(f, l) && in_viewx(f, l);
}

static inline int c_in_view(struct f_data_c *f)
{
	return f->vs->current_link != -1 && in_view(f, &f->f_data->links[f->vs->current_link]);
}

int next_in_view(struct f_data_c *f, int p, int d, int (*fn)(struct f_data_c *, struct link *), void (*cntr)(struct f_data_c *, struct link *))
{
	int p1 = f->f_data->nlinks - 1;
	int p2 = 0;
	int y;
	int yl = f->vs->view_pos + f->yw;
	if (yl > f->f_data->y) yl = f->f_data->y;
	for (y = f->vs->view_pos < 0 ? 0 : f->vs->view_pos; y < yl; y++) {
		if (f->f_data->lines1[y] && f->f_data->lines1[y] - f->f_data->links < p1) p1 = f->f_data->lines1[y] - f->f_data->links;
		if (f->f_data->lines2[y] && f->f_data->lines2[y] - f->f_data->links > p2) p2 = f->f_data->lines2[y] - f->f_data->links;
	}
	/*while (p >= 0 && p < f->f_data->nlinks) {*/
	while (p >= p1 && p <= p2) {
		if (fn(f, &f->f_data->links[p])) {
			f->vs->current_link = p;
			if (cntr) cntr(f, &f->f_data->links[p]);
			return 1;
		}
		p += d;
	}
	f->vs->current_link = -1;
	return 0;
}

void set_pos_x(struct f_data_c *f, struct link *l)
{
	int i;
	int xm = 0;
	int xl = MAXINT;
	for (i = 0; i < l->n; i++) {
		if (l->pos[i].y >= f->vs->view_pos && l->pos[i].y < f->vs->view_pos + f->yw) {
			if (l->pos[i].x >= xm) xm = l->pos[i].x + 1;
			if (l->pos[i].x < xl) xl = l->pos[i].x;
		}
	}
	if (xl == MAXINT) return;
	/*if ((f->vs->view_posx = xm - f->xw) > xl) f->vs->view_posx = xl;*/
	if (f->vs->view_posx + f->xw < xm) f->vs->view_posx = xm - f->xw;
	if (f->vs->view_posx > xl) f->vs->view_posx = xl;
}

void set_pos_y(struct f_data_c *f, struct link *l)
{
	int i;
	int ym = 0;
	int yl = f->f_data->y;
	for (i = 0; i < l->n; i++) {
		if (l->pos[i].y >= ym) ym = l->pos[i].y + 1;
		if (l->pos[i].y < yl) yl = l->pos[i].y;
	}
	if ((f->vs->view_pos = (ym + yl) / 2 - f->f_data->opt.yw / 2) > f->f_data->y - f->f_data->opt.yw) f->vs->view_pos = f->f_data->y - f->f_data->opt.yw;
	if (f->vs->view_pos < 0) f->vs->view_pos = 0;
}

void find_link(struct f_data_c *f, int p, int s)
{ /* p=1 - top, p=-1 - bottom, s=0 - pgdn, s=1 - down */
	int y;
	int l;
	struct link *link;
	struct link **line = p == -1 ? f->f_data->lines2 : f->f_data->lines1;
	if (p == -1) {
		y = f->vs->view_pos + f->yw - 1;
		if (y >= f->f_data->y) y = f->f_data->y - 1;
	} else {
		y = f->vs->view_pos;
		if (y < 0) y = 0;
	}
	if (y < 0 || y >= f->f_data->y) goto nolink;
	link = NULL;
	do {
		if (line[y] && (!link || (p > 0 ? line[y] < link : line[y] > link))) link = line[y];
		y += p;
	} while (!(y < 0 || y < f->vs->view_pos || y >= f->vs->view_pos + f->f_data->opt.yw || y >= f->f_data->y));
	if (!link) goto nolink;
	l = link - f->f_data->links;
	if (s == 0) {
		next_in_view(f, l, p, in_view, NULL);
		return;
	}
	f->vs->current_link = l;
	set_pos_x(f, link);
	return;
	nolink:
	f->vs->current_link = -1;
}

void page_down(struct session *ses, struct f_data_c *f, int a)
{
	if (f->vs->view_pos + f->f_data->opt.yw < f->f_data->y) f->vs->view_pos += f->f_data->opt.yw, find_link(f, 1, a);
	else find_link(f, -1, a);
}

void page_up(struct session *ses, struct f_data_c *f, int a)
{
	f->vs->view_pos -= f->yw;
	find_link(f, -1, a);
	if (f->vs->view_pos < 0) f->vs->view_pos = 0/*, find_link(f, 1, a)*/;
}

void set_textarea(struct session *, struct f_data_c *, int);

void down(struct session *ses, struct f_data_c *f, int a)
{
	int l = f->vs->current_link;
	/*if (f->vs->current_link >= f->nlinks - 1) return;*/
	if (f->vs->current_link == -1 || !next_in_view(f, f->vs->current_link+1, 1, in_viewy, set_pos_x)) page_down(ses, f, 1);
	if (l != f->vs->current_link) set_textarea(ses, f, KBD_UP);
}

void up(struct session *ses, struct f_data_c *f, int a)
{
	int l = f->vs->current_link;
	/*if (f->vs->current_link == 0) return;*/
	if (f->vs->current_link == -1 || !next_in_view(f, f->vs->current_link-1, -1, in_viewy, set_pos_x)) page_up(ses, f, 1);
	if (l != f->vs->current_link) set_textarea(ses, f, KBD_DOWN);
}

void scroll(struct session *ses, struct f_data_c *f, int a)
{
	if (f->vs->view_pos + f->f_data->opt.yw >= f->f_data->y && a > 0) return;
	f->vs->view_pos += a;
	if (f->vs->view_pos > f->f_data->y - f->f_data->opt.yw && a > 0) f->vs->view_pos = f->f_data->y - f->f_data->opt.yw;
	if (f->vs->view_pos < 0) f->vs->view_pos = 0;
	if (c_in_view(f)) return;
	find_link(f, a < 0 ? -1 : 1, 0);
}

void hscroll(struct session *ses, struct f_data_c *f, int a)
{
	f->vs->view_posx += a;
	if (f->vs->view_posx >= f->f_data->x) f->vs->view_posx = f->f_data->x - 1;
	if (f->vs->view_posx < 0) f->vs->view_posx = 0;
	if (c_in_view(f)) return;
	find_link(f, 1, 0);
	/* !!! FIXME: check right margin */
}

void home(struct session *ses, struct f_data_c *f, int a)
{
	f->vs->view_pos = f->vs->view_posx = 0;
	find_link(f, 1, 0);
}

void x_end(struct session *ses, struct f_data_c *f, int a)
{
	f->vs->view_posx = 0;
	if (f->vs->view_pos < f->f_data->y - f->f_data->opt.yw) f->vs->view_pos = f->f_data->y - f->f_data->opt.yw;
	if (f->vs->view_pos < 0) f->vs->view_pos = 0;
	find_link(f, -1, 0);
}

int has_form_submit(struct f_data *f, struct form_control *form)
{
	struct form_control *i;
	int q = 0;
	if (F) return 0;
	foreach (i, f->forms) if (i->form_num == form->form_num) {
		if ((i->type == FC_SUBMIT || i->type == FC_IMAGE)) return 1;
		q = 1;
	}
	if (!q) internal("form is not on list");
	return 0;
}

struct submitted_value {
	struct submitted_value *next;
	struct submitted_value *prev;
	int type;
	unsigned char *name;
	unsigned char *value;
	void *file_content;
	int fc_len;
	int position;
#ifdef FORM_SAVE
	struct form_control *fc;
#endif
};

void free_succesful_controls(struct list_head *submit)
{
	struct submitted_value *v;
	foreach(v, *submit) {
		if (v->name) mem_free(v->name);
		if (v->value) mem_free(v->value);
		if (v->file_content) mem_free(v->file_content);
	}
	free_list(*submit);
}

unsigned char *encode_textarea(unsigned char *t)
{
	int len = 0;
	unsigned char *o = init_str();
	for (; *t; t++) {
		if (*t != '\n') add_chr_to_str(&o, &len, *t);
		else add_to_str(&o, &len, "\r\n");
	}
	return o;
}

static void fix_control_order(struct list_head *l)
{
	int ch;
	do { 
		struct submitted_value *sub, *nx;
		ch = 0;
		foreach(sub, *l) if (sub->next != (void *)l)
			if (sub->next->position < sub->position) {
				nx = sub->next;
				del_from_list(sub);
				add_at_pos(nx, sub);
				sub = nx;
				ch = 1;
			}
		foreachback(sub, *l) if (sub->next != (void *)l)
			if (sub->next->position < sub->position) {
				nx = sub->next;
				del_from_list(sub);
				add_at_pos(nx, sub);
				sub = nx;
				ch = 1;
			}
	} while (ch);
}

void get_succesful_controls(struct f_data_c *f, struct form_control *fc, struct list_head *subm)
{
	int ch;
	struct form_control *form;
	init_list(*subm);
	foreach(form, f->f_data->forms) {
		if (form->form_num == fc->form_num && ((form->type != FC_SUBMIT && form->type != FC_IMAGE && form->type != FC_RESET && form->type != FC_BUTTON ) || form == fc) && form->name && form->name[0]) {
			struct submitted_value *sub;
			struct form_state *fs;
			int fi = 0;
			int svl;
			if (!(fs = find_form_state(f, form))) continue;
			if ((form->type == FC_CHECKBOX || form->type == FC_RADIO) && !fs->state) continue;
			if (form->type == FC_BUTTON) continue;
			if (form->type == FC_SELECT && !form->nvalues) continue;
			fi_rep:
			if (!(sub = mem_alloc(sizeof(struct submitted_value)))) continue;
			memset(sub, 0, sizeof(struct submitted_value));
			sub->type = form->type;
			sub->name = stracpy(form->name);
			switch (form->type) {
				case FC_TEXT:
				case FC_PASSWORD:
				case FC_FILE:
					sub->value = stracpy(fs->value);
					break;
				case FC_TEXTAREA:
					sub->value = encode_textarea(fs->value);
					break;
				case FC_CHECKBOX:
				case FC_RADIO:
				case FC_SUBMIT:
				case FC_HIDDEN:
					sub->value = stracpy(form->default_value);
					break;
				case FC_SELECT:
					fixup_select_state(form, fs);
					sub->value = stracpy(fs->value);
					break;
				case FC_IMAGE:
					add_to_strn(&sub->name, fi ? ".x" : ".y");
					/*sub->value = stracpy("0");*/
					sub->value = init_str();
					svl = 0;
					add_num_to_str(&sub->value, &svl, fi ? ismap_x : ismap_y);
					break;
				default:
					internal("bad form control type");
					mem_free(sub);
					continue;
			}
			sub->position = form->form_num + form->ctrl_num;
			add_to_list(*subm, sub);
			if (form->type == FC_IMAGE && !fi) {
				fi = 1;
				goto fi_rep;
			}
		}
	}
        fix_control_order(subm);
}

unsigned char *strip_file_name(unsigned char *f)
{
	unsigned char *n;
	unsigned char *l = f - 1;
	for (n = f; *n; n++) if (dir_sep(*n)) l = n;
	return l + 1;
}

static inline int safe_char(unsigned char c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c== '.' || c == '-' || c == '_';
}

void encode_string(unsigned char *name, unsigned char **data, int *len)
{
	for (; *name; name++) {
		if (*name == ' ') add_chr_to_str(data, len, '+');
		else if (safe_char(*name)) add_chr_to_str(data, len, *name);
		else {
			unsigned char n[4];
			sprintf(n, "%%%02X", *name);
			add_to_str(data, len, n);
		}
	}
}

void encode_controls(struct list_head *l, unsigned char **data, int *len,
		     int cp_from, int cp_to)
{
	struct submitted_value *sv;
	int lst = 0;
	char *p2;
	struct conv_table *convert_table = get_translation_table(cp_from, cp_to);
	*len = 0;
	*data = init_str();
	foreach(sv, *l) {
		unsigned char *p = sv->value;
		if (lst) add_to_str(data, len, "&"); else lst = 1;
		encode_string(sv->name, data, len);
		add_to_str(data, len, "=");
		if (sv->type == FC_TEXT || sv->type == FC_PASSWORD || sv->type == FC_TEXTAREA)
			p2 = convert_string(convert_table, p, strlen(p), NULL);
		else p2 = stracpy(p);
		encode_string(p2, data, len);
		mem_free(p2);
	}
}

#define BL	32

void encode_multipart(struct session *ses, struct list_head *l, unsigned char **data, int *len,
		      unsigned char *bound, int cp_from, int cp_to)
{
	int *nbp, *bound_ptrs = DUMMY;
	int nbound_ptrs = 0;
	unsigned char *m1, *m2;
	struct submitted_value *sv;
	int i, j;
	int flg = 0;
	char *p;
	struct conv_table *convert_table = get_translation_table(cp_from, cp_to);
	memset(bound, 'x', BL);
	*len = 0;
	*data = init_str();
	foreach(sv, *l) {
		bnd:
		add_to_str(data, len, "--");
		if (!(nbound_ptrs & (ALLOC_GR-1))) {
			if (!(nbp = mem_realloc(bound_ptrs, (nbound_ptrs + ALLOC_GR) * sizeof(int)))) goto xx;
			bound_ptrs = nbp;
		}
		bound_ptrs[nbound_ptrs++] = *len;
		xx:
		add_bytes_to_str(data, len, bound, BL);
		if (flg) break;
		add_to_str(data, len, "\r\nContent-Disposition: form-data; name=\"");
		add_to_str(data, len, sv->name);
		if (sv->type == FC_FILE) {
			add_to_str(data, len, "\"; filename=\"");
			add_to_str(data, len, strip_file_name(sv->value));
				/* It sends bad data if the file name contains ", but
				   Netscape does the same */
		}
		add_to_str(data, len, "\"\r\n\r\n");
		if (sv->type != FC_FILE) {
			if (sv->type == FC_TEXT || sv->type == FC_PASSWORD || sv->type == FC_TEXTAREA)
				p = convert_string(convert_table, sv->value, strlen(sv->value), NULL);
			else p = stracpy(sv->value);
			add_to_str(data, len, p);
			mem_free(p);
		} else {
			int fh, rd;
#define F_BUFLEN 1024
			unsigned char buffer[F_BUFLEN];
			/*if (!check_file_name(sv->value)) {
				err = "File access forbidden";
				goto error;
			}*/
			if (*sv->value) {
				if (anonymous) goto error;
				if ((fh = open(sv->value, O_RDONLY)) == -1) goto error;
                                set_bin(fh);
                                do {
					if ((rd = read(fh, buffer, F_BUFLEN)) == -1) goto error;
					if (rd) add_bytes_to_str(data, len, buffer, rd);
				} while (rd);
				close(fh);
			}
		}
		add_to_str(data, len, "\r\n");
	}
	if (!flg) {
		flg = 1;
		goto bnd;
	}
	add_to_str(data, len, "--\r\n");
	memset(bound, '0', BL);
	again:
	for (i = 0; i <= *len - BL; i++) {
		for (j = 0; j < BL; j++) if ((*data)[i + j] != bound[j]) goto nb;
		for (j = BL - 1; j >= 0; j--)
			if (bound[j]++ >= '9') bound[j] = '0';
			else goto again;
		internal("Counld not assing boundary");
		nb:;
	}
	for (i = 0; i < nbound_ptrs; i++) memcpy(*data + bound_ptrs[i], bound, BL);
	mem_free(bound_ptrs);
	return;
	error:
	mem_free(bound_ptrs);
	mem_free(*data);
	*data = NULL;
	m1 = stracpy(sv->value);
	m2 = stracpy(strerror(errno));
	msg_box(ses->term, getml(m1, m2, NULL), TXT(T_ERROR_WHILE_POSTING_FORM), AL_CENTER | AL_EXTD_TEXT, TXT(T_COULD_NOT_GET_FILE), " ", m1, ": ", m2, NULL, ses, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
}

void reset_form(struct f_data_c *f, int form_num)
{
	struct form_control *form;
	foreach(form, f->f_data->forms) if (form->form_num == form_num) {
		struct form_state *fs;
		if ((fs = find_form_state(f, form))) init_ctrl(form, fs);
	}
}
		
unsigned char *get_form_url(struct session *ses, struct f_data_c *f, struct form_control *form, int *onsubmit)
{
	struct list_head submit;
	unsigned char *data;
	unsigned char bound[BL];
	int len;
	unsigned char *go = NULL;
	int cp_from, cp_to;
	if (!form) return NULL;
	if (form->type == FC_RESET) {
		reset_form(f, form->form_num);
		if (F) draw_fd(f);
		return NULL;
	}
	if (onsubmit)*onsubmit=0;
#ifdef JS
	if (form->onsubmit)
	{
		jsint_execute_code(f,form->onsubmit,strlen(form->onsubmit),-1,form->form_num,form->form_num);
		if (onsubmit)*onsubmit=1;
	}
#endif
	if (!form->action) return NULL;
	get_succesful_controls(f, form, &submit);
	cp_from = ses->term->spec->charset;
	cp_to = f->f_data->cp;
	if (form->method == FM_GET || form->method == FM_POST)
		encode_controls(&submit, &data, &len, cp_from, cp_to);
	else
		encode_multipart(ses, &submit, &data, &len, bound, cp_from, cp_to);
	if (!data) goto ff;
	if (!strncasecmp(form->action,"javascript:",11))
	{
		go=stracpy(form->action);
		goto x;
	}
	if (form->method == FM_GET) {
		if ((go = mem_alloc(strlen(form->action) + 1 + len + 1))) {
			unsigned char *pos;
			strcpy(go, form->action);
			if ((pos = strchr(go, '#'))) {
				unsigned char *poss = pos;
				pos = stracpy(pos);
				*poss = 0;
			}
			if (strchr(go, '?')) strcat(go, "&");
			else strcat(go, "?");
			strcat(go, data);
			if (pos) strcat(go, pos), mem_free(pos);
		}
	} else {
		int l = 0;
		int i;
		go = init_str();
		if (!go) goto x;
		add_to_str(&go, &l, form->action);
		add_chr_to_str(&go, &l, POST_CHAR);
		if (form->method == FM_POST) add_to_str(&go, &l, "application/x-www-form-urlencoded\n");
		else {
			add_to_str(&go, &l, "multipart/form-data; boundary=");
			add_bytes_to_str(&go, &l, bound, BL);
			add_to_str(&go, &l, "\n");
		}
		for (i = 0; i < len; i++) {
			unsigned char p[3];
			sprintf(p, "%02x", (int)data[i]);
			add_to_str(&go, &l, p);
		}
	}
	x:
	mem_free(data);
	ff:
	free_succesful_controls(&submit);
	return go;
}

int ismap_link = 0, ismap_x = 0, ismap_y = 0;

/* if onsubmit is not NULL it will contain 1 if link is submit and the form has an onsubmit handler */
unsigned char *get_link_url(struct session *ses, struct f_data_c *f, struct link *l, int *onsubmit)
{
	if (l->type == L_LINK) {
		if (!l->where) {
			if (l->where_img && (!F || (!f->f_data->opt.display_images && f->f_data->opt.plain != 2))) return stracpy(l->where_img);
			return NULL;
		}
		if (ismap_link && strlen(l->where) >= 4 && !strcmp(l->where + strlen(l->where) - 4, "?0,0")) {
			unsigned char *nu = init_str();
			int ll = 0;
			add_bytes_to_str(&nu, &ll, l->where, strlen(l->where) - 3);
			add_num_to_str(&nu, &ll, ismap_x);
			add_chr_to_str(&nu, &ll, ',');
			add_num_to_str(&nu, &ll, ismap_y);
			return nu;
		}
		return stracpy(l->where);
	}
	if (l->type != L_BUTTON && l->type != L_FIELD) return NULL;
	return get_form_url(ses, f, l->form, onsubmit);
}

struct menu_item *clone_select_menu(struct menu_item *m)
{
	struct menu_item *n = DUMMY;
	int i = 0;
	do {
		n = mem_realloc(n, (i + 1) * sizeof(struct menu_item));
		n[i].text = stracpy(m->text);
		n[i].rtext = stracpy(m->rtext);
		n[i].hotkey = stracpy(m->hotkey);
		n[i].in_m = m->in_m;
		n[i].free_i = 0;
		if ((n[i].func = m->func) != MENU_FUNC do_select_submenu) {
			n[i].data = m->data;
		} else n[i].data = clone_select_menu(m->data);
		i++;
	} while (m++->text);
	return n;
}

void free_select_menu(struct menu_item *m)
{
	struct menu_item *om = m;
	do {
		if (m->text) mem_free(m->text);
		if (m->rtext) mem_free(m->rtext);
		if (m->hotkey) mem_free(m->hotkey);
		if (m->func == MENU_FUNC do_select_submenu) free_select_menu(m->data);
	} while (m++->text);
	mem_free(om);
}

void set_frame(struct session *ses, struct f_data_c *f, int a)
{
	if (f == ses->screen) return;
	goto_url_not_from_dialog(ses, f->loc->url);
}

/* pokud je a==1, tak se nebude submitovat formular, kdyz kliknu na input field a formular nema submit */
/* New>0 means request for new tab */
int enter(struct session *ses, struct f_data_c *f, int a, int new)
{
	struct link *link;
	unsigned char *u;
	if (!f->f_data || f->vs->current_link == -1 || f->vs->current_link >= f->f_data->nlinks) return 1;
	link = &f->f_data->links[f->vs->current_link];
#ifdef JS
	if (link->js_event&&link->js_event->click_code)
		jsint_execute_code(f,link->js_event->click_code,strlen(link->js_event->click_code),-1,(link->type==L_BUTTON&&link->form&&link->form->type==FC_SUBMIT)?link->form->form_num:-1,-1);
#endif
	if (link->type == L_LINK || link->type == L_BUTTON) {
		int has_onsubmit;
		if (link->type==L_BUTTON&&link->form->type==FC_BUTTON)return 1;
		submit:
		if ((u = get_link_url(ses, f, link, &has_onsubmit))) {
#ifdef JS
			struct js_event_spec *s=link->js_event;
#endif
			if (strlen(u) >= 4 && !casecmp(u, "MAP@", 4))
				goto_imgmap(ses, u + 4, stracpy(u + 4), stracpy(link->target));
			else goto_url_f(
                                        ses,
                                        NULL,
                                        u,
                                        link->target,
                                        f,
                                        (link->type==L_BUTTON&&link->form&&link->form->type==FC_SUBMIT)?link->form->form_num:-1,
#ifdef JS
                                        (s&&(/*s->keyup_code||s->keydown_code||s->keypress_code||s->change_code||s->blur_code||s->focus_code||s->move_code||s->over_code||s->out_code||*/s->down_code||s->up_code||s->click_code||s->dbl_code))||has_onsubmit
#else
                                        0
#endif
                                        ,0,
                                        new
                                       );
			mem_free(u);
			return 2;
		}
		return 1;
	}
	if (link->type == L_CHECKBOX) {
		struct form_state *fs = find_form_state(f, link->form);
		if (link->form->ro) return 1;
		if (link->form->type == FC_CHECKBOX) fs->state = !fs->state;
		else {
			struct form_control *fc;
			int re = 0;
			foreach(fc, f->f_data->forms)
				if (fc->form_num == link->form->form_num && fc->type == FC_RADIO && !xstrcmp(fc->name, link->form->name)) {
					struct form_state *ffs = find_form_state(f, fc);
					if (ffs) ffs->state = 0;
					re = 1;
				}
			fs->state = 1;
			if (F && re) {
				draw_fd(f);
			}
		}
		return 1;
	}
	if (link->type == L_SELECT) {
		struct menu_item *m = clone_select_menu(link->form->menu);
		if (link->form->ro || !m) return 1;
		/* execute onfocus code of the select object */
#ifdef JS
		if (link->js_event&&link->js_event->focus_code)
		{
			jsint_execute_code(f,link->js_event->focus_code,strlen(link->js_event->focus_code),-1,-1,-1);
		}
#endif
		add_empty_window(ses->term, (void (*)(void *))free_select_menu, m);
		do_select_submenu(ses->term, m, ses);
		return 1;
	}
	if (link->type == L_FIELD || link->type == L_AREA) {
		/* pri enteru v textovem policku se bude posilat vzdycky       -- Brain */
		if (!has_form_submit(f->f_data, link->form) && (!a || !F)) goto submit;
#ifdef JS
		/* process onfocus handler */
		if (!ses->locked_link&&f->vs&&f->f_data&&f->vs->current_link>=0&&f->vs->current_link<f->f_data->nlinks)
		{
			struct link *lnk=&(f->f_data->links[f->vs->current_link]);
			if (lnk->js_event&&lnk->js_event->focus_code)
				jsint_execute_code(f,lnk->js_event->focus_code,strlen(lnk->js_event->focus_code),-1,-1,-1);
		}
#endif
#ifdef G
		if (F) {
			ses->locked_link = 1;
			return 2;
		}
#endif
		down(ses, f, 0);
		return 1;
	}
	internal("bad link type %d", link->type);
	return 1;
}

void toggle(struct session *ses, struct f_data_c *f, int a)
{
	if (!f || !f->vs) return;
	if (f->vs->plain == -1) f->vs->plain = 1;
	else f->vs->plain = f->vs->plain ^ 1;
	html_interpret_recursive(f);
	draw_formatted(ses);
}

void back(struct session *ses, struct f_data_c *f, int a)
{
	go_back(ses, NULL);
}

void selected_item(struct terminal *term, void *pitem, struct session *ses)
{
	int item = (int)pitem;
	int old_item=item;
	struct f_data_c *f = current_frame(ses);
	struct link *l;
	struct form_state *fs;
	if (!f) return;
	if (f->vs->current_link == -1) return;
	l = &f->f_data->links[f->vs->current_link];
	if (l->type != L_SELECT) return;
	if ((fs = find_form_state(f, l->form))) {
		struct form_control *form= l->form;
		if (item >= 0 && item < form->nvalues) {
			old_item=fs->state;
			fs->state = item;
			if (fs->value) mem_free(fs->value);
			fs->value = stracpy(form->values[item]);
		}
		fixup_select_state(form, fs);
	}
	f->active = 1;
#ifdef G
	if (F) {
		f->xl = -1;
		f->yl = -1;
	}
#endif
	/* execute onchange handler */
#ifdef JS
	if (old_item!=item&&l->js_event&&l->js_event->change_code)
		jsint_execute_code(f,l->js_event->change_code,strlen(l->js_event->change_code),-1,-1,-1);
#endif
	/* execute onblur handler */
#ifdef JS
	if (l->js_event&&l->js_event->blur_code)
		jsint_execute_code(f,l->js_event->blur_code,strlen(l->js_event->blur_code),-1,-1,-1);
#endif
	draw_to_window(ses->win, (void (*)(struct terminal *, void *))draw_doc, f);
	change_screen_status(ses);
	print_screen_status(ses);
	/*if (!has_form_submit(f->f_data, l->form)) {
		goto_form(ses, f, l->form, l->target);
	}*/
}

int get_current_state(struct session *ses)
{
	struct f_data_c *f = current_frame(ses);
	struct link *l;
	struct form_state *fs;
	if (!f) return -1;
	if (f->vs->current_link == -1) return -1;
	l = &f->f_data->links[f->vs->current_link];
	if (l->type != L_SELECT) return -1;
	if ((fs = find_form_state(f, l->form))) return fs->state;
	return -1;
}


#ifdef JS
/* executes onkey-press/up/down handler */
static void field_op_changed(struct f_data_c *f, struct link *lnk)
{
	if (lnk->js_event&&lnk->js_event->keydown_code)
		jsint_execute_code(f,lnk->js_event->keydown_code,strlen(lnk->js_event->keydown_code),-1,-1,-1);
	if (lnk->js_event&&lnk->js_event->keypress_code)
		jsint_execute_code(f,lnk->js_event->keypress_code,strlen(lnk->js_event->keypress_code),-1,-1,-1);
	if (lnk->js_event&&lnk->js_event->keyup_code)
		jsint_execute_code(f,lnk->js_event->keyup_code,strlen(lnk->js_event->keyup_code),-1,-1,-1);
}
#endif

int field_op(struct session *ses, struct f_data_c *f, struct link *l, struct event *ev, int rep)
{
	struct form_control *form = l->form;
	struct form_state *fs;
	int x = 1;

	if (!form) {
		internal("link has no form control");
		return 0;
	}
	if (l->form->ro == 2) return 0;
	if (!(fs = find_form_state(f, form))) return 0;
	if (!fs->value) return 0;
	if (ev->ev == EV_KBD) {
		if (ev->x == KBD_LEFT) {
			if (!F) fs->state = fs->state ? fs->state - 1 : 0;
#ifdef G
			else {
				unsigned char *p = fs->value + fs->state;
				BACK_UTF_8(p, fs->value);
				fs->state = p - fs->value;
			}
#endif
		}
                else if (ev->x == KBD_RIGHT) {
			if (fs->state < strlen(fs->value)) {
				if (!F) fs->state = fs->state + 1;
#ifdef G
				else {
					unsigned char *p = fs->value + fs->state;
					FWD_UTF_8(p);
					fs->state = p - fs->value;
				}
#endif
			} else fs->state = strlen(fs->value);
		}
		else if (ev->x == KBD_HOME) {
			if (form->type == FC_TEXTAREA) {
				struct line_info *ln;
				if ((ln = format_text(fs->value, form->cols, form->wrap))) {
					int y;
					for (y = 0; ln[y].st; y++) if (fs->value + fs->state >= ln[y].st && fs->value + fs->state < ln[y].en + (ln[y+1].st != ln[y].en)) {
						fs->state = ln[y].st - fs->value;
						goto x;
					}
					fs->state = 0;
					x:
					mem_free(ln);
				}
			} else fs->state = 0;
		} else if (ev->x == KBD_UP) {
			if (form->type == FC_TEXTAREA) {
				struct line_info *ln;
				if ((ln = format_text(fs->value, form->cols, form->wrap))) {
					int y;
					rep1:
					for (y = 0; ln[y].st; y++) if (fs->value + fs->state >= ln[y].st && fs->value + fs->state < ln[y].en + (ln[y+1].st != ln[y].en)) {
						if (!y) {
							/*if (F) goto xx;*/
							mem_free(ln);
							goto b;
						}
						/*fs->state -= ln[y].st - ln[y-1].st;*/
						fs->state = utf8_add(ln[y-1].st, utf8_diff(fs->value + fs->state, ln[y].st)) - fs->value;
						if (fs->value + fs->state > ln[y-1].en) fs->state = ln[y-1].en - fs->value;
						goto xx;
					}
					mem_free(ln);
					goto b;
					xx:
					if (rep) goto rep1;
					mem_free(ln);
				}
				
			} else x = 0;
		} else if (ev->x == KBD_DOWN) {
			if (form->type == FC_TEXTAREA) {
				struct line_info *ln;
				if ((ln = format_text(fs->value, form->cols, form->wrap))) {
					int y;
					rep2:
					for (y = 0; ln[y].st; y++) if (fs->value + fs->state >= ln[y].st && fs->value + fs->state < ln[y].en + (ln[y+1].st != ln[y].en)) {
						if (!ln[y+1].st) {
							/*if (F) goto yy;*/
							mem_free(ln);
							goto b;
						}
						/*fs->state += ln[y+1].st - ln[y].st;*/
						fs->state = utf8_add(ln[y+1].st, utf8_diff(fs->value + fs->state, ln[y].st)) - fs->value;
						if (fs->value + fs->state > ln[y+1].en) fs->state = ln[y+1].en - fs->value;
						goto yy;
					}
					mem_free(ln);
					goto b;
					yy:
					if (rep) goto rep2;
					mem_free(ln);
				}
				
			} else x = 0;
		} else if (ev->x == KBD_END) {
			if (form->type == FC_TEXTAREA) {
				struct line_info *ln;
				if ((ln = format_text(fs->value, form->cols, form->wrap))) {
					int y;
					for (y = 0; ln[y].st; y++) if (fs->value + fs->state >= ln[y].st && fs->value + fs->state < ln[y].en + (ln[y+1].st != ln[y].en)) {
						fs->state = ln[y].en - fs->value;
						if (fs->state && fs->state < strlen(fs->value) && ln[y+1].st == ln[y].en) fs->state--;
						goto yyyy;
					}
					fs->state = strlen(fs->value);
					yyyy:
					mem_free(ln);
				}
			} else fs->state = strlen(fs->value);
		} else if (!ev->y && (ev->x >= 32 && ev->x < gf_val(256, MAXINT))) {
			if (!form->ro && strlen(fs->value) < form->maxlength) {
				unsigned char *v;
				if ((v = mem_realloc(fs->value, strlen(fs->value) + 12))) {
					unsigned char a_[2];
					unsigned char *nw;
					int ll;
					if (!F) {
						nw = a_;
						a_[0] = ev->x;
						a_[1] = 0;
#ifdef G
					} else {
						nw = encode_utf_8(ev->x);
#endif
					}
					ll = strlen(nw);
					if (ll > 10) goto bad;
					fs->value = v;
					memmove(v + fs->state + ll, v + fs->state, strlen(v + fs->state) + 1);
					memcpy(&v[fs->state], nw, ll);
					fs->state += ll;
#ifdef JS
					fs->changed=1;
					field_op_changed(f,l);
#endif
					bad:;
				}
			}
                } else if ((ev->x == KBD_INS && ev->y == KBD_CTRL)
                           || (upcase(ev->x) == 'Z' && ev->y == KBD_CTRL)) {

                        if (!F)
                                set_clipboard_text(fs->value);
#ifdef G
                        else
                                drv->put_to_clipboard(ses->term->dev,fs->value,strlen(fs->value));
#endif

                } else if ((ev->x == KBD_DEL && ev->y == KBD_SHIFT)
                           || (upcase(ev->x) == 'X' && ev->y == KBD_CTRL)) {

                        if (!F)
                                set_clipboard_text(fs->value);
#ifdef G
                        else
                                drv->put_to_clipboard(ses->term->dev,fs->value,strlen(fs->value));
#endif

                        if (!form->ro) fs->value[0] = 0;
			fs->state = 0;
#ifdef JS
			fs->changed=1;
			field_op_changed(f,l);
#endif
                } else if ((ev->x == KBD_INS && ev->y == KBD_SHIFT)
                           || (upcase(ev->x) == 'V' && ev->y == KBD_CTRL)) {
                        if(!F)
                                goto paste;
#ifdef G
                        else
                                drv->request_clipboard(ses->term->dev);
#endif

                } else if ( F && ev->x == KBD_PASTE ) {
			char * clipboard = NULL;
                        struct conv_table *conv = get_translation_table(get_cp_index(options_get("text_selection_clipboard_charset")),get_cp_index("UTF-8"));
                        unsigned char *text;

                paste:
                        if(!F)
                                text = get_clipboard_text();
#ifdef G
                        else
                                text = drv->get_from_clipboard(ses->term->dev);
#endif
                        clipboard = convert_string(conv, text, strlen(text), NULL);
                        mem_free(text);

                        if (!clipboard) goto brk;
			if (!form->ro && strlen(clipboard) <= form->maxlength) {
				unsigned char *v;
 				int orig_len = strlen(fs->value);
 				int orig_index = fs->state;
 				int added_len = strlen(clipboard);

 				if ((v = mem_realloc(fs->value, orig_len + added_len + 1))) {
  					fs->value = v;
 					memmove(v + orig_index + added_len, v + orig_index, orig_len - orig_index + 1);
 					memcpy(v + orig_index, clipboard, strlen(clipboard));
  					fs->state = strlen(fs->value);
  				}
			}
			mem_free(clipboard);
#ifdef JS
			fs->changed=1;
			field_op_changed(f,l);
#endif
			brk:;
		} else if (ev->x == KBD_ENTER && form->type == FC_TEXTAREA) {
			if (!form->ro && strlen(fs->value) < form->maxlength) {
				unsigned char *v;
				if ((v = mem_realloc(fs->value, strlen(fs->value) + 2))) {
					fs->value = v;
					memmove(v + fs->state + 1, v + fs->state, strlen(v + fs->state) + 1);
					v[fs->state++] = '\n';
#ifdef JS
					fs->changed=1;
					field_op_changed(f,l);
#endif
				}
			}
		} else if (ev->x == KBD_BS) {
			if (!form->ro && fs->state) {
				int ll = 1;
#ifdef G
				if (F) {
					unsigned char *p = fs->value + fs->state;
					BACK_UTF_8(p, fs->value);
					ll = fs->value + fs->state - p;
				}
#endif
				memmove(fs->value + fs->state - ll, fs->value + fs->state, strlen(fs->value + fs->state) + 1), fs->state -= ll
#ifdef JS
				, fs->changed=1, field_op_changed(f,l)
#endif
				;
			}
		} else if (ev->x == KBD_DEL) {
				int ll = 1;
#ifdef G
				if (F) {
					unsigned char *p = fs->value + fs->state;
					FWD_UTF_8(p);
					ll = p - (fs->value + fs->state);
				}
#endif
			if (!form->ro && fs->state < strlen(fs->value)) memmove(fs->value + fs->state, fs->value + fs->state + ll, strlen(fs->value + fs->state + ll) + 1)
#ifdef JS
				, fs->changed=1, field_op_changed(f,l)
#endif
				;
		} else if (upcase(ev->x) == 'U' && ev->y == KBD_CTRL) {
			if (!form->ro) memmove(fs->value, fs->value + fs->state, strlen(fs->value + fs->state) + 1);
			fs->state = 0;
#ifdef JS
			fs->changed=1;
			field_op_changed(f,l);
#endif
                } else if (F && ev->x == KBD_ESC) {
                        ses->locked_link=0;
                } else {
                b:
                        x = 0;
		}
#ifdef G
        } else if (ev->ev==EV_MOUSE && F &&
                   (ev->b & BM_ACT) == B_DOWN &&
                   (ev->b & BM_BUTT) == B_MIDDLE){
                drv->request_clipboard(ses->term->dev);
                x=1;
#endif
        } else x = 0;
        if (!F && x) x_draw_form_entry(ses, f, l);
	return x;
}

void set_textarea(struct session *ses, struct f_data_c *f, int kbd)
{
	if (f->vs->current_link != -1 && f->f_data->links[f->vs->current_link].type == L_AREA) {
		struct event ev = { EV_KBD, 0, 0, 0 };
		ev.x = kbd;
		field_op(ses, f, &f->f_data->links[f->vs->current_link], &ev, 1);
	}
}

void search_for_back(struct session *ses, unsigned char *str)
{
	struct f_data_c *f = current_frame(ses);
	if (!f || !str || !str[0]) return;
	if (ses->search_word) mem_free(ses->search_word);
	ses->search_word = stracpy(str);
	if (ses->last_search_word) mem_free(ses->last_search_word);
	ses->last_search_word = stracpy(str);
	ses->search_direction = -1;
	find_next(ses, f, 1);
}

void search_for(struct session *ses, unsigned char *str)
{
	struct f_data_c *f = current_frame(ses);
	if (!f || !f->vs || !f->f_data || !str || !str[0]) return;
	if (ses->search_word) mem_free(ses->search_word);
	ses->search_word = stracpy(str);
	if (ses->last_search_word) mem_free(ses->last_search_word);
	ses->last_search_word = stracpy(str);
	ses->search_direction = 1;
	find_next(ses, f, 1);
}

#define HASH_SIZE	4096

#define HASH(p) (((p.y << 6) + p.x) & (HASH_SIZE - 1))

int point_intersect(struct point *p1, int l1, struct point *p2, int l2)
{
	int i, j;
	static char hash[HASH_SIZE];
	static char init = 0;
	if (!init) memset(hash, 0, HASH_SIZE), init = 1;
	for (i = 0; i < l1; i++) hash[HASH(p1[i])] = 1;
	for (j = 0; j < l2; j++) if (hash[HASH(p2[j])]) {
		for (i = 0; i < l1; i++) if (p1[i].x == p2[j].x && p1[i].y == p2[j].y) {
			for (i = 0; i < l1; i++) hash[HASH(p1[i])] = 0;
			return 1;
		}
	}
	for (i = 0; i < l1; i++) hash[HASH(p1[i])] = 0;
	return 0;
}

int find_next_link_in_search(struct f_data_c *f, int d)
{
	struct point *pt;
	int len;
	struct link *link;
	if (d == -2 || d == 2) {
		d /= 2;
		find_link(f, d, 0);
		if (f->vs->current_link == -1) return 1;
	} else nx:if (f->vs->current_link == -1 || !(next_in_view(f, f->vs->current_link + d, d, in_view, NULL))) {
		find_link(f, d, 0);
		return 1;
	}
	link = &f->f_data->links[f->vs->current_link];
	get_searched(f, &pt, &len);
	if (point_intersect(pt, len, link->pos, link->n)) {
		mem_free(pt);
		return 0;
	}
	mem_free(pt);
	goto nx;
}

void find_next(struct session *ses, struct f_data_c *f, int a)
{
	int min, max;
	int c = 0;
	int p;
	if (!f->f_data || !f->vs) goto no;
	p = f->vs->view_pos;
	if (!F && !a && ses->search_word) {
		if (!(find_next_link_in_search(f, ses->search_direction))) return;
		p += ses->search_direction * f->yw;
	}
	if (!ses->search_word) {
		if (!ses->last_search_word) {
			no:
			msg_box(ses->term, NULL, TXT(T_SEARCH), AL_CENTER, TXT(T_NO_PREVIOUS_SEARCH), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
			return;
		}
		ses->search_word = stracpy(ses->last_search_word);
	}
#ifdef G
	if (F) {
		g_find_next(f, a);
		return;
	}
#endif
	get_search_data(f->f_data);
	do {
		if (is_in_range(f->f_data, p, f->yw, ses->search_word, &min, &max)) {
			f->vs->view_pos = p;
			if (max >= min) {
				if (max > f->vs->view_posx + f->xw) f->vs->view_posx = max - f->xw;
				if (min < f->vs->view_posx) f->vs->view_posx = min;
			}
			set_link(f);
			find_next_link_in_search(f, ses->search_direction * 2);
			return;
		}
		if ((p += ses->search_direction * f->yw) > f->f_data->y) p = 0;
		if (p < 0) {
			p = 0;
			while (p < f->f_data->y) p += f->yw;
			p -= f->yw;
		}
	} while ((c += f->yw) < f->f_data->y + f->yw);
	msg_box(ses->term, NULL, TXT(T_SEARCH), AL_CENTER, TXT(T_SEARCH_STRING_NOT_FOUND), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
}

void find_next_back(struct session *ses, struct f_data_c *f, int a)
{
	ses->search_direction = - ses->search_direction;
	find_next(ses, f, a);
	ses->search_direction = - ses->search_direction;
}

void rep_ev(struct session *ses, struct f_data_c *fd, void (*f)(struct session *, struct f_data_c *, int), int a)
{
	int i = ses->kbdprefix.rep ? ses->kbdprefix.rep_num : 1;
	while (i--) f(ses, fd, a);
}

struct link *choose_mouse_link(struct f_data_c *f, struct event *ev)
{
	struct link *l1 = f->f_data->links + f->f_data->nlinks;
	struct link *l2 = f->f_data->links;
	struct link *l;
	int i;
        if (!f->f_data->nlinks) return NULL;
	if (ev->x < 0 || ev->y < 0 || ev->x >= f->xw || ev->y >= f->yw) return NULL;
	for (i = f->vs->view_pos; i < f->f_data->y && i < f->vs->view_pos + f->yw; i++) {
		if (f->f_data->lines1[i] && f->f_data->lines1[i] < l1) l1 = f->f_data->lines1[i];
		if (f->f_data->lines2[i] && f->f_data->lines2[i] > l2) l2 = f->f_data->lines2[i];
	}
	for (l = l1; l <= l2; l++) {
		int i;
		for (i = 0; i < l->n; i++) if (l->pos[i].x - f->vs->view_posx == ev->x && l->pos[i].y - f->vs->view_pos == ev->y) return l;
	}
	return NULL;
}

void goto_link_number(struct session *ses, unsigned char *num)
{
	int n = atoi(num);
	struct f_data_c *f = current_frame(ses);
	struct link *link;
	if (!f) return;
	if (n < 0 || n > f->f_data->nlinks) return;
	f->vs->current_link = n - 1;
	link = &f->f_data->links[f->vs->current_link];
	check_vs(f);
	if (link->type != L_AREA && link->type != L_FIELD) enter(ses, f, 0, 0);
}


/* l must be a valid link, ev must be a mouse event */
int find_pos_in_link(struct f_data_c *fd,struct link *l,struct event *ev,int *xx,int *yy)
{
	int a;
	int minx,miny;
	int found=0;
	
	if (!l->n)return 1;
	minx=l->pos[0].x;miny=l->pos[0].y;
	for (a=0;a<l->n;a++)
	{
		if (l->pos[a].x<minx)minx=l->pos[a].x;
		if (l->pos[a].y<miny)miny=l->pos[a].y;
		if (l->pos[a].x-fd->vs->view_posx==ev->x && l->pos[a].y-fd->vs->view_pos==ev->y)(*xx=l->pos[a].x),(*yy=l->pos[a].y),found=1;
	}
	if (!found)return 1;
	*xx-=minx;
	*yy-=miny;
	return 0;
}


int frame_ev(struct session *ses, struct f_data_c *fd, struct event *ev)
{
	int x = 1;
	if (!fd || !fd->vs || !fd->f_data) return 0;
        if (fd->vs->current_link >= 0 && (fd->f_data->links[fd->vs->current_link].type == L_FIELD || fd->f_data->links[fd->vs->current_link].type == L_AREA))
                if (field_op(ses, fd, &fd->f_data->links[fd->vs->current_link], ev, 0)) return 1;
	if (ev->ev == EV_KBD && ev->x >= '0'+!ses->kbdprefix.rep && ev->x <= '9' && (!fd->f_data->opt.num_links || ev->y)) {
		if (!ses->kbdprefix.rep) ses->kbdprefix.rep_num = 0;
		if ((ses->kbdprefix.rep_num = ses->kbdprefix.rep_num * 10 + ev->x - '0') > 65536) ses->kbdprefix.rep_num = 65536;
		ses->kbdprefix.rep = 1;
		return 1;
        }
        if (ev->ev == EV_KBD) {
		if (ev->x == KBD_PAGE_DOWN || (ev->x == ' ' && (!ev->y || ev->y == KBD_CTRL))) rep_ev(ses, fd, page_down, 0);
		else if (ev->x == KBD_PAGE_UP || (upcase(ev->x) == 'B' && (!ev->y || ev->y == KBD_CTRL))) rep_ev(ses, fd, page_up, 0);
		else if (ev->x == KBD_DOWN) rep_ev(ses, fd, down, 0);
		else if (ev->x == KBD_UP) rep_ev(ses, fd, up, 0);
                else if (ev->x == KBD_INS || (upcase(ev->x) == 'P' && ev->y == KBD_CTRL)) rep_ev(ses, fd, scroll, -1 - !ses->kbdprefix.rep);
		else if (ev->x == KBD_DEL || (upcase(ev->x) == 'N' && ev->y == KBD_CTRL)) rep_ev(ses, fd, scroll, 1 + !ses->kbdprefix.rep);
		else if (ev->x == '[') rep_ev(ses, fd, hscroll, -1 - 7 * !ses->kbdprefix.rep);
		else if (ev->x == ']') rep_ev(ses, fd, hscroll, 1 + 7 * !ses->kbdprefix.rep);
		/*else if (upcase(ev->x) == 'Y' && ev->y == KBD_CTRL) rep_ev(ses, fd, scroll, -1);
		else if (upcase(ev->x) == 'E' && ev->y == KBD_CTRL) rep_ev(ses, fd, scroll, 1);*/
		else if (ev->x == KBD_HOME) rep_ev(ses, fd, home, 0);
		else if (ev->x == KBD_END) rep_ev(ses, fd, x_end, 0);
		else if ((ev->x == KBD_RIGHT && !ev->y) || ev->x == KBD_ENTER) x = enter(ses, fd, 0, 0);
		else if (upcase(ev->x) == 'D' && !(ev->y & KBD_ALT)) {
			if (!anonymous) frm_download(ses, fd);
		} else if (ev->x == '/') search_dlg(ses, fd, 0);
		else if (ev->x == '?') search_back_dlg(ses, fd, 0);
		else if (ev->x == 'n' && !(ev->y & KBD_ALT)) find_next(ses, fd, 0);
		else if (ev->x == 'N' && !(ev->y & KBD_ALT)) find_next_back(ses, fd, 0);
		else if (upcase(ev->x) == 'F' && !(ev->y & KBD_ALT)) set_frame(ses, fd, 0);
		else if (ev->x >= '1' && ev->x <= '9' && !ev->y) {
			struct f_data *f_data = fd->f_data;
			int nl, lnl;
			unsigned char d[2];
			d[0] = ev->x;
			d[1] = 0;
			nl = f_data->nlinks, lnl = 1;
			while (nl) nl /= 10, lnl++;
			if (lnl > 1) input_field(ses->term, NULL, TXT(T_GO_TO_LINK), TXT(T_ENTER_LINK_NUMBER), TXT(T_OK), TXT(T_CANCEL), ses, NULL, lnl, d, 1, f_data->nlinks, check_number, (void (*)(void *, unsigned char *)) goto_link_number, NULL);
                }
		else x = 0;
        } else if (ev->ev == EV_MOUSE) {
		struct link *l = choose_mouse_link(fd, ev);

                if ((ev->b & BM_BUTT) >= B_WHEELUP) {
			if ((ev->b & BM_ACT) != B_DOWN) {
				/* We handle only B_DOWN case... */
			} else if ((ev->b & BM_BUTT) == B_WHEELUP) {
				rep_ev(ses, fd, scroll, -2);
			} else if ((ev->b & BM_BUTT) == B_WHEELDOWN) {
				rep_ev(ses, fd, scroll, 2);
                        }
                } else if (l) {
			struct form_state *fs;
			x = 1;
			fd->vs->current_link = l - fd->f_data->links;
                        if (l->type == L_LINK || l->type == L_BUTTON || l->type == L_CHECKBOX || l->type == L_SELECT)
                                if ((ev->b & BM_ACT) == B_UP &&
                                    (ev->b & BM_BUTT) <= B_RIGHT) {
                                        fd->active = 1;
                                        draw_to_window(ses->win, (void (*)(struct terminal *, void *))draw_doc_c, fd);
                                        change_screen_status(ses);
                                        print_screen_status(ses);
                                        if ((ev->b & BM_BUTT) == B_LEFT)
                                                x = enter(ses, fd, 0, 0);
                                        else if ((ev->b & BM_BUTT) == B_MIDDLE)
                                                x = enter(ses, fd, 0, 1);
                                        else if ((ev->b & BM_BUTT) == B_RIGHT)
                                                link_menu(ses->term, NULL, ses);
                                }

			/* if links is a field, set cursor position */
			if (l->form&&(l->type==L_AREA||l->type==L_FIELD)&&(fs=find_form_state(fd,l->form)))
			{
				int xx,yy;
			
				if (l->type==L_AREA)
				{
					struct line_info *ln;

					if (!find_pos_in_link(fd,l,ev,&xx,&yy))
					{

						xx+=fs->vpos;
						xx=xx<0?0:xx;
						yy+=fs->vypos;
						if ((ln = format_text(fs->value, l->form->cols, l->form->wrap))) {
							int a;
							for (a = 0; ln[a].st; a++) if (a==yy){
								int bla=ln[a].en-ln[a].st;
							
								fs->state=ln[a].st-fs->value;
								fs->state += xx<bla?xx:bla;
								break;
							}
							mem_free(ln);
						}
					}
				}
				if (l->type==L_FIELD)
				{
					if (!find_pos_in_link(fd,l,ev,&xx,&yy))
					{
						xx+=fs->vpos;
						fs->state=xx < strlen(fs->value) ? (xx<0?0:xx) : strlen(fs->value);
					}
				}
                        }
                }
                else
                        if (ev->b == (B_UP | B_RIGHT)){
                                set_window_ptr(ses->win, ev->x, ev->y);
                                do_background_menu(ses->term, NULL, ses);
                                x=0;
                        }
        } else x = 0;
	ses->kbdprefix.rep = 0;
	return x;
}

struct f_data_c *current_frame(struct session *ses)
{
	struct f_data_c *fd, *fdd;
	fd = ses->screen;
	while (!list_empty(fd->subframes)) {
		int n = fd->vs->frame_pos;
		if (n == -1) break;
		foreach(fdd, fd->subframes) if (!n--) {
			fd = fdd;
			goto r;
		}
		fd = fd->subframes.next;
		r:;
	}
	return fd;
}

int is_active_frame(struct session *ses, struct f_data_c *f)
{
	struct f_data_c *fd, *fdd;
	fd = ses->screen;
	if (f == fd) return 1;
	while (!list_empty(fd->subframes)) {
		int n = fd->vs->frame_pos;
		if (n == -1) break;
		foreach(fdd, fd->subframes) if (!n--) {
			fd = fdd;
			goto r;
		}
		fd = fd->subframes.next;
		r:;
		if (f == fd) return 1;
	}
	return 0;
}

int send_to_frame(struct session *ses, struct event *ev)
{
	int r;
	struct f_data_c *fd;
	int previous_link;
	fd = current_frame(ses);
	previous_link=fd->vs ? fd->vs->current_link : -1;
	if (!fd) {
		/*internal("document not formatted");*/
		return 0;
	}
	if (!F) r = frame_ev(ses, fd, ev);
#ifdef G
	else r = g_frame_ev(ses, fd, ev);
#endif
	if (r == 1) {
		fd->active = 1;
		draw_to_window(ses->win, (void (*)(struct terminal *, void *))draw_doc_c, fd);
		change_screen_status(ses);
		print_screen_status(ses);
	}
	if (r == 3) draw_fd_nrd(fd);
	if (!F && fd->vs) {
#ifdef JS
		if (previous_link!=fd->vs->current_link&&fd->f_data&&previous_link>=0&&previous_link<fd->f_data->nlinks) /* link has changed */
		{
			struct link *l=&(fd->f_data->links[previous_link]);
			struct form_state *fs;

			/* process onchange code, if previous link was a textarea or a textfield and has changed */
			if ((l->type==L_FIELD||l->type==L_AREA) && (fs=find_form_state(fd,l->form)) && fs->changed && l->js_event && l->js_event->change_code)
				fs->changed=0,jsint_execute_code(fd,l->js_event->change_code,strlen(l->js_event->change_code),-1,-1,-1);

			/* process blur and mouse-out handlers */
			if (l->js_event&&l->js_event->blur_code)
				jsint_execute_code(fd,l->js_event->blur_code,strlen(l->js_event->blur_code),-1,-1,-1);
			if (l->js_event&&l->js_event->out_code)
				jsint_execute_code(fd,l->js_event->out_code,strlen(l->js_event->out_code),-1,-1,-1);
		}
		if (previous_link!=fd->vs->current_link&&fd->f_data&&fd->vs->current_link>=0&&fd->vs->current_link<fd->f_data->nlinks)
		{
			struct link *l=&(fd->f_data->links[fd->vs->current_link]);

			/* process focus and mouse-over handlers */
			if (l->js_event&&l->js_event->focus_code)
				jsint_execute_code(fd,l->js_event->focus_code,strlen(l->js_event->focus_code),-1,-1,-1);
			if (l->js_event&&l->js_event->over_code)
				jsint_execute_code(fd,l->js_event->over_code,strlen(l->js_event->over_code),-1,-1,-1);
		}
#endif
	}
	return r;
}

void next_frame(struct session *ses, int p)
{
	int n;
	struct view_state *vs;
	struct f_data_c *fd, *fdd;

	if (!(fd = current_frame(ses))) return;
	while ((fd = fd->parent)) {
		n = 0;
		foreach(fdd, fd->subframes) n++;
		vs = fd->vs;
		vs->frame_pos += p;
		if (vs->frame_pos < -!fd->f_data->frame_desc) { vs->frame_pos = n - 1; continue; }
		if (vs->frame_pos >= n) { vs->frame_pos = -!fd->f_data->frame_desc; continue; }
		break;
	}
	if (!fd) fd = ses->screen;
	vs = fd->vs;
	n = 0;
	foreach(fdd, fd->subframes) if (n++ == vs->frame_pos) {
		fd = fdd;
		next_sub:
		if (list_empty(fd->subframes)) break;
		fd = p < 0 ? fd->subframes.prev : fd->subframes.next;
		vs = fd->vs;
		vs->frame_pos = -1;
		if (!fd->f_data || (!fd->f_data->frame_desc && p > 0)) break;
		if (p < 0) foreach(fdd, fd->subframes) vs->frame_pos++;
		else vs->frame_pos = 0;
		goto next_sub;
	}
}

void do_for_frame(struct session *ses, void (*f)(struct session *, struct f_data_c *, int), int a)
{
	struct f_data_c *fd = current_frame(ses);
	if (!fd) {
		/*internal("document not formatted");*/
		return;
	}
	f(ses, fd, a);
}

void do_mouse_event(struct session *ses, struct event *ev)
{
	struct event evv;
	struct f_data_c *fdd, *fd = current_frame(ses);
	if (!fd) return;
	if (ev->x >= fd->xp && ev->x < fd->xp + fd->xw &&
	    ev->y >= fd->yp && ev->y < fd->yp + fd->yw) goto ok;
#ifdef G
	if (ses->scrolling) goto ok;
#endif
	r:
	next_frame(ses, 1);
	fdd = current_frame(ses);
	/*o = &fdd->f_data->opt;*/
	if (ev->x >= fdd->xp && ev->x < fdd->xp + fdd->xw &&
	    ev->y >= fdd->yp && ev->y < fdd->yp + fdd->yw) {
		draw_formatted(ses);
		fd = fdd;
		goto ok;
	}
	if (fdd != fd) goto r;
	return;
	ok:
	memcpy(&evv, ev, sizeof(struct event));
	evv.x -= fd->xp;
	evv.y -= fd->yp;
	send_to_frame(ses, &evv);
}

void send_event(struct session *ses, struct event *ev)
{
	if (ev->ev == EV_KBD) {
		if (send_to_frame(ses, ev)) return;

                if (upcase(ev->x) == 'C' && !ev->y) {
                        close_tab(ses->term);
                        /* We need to return here as we killed ses just now */
                        return;
                }
		if ((ev->x == '>' && !ev->y) ||
                    (ev->x == KBD_RIGHT && (ev->y & KBD_ALT))) {
			switch_to_tab(ses->term, ses->term->current_tab+1);
			goto x;
		}
		if ((ev->x == '<' && !ev->y) ||
		    (ev->x == KBD_LEFT && (ev->y & KBD_ALT))) {
			switch_to_tab(ses->term, ses->term->current_tab-1);
			goto x;
		}
		if (ev->y & KBD_ALT) {
			struct window *m;
			ev->y &= ~KBD_ALT;
			activate_bfu_technology(ses, -1);
			m = ses->term->windows.next;
                        m->handler(m, ev, 0);
			if (ses->term->windows.next == m) {
				delete_window(m);
			} else goto x;
			ev->y |= ~KBD_ALT;
		}
		if (ev->x == KBD_ESC || ev->x == KBD_F9) {
			activate_bfu_technology(ses, -1);
			goto x;
		}
		if (ev->x == KBD_F10) {
			activate_bfu_technology(ses, 0);
			goto x;
		}
		if (ev->x == KBD_TAB) {
			next_frame(ses, ev->y ? -1 : 1);
			draw_formatted(ses);
		}
                if ((!F || options_get_bool("keyboard_navigation") || ev->y == KBD_CTRL) && ev->x == KBD_LEFT) {
			back(ses, NULL, 0);
			goto x;
		}
		if (upcase(ev->x) == 'Z' && !ev->y) {
			back(ses, NULL, 0);
			goto x;
		}
		if (F && (ev->x == KBD_BS)) {
			back(ses, NULL, 0);
			goto x;
		}
                if (ev->x == '`' && !ev->y) {
                        go_forward(ses);
                        goto x;
                }

		if (upcase(ev->x) == 'R' && ev->y == KBD_CTRL) {
			reload(ses, -1);
			goto x;
		}
		if (ev->x == 'g' && !ev->y) {
			quak:
                        dialog_goto_url(ses,NULL);
			goto x;
		}
                if (ev->x == 'G' && !ev->y) {
			unsigned char *s;
			if (list_empty(ses->history)) goto quak;
			s = stracpy(ses->screen->rq->url);
			if (!s) goto quak;
			if (strchr(s, POST_CHAR)) *strchr(s, POST_CHAR) = 0;
			dialog_goto_url(ses, s);
			mem_free(s);
			goto x;
		}
		if (upcase(ev->x) == 'G' && ev->y == KBD_CTRL) {
			if (!ses->screen->vs||ses->screen->vs->current_link == -1)goto quak;
			dialog_goto_url(ses, ses->screen->f_data->links[ses->screen->vs->current_link].where);
			goto x;
		}
		if (upcase(ev->x) == 'S') {
			if (!anonymous) menu_bookmark_manager(ses->term, NULL, ses);
			goto x;
		}
		if ((upcase(ev->x) == 'Q' && !ev->y) || ev->x == KBD_CTRL_C) {
			exit_prog(ses->term, (void *)(ev->x == KBD_CTRL_C), ses);
			goto x;
		}

		if (upcase(ev->x) == 'H' && ev->y == KBD_CTRL) {
                        history_menu(ses->term, NULL, ses);
                        goto x;
                }
                if (ev->x == KBD_CLOSE){
			really_exit_prog(ses);
			goto x;
		}
		if (ev->x == '=') {
			state_msg(ses);
			goto x;
		}
		if (ev->x == '\\') {
			toggle(ses, ses->screen, 0);
			goto x;
		}
		if (upcase(ev->x) == 'O' && ev->y == KBD_CTRL) {
			menu_options_manager(ses->term,NULL,ses);
			goto x;
		}
		if (upcase(ev->x) == 'I' && ev->y == KBD_CTRL) {
			menu_fontlist_manager(ses->term,NULL,ses);
			goto x;
		}
		if (ev->x == '-') {
			if(!anonymous) menu_blocklist_manager(ses->term,NULL,ses);
			goto x;
		}
		if (ev->x == '|') {
			head_msg(ses);
			goto x;
		}
#ifdef HAVE_LUA
                if (ev->x == ',') {
			if(!anonymous) dialog_lua_console(ses);
			goto x;
		}
#endif
#ifdef G
                if (ev->x == 'P') {
			if(!anonymous) dialog_print(ses);
			goto x;
		}
#endif
                if (upcase(ev->x) == 'A') {
                        stop_button_pressed(ses);
                        goto x;
		}
#ifdef G
 		if (F && ev->x == ')') {
			options_set_int("html_images_scale", 1.2*options_get_int("html_images_scale"));
			options_set_int("html_font_size", 1.2*options_get_int("html_font_size"));
			html_interpret_recursive(ses->screen);
			draw_formatted(ses);
			goto x;
 		}
 		if (F && ev->x == '(') {
			options_set_int("html_images_scale", options_get_int("html_images_scale")*1./1.2);
			options_set_int("html_font_size", options_get_int("html_font_size")*1./1.2);
			html_interpret_recursive(ses->screen);
			draw_formatted(ses);
			goto x;
                }
                if (upcase(ev->x) == 'I' && !ev->y) {
                        options_set_bool("html_images_display",!options_get_bool("html_images_display"));
                        html_interpret_recursive(ses->screen);
                        draw_formatted(ses);
                        goto x;
                }
#endif
                if (ev->x == KBD_INS && ev->y == KBD_CTRL){
                        if(!F)
                                set_clipboard_text(cur_loc(ses)->url);
#ifdef G
                        else
                                drv->put_to_clipboard(ses->term->dev,(cur_loc(ses))->url,strlen((cur_loc(ses))->url));
#endif
                        goto x;
                }
        }
	if (ev->ev == EV_MOUSE) {
                if (ses->locked_link &&
                    (ev->b & BM_BUTT) != B_MIDDLE) { /* Middle key press means pasting */

                        if ((ev->b & BM_ACT) != B_MOVE) {
                                ses->locked_link = 0;
#ifdef JS
                                /* process onblur handler of current link */
                                if (ses->screen&&ses->screen->vs&&ses->screen->f_data&&ses->screen->vs->current_link>=0&&ses->screen->vs->current_link<ses->screen->f_data->nlinks)
                                {
                                        struct link *lnk=&(ses->screen->f_data->links[ses->screen->vs->current_link]);
                                        struct form_state *fs;
					/* select se dela jinde */
					if (lnk->type!=L_SELECT&&lnk->js_event&&lnk->js_event->blur_code)
						jsint_execute_code(current_frame(ses),lnk->js_event->blur_code,strlen(lnk->js_event->blur_code),-1,-1,-1);

					/* execute onchange handler of text field/area */
					if ((lnk->type==L_AREA||lnk->type==L_FIELD)&&lnk->js_event&&lnk->js_event->change_code&&(fs=find_form_state(ses->screen,lnk->form))&&fs->changed)
						fs->changed=0,jsint_execute_code(current_frame(ses),lnk->js_event->change_code,strlen(lnk->js_event->change_code),-1,-1,-1);
					
                                }
#endif
				clr_xl(ses->screen);
				draw_formatted(ses);
                        } else return;
                }

                if (!options_get_bool("hide_menus") && ev->y < gf_val(1, G_BFU_FONT_SIZE) && (ev->b & BM_ACT) == B_DOWN) {
#ifdef G
                        if(!F) goto bfu_tech; /* text mode */

				{/* Menubar -  drawings and size computations are in view_gr / draw_title() func / */

					int x=0;

									/* Back */
					x += ses->back_size;
					if ( ev->x < x ) {
						go_back(ses, NULL);
						goto x;
					}

									/* History */
					x += ses->history_size;
					if ( ev->x < x ) {
											history_menu(ses->term, NULL, ses);
						goto x;
					}

					/* Forward */
					x += ses->forward_size;
					if ( ev->x < x ) {
											go_forward(ses);
											goto x;
					}

					/* Reload */
					x += ses->reload_size;
					if ( ev->x < x ) {
						reload(ses,-1);
						goto x;
					}

					/* Bookmarks */
					x += ses->bookmarks_size;
					if ( ev->x < x ) {
						if (!anonymous) menu_bookmark_manager(ses->term, NULL, ses);
						goto x;
					}

					/* Home */
					x += ses->home_size;
					if ( ev->x < x ) {
											unsigned char *h;
											if ((h = options_get("homepage")) && *h) {
													goto_url(ses, h);
											}
											goto x;
					}

					/* Stop */
					x += ses->stop_size;
					if ( ev->x < x ) {
											stop_button_pressed(ses);
											goto x;
					}
				}
#endif
                bfu_tech: /* Main menu */
				{
					struct window *m;
					activate_bfu_technology(ses, -1);
					m = ses->term->windows.next;
									m->handler(m, ev, 0);
					goto x;
				}
			}
			/* Tabs */
			if (ev->y >= ses->term->y-2*(gf_val(1,G_BFU_FONT_SIZE)) &&
				ev->y < ses->term->y-gf_val(1,G_BFU_FONT_SIZE) &&
				(ev->b & BM_ACT) == B_DOWN &&
				(ev->b & BM_BUTT) < B_WHEELUP &&
				options_get_bool("tabs_show")) {
					int number = number_of_tabs(ses->term);
					int tab_width = ses->term->x / number;
					int tab = ev->x / tab_width;

		if (number>1 ||
						options_get_bool("tabs_show_if_single")){

							switch_to_tab(ses->term, tab);
							goto x;
					}
			}

			do_mouse_event(ses, ev);
        }
	return;
	x:
	ses->kbdprefix.rep = 0;
}

void send_enter(struct terminal *term, void *xxx, struct session *ses)
{
	struct event ev = { EV_KBD, KBD_ENTER, 0, 0 };
	send_event(ses, &ev);
}

void send_copy_link_location(struct terminal *term, void *xxx, struct session *ses)
{
	struct f_data_c *fd = current_frame(ses);
	if (!fd) return;
	if (fd->vs->current_link == -1) return;

        if (ses->screen->vs&&ses->screen->vs->current_link != -1){
                if(!F)
                        set_clipboard_text(ses->screen->f_data->links[ses->screen->vs->current_link].where);
#ifdef G
                else
                        drv->put_to_clipboard(term->dev,ses->screen->f_data->links[ses->screen->vs->current_link].where,strlen(ses->screen->f_data->links[ses->screen->vs->current_link].where));
#endif
        }
}

void send_copy_url_location(struct terminal *term, void *xxx, struct session *ses)
{
	unsigned char * url;
 	struct location * current_location;

	if (list_empty(ses->history)) return;

 	current_location = cur_loc(ses);

 	if(current_location) {
 	        url = current_location->url;

                if(!F)
                        set_clipboard_text(url);
#ifdef G
                else
                        drv->put_to_clipboard(term->dev,url,strlen(url));
#endif
	}
}

void frm_download(struct session *ses, struct f_data_c *fd)
{
	struct link *link;
	if (fd->vs->current_link == -1) return;
	if (ses->dn_url) mem_free(ses->dn_url), ses->dn_url = NULL;
	link = &fd->f_data->links[fd->vs->current_link];
	if (link->type != L_LINK && link->type != L_BUTTON) return;
	if ((ses->dn_url = get_link_url(ses, fd, link, NULL))) {
		if (!casecmp(ses->dn_url, "MAP@", 4)) {
			mem_free(ses->dn_url);
			ses->dn_url = NULL;
			return;
		}
		query_file(ses, ses->dn_url, start_download, NULL);
	}
}

void send_download_image(struct terminal *term, void *xxx, struct session *ses)
{
	struct f_data_c *fd = current_frame(ses);
	if (!fd) return;
	if (fd->vs->current_link == -1) return;
	if (ses->dn_url) mem_free(ses->dn_url);
	if ((ses->dn_url = stracpy(fd->f_data->links[fd->vs->current_link].where_img)))
		query_file(ses, ses->dn_url, start_download, NULL);
}

void send_download(struct terminal *term, void *xxx, struct session *ses)
{
	struct f_data_c *fd = current_frame(ses);
	if (!fd) return;
	if (fd->vs->current_link == -1) return;
	if (ses->dn_url) mem_free(ses->dn_url);
	if ((ses->dn_url = get_link_url(ses, fd, &fd->f_data->links[fd->vs->current_link], NULL)))
		query_file(ses, ses->dn_url, start_download, NULL);
}

/* open a link in a new xterm */
void send_open_in_new_xterm(struct terminal *term, void (*open_window)(struct terminal *term, unsigned char *, unsigned char *), struct session *ses)
{
#ifndef __XBOX__
        struct f_data_c *fd = current_frame(ses);
        if (!fd) return;
        if (fd->vs->current_link == -1) return;
        if (ses->dn_url) mem_free(ses->dn_url);
        if ((ses->dn_url = get_link_url(ses, fd, &fd->f_data->links[fd->vs->current_link], NULL))) {
		unsigned char *enc_url = encode_url(ses->dn_url);
		open_window(term, path_to_exe, enc_url);
		mem_free(enc_url);
	}
#endif
}

/* just open new xterm */
void send_open_new_xterm(struct terminal *term, void (*open_window)(struct terminal *, unsigned char *, unsigned char *), struct session *ses)
{
#ifndef __XBOX__
	int l;
        if (ses->dn_url) mem_free(ses->dn_url);
	ses->dn_url = init_str();
	l = 0;
        add_to_str(&ses->dn_url, &l, "-base-session ");
	add_num_to_str(&ses->dn_url, &l, ses->id);
	open_window(term, path_to_exe, ses->dn_url);
#endif
}

void open_in_new_window(struct terminal *term, void (*xxx)(struct terminal *, void (*)(struct terminal *, unsigned char *, unsigned char *), struct session *ses), struct session *ses)
{
	struct menu_item *mi;
	struct open_in_new *oin, *oi;
	if (!(oin = get_open_in_new(term->environment))) return;
	if (!oin[1].text) {
		xxx(term, oin[0].fn, ses);
		mem_free(oin);
		return;
	}
	if (!(mi = new_menu(1))) {
		mem_free(oin);
		return;
	}
	for (oi = oin; oi->text; oi++) add_to_menu(&mi, oi->text, "", oi->hk, MENU_FUNC xxx, oi->fn, 0);
	mem_free(oin);
	do_menu(term, mi, ses);
}

int can_open_in_new(struct terminal *term)
{
	struct open_in_new *oin = get_open_in_new(term->environment);
	if (!oin) return 0;
	if (!oin[1].text) {
		mem_free(oin);
		return 1;
	}
	mem_free(oin);
	return 2;
}

void save_url(struct session *ses, unsigned char *url)
{
	unsigned char *u;
	if (!(u = translate_url(url, ses->term->cwd))) {
		struct status stat = { NULL, NULL, NULL, NULL, S_BAD_URL, PRI_CANCEL, 0, NULL, NULL };
		print_error_dialog(ses, &stat, TXT(T_ERROR));
		return;
	}
	if (ses->dn_url) mem_free(ses->dn_url);
	ses->dn_url = u;
	query_file(ses, ses->dn_url, start_download, NULL);
}

void send_image(struct terminal *term, void *xxx, struct session *ses)
{
	unsigned char *u;
	struct f_data_c *fd = current_frame(ses);
	if (!fd) return;
	if (fd->vs->current_link == -1) return;
	if (!(u = fd->f_data->links[fd->vs->current_link].where_img)) return;
	goto_url_not_from_dialog(ses, u);
}

void save_as(struct terminal *term, void *xxx, struct session *ses)
{
	struct location *l;
	if (list_empty(ses->history)) return;
	l = cur_loc(ses);
	if (ses->dn_url) mem_free(ses->dn_url);
	if ((ses->dn_url = stracpy(ses->screen->rq->url)))
		query_file(ses, ses->dn_url, start_download, NULL);
}

void save_formatted_finish(struct terminal *term, int h, void *data, int resume)
{
        struct f_data *f_data = data;

	if (h == -1) return;
	if (dump_to_file(f_data, h))
                msg_box(term, NULL, TXT(T_SAVE_ERROR), AL_CENTER, TXT(T_ERROR_WRITING_TO_FILE), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);

        close(h);
}

void save_formatted(struct session *ses, unsigned char *file)
{
	struct f_data_c *f = current_frame(ses);

	if (!f || !f->f_data) return;
        create_download_file(ses->term, file, NULL, 0, 0,
			     save_formatted_finish, f->f_data);
}

void menu_save_formatted(struct terminal *term, void *xxx, struct session *ses)
{
	struct f_data_c *f;
	if (!(f = current_frame(ses)) || !f->f_data) return;
	query_file(ses, f->rq->url, save_formatted, NULL);
}

#ifdef FORM_SAVE

#include "js/md5.h" /* We need to do something with this line --karpov */

static unsigned char *get_form_id(struct f_data_c *f, struct form_control *fc)
{
	int l, nv = 0;
	struct form_control *form;
	struct list_head ff;
	unsigned char *s, *md5, *url, *end;
	struct submitted_value *sub;

	init_list(ff);

	foreach(form, f->f_data->forms) {
		if (form->form_num == fc->form_num && form->name && form->name[0]) {
			int i = 0;
			
			if (form->type == FC_RESET) continue; /* ignore RESET buttons */
			if ( !(sub = mem_alloc(sizeof(struct submitted_value)))) goto yy;
			memset(sub, 0, sizeof(struct submitted_value));
			sub->type = form->type;
			sub->name = stracpy(form->name);
			switch (form->type) {
				case FC_TEXT:
				case FC_PASSWORD:
					/* sub->value = stracpy(""); */
					nv++;
					break;
				case FC_SUBMIT:
					sub->value = stracpy(form->default_value);
					break;
				case FC_CHECKBOX:
				case FC_RADIO:
					sub->value = stracpy(form->default_value);
					nv++;
					break;
				case FC_TEXTAREA:
					/* sub->value = stracpy(""); */
					nv++;
					break;
				case FC_SELECT:
					sub->value = init_str();
					l = 0;
					for (i = 0; i < form->nvalues; i++)
						add_to_str(&sub->value, &l, form->values[i]);
					nv++;
					break;
				case FC_FILE:
				case FC_HIDDEN:
				case FC_IMAGE:
				case FC_BUTTON:
					break;
				default:
					internal("bad form control type");
					mem_free(sub);
					continue;
			}
			sub->position = form->form_num + form->ctrl_num;
			add_to_list(ff, sub);
		}
	}

	if (! nv) { yy:; free_succesful_controls(&ff); return NULL; }

	fix_control_order(&ff);
	s = init_str();
	l = 0;

	url = fc->action;
	end = url + strlen(url);
	if (end > url) {
		unsigned char *p = end;
		while (--p > url) if (*p == '?' || *p == '#') end = p;
	}
	add_bytes_to_str(&s, &l, url, (end - url));
	foreach(sub, ff) {
		add_num_to_str(&s, &l, sub->type);
		if (sub->name) add_to_str(&s, &l, sub->name);
		if (sub->value) add_to_str(&s, &l, sub->value);
	}
	free_succesful_controls(&ff);

	md5 = MD5Data(s, l, NULL);					
	mem_free(s);
	if (! md5) return NULL;

	s = init_str();
	l = 0;
	url = f->loc->url;
	end = url + strlen(url);
	if (end > url) {
		unsigned char *p = end;
		while (--p > url) if (*p == '?' || *p == '#') end = p;
	}
	add_chr_to_str(&s, &l, 'i');
	add_bytes_to_str(&s, &l, url, (end - url));
	add_chr_to_str(&s, &l, '|');
	add_to_str(&s, &l, md5);
	mem_free(md5);
	return s;
}

static void get_data_controls(struct f_data_c *f, struct form_control *fc, struct list_head *subm)
{
	struct form_control *form;
	struct submitted_value *sub;
	struct form_state *fs;

	init_list(*subm);
	foreach(form, f->f_data->forms) {
		if (form->form_num == fc->form_num && form->name && form->name[0]) {
			int tp = form->type; 

			if (tp != FC_TEXT && tp != FC_PASSWORD && tp != FC_TEXTAREA
					&& tp != FC_CHECKBOX && tp != FC_RADIO && tp != FC_SELECT) continue;
			if (! (fs = find_form_state(f, form))) continue;
			if (!(sub = mem_alloc(sizeof(struct submitted_value)))) {
				free_succesful_controls(subm);
				return;
			}
			memset(sub, 0, sizeof(struct submitted_value));
			sub->type = form->type;
			sub->name = stracpy(form->name);
			switch (form->type) {
				case FC_TEXT:
				case FC_PASSWORD:
				/* case FC_FILE: */
					sub->value = stracpy(fs->value);
					break;
				case FC_TEXTAREA:
					sub->value = encode_textarea(fs->value);
					break;
				case FC_CHECKBOX:
				case FC_RADIO:
					sub->value = stracpy(fs->state ? "y" : "n"); 
					break;
				case FC_SELECT:
					fixup_select_state(form, fs);
					sub->value = stracpy(fs->value);
					break;
				default:
					internal("bad form control type");
					mem_free(sub);
					continue;
			}
			sub->position = form->form_num + form->ctrl_num;
			sub->fc = form;
			add_to_list(*subm, sub);
		}
	}
	fix_control_order(subm);
}

static GDBM_FILE form_db;
static unsigned char form_file[MAX_STR_LEN] = "";
		
static void save_form(struct terminal *term, void *xxx, struct session *ses)
{
	struct f_data_c *fd = current_frame(ses);
	unsigned char *key, *data; 
	struct link *l;
	struct list_head submit;
	struct submitted_value *sub;
	struct conv_table *convert_table;
	int cp_from, cp_to;
	int len = 0, lst = 0;
	char *p2;

	if (! fd || fd->vs->current_link == -1) return;
	l = &fd->f_data->links[fd->vs->current_link];

        if (l->type == L_LINK) return;

	if (! (key = get_form_id(fd, l->form))) return;

	cp_from = ses->term->spec->charset;
	cp_to = get_cp_index("utf-8");
	convert_table = get_translation_table(cp_from, cp_to);
	get_data_controls(fd, l->form, &submit);

	data = init_str();
	foreach(sub, submit) {
		unsigned char *p = sub->value;
		if (lst) add_to_str(&data, &len, "&"); else lst = 1;

		add_num_to_str(&data, &len, sub->type);
		add_chr_to_str(&data, &len, '|');

		encode_string(sub->name, &data, &len);
		add_to_str(&data, &len, "=");
		if (sub->type == FC_TEXT || sub->type == FC_PASSWORD || sub->type == FC_TEXTAREA)
			p2 = convert_string(convert_table, p, strlen(p), NULL);
		else 
			p2 = stracpy(p);
		encode_string(p2, &data, &len);
		mem_free(p2);
	}
	free_succesful_controls(&submit);

	if (! len) { mem_free(key); return; }

	if ((form_db = db_open(form_file, "rw"))) {
		db_insert(form_db, key, data, len);
		db_close(form_db, 50);
	}
	mem_free(data);
	mem_free(key);
}

static inline int unhx(unsigned char a)
{
	if (a >= '0' && a <= '9') return a - '0';
	if (a >= 'A' && a <= 'F') return a - 'A' + 10;
	if (a >= 'a' && a <= 'f') return a - 'a' + 10;
	return -1;
}

static unsigned char *
decode_string(unsigned char *url)
{
	unsigned char *u = init_str();
	int l = 0;
	for (; *url; url++) {
		if (*url == '+') add_chr_to_str(&u, &l, ' ');
		else if (*url != '%' || unhx(url[1]) == -1 || unhx(url[2]) == -1) add_chr_to_str(&u, &l, *url);
		else add_chr_to_str(&u, &l, (unhx(url[1]) << 4) + unhx(url[2])), url += 2;
	}
	return u;
}

static void load_controls(struct f_data_c *f, struct list_head *l, unsigned char *data, int len,
						int cp_from, int cp_to)
{
	struct submitted_value *sv = NULL;
	unsigned char *p, *q, *name, *value;
	struct conv_table *convert_table = get_translation_table(cp_from, cp_to);
	struct form_state *fs;
	int i, type;

	name = data;
	value = NULL;
	for (i = 0; i <= len; i++) {
		if (data[i] == '&' || i == len) {
			data[i] = 0;              
			p = q = &data[i]; 
			for (--p; p > name; p--) if (*p == '=') {
				*p = 0; 
				value = ++p; 
				break;
			}
			if (! name[0] || ! value) return;
			type = strtol(name, (char **)&p, 0); 
			name = decode_string(++p);
			sv = (! sv) ? l->next : sv->next;
  			if (sv == (void *) l || !sv->fc || type != sv->type || strcmp(sv->name, name)) {
				mem_free(name);
				return;
			}
			mem_free(name);
			if (sv->value) mem_free(sv->value);
			sv->value = decode_string(value);
			name = q + 1;
			value = NULL;
		}
	}

	foreach(sv, *l) {
		if (! (fs = find_form_state(f, sv->fc))) continue;
		if (fs->value) mem_free(fs->value), fs->value = NULL;
		switch (sv->type) {
		case FC_TEXT:
		case FC_PASSWORD:
		case FC_TEXTAREA:
			fs->value = convert_string(convert_table, sv->value, strlen(sv->value), NULL);;
			fs->state = strlen(fs->value);
			fs->vpos = 0;
			break;
		case FC_RADIO:
		case FC_CHECKBOX:
			fs->state = (sv->value[0] == 'y');
			break;
		case FC_SELECT:
			fs->value = stracpy(sv->value);
			fixup_select_state(sv->fc, fs);
			break;
		default:
			/* should not come here */
			break;
		}
	}
	if (F) draw_fd(f);
}

static void load_form(struct terminal *term, void *xxx, struct session *ses)
{
	struct f_data_c *fd = current_frame(ses);
	struct link *l;
	struct list_head submit;
	unsigned char *data, *key;
	int dalen;
	int cp_from, cp_to;

	if (! fd || fd->vs->current_link == -1) return;

	l = &fd->f_data->links[fd->vs->current_link];

        if (l->type == L_LINK) return;

	if (! (key = get_form_id(fd, l->form))) return;

	if ((form_db = db_open(form_file, "r"))) {
		if (db_fetch(form_db, key, &data, &dalen)) goto xx;

		get_data_controls(fd, l->form, &submit);
		cp_to = fd->f_data->cp;
		cp_from = get_cp_index("utf-8");
		load_controls(fd, &submit, data, dalen, cp_from, cp_to);
		free_succesful_controls(&submit);
		mem_free(data);
xx:
		db_close(form_db, 0);
	}
	mem_free(key);
}

static void delete_form(struct terminal *term, void *xxx, struct session *ses)
{
	struct f_data_c *fd = current_frame(ses);
	struct link *l;
	unsigned char *key;

	if (! fd || fd->vs->current_link == -1) return;

	l = &fd->f_data->links[fd->vs->current_link];

        if (l->type == L_LINK) return;

        if (! (key = get_form_id(fd, l->form))) return;

        if ((form_db = db_open(form_file, "rw"))) {
		db_delete(form_db, key);
		db_close(form_db, 50);
	}
	mem_free(key);
}

static int check_form(struct f_data_c *f, struct form_control *fc)
{
	unsigned char *key;
	int r = 0;
 
	if (form_file[0] == 0) {
		snprintf(form_file, MAX_STR_LEN, "%sform.gdbm", links_home);
		form_file[MAX_STR_LEN - 1] = 0;
	}

	if (! (key = get_form_id(f, fc))) return -1;	/* nothing to save */
	if ((form_db = db_open(form_file, "r"))) {
		if (db_exists(form_db, key)) r = 1;
		db_close(form_db, 0);
	}
	mem_free(key);
	return r;
}

#endif

void link_menu(struct terminal *term, void *xxx, struct session *ses)
{
	struct f_data_c *f = current_frame(ses);
	struct link *link;
	struct menu_item *mi;
	if (!(mi = new_menu(1))) return;
	if (!f || !f->vs || !f->f_data) goto x;
	if (f->vs->current_link == -1) goto no_l;
	link = &f->f_data->links[f->vs->current_link];
	if (link->type == L_LINK && link->where) {
		if (strlen(link->where) >= 4 && !casecmp(link->where, "MAP@", 4)) {
			if (!F) {
				add_to_menu(&mi, TXT(T_DISPLAY_USEMAP), ">", TXT(T_HK_DISPLAY_USEMAP), MENU_FUNC send_enter, NULL, 1);
			}
		}
		else {
			int c = can_open_in_new(term);
                        add_to_menu(&mi, TXT(T_FOLLOW_LINK), "", TXT(T_HK_FOLLOW_LINK), MENU_FUNC send_enter, NULL, 0);
 /*BOOK0018010x000bb*/                        if (c) add_to_menu(&mi, c - 1 ? TXT(T_OPEN_IN) : TXT(T_OPEN_IN_NEW_WINDOW), c - 1 ? ">" : "", TXT(T_HK_OPEN_IN_NEW_WINDOW), MENU_FUNC open_in_new_window, send_open_in_new_xterm, c - 1);
			if (!anonymous) add_to_menu(&mi, TXT(T_DOWNLOAD_LINK), "d", TXT(T_HK_DOWNLOAD_LINK), MENU_FUNC send_download, NULL, 0);

		}
	}
	if (link->type == L_BUTTON && link->form) {
		if (link->form->type == FC_RESET) add_to_menu(&mi, TXT(T_RESET_FORM), "", TXT(T_HK_RESET_FORM), MENU_FUNC send_enter, NULL, 0);
		else if (link->form->type==FC_BUTTON);
		else if (link->form->type == FC_SUBMIT || link->form->type == FC_IMAGE) {
			int c = can_open_in_new(term);

			add_to_menu(&mi, TXT(T_SUBMIT_FORM), "", TXT(T_HK_SUBMIT_FORM), MENU_FUNC send_enter, NULL, 0);
			if (c && link->form->method == FM_GET) add_to_menu(&mi, TXT(T_SUBMIT_FORM_AND_OPEN_IN_NEW_WINDOW), c - 1 ? ">" : "", TXT(T_HK_SUBMIT_FORM_AND_OPEN_IN_NEW_WINDOW), MENU_FUNC open_in_new_window, send_open_in_new_xterm, c - 1);
			if (!anonymous) add_to_menu(&mi, TXT(T_SUBMIT_FORM_AND_DOWNLOAD), "d", TXT(T_HK_SUBMIT_FORM_AND_DOWNLOAD), MENU_FUNC send_download, NULL, 0);
		}
	}
#ifdef FORM_SAVE
        /* Just a hack, must be strict comparation of all L_* types */
        if(link->type > L_LINK && link->form){
		struct form_control *fc = link->form;
                /* Do we really need to handle javascript here? */
		if (fc->action && strncasecmp(fc->action,"javascript:",11) && (fc->method == FM_GET || fc->method == FM_POST)) {
			int r = check_form(f, fc);

			if (r != -1)
				add_to_menu(&mi, TXT(T_SAVE_FORM), "", TXT(T_HK_SAVE_FORM), MENU_FUNC save_form, NULL, 0);
			if (r == 1) {
				add_to_menu(&mi, TXT(T_LOAD_FORM), "", TXT(T_HK_LOAD_FORM), MENU_FUNC load_form, NULL, 0);
				add_to_menu(&mi, TXT(T_DELETE_FORM), "", TXT(T_HK_DELETE_FORM), MENU_FUNC delete_form, NULL, 0);
			}
		}

        }
#endif
        if (link->where_img) {
		if (!F || f->f_data->opt.plain != 2) add_to_menu(&mi, TXT(T_VIEW_IMAGE), "", TXT(T_HK_VIEW_IMAGE), MENU_FUNC send_image, NULL, 0);
		if (!anonymous) add_to_menu(&mi, TXT(T_DOWNLOAD_IMAGE), "", TXT(T_HK_DOWNLOAD_IMAGE), MENU_FUNC send_download_image, NULL, 0);
	}

        if (link->type == L_LINK && link->where) {
                add_to_menu(&mi, TXT(T_COPY_LINK_LOCATION), "", TXT(T_HK_COPY_LINK_LOCATION), MENU_FUNC send_copy_link_location, NULL, 0);
        }
	x:
	no_l:
	if (!mi->text) add_to_menu(&mi, TXT(T_NO_LINK_SELECTED), "", M_BAR, NULL, NULL, 0);
        do_menu(term, mi, ses);
}

unsigned char *print_current_titlex(struct f_data_c *fd, int w)
{
	int ml = 0, pl = 0;
	unsigned char *m, *p;
	if (!fd || !fd->vs || !fd->f_data) return NULL;
	w -= 1;
	p = init_str();
	if (fd->yw < fd->f_data->y) {
		int pp, pe;
		if (fd->yw) {
			pp = (fd->vs->view_pos + fd->yw / 2) / fd->yw + 1;
			pe = (fd->f_data->y + fd->yw - 1) / fd->yw;
		} else pp = pe = 1;
		if (pp > pe) pp = pe;
		if (fd->vs->view_pos + fd->yw >= fd->f_data->y) pp = pe;
		if (fd->f_data->title) add_chr_to_str(&p, &pl, ' ');
		add_to_str(&p, &pl, "(p");
		add_num_to_str(&p, &pl, pp);
		add_to_str(&p, &pl, " of ");
		add_num_to_str(&p, &pl, pe);
		add_chr_to_str(&p, &pl, ')');
	}
	if (!fd->f_data->title) return p;
	m = init_str();
	add_to_str(&m, &ml, fd->f_data->title);
	if (ml + pl > w) if ((ml = w - pl) < 0) ml = 0;
	add_to_str(&m, &ml, p);
	mem_free(p);
	return m;
}

unsigned char *print_current_linkx(struct f_data_c *fd, struct terminal *term)
{
	int ll = 0;
	struct link *l;
	unsigned char *m;
	if (!fd || !fd->vs || !fd->f_data) return NULL;
	if (fd->vs->current_link == -1 || fd->vs->current_link >= fd->f_data->nlinks || fd->f_data->frame_desc) return NULL;
	l = &fd->f_data->links[fd->vs->current_link];
	if (l->type == L_LINK) {
		if (!l->where && l->where_img) {
			m = init_str();
			ll = 0;
                        if (l->img_alt)
			{
				unsigned char *txt;
				struct conv_table* ct;

				ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
				txt = convert_string(ct, l->img_alt, strlen(l->img_alt), &fd->f_data->opt);
				add_to_str(&m, &ll, txt);
				mem_free(txt);
			}
			else
			{
				add_to_str(&m, &ll, _(TXT(T_IMAGE), term));
				add_to_str(&m, &ll, " ");
				add_to_str(&m, &ll, l->where_img);
			}
			goto p;
		}
		if (l->where && strlen(l->where) >= 4 && !casecmp(l->where, "MAP@", 4)) {
			m = init_str();
			ll = 0;
			add_to_str(&m, &ll, _(TXT(T_USEMAP), term));
			add_to_str(&m, &ll, " ");
			add_to_str(&m, &ll, l->where + 4);
			goto p;
		}
		if (l->where) {
			m = stracpy(l->where);
			goto p;
		}
#ifdef JS
		m = print_js_event_spec(l->js_event);
#else
                m = init_str();
#endif
                goto p;
	}
	if (!l->form) return NULL;
	if (l->type == L_BUTTON) {
		if (l->form->type == FC_BUTTON){
			unsigned char *n;
			unsigned char *txt;
			m = init_str();
			ll=0;
			add_to_str(&m, &ll, _(TXT(T_BUTTON), term));
#ifdef JS
			add_to_str(&m, &ll, " ");
			n=print_js_event_spec(l->js_event);
			if (fd->f_data)
			{
				struct conv_table* ct;
		
				ct=get_translation_table(fd->f_data->cp,fd->f_data->opt.cp);
				txt=convert_string(ct,n,strlen(n),NULL);
				mem_free(n);
			}
			else
				txt=n;
			add_to_str(&m, &ll, txt);
			mem_free(txt);
#endif
                        goto p;
		}
		if (l->form->type == FC_RESET) {
			m = stracpy(_(TXT(T_RESET_FORM), term));
			goto p;
		}
		if (!l->form->action) return NULL;
		m = init_str();
		ll = 0;
		if (l->form->method == FM_GET) add_to_str(&m, &ll, _(TXT(T_SUBMIT_FORM_TO), term));
		else add_to_str(&m, &ll, _(TXT(T_POST_FORM_TO), term));
		add_to_str(&m, &ll, " ");
		add_to_str(&m, &ll, l->form->action);
		goto p;
	}
	if (l->type == L_CHECKBOX || l->type == L_SELECT || l->type == L_FIELD || l->type == L_AREA) {
		m = init_str();
		ll = 0;
		if (l->form->type == FC_RADIO) add_to_str(&m, &ll, _(TXT(T_RADIO_BUTTON), term));
		else if (l->form->type == FC_CHECKBOX) add_to_str(&m, &ll, _(TXT(T_CHECKBOX), term));
		else if (l->form->type == FC_SELECT) add_to_str(&m, &ll, _(TXT(T_SELECT_FIELD), term));
		else if (l->form->type == FC_TEXT) add_to_str(&m, &ll, _(TXT(T_TEXT_FIELD), term));
		else if (l->form->type == FC_TEXTAREA) add_to_str(&m, &ll, _(TXT(T_TEXT_AREA), term));
		else if (l->form->type == FC_FILE) add_to_str(&m, &ll, _(TXT(T_FILE_UPLOAD), term));
		else if (l->form->type == FC_PASSWORD) add_to_str(&m, &ll, _(TXT(T_PASSWORD_FIELD), term));
		else {
			mem_free(m);
			return NULL;
		}
		if (l->form->name && l->form->name[0]) add_to_str(&m, &ll, ", "), add_to_str(&m, &ll, _(TXT(T_NAME), term)), add_to_str(&m, &ll, " "), add_to_str(&m, &ll, l->form->name);
		if ((l->form->type == FC_CHECKBOX || l->form->type == FC_RADIO) && l->form->default_value && l->form->default_value[0]) add_to_str(&m, &ll, ", "), add_to_str(&m, &ll, _(TXT(T_VALUE), term)), add_to_str(&m, &ll, " "), add_to_str(&m, &ll, l->form->default_value);
		                       /* pri enteru se bude posilat vzdycky   -- Brain */
		if (l->type == L_FIELD && !has_form_submit(fd->f_data, l->form)  && l->form->action) {
			add_to_str(&m, &ll, ", ");
			add_to_str(&m, &ll, _(TXT(T_HIT_ENTER_TO), term));
			add_to_str(&m, &ll, " ");
			if (l->form->method == FM_GET) add_to_str(&m, &ll, _(TXT(T_SUBMIT_TO), term));
			else add_to_str(&m, &ll, _(TXT(T_POST_TO), term));
			add_to_str(&m, &ll, " ");
			add_to_str(&m, &ll, l->form->action);
		}
		goto p;
	}
	p:
	return m;
}

unsigned char *print_current_link(struct session *ses)
{
	return print_current_linkx(current_frame(ses), ses->term);
}

unsigned char *print_current_title(struct session *ses)
{
	return print_current_titlex(current_frame(ses), ses->term->x);
}

void loc_msg(struct terminal *term, struct location *lo, struct f_data_c *frame)
{
#ifdef GLOBHIST
	struct global_history_item *history_item;
#endif
	struct cache_entry *ce;
	unsigned char *s;
	int l = 0;
	unsigned char *a;
	if (!lo || !frame->vs || !frame->f_data) {
		msg_box(term, NULL, TXT(T_INFO), AL_LEFT, TXT(T_YOU_ARE_NOWHERE), NULL, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
		return;
	}
	s = init_str();
	add_to_str(&s, &l, _(TXT(T_URL), term));
	add_to_str(&s, &l, ": ");
	if (strchr(lo->url, POST_CHAR)) add_bytes_to_str(&s, &l, lo->url, (unsigned char *)strchr(lo->url, POST_CHAR) - (unsigned char *)lo->url);
	else add_to_str(&s, &l, lo->url);

#ifdef GLOBHIST
	add_to_str(&s, &l, "\n");
	add_to_str(&s, &l, _(TXT(T_LAST_VISIT_TIME), term));
	add_to_str(&s, &l, ": ");
	history_item = get_global_history_item(lo->url);
	if (history_item) {
		/* Stupid ctime() adds a newline, and we don't want that, so we
		 * use add_bytes_to_str. -- Miciah */
		a = ctime(&history_item->last_visit);
		add_bytes_to_str(&s, &l, a, strlen(a) - 1);
	} else {
		add_to_str(&s, &l, _(TXT(T_UNKNOWN), term));
	}
#endif

        if (!get_cache_entry(lo->url, &ce)) {
		add_to_str(&s, &l, "\n");
		add_to_str(&s, &l, _(TXT(T_SIZE), term));
		add_to_str(&s, &l, ": ");
		add_num_to_str(&s, &l, ce->length);
		if (ce->incomplete) {
			add_to_str(&s, &l, " (");
			add_to_str(&s, &l, _(TXT(T_INCOMPLETE), term));
			add_to_str(&s, &l, ")");
		}
		add_to_str(&s, &l, "\n");
		add_to_str(&s, &l, _(TXT(T_CODEPAGE), term));
		add_to_str(&s, &l, ": ");
		add_to_str(&s, &l, get_cp_name(frame->f_data->cp));
		if (frame->f_data->ass == 1) add_to_str(&s, &l, " ("), add_to_str(&s, &l, _(TXT(T_ASSUMED), term)), add_to_str(&s, &l, ")");
		if (frame->f_data->ass == 2) add_to_str(&s, &l, " ("), add_to_str(&s, &l, _(TXT(T_IGNORING_SERVER_SETTING), term)), add_to_str(&s, &l, ")");
		if ((a = parse_http_header(ce->head, "Content-Type", NULL))) {
			add_to_str(&s, &l, "\n");
			add_to_str(&s, &l, _(TXT(T_CONTENT_TYPE), term));
			add_to_str(&s, &l, ": ");
			add_to_str(&s, &l, a);
			mem_free(a);
		}
		if ((a = parse_http_header(ce->head, "Server", NULL))) {
			add_to_str(&s, &l, "\n");
			add_to_str(&s, &l, _(TXT(T_SERVER), term));
			add_to_str(&s, &l, ": ");
			add_to_str(&s, &l, a);
			mem_free(a);
		}
		if ((a = parse_http_header(ce->head, "Date", NULL))) {
			add_to_str(&s, &l, "\n");
			add_to_str(&s, &l, _(TXT(T_DATE), term));
			add_to_str(&s, &l, ": ");
			add_to_str(&s, &l, a);
			mem_free(a);
		}
		if (ce->last_modified) {
			add_to_str(&s, &l, "\n");
			add_to_str(&s, &l, _(TXT(T_LAST_MODIFIED), term));
			add_to_str(&s, &l, ": ");
			add_to_str(&s, &l, ce->last_modified);
		}
#ifdef HAVE_SSL
		if (ce->ssl_info) {
			add_to_str(&s, &l, "\n");
			add_to_str(&s, &l, "SSL cipher: ");
			add_to_str(&s, &l, ce->ssl_info);
		}
#endif
		if (ce->encoding_info) {
			add_to_str(&s, &l, "\n");
			add_to_str(&s, &l, _(TXT(T_ENCODING), term));
			add_to_str(&s, &l, ": ");
			add_to_str(&s, &l, ce->encoding_info);
		}
	}
	if ((a = print_current_linkx(frame, term))) {
		add_to_str(&s, &l, "\n\n");
		add_to_str(&s, &l, _(TXT(T_LINK), term));
		add_to_str(&s, &l, ": ");
		add_to_str(&s, &l, a);
		mem_free(a);
	}
	msg_box(term, getml(s, NULL), TXT(T_INFO), AL_LEFT, s, NULL, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
}

void state_msg(struct session *ses)
{
	if (list_empty(ses->history)) loc_msg(ses->term, NULL, NULL);
	else loc_msg(ses->term, cur_loc(ses), ses->screen);
}

/* Headers info. message box. */
void head_msg(struct session *ses)
{
	struct cache_entry *ce;

	if (!have_location(ses)) {
		msg_box(ses->term, NULL,
			TXT(T_HEADER_INFO), AL_LEFT,
			TXT(T_YOU_ARE_NOWHERE),
			NULL, 1,
			TXT(T_OK), NULL, B_ENTER | B_ESC);
		return;
	}

	if (!find_in_cache(cur_loc(ses)->url, &ce)) {
		unsigned char *headers;

		if (ce->head) {
			headers = stracpy(ce->head);
			if (!headers) return;

			if (*headers)  {
				int i = 0, j = 0;

				/* Sanitize headers string. */
				/* XXX: Do we need to check length and limit
				 * it to something reasonable ? */

				while (ce->head[i]) {
					/* Check for control chars. */
					if (ce->head[i] < ' '
					    && ce->head[i] != '\n') {
						/* Ignore '\r' but replace
						 * others control chars with
						 * a visible char. */
						if (ce->head[i] != '\r') {
							 headers[j] = '*';
							 j++;
						}
					} else {
						headers[j] = ce->head[i];
						j++;
					}
					i++;
				}

				/* Ensure null termination. */
				headers[j] = '\0';

				/* Remove all ending '\n' if any. */
				while (j && headers[--j] == '\n')
					headers[j] = '\0';
			}

		} else {
			headers = stracpy("");
			if (!headers) return;
		}

		/* Headers info message box. */
		msg_box(ses->term, getml(headers, NULL),
			TXT(T_HEADER_INFO), AL_LEFT,
			headers,
			NULL, 1,
			TXT(T_OK), NULL, B_ENTER | B_ESC);
        }
}
