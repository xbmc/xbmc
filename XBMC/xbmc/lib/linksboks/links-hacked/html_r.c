/* html_r.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

#define format format_

struct f_data *init_formatted(struct document_options *opt)
{
	struct f_data *scr;
	if (!(scr = mem_calloc(sizeof(struct f_data)))) return NULL;
	copy_opt(&scr->opt, opt);
	scr->data = DUMMY;
	scr->nlinks = 0;
	scr->links = DUMMY;
	init_list(scr->forms);
	init_list(scr->tags);
	init_list(scr->nodes);
#ifdef G
	scr->n_images=0;
	init_list(scr->images);
	scr->search_positions = DUMMY;
	init_list(scr->image_refresh);
#endif
	return scr;
}

void destroy_fc(struct form_control *fc)
{
	int i;
	if (fc->action) mem_free(fc->action);
	if (fc->target) mem_free(fc->target);
	if (fc->form_name) mem_free(fc->form_name);
	if (fc->onsubmit) mem_free(fc->onsubmit);
	if (fc->name) mem_free(fc->name);
	if (fc->alt) mem_free(fc->alt);
	if (fc->default_value) mem_free(fc->default_value);
	for (i = 0; i < fc->nvalues; i++) {
		if (fc->values[i]) mem_free(fc->values[i]);
		if (fc->labels[i]) mem_free(fc->labels[i]);
	}
	if (fc->values) mem_free(fc->values);
	if (fc->labels) mem_free(fc->labels);
	if (fc->menu) free_menu(fc->menu);
}

void free_frameset_desc(struct frameset_desc *fd)
{
	int i;
	for (i = 0; i < fd->n; i++) {
		if (fd->f[i].subframe) free_frameset_desc(fd->f[i].subframe);
		if (fd->f[i].name) mem_free(fd->f[i].name);
		if (fd->f[i].url) mem_free(fd->f[i].url);
	}
#ifdef JS
	if (fd->onload_code)mem_free(fd->onload_code);
#endif
	mem_free(fd);
}

struct frameset_desc *copy_frameset_desc(struct frameset_desc *fd)
{
	int i;
	struct frameset_desc *new;
	if (!(new = mem_alloc(sizeof(struct frameset_desc) + fd->n * sizeof(struct frame_desc)))) return NULL;
	memcpy(new, fd, sizeof(struct frameset_desc) + fd->n * sizeof(struct frame_desc));
#ifdef JS
	if (new->onload_code) new->onload_code = stracpy(new->onload_code);
#endif
	for (i = 0; i < new->n; i++) {
		if (new->f[i].subframe) new->f[i].subframe = copy_frameset_desc(new->f[i].subframe);
		if (new->f[i].name) new->f[i].name = stracpy(new->f[i].name);
		if (new->f[i].url) new->f[i].url = stracpy(new->f[i].url);
	}
	return new;
}

void free_additional_files(struct additional_files **a)
{
	struct additional_file *af;
	if (!*a) return;
	if (--(*a)->refcount) {
		*a = NULL;
		return;
	}
	foreach(af, (*a)->af) release_object(&af->rq);
	free_list((*a)->af);
	mem_free(*a);
	*a = NULL;
}

void clear_formatted(struct f_data *scr)
{
	int n;
	int y;
	struct form_control *fc;
	if (!scr) return;
#ifdef G
	if (scr->root) scr->root->destruct(scr->root);
#endif
	release_object(&scr->rq);
	free_additional_files(&scr->af);
	if (scr->title) mem_free(scr->title);
	if (/*!scr->frame_desc_link &&*/ scr->frame_desc) {
		free_frameset_desc(scr->frame_desc);
	}
	for (n = 0; n < scr->nlinks; n++) {
		struct link *l = &scr->links[n];
		if (l->where) mem_free(l->where);
		if (l->target) mem_free(l->target);
		if (l->where_img) mem_free(l->where_img);
		if (l->img_alt) mem_free(l->img_alt);
		if (l->pos) mem_free(l->pos);
#ifdef JS
		free_js_event_spec(l->js_event);
#endif
        }
	mem_free(scr->links);
	if (!F) for (y = 0; y < scr->y; y++) mem_free(scr->data[y].d);
	mem_free(scr->data);
	if (scr->lines1) mem_free(scr->lines1);
	if (scr->lines2) mem_free(scr->lines2);
	if (scr->opt.framename) mem_free(scr->opt.framename);
	foreach(fc, scr->forms) {
		destroy_fc(fc);
	}
	free_list(scr->forms);
	free_list(scr->tags);
	free_list(scr->nodes);
	if (scr->search) mem_free(scr->search);
	if (scr->slines1) mem_free(scr->slines1);
	if (scr->slines2) mem_free(scr->slines2);
#ifdef G
	free_list(scr->image_refresh);
	if (scr->srch_string) mem_free(scr->srch_string);
	if (scr->last_search) mem_free(scr->last_search);
	if (scr->search_positions) mem_free(scr->search_positions);
#endif
#ifdef JS
	jsint_destroy_document_description(scr);
#endif
        if (scr->script_href_base) mem_free(scr->script_href_base);
}

void destroy_formatted(struct f_data *scr)
{
	if (scr->fd) {
		internal("trying to free locked formatted data");
		return;
	}
	clear_formatted(scr);
	/*del_from_list(scr);*/
	mem_free(scr);
}

static inline int color_distance(struct rgb *c1, struct rgb *c2)
{
	return
		3 * (c1->r - c2->r) * (c1->r - c2->r) +
		4 * (c1->g - c2->g) * (c1->g - c2->g) +
		2 * (c1->b - c2->b) * (c1->b - c2->b);
}

struct rgb palette[] = {
/*	{0x00, 0x00, 0x00},
	{0x80, 0x00, 0x00},
	{0x00, 0x80, 0x00},
	{0x80, 0x80, 0x00},
	{0x00, 0x00, 0x80},
	{0x80, 0x00, 0x80},
	{0x00, 0x80, 0x80},
	{0xC0, 0xC0, 0xC0},
	{0x80, 0x80, 0x80},
	{0xff, 0x00, 0x00},
	{0x00, 0xff, 0x00},
	{0xff, 0xff, 0x00},
	{0x00, 0x00, 0xff},
	{0xff, 0x00, 0xff},
	{0x00, 0xff, 0xff},
	{0xff, 0xff, 0xff},*/
	/*{0x00, 0x00, 0x00},
	{0xaa, 0x00, 0x00},
	{0x00, 0xaa, 0x00},
	{0xaa, 0x55, 0x00},
	{0x00, 0x00, 0xaa},
	{0xaa, 0x00, 0xaa},
	{0x00, 0xaa, 0xaa},
	{0xaa, 0xaa, 0xaa},
	{0x55, 0x55, 0x55},
	{0xff, 0x55, 0x55},
	{0x55, 0xff, 0x55},
	{0xff, 0xff, 0x55},
	{0x55, 0x55, 0xff},
	{0xff, 0x55, 0xff},
	{0x55, 0xff, 0xff},
	{0xff, 0xff, 0xff},*/
	{0x00, 0x00, 0x00},
	{0x80, 0x00, 0x00},
	{0x00, 0x80, 0x00},
	{0xaa, 0x55, 0x00},
	{0x00, 0x00, 0x80},
	{0x80, 0x00, 0x80},
	{0x00, 0x80, 0x80},
	{0xaa, 0xaa, 0xaa},
	{0x55, 0x55, 0x55},
	{0xff, 0x55, 0x55},
	{0x55, 0xff, 0x55},
	{0xff, 0xff, 0x55},
	{0x55, 0x55, 0xff},
	{0xff, 0x55, 0xff},
	{0x55, 0xff, 0xff},
	{0xff, 0xff, 0xff},
	{-1, -1, -1}
};

/*struct rgb rgbcache = {0, 0, 0};
int rgbcache_c = 0;

static inline int find_nearest_color(struct rgb *r, int l)
{
	int dist, dst, min, i;
	if (r->r == rgbcache.r && r->g == rgbcache.g && r->b == rgbcache.b) return rgbcache_c;
	dist = 0xffffff;
	min = 0;
	for (i = 0; i < l; i++) if ((dst = color_distance(r, &palette[i])) < dist)
		dist = dst, min = i;
	return min;
}*/

struct rgb_cache_entry {
	int color;
	int l;
	struct rgb rgb;
};

#define RGB_HASH_SIZE 4096

#define HASH_RGB(r, l) ((((r)->r << 3) + ((r)->g << 2) + (r)->b + (l)) & (RGB_HASH_SIZE - 1))

static inline int find_nearest_color(struct rgb *r, int l)
{
	int dist, dst, min, i;
	static struct rgb_cache_entry rgb_cache[RGB_HASH_SIZE];
	static int cache_init = 0;
	int h;
	if (!cache_init) goto initialize;
	back:
	h = HASH_RGB(r, l);
	if (rgb_cache[h].color != -1 && rgb_cache[h].l == l && rgb_cache[h].rgb.r == r->r && rgb_cache[h].rgb.g == r->g && rgb_cache[h].rgb.b == r->b) return rgb_cache[h].color;
	dist = 0xffffff;
	min = 0;
	for (i = 0; i < l; i++) if ((dst = color_distance(r, &palette[i])) < dist)
		dist = dst, min = i;
	rgb_cache[h].color = min;
	rgb_cache[h].l = l;
	rgb_cache[h].rgb.r = r->r;
	rgb_cache[h].rgb.g = r->g;
	rgb_cache[h].rgb.b = r->b;
	return min;

	initialize:
	for (h = 0; h < RGB_HASH_SIZE; h++) rgb_cache[h].color = -1;
	cache_init = 1;
	goto back;
}

static inline int fg_color(int fg, int bg)
{
	int l = bg < fg ? bg : fg;
	int h = bg < fg ? fg : bg;
	if (l == h || (!l && (h == 4 || h == 8 || h == 12)) ||
	   (l == 1 && (h == 3 || h == 5 || h == 8 || h == 12)) ||
	   (l == 2 && h == 6) || (l == 3 && (h == 5 || h == 12)) ||
	   (l == 4 && (h == 8 || h == 12)) || (l == 5 && (h == 8 || h == 12)))
	   	return (fg == 4 || fg == 12) && (bg == 0 || bg == 8) ? 6 : (7 - 7 * (bg == 2 || bg == 6 || bg == 7));
	return fg;
}

#define XALIGN(x) (((x)+0x7f)&~0x7f)

int nowrap = 0;

static inline int xpand_lines(struct part *p, int y)
{
	/*if (y >= p->y) p->y = y + 1;*/
	if (!p->data) return 0;
	y += p->yp;
	if (y >= p->data->y) {
		int i;
		if (XALIGN(y + 1) >= XALIGN(p->data->y)) {
			struct line *l;
			if (!(l = mem_realloc(p->data->data, XALIGN(y+1)*sizeof(struct line))))
				return -1;
			p->data->data = l;
		}
		for (i = p->data->y; i <= y; i++) {
			p->data->data[i].l = 0;
			p->data->data[i].c = p->bgcolor;
			p->data->data[i].d = DUMMY;
		}
		p->data->y = i;
	}
	return 0;
}

static inline int xpand_line(struct part *p, int y, int x)
{
	if (!p->data) return 0;
	x += p->xp;
	y += p->yp;
#ifdef DEBUG
	if (y >= p->data->y) {
		internal("line does not exist");
		return -1;
	}
#endif
	if (x >= p->data->data[y].l) {
		int i;
		if (XALIGN(x+1) >= XALIGN(p->data->data[y].l)) {
			chr *l;
			if (!(l = mem_realloc(p->data->data[y].d, XALIGN(x+1)*sizeof(chr))))
				return -1;
			p->data->data[y].d = l;
		}
		for (i = p->data->data[y].l; i <= x; i++)
			p->data->data[y].d[i] = (p->data->data[y].c << 11) | ' ';
		p->data->data[y].c = p->bgcolor;
		p->data->data[y].l = i;
	}
	return 0;
}

int r_xpand_spaces(struct part *p, int l)
{
	unsigned char *c;
	if (!(c = mem_realloc(p->spaces, l + 1))) return -1;
	memset(c + p->spl, 0, l - p->spl + 1);
	p->spl = l + 1;
	p->spaces = c;
	return 0;
}

static inline int xpand_spaces(struct part *p, int l)
{
	if (l >= p->spl) return r_xpand_spaces(p, l);
	return 0;
}

#define POS(x, y) (p->data->data[p->yp + (y)].d[p->xp + (x)])
#define LEN(y) (p->data->data[p->yp + (y)].l - p->xp < 0 ? 0 : p->data->data[p->yp + (y)].l - p->xp)
#define SLEN(y, x) p->data->data[p->yp + (y)].l = p->xp + x;
#define X(x) (p->xp + (x))
#define Y(y) (p->yp + (y))

static inline void set_hchar(struct part *p, int x, int y, unsigned c)
{
	if (xpand_lines(p, y)) return;
	if (xpand_line(p, y, x)) return;
	POS(x, y) = c;
}

static inline void set_hchars(struct part *p, int x, int y, int xl, unsigned c)
{
	if (xpand_lines(p, y)) return;
	if (xpand_line(p, y, x+xl-1)) return;
	for (; xl; xl--, x++) POS(x, y) = c;
}

void xset_hchar(struct part *p, int x, int y, unsigned c)
{
	set_hchar(p, x, y, c);
}

void xset_hchars(struct part *p, int x, int y, int xl, unsigned c)
{
	set_hchars(p, x, y, xl, c);
}

int xxpand_lines(struct part *p, int y)
{
	return xpand_lines(p, y);
}

int xxpand_line(struct part *p, int y, int x)
{
	return xpand_line(p, y, x);
}

static inline void set_hline(struct part *p, int x, int y, int xl, unsigned char *d, unsigned c, int spc)
{
	if (xpand_lines(p, y)) return;
	if (xpand_line(p, y, x+xl-1)) return;
	if (spc && xpand_spaces(p, x+xl-1)) return;
	for (; xl; xl--, x++, d++) {
		if (spc) p->spaces[x] = *d == ' ';
		if (p->data) POS(x, y) = *d | c;
	}
}

int last_link_to_move;
struct tag *last_tag_to_move;
struct tag *last_tag_for_newline;

static inline void move_links(struct part *p, int xf, int yf, int xt, int yt)
{
	int n;
	struct tag *t;
	int w = 0;
	if (!p->data) return;
	xpand_lines(p, yt);
	for (n = last_link_to_move; n < p->data->nlinks; n++) {
		int i;
		struct link *link = &p->data->links[n];
			/*printf("ml: %d %d %d %d",link->pos[0].x,link->pos[0].y,X(xf),Y(yf));fflush(stdout);sleep(1);*/
		for (i = 0; i < link->n; i++) if (link->pos[i].y == Y(yf)) {
			w = 1;
			if (link->pos[i].x >= X(xf)) {
				if (yt >= 0) link->pos[i].y = Y(yt), link->pos[i].x += -xf + xt;
				else memmove(&link->pos[i], &link->pos[i+1], (link->n-i-1) * sizeof(struct point)), link->n--, i--;
			}
		}
		/*if (!link->n) {
			if (link->where) mem_free(link->where);
			if (link->target) mem_free(link->target);
			if (link->where_img) mem_free(link->where_img);
			if (link->img_alt) mem_free(link->img_alt);
			if (link->pos) mem_free(link->pos);
			memmove(link, link + 1, (p->data->nlinks - n - 1) * sizeof(struct link));
			p->data->nlinks --;
			n--;
		}*/
		if (!w /*&& n >= 0*/) last_link_to_move = n;
	}
	w = 0;
	if (yt >= 0) for (t = last_tag_to_move->next; (void *)t != &p->data->tags; t = t->next) {
		if (t->y == Y(yf)) {
			w = 1;
			if (t->x >= X(xf)) {
				t->y = Y(yt), t->x += -xf + xt;
			}
		}
		if (!w) last_tag_to_move = t;
	}
}

static inline void copy_chars(struct part *p, int x, int y, int xl, chr *d)
{
	if (xl <= 0) return;
	if (xpand_lines(p, y)) return;
	if (xpand_line(p, y, x+xl-1)) return;
	for (; xl; xl--, x++, d++) POS(x, y) = *d;
}

static inline void move_chars(struct part *p, int x, int y, int nx, int ny)
{
	if (LEN(y) - x <= 0) return;
	copy_chars(p, nx, ny, LEN(y) - x, &POS(x, y));
	SLEN(y, x);
	move_links(p, x, y, nx, ny);
}

static inline void shift_chars(struct part *p, int y, int s)
{
	chr *a;
	int l = LEN(y);
	if (!(a = mem_alloc(l * sizeof(chr)))) return;
	memcpy(a, &POS(0, y), l * sizeof(chr));
	set_hchars(p, 0, y, s, (p->data->data[y].c << 11) | ' ');
	copy_chars(p, s, y, l, a);
	mem_free(a);
	move_links(p, 0, y, s, y);
}

static inline void del_chars(struct part *p, int x, int y)
{
	SLEN(y, x);
	move_links(p, x, y, -1, -1);
}

#define rm(x) ((x).width - (x).rightmargin > 0 ? (x).width - (x).rightmargin : 0)

void line_break(struct part *);

int split_line(struct part *p)
{
	int i;
	/*if (!p->data) goto r;*/
	/*printf("split: %d,%d   , %d,%d,%d\n",p->cx,p->cy,par_format.rightmargin,par_format.leftmargin,p->cx);*/
	for (i = rm(par_format); i >= par_format.leftmargin; i--)
		if (i < p->spl && p->spaces[i]) goto split;
	/*for (i = p->cx - 1; i > rm(par_format) && i > par_format.leftmargin; i--)*/
	for (i = par_format.leftmargin; i < p->cx ; i++)
		if (i < p->spl && p->spaces[i]) goto split;
	/*for (i = rm(par_format); i >= par_format.leftmargin; i--)
		if ((POS(i, p->cy) & 0xff) == ' ') goto split;
	for (i = p->cx - 1; i > rm(par_format) && i > par_format.leftmargin; i--)
		if ((POS(i, p->cy) & 0xff) == ' ') goto split;*/
	if (p->cx + par_format.rightmargin > p->x) p->x = p->cx + par_format.rightmargin;
	/*if (p->y < p->cy + 1) p->y = p->cy + 1;
	p->cy++; p->cx = -1;
	memset(p->spaces, 0, p->spl);
	if (p->data) xpand_lines(p, p->cy + 1);*/
	/*line_break(p);*/
	return 0;
	split:
	if (i + par_format.rightmargin > p->x) p->x = i + par_format.rightmargin;
	if (p->data) {
#ifdef DEBUG
		if ((POS(i, p->cy) & 0xff) != ' ') internal("bad split: %c", (char)POS(i, p->cy));
#endif
		move_chars(p, i+1, p->cy, par_format.leftmargin, p->cy+1);
		del_chars(p, i, p->cy);
	}
	memmove(p->spaces, p->spaces + i + 1, p->spl - i - 1);
	memset(p->spaces + p->spl - i - 1, 0, i + 1);
	memmove(p->spaces + par_format.leftmargin, p->spaces, p->spl - par_format.leftmargin);
	p->cy++; p->cx -= i - par_format.leftmargin + 1;
	/*return 1 + (p->cx == par_format.leftmargin);*/
	if (p->cx == par_format.leftmargin) p->cx = -1;
	if (p->y < p->cy + (p->cx != -1)) p->y = p->cy + (p->cx != -1);
	return 1 + (p->cx == -1);
}

void align_line(struct part *p, int y)
{
	int na;
	if (!p->data) return;
	if (!LEN(y) || par_format.align == AL_LEFT || par_format.align == AL_NO || par_format.align == AL_BLOCK /* !!! fixme! */) return;
	na = rm(par_format) - LEN(y);
	if (par_format.align == AL_CENTER) na /= 2;
	if (na > 0) shift_chars(p, y, na);
}

struct link *new_link(struct f_data *f)
{
	if (!f) return NULL;
	if (!(f->nlinks & (ALLOC_GR - 1))) {
		struct link *l;
		if (!(l = mem_realloc(f->links, (f->nlinks + ALLOC_GR) * sizeof(struct link)))) return NULL;
		f->links = l;
	}
	memset(&f->links[f->nlinks], 0, sizeof(struct link));
#ifdef G
	f->links[f->nlinks].r.x1 = MAXINT;
	f->links[f->nlinks].r.y1 = MAXINT;
#endif
	return &f->links[f->nlinks++];
}

void html_tag(struct f_data *f, unsigned char *t, int x, int y)
{
	struct tag *tag;
	if (!f) return;
	if ((tag = mem_alloc(sizeof(struct tag) + strlen(t) + 1))) {
		tag->x = x;
		tag->y = y;
		strcpy(tag->name, t);
		add_to_list(f->tags, tag);
		if ((void *)last_tag_for_newline == &f->tags) last_tag_for_newline = tag;
	}
}

unsigned char *last_link = NULL;
unsigned char *last_target = NULL;
unsigned char *last_image = NULL;
struct form_control *last_form = NULL;
#ifdef JS
struct js_event_spec *last_js_event = NULL;
#endif

int nobreak;

struct conv_table *convert_table;

void put_chars(struct part *, unsigned char *, int);

#define CH_BUF	256

void put_chars_conv(struct part *p, unsigned char *c, int l)
{
	static char buffer[CH_BUF];
	int bp = 0;
	int pp = 0;
	if (format.attr & AT_GRAPHICS) {
		put_chars(p, c, l);
		return;
	}
	if (!l) put_chars(p, NULL, 0);
	while (pp < l) {
		unsigned char *e;
		if (c[pp] < 128 && c[pp] != '&') {
			putc:
			buffer[bp++] = c[pp++];
			if (bp < CH_BUF) continue;
			goto flush;
		}
		if (c[pp] != '&') {
			struct conv_table *t;
			int i;
			if (!convert_table) goto putc;
			t = convert_table;
			i = pp;
			decode:
			if (!t[c[i]].t) {
				e = t[c[i]].u.str;
			} else {
				t = t[c[i++]].u.tbl;
				if (i >= l) goto putc;
				goto decode;
			}
			pp = i + 1;
		} else {
			int i = pp + 1;
			if (d_opt->plain & 1) goto putc;
			while (i < l && c[i] != ';' && c[i] != '&' && c[i] > ' ') i++;
			if (!(e = get_entity_string(&c[pp + 1], i - pp - 1, d_opt->cp))) goto putc;
			pp = i + (i < l && c[i] == ';');
		}
		if (!e[0]) continue;
		if (!e[1]) {
			buffer[bp++] = e[0];
			if (bp < CH_BUF) continue;
			flush:
			e = "";
			goto flush1;
		}
		while (*e) {
			buffer[bp++] = *(e++);
			if (bp < CH_BUF) continue;
			flush1:
			put_chars(p, buffer, bp);
			bp = 0;
		}
	}
	if (bp) put_chars(p, buffer, bp);
}

void put_chars(struct part *p, unsigned char *c, int l)
{
	static struct text_attrib_beginning ta_cache = { -1, { 0, 0, 0 }, { 0, 0, 0 } };
	static int bg_cache;
	static int fg_cache;

	int bg, fg;
	int i;
	struct link *link;
	struct point *pt;
	/*printf("%d-", p->cx);for (i=0; i<l; i++) printf("%c", c[i]); printf("-\n");sleep(1);*/
	while (par_format.align != AL_NO && p->cx == -1 && l && *c == ' ') c++, l--;
	if (!l) return;
	if (c[0] != ' ' || (c[1] && c[1] != ' ')) {
		last_tag_for_newline = (void *)&p->data->tags;
	}
	if (p->cx == -1) p->cx = par_format.leftmargin;
	if (last_link || last_image || last_form || format.link || format.image 
            || format.form
#ifdef JS
            || format.js_event || last_js_event
#endif
           ) goto process_link;
	no_l:
	/*printf("%d %d\n",p->cx, p->cy);*/
	if (memcmp(&ta_cache, &format, sizeof(struct text_attrib_beginning))) goto format_change;
	bg = bg_cache, fg = fg_cache;
	end_format_change:
	if (p->cx == par_format.leftmargin && *c == ' ' && par_format.align != AL_NO) c++, l--;
	if (p->y < p->cy + 1) p->y = p->cy + 1;
	if (nowrap && p->cx + l > rm(par_format)) return;
	set_hline(p, p->cx, p->cy, l, c, (((fg&0x08)<<3)|(bg<<3)|(fg&0x07))<<8, 1);
	p->cx += l;
	nobreak = 0;
	if (par_format.align != AL_NO)
		while (p->cx > rm(par_format) && p->cx > par_format.leftmargin) {
			int x;
			/*if (p->cx > p->x) {
				p->x = p->cx + par_format.rightmargin;
				if (c[l - 1] == ' ') p->x--;
			}*/
			if (!(x = split_line(p))) break;
			/*if (LEN(p->cy-1) > p->x) p->x = LEN(p->cy-1);*/
			align_line(p, p->cy - 1);
			nobreak = x - 1;
		}
	if ((p->xa += l) - (c[l-1] == ' ' && par_format.align != AL_NO) + par_format.leftmargin + par_format.rightmargin > p->xmax) p->xmax = p->xa - (c[l-1] == ' ' && par_format.align != AL_NO) + par_format.leftmargin + par_format.rightmargin;
	return;

	/* !!! WARNING: THE FOLLOWING CODE IS SHADOWED IN HTML_GR.C */
	
	process_link:
	if ((last_link /*|| last_target*/ || last_image || last_form) &&
	    !xstrcmp(format.link, last_link) && !xstrcmp(format.target, last_target) &&
	    !xstrcmp(format.image, last_image) && format.form == last_form
#ifdef JS
	    && ((!format.js_event && !last_js_event) || !compare_js_event_spec(format.js_event, last_js_event))
#endif
           ) {
		if (!p->data) goto x;
		link = &p->data->links[p->data->nlinks - 1];
		if (!p->data->nlinks) {
			internal("no link");
			goto no_l;
		}
		goto set_link;
		x:;
	} else {
		if (last_link) mem_free(last_link);
		if (last_target) mem_free(last_target);
		if (last_image) mem_free(last_image);
#ifdef JS
		free_js_event_spec(last_js_event);
		last_js_event = NULL;
#endif
                last_link = last_target = last_image = NULL;
		last_form = NULL;
		if (!(format.link || format.image || format.form 
#ifdef JS
		|| format.js_event
#endif
                )) goto no_l;
		if (d_opt->num_links) {
			unsigned char s[64];
			unsigned char *fl = format.link, *ft = format.target, *fi = format.image;
			struct form_control *ff = format.form;
#ifdef JS
			struct js_event_spec *js = format.js_event;
			format.js_event = NULL;
#endif
                        format.link = format.target = format.image = NULL;
			format.form = NULL;
			s[0] = '[';
			snzprint(s + 1, 62, p->link_num);
			strcat(s, "]");
			put_chars(p, s, strlen(s));
			if (ff && ff->type == FC_TEXTAREA) line_break(p);
			if (p->cx == -1) p->cx = par_format.leftmargin;
			format.link = fl, format.target = ft, format.image = fi;
			format.form = ff;
#ifdef JS
			format.js_event = js;
#endif
                }
		p->link_num++;
		last_link = stracpy(format.link);
		last_target = stracpy(format.target);
		last_image = stracpy(format.image);
		last_form = format.form;
#ifdef JS
		copy_js_event_spec(&last_js_event, format.js_event);
#endif
                if (!p->data) goto no_l;
		if (!(link = new_link(p->data))) goto no_l;
		link->num = p->link_num - 1;
		link->pos = DUMMY;
#ifdef JS
		copy_js_event_spec(&link->js_event, format.js_event);
#endif
                if (!last_form) {
			link->type = L_LINK;
			link->where = stracpy(last_link);
			link->target = stracpy(last_target);
		} else {
			link->type = last_form->type == FC_TEXT || last_form->type == FC_PASSWORD || last_form->type == FC_FILE ? L_FIELD : last_form->type == FC_TEXTAREA ? L_AREA : last_form->type == FC_CHECKBOX || last_form->type == FC_RADIO ? L_CHECKBOX : last_form->type == FC_SELECT ? L_SELECT : L_BUTTON;
			link->form = last_form;
			link->target = stracpy(last_form->target);
		}
		link->where_img = stracpy(last_image);
		if (link->type != L_FIELD && link->type != L_AREA) {
			bg = find_nearest_color(&format.clink, 8);
			fg = find_nearest_color(&format.bg, 8);
			fg = fg_color(fg, bg);
		} else {
			fg = find_nearest_color(&format.fg, 8);
			bg = find_nearest_color(&format.bg, 8);
			fg = fg_color(fg, bg);
		}
		link->sel_color = ((fg & 8) << 3) | (fg & 7) | (bg << 3);
		link->n = 0;
		set_link:
		if ((pt = mem_realloc(link->pos, (link->n + l) * sizeof(struct point)))) {
			link->pos = pt;
			for (i = 0; i < l; i++) pt[link->n + i].x = X(p->cx) + i,
						pt[link->n + i].y = Y(p->cy);
			link->n += i;
		}
	}
	goto no_l;

		format_change:
		bg = find_nearest_color(&format.bg, 8);
		fg = find_nearest_color(&format.fg, 16);
		fg = fg_color(fg, bg);
		if (format.attr & AT_ITALIC) fg = fg ^ 0x01;
		if (format.attr & AT_UNDERLINE) fg = (fg ^ 0x04) | 0x08;
		if (format.attr & AT_BOLD) fg = fg | 0x08;
		fg = fg_color(fg, bg);
		if (format.attr & AT_GRAPHICS) bg = bg | 0x10;
		memcpy(&ta_cache, &format, sizeof(struct text_attrib_beginning));
		fg_cache = fg; bg_cache = bg;
		goto end_format_change;
}

void line_break(struct part *p)
{
	struct tag *t;
	/*printf("-break-\n");*/
	if (p->cx + par_format.rightmargin > p->x) p->x = p->cx + par_format.rightmargin;
	if (nobreak) {
		/*if (p->y < p->cy) p->y = p->cy;*/
		nobreak = 0;
		p->cx = -1;
		p->xa = 0;
		return;
	}
	if (!p->data) goto e;
	/*move_links(p, p->cx, p->cy, 0, p->cy + 1);*/
	xpand_lines(p, p->cy + 1);
	if (p->cx > par_format.leftmargin && LEN(p->cy) > p->cx - 1 && (POS(p->cx-1, p->cy) & 0xff) == ' ') del_chars(p, p->cx-1, p->cy), p->cx--;
	/*if (LEN(p->cy) > p->x) p->x = LEN(p->cy);*/
	if (p->cx > 0) align_line(p, p->cy);
	if (p->data) for (t = last_tag_for_newline; t && (void *)t != &p->data->tags; t = t->prev) {
		t->x = X(0);
		t->y = Y(p->cy + 1);
	}
	e:
	p->cy++; p->cx = -1; p->xa = 0; /*if (p->y < p->cy) p->y = p->cy;*/
	memset(p->spaces, 0, p->spl);
}

int g_ctrl_num;

/* SHADOWED IN g_html_form_control */
void html_form_control(struct part *p, struct form_control *fc)
{
	if (!p->data) {
		/*destroy_fc(fc);
		mem_free(fc);*/
		add_to_list(p->uf, fc);
		return;
	}
	fc->g_ctrl_num = g_ctrl_num++;
	if (fc->type == FC_TEXT || fc->type == FC_PASSWORD || fc->type == FC_TEXTAREA) {
		unsigned char *dv = convert_string(convert_table, fc->default_value, strlen(fc->default_value), d_opt);
		if (dv) {
			mem_free(fc->default_value);
			fc->default_value = dv;
		}
		/*
		for (i = 0; i < fc->nvalues; i++) if ((dv = convert_string(convert_table, fc->values[i], strlen(fc->values[i]), d_opt))) {
			mem_free(fc->values[i]);
			fc->values[i] = dv;
		}
		*/
	}
	add_to_list(p->data->forms, fc);
}

void add_frameset_entry(struct frameset_desc *fsd, struct frameset_desc *subframe, unsigned char *name, unsigned char *url, int marginwidth, int marginheight)
{
	if (fsd->yp >= fsd->y) return;
	fsd->f[fsd->xp + fsd->yp * fsd->x].subframe = subframe;
	fsd->f[fsd->xp + fsd->yp * fsd->x].name = stracpy(name);
	fsd->f[fsd->xp + fsd->yp * fsd->x].url = stracpy(url);
	fsd->f[fsd->xp + fsd->yp * fsd->x].marginwidth = marginwidth;
	fsd->f[fsd->xp + fsd->yp * fsd->x].marginheight = marginheight;
	if (++fsd->xp >= fsd->x) fsd->xp = 0, fsd->yp++;
}

struct frameset_desc *create_frameset(struct f_data *fda, struct frameset_param *fp)
{
	int i;
	struct frameset_desc *fd;
	if (!fp->x || !fp->y) {
		internal("zero size of frameset");
		return NULL;
	}
	if (!(fd = mem_alloc(sizeof(struct frameset_desc) + fp->x * fp->y * sizeof(struct frame_desc)))) return NULL;
	memset(fd, 0, sizeof(struct frameset_desc) + fp->x * fp->y * sizeof(struct frame_desc));
	fd->n = fp->x * fp->y;
	fd->x = fp->x;
	fd->y = fp->y;
	for (i = 0; i < fd->n; i++) {
		fd->f[i].xw = fp->xw[i % fp->x];
		fd->f[i].yw = fp->yw[i / fp->x];
	}
	if (fp->parent) add_frameset_entry(fp->parent, fd, NULL, NULL, -1, -1);
	else if (!fda->frame_desc) fda->frame_desc = fd;
	     else mem_free(fd), fd = NULL;
	return fd;
}

void create_frame(struct frame_param *fp)
{
	add_frameset_entry(fp->parent, NULL, fp->name, fp->url, fp->marginwidth, fp->marginheight);
}

void process_script(struct f_data *f, unsigned char *t)
{
	if (!f->script_href_base) f->script_href_base = stracpy(format.href_base);
	if (t) {
		unsigned char *u;
		u = join_urls(f->script_href_base, t);
		if (u) {
			request_additional_file(f, u);
			mem_free(u);
		}
	}
	f->are_there_scripts = 1;
}

void *html_special(struct part *p, int c, ...)
{
	va_list l;
	unsigned char *t;
	struct form_control *fc;
	struct frameset_param *fsp;
	struct frame_param *fp;
	va_start(l, c);
	switch (c) {
		case SP_TAG:
			t = va_arg(l, unsigned char *);
			va_end(l);
			html_tag(p->data, t, X(p->cx), Y(p->cy));
			break;
		case SP_CONTROL:
			fc = va_arg(l, struct form_control *);
			va_end(l);
			html_form_control(p, fc);
			break;
		case SP_TABLE:
			return convert_table;
		case SP_USED:
			return (void *)!!p->data;
		case SP_FRAMESET:
			fsp = va_arg(l, struct frameset_param *);
			va_end(l);
			return create_frameset(p->data, fsp);
		case SP_FRAME:
			fp = va_arg(l, struct frame_param *);
			va_end(l);
			create_frame(fp);
			break;
		case SP_NOWRAP:
			nowrap = va_arg(l, int);
			va_end(l);
			break;
		case SP_SCRIPT:
			t = va_arg(l, unsigned char *);
			va_end(l);
			if (p->data) process_script(p->data, t);
			break;
		case SP_REFRESH:
			{
				ttime timer;
				t = va_arg(l, unsigned char *);
				timer = va_arg(l, ttime);
				va_end(l);
                	        fd_refresh_install(((struct f_data *)p->data)->fd, t, timer);
                        	break;
			}
        	default:
			va_end(l);
			internal("html_special: unknown code %d", c);
	}
	return NULL;
}

void do_format(char *start, char *end, struct part *part, unsigned char *head)
{
	pr(
	parse_html(start, end, (void (*)(void *, unsigned char *, int)) put_chars_conv, (void (*)(void *)) line_break, (void *(*)(void *, int, ...)) html_special, part, head);
	/*if ((part->y -= line_breax) < 0) part->y = 0;*/
	);
}

int margin;

struct table_cache_entry {
	struct table_cache_entry *next;
        struct table_cache_entry *prev;
        struct table_cache_entry *hash_next;
	unsigned char *start;
	unsigned char *end;
	int align;
	int m;
	int width;
	int xs;
	int link_num;
	struct part p;
};

struct list_head table_cache = { &table_cache, &table_cache };

#define TC_HASH_SIZE	4096

struct table_cache_entry *table_cache_hash[TC_HASH_SIZE];

void free_table_cache()
{
	struct table_cache_entry *tce;
	foreach(tce, table_cache) {
		int hash = ((int)(unsigned long)tce->start + tce->xs) & (TC_HASH_SIZE - 1);
		table_cache_hash[hash] = NULL;
	}
        free_list(table_cache);
}

struct part *format_html_part(unsigned char *start, unsigned char *end, int align, int m, int width, struct f_data *data, int xs, int ys, unsigned char *head, int link_num)
{
	struct part *p;
	struct html_element *e;
	int llm = last_link_to_move;
	struct tag *ltm = last_tag_to_move;
	/*struct tag *ltn = last_tag_for_newline;*/
	int lm = margin;
	int ef = empty_format;
	struct form_control *fc;
	struct table_cache_entry *tce;
	if (!data) {
		tce = table_cache_hash[((int)(unsigned long)start + xs) & (TC_HASH_SIZE - 1)];
		while (tce) {
			if (tce->start == start && tce->end == end && tce->align == align && tce->m == m && tce->width == width && tce->xs == xs && tce->link_num == link_num) {
				if ((p = mem_alloc(sizeof(struct part)))) {
					memcpy(p, &tce->p, sizeof(struct part));
					return p;
				}
			}
			tce = tce->hash_next;
		}
	}
	if (ys < 0) {
		internal("format_html_part: ys == %d", ys);
		return NULL;
	}
	if (data) {
		struct node *n;
		if ((n = mem_alloc(sizeof(struct node)))) {
			n->x = xs;
			n->y = ys;
			n->xw = !table_level ? MAXINT : width;
			add_to_list(data->nodes, n);
		}
		/*sdbg(data);*/
	}
	last_link_to_move = data ? data->nlinks : 0;
	last_tag_to_move = data ? (void *)&data->tags : NULL;
	last_tag_for_newline = data ? (void *)&data->tags: NULL;
	margin = m;
	empty_format = !data;
	if (last_link) mem_free(last_link);
	if (last_image) mem_free(last_image);
	if (last_target) mem_free(last_target);
#ifdef JS
	free_js_event_spec(last_js_event);
	last_js_event = NULL;
#endif
        last_link = last_image = last_target = NULL;
	last_form = NULL;
	nobreak = 1;
	if (!(p = mem_alloc(sizeof(struct part)))) goto ret;
	p->x = p->y = 0;
	p->data = data;
	p->xp = xs; p->yp = ys;
	p->xmax = p->xa = 0;
	p->bgcolor = find_nearest_color(&par_format.bgcolor, 8);
	p->spaces = DUMMY;
	p->spl = 0;
	p->link_num = link_num;
	init_list(p->uf);
	html_stack_dup();
	e = &html_top;
	html_top.dontkill = 2;
	html_top.namelen = 0;
	par_format.align = align;
	par_format.leftmargin = m;
	par_format.rightmargin = m;
	par_format.width = width;
	par_format.list_level = 0;
	par_format.list_number = 0;
	par_format.dd_margin = 0;
	p->cx = -1;
	p->cy = 0;
	do_format(start, end, p, head);
	if (p->xmax < p->x) p->xmax = p->x;
	nobreak = 0;
	line_breax = 1;
	if (last_link) mem_free(last_link);
	if (last_image) mem_free(last_image);
	if (last_target) mem_free(last_target);
#ifdef JS
	free_js_event_spec(last_js_event);
#endif
        while (&html_top != e) {
		kill_html_stack_item(&html_top);
		if (!&html_top || (void *)&html_top == (void *)&html_stack) {
			internal("html stack trashed");
			break;
		}
	}
	html_top.dontkill = 0;
	kill_html_stack_item(&html_top);
	mem_free(p->spaces);
	if (data) {
		struct node *n = data->nodes.next;
		n->yw = ys - n->y + p->y;
	}
	foreach(fc, p->uf) destroy_fc(fc);
	free_list(p->uf);
	ret:
	last_link_to_move = llm;
	last_tag_to_move = ltm;
	/*last_tag_for_newline = ltn;*/
	margin = lm;
	empty_format = ef;
	last_link = last_image = last_target = NULL;
	last_form = NULL;
#ifdef JS
	last_js_event = NULL;
#endif

	if (table_level > 1 && !data && ((tce = mem_alloc(sizeof(struct table_cache_entry))))) {
                int hash;
		tce->start = start;
		tce->end = end;
		tce->align = align;
		tce->m = m;
		tce->width = width;
		tce->xs = xs;
		tce->link_num = link_num;
		memcpy(&tce->p, p, sizeof(struct part));
		add_to_list(table_cache, tce);
		hash = ((int)(unsigned long)start + xs) & (TC_HASH_SIZE - 1);
		tce->hash_next = table_cache_hash[hash];
		table_cache_hash[hash] = tce;
	}
	return p;
}

void release_part(struct part *p)
{
	mem_free(p);
}

void push_base_format(unsigned char *url, struct document_options *opt, int frame)
{
	struct html_element *e;
	if (html_stack.next != &html_stack) {
		internal("something on html stack");
		init_list(html_stack);
	}
	if (!(e = mem_alloc(sizeof(struct html_element)))) return;
	memset(e, 0, sizeof(struct html_element));
	add_to_list(html_stack, e);
	format.attr = opt->plain & 1 ? AT_FIXED : 0;
	format.fontsize = 3;
	format.link = format.target = format.image = format.select = NULL;
	format.form = NULL;
	memcpy(&format.fg, &opt->default_fg, sizeof(struct rgb));
	memcpy(&format.bg, &opt->default_bg, sizeof(struct rgb));
	memcpy(&format.clink, &opt->default_link, sizeof(struct rgb));
	memcpy(&format.vlink, &opt->default_vlink, sizeof(struct rgb));
	format.href_base = stracpy(url);
	format.target_base = stracpy(opt->framename);
	par_format.align = opt->plain & 1 ? AL_NO : AL_LEFT;
	par_format.leftmargin = opt->plain & 1 ? 0 : opt->margin;
	par_format.rightmargin = opt->plain & 1 ? 0 : opt->margin;
	if (frame && par_format.leftmargin) par_format.leftmargin = 1;
	if (frame && par_format.rightmargin) par_format.rightmargin = 1;
	par_format.width = opt->xw;
	par_format.list_level = par_format.list_number = 0;
	par_format.dd_margin = opt->margin;
	par_format.flags = 0;
	memcpy(&par_format.bgcolor, &opt->default_bg, sizeof(struct rgb));
	html_top.invisible = 0;
	html_top.name = NULL; html_top.namelen = 0; html_top.options = NULL;
	html_top.linebreak = 1;
	html_top.dontkill = 1;
}

struct conv_table *get_convert_table(unsigned char *head, int to, int def, int *frm, int *aa, int hard)
{
	int from = -1;
	unsigned char *a, *b;
	unsigned char *p = head;
	while (from == -1 && p && (a = parse_http_header(p, "Content-Type", &p))) {
		if ((b = parse_header_param(a, "charset"))) {
			from = get_cp_index(b);
			mem_free(b);
		}
		mem_free(a);
	}
	if (from == -1 && head && (a = parse_http_header(head, "Content-Charset", NULL))) {
		from = get_cp_index(a);
		mem_free(a);
	}
	if (from == -1 && head && (a = parse_http_header(head, "Charset", NULL))) {
		from = get_cp_index(a);
		mem_free(a);
	}
	if (aa) {
		*aa = from == -1;
		if (hard && !*aa) *aa = 2;
	}
	if (hard || from == -1) from = def;
	if (frm) *frm = from;
	return get_translation_table(from, to);
}

struct document_options dd_opt = { 0 };

struct document_options *d_opt = &dd_opt;

struct f_data *current_f_data = NULL;

void really_format_html(struct cache_entry *ce, unsigned char *start, unsigned char *end, struct f_data *screen, int frame)
{
	unsigned char *url = ce->url;
	unsigned char *head, *t;
	int hdl;
	int i;
	unsigned char *bg = NULL, *bgcolor = NULL;
	current_f_data = screen;
	memset(table_cache_hash, 0, sizeof(table_cache_hash));
	d_opt = &screen->opt;
	screen->use_tag = ce->count;
	startf = start;
	eofff = end;
	head = init_str(), hdl = 0;
	if (ce->head) add_to_str(&head, &hdl, ce->head);
	scan_http_equiv(start, end, &head, &hdl, &t, d_opt->plain ? NULL : &bg, d_opt->plain ? NULL : &bgcolor);
	convert_table = get_convert_table(head, screen->opt.cp, screen->opt.assume_cp, &screen->cp, &screen->ass, screen->opt.hard_assume);
	i = d_opt->plain; d_opt->plain = 0;
        if(screen->title)
                mem_free(screen->title);
        screen->title = convert_string(convert_table, t, strlen(t), d_opt);
	d_opt->plain = i;
	mem_free(t);
	push_base_format(url, &screen->opt, frame);
	table_level = 0;
	g_ctrl_num = 0;
	last_form_tag = NULL;
	last_form_attr = NULL;
	last_input_tag = NULL;
	if (!F) {
		struct part *rp;
		if ((rp = format_html_part(start, end, par_format.align, par_format.leftmargin, screen->opt.xw, screen, 0, 0, head, 1))) release_part(rp);
#ifdef G
	} else {
		struct g_part *rp;
		if ((rp = g_format_html_part(start, end, par_format.align, par_format.leftmargin, screen->opt.xw - G_SCROLL_BAR_WIDTH, head, 1, bg, bgcolor, screen))) {
			int w = screen->opt.xw;
			int h = screen->opt.yw;
			screen->x = rp->x;
			screen->y = rp->root->yw;
			if (screen->x > w) w = screen->x;
			if (screen->y > h) h = screen->y;
			g_x_extend_area(rp->root, w, h);
			screen->root = (struct g_object *)rp->root, rp->root = NULL;
			screen->hsb = 0, screen->vsb = 0;
			screen->hsbsize = 0, screen->vsbsize = 0;
			x:
			if (!screen->hsb && screen->x > screen->opt.xw - screen->vsb * G_SCROLL_BAR_WIDTH) {
				screen->hsb = 1;
				goto x;
			}
			if (!screen->vsb && screen->y > screen->opt.yw - screen->hsb * G_SCROLL_BAR_WIDTH) {
				screen->vsb = 1;
				goto x;
			}
			if (screen->hsb) screen->hsbsize = screen->opt.xw - screen->vsb * G_SCROLL_BAR_WIDTH;
			if (screen->vsb) screen->vsbsize = screen->opt.yw - screen->hsb * G_SCROLL_BAR_WIDTH;
			if (screen->hsb < 0) screen->hsb = 0;
			if (screen->vsb < 0) screen->vsb = 0;
			g_release_part(rp);
			mem_free(rp);
			get_parents(screen, screen->root);
		}
#endif
	}
	mem_free(head);
	if (bg) mem_free(bg);
	if (bgcolor) mem_free(bgcolor);
	if (!F) {
		screen->x = 0;
		for (i = screen->y - 1; i >= 0; i--) {
			if (!screen->data[i].l) mem_free(screen->data[i].d), screen->y--;
			else break;
		}
		for (i = 0; i < screen->y; i++) if (screen->data[i].l > screen->x) screen->x = screen->data[i].l;
	}
	if (form.action) mem_free(form.action), form.action = NULL;
	if (form.target) mem_free(form.target), form.target = NULL;
	if (form.form_name) mem_free(form.form_name), form.form_name = NULL;
	if (form.onsubmit) mem_free(form.onsubmit), form.onsubmit = NULL;
	kill_html_stack_item(html_stack.next);
	if (html_stack.next != &html_stack) {
		internal("html stack not empty after operation");
		init_list(html_stack);
	}
	screen->bg = 007 << 8; /* !!! FIXME */
	sort_links(screen);
	current_f_data = NULL;
	d_opt = &dd_opt;
}

int compare_opt(struct document_options *o1, struct document_options *o2)
{
	double kdo_si_hraje_nezlobi____a_nebo_to_je_PerM=o1->bfu_aspect-o2->bfu_aspect;
	
	if (o1->xw == o2->xw &&
	    o1->yw == o2->yw &&
	    o1->xp == o2->xp &&
	    o1->yp == o2->yp &&
	    o1->col == o2->col &&
	    o1->cp == o2->cp &&
	    o1->assume_cp == o2->assume_cp &&
	    o1->hard_assume == o2->hard_assume &&
	    o1->tables == o2->tables &&
	    o1->frames == o2->frames &&
	    o1->images == o2->images &&
	    o1->margin == o2->margin &&
#ifdef JS
	    o1->js_enable == o2->js_enable &&
#endif
            o1->plain == o2->plain &&
	    o1->font_size == o2->font_size &&
	    !memcmp(&o1->default_fg, &o2->default_fg, sizeof(struct rgb)) &&
	    !memcmp(&o1->default_bg, &o2->default_bg, sizeof(struct rgb)) &&
	    !memcmp(&o1->default_link, &o2->default_link, sizeof(struct rgb)) &&
	    !memcmp(&o1->default_vlink, &o2->default_vlink, sizeof(struct rgb)) &&
	    o1->num_links == o2->num_links &&
	    o1->table_order == o2->table_order &&
	    o1->display_images == o2->display_images &&
	    o1->image_scale == o2->image_scale &&
	    o1->aspect_on == o2->aspect_on &&
	    kdo_si_hraje_nezlobi____a_nebo_to_je_PerM<=0.0001 &&
	    kdo_si_hraje_nezlobi____a_nebo_to_je_PerM>=-0.0001 &&
	    ((o1->framename && o2->framename && !_stricmp(o1->framename, o2->framename)) || (!o1->framename && !o2->framename))) return 0;
	return 1;
}

void copy_opt(struct document_options *o1, struct document_options *o2)
{
	memcpy(o1, o2, sizeof(struct document_options));
	o1->framename = stracpy(o2->framename);
}

void add_srch_chr(struct f_data *f, unsigned char c, int x, int y, int nn)
{
	int n = f->nsearch;
	if (c == ' ' && (!n || f->search[n - 1].c == ' ')) return;
	f->search[n].c = c;
	f->search[n].x = x;
	f->search[n].y = y;
	f->search[n].n = nn;
	f->nsearch++;
}

/*void sdbg(struct f_data *f)
{
	struct node *n;
	foreachback(n, f->nodes) {
		int xm = n->x + n->xw, ym = n->y + n->yw;
		printf("%d %d - %d %d\n", n->x, n->y, xm, ym);
		fflush(stdout);
	}
	debug("!");
}*/

void sort_srch(struct f_data *f)
{
	int i;
	int *min, *max;
	if (!(f->slines1 = mem_alloc(f->y * sizeof(struct search *)))) return;
	if (!(f->slines2 = mem_alloc(f->y * sizeof(struct search *)))) {
		mem_free(f->slines1);
		return;
	}
	if (!(min = mem_alloc(f->y * sizeof(int)))) {
		mem_free(f->slines1);
		mem_free(f->slines2);
		return;
	}
	if (!(max = mem_alloc(f->y * sizeof(int)))) {
		mem_free(f->slines1);
		mem_free(f->slines2);
		mem_free(min);
		return;
	}
	memset(f->slines1, 0, f->y * sizeof(struct search *));
	memset(f->slines2, 0, f->y * sizeof(struct search *));
	for (i = 0; i < f->y; i++) min[i] = MAXINT, max[i] = 0;
	for (i = 0; i < f->nsearch; i++) {
		struct search *s = &f->search[i];
		if (s->x < min[s->y]) min[s->y] = s->x, f->slines1[s->y] = s;
		if (s->x + s->n > max[s->y]) max[s->y] = s->x + s->n, f->slines2[s->y] = s;
	}
	mem_free(min);
	mem_free(max);
}

int get_srch(struct f_data *f)
{
	struct node *n;
	int cnt = 0;
	int cc = !f->search;
	foreachback(n, f->nodes) {
		int x, y;
		int xm = n->x + n->xw, ym = n->y + n->yw;
		/*printf("%d %d - %d %d\n", n->x, n->y, xm, ym);
		fflush(stdout);*/
		for (y = n->y; y < ym && y < f->y; y++) {
			int ns = 1;
			for (x = n->x; x < xm && x < f->data[y].l; x++) {
				unsigned char c = f->data[y].d[x];
				if (c < ' ') c = ' ' ;
				if (c == ' ' && ns) continue;
				if (ns) {
					if (!cc) add_srch_chr(f, c, x, y, 1);
					else cnt++;
					ns = 0;
					continue;
				}
				if (c != ' ')  if (!cc) add_srch_chr(f, c, x, y, 1);
					       else cnt++;
				else {
					int xx;
					for (xx = x + 1; xx < xm && xx < f->data[y].l; xx++) if ((unsigned char) f->data[y].d[xx] >= ' ') goto ja_uz_z_toho_programovani_asi_zcvoknu;
					xx = x;
					ja_uz_z_toho_programovani_asi_zcvoknu:
					if (!cc) add_srch_chr(f, ' ', x, y, xx - x);
					else cnt++;
					if (xx == x) break;
					x = xx - 1;
				}
			}
			if (!cc) add_srch_chr(f, ' ', x, y, 0);
			else cnt++;
		}
	}
	return cnt;
	
}

void get_search_data(struct f_data *f)
{
	int n;
	if (f->search) return;
	n = get_srch(f);
	f->nsearch = 0;
	if (!(f->search = mem_alloc(n * sizeof(struct search)))) return;
	get_srch(f);
	while (f->nsearch && f->search[f->nsearch - 1].c == ' ') f->nsearch--;
	/*debug("!");*/
	sort_srch(f);
}
