/* view_gr.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "cfg.h"

#ifdef G

#include "links.h"

int *highlight_positions = NULL;
int n_highlight_positions = 0;
int highlight_position_size = 0;

static int root_x = 0;
static int root_y = 0;

void g_get_search_data(struct f_data *f);

static int previous_link=-1;	/* for mouse event handlers */

void g_draw_background(struct graphics_device *dev, struct background *bg, int x, int y, int xw, int yw)
{
	if (xw > 4096) {
		int halfx = x + xw / 2;
		if (dev->clip.x1 < halfx) g_draw_background(dev, bg, x, y, halfx - x, yw);
		if (dev->clip.x2 > halfx) g_draw_background(dev, bg, halfx, y, x + xw - halfx, yw);
		return;
	}
	if (yw > 4096) {
		int halfy = y + yw / 2;
		if (dev->clip.y1 < halfy) g_draw_background(dev, bg, x, y, xw, halfy - y);
		if (dev->clip.y2 > halfy) g_draw_background(dev, bg, x, halfy, xw, y + yw - halfy);
		return;
	}
	if (bg->img) {
		img_draw_decoded_image(dev, bg->u.img, x, y, xw, yw, x - root_x, y - root_y);
	} else {
		drv->fill_area(dev, x, y, x + xw, y + yw
			       , dip_get_color_sRGB(bg->u.sRGB));
	}
}

void g_release_background(struct background *bg)
{
	if (bg->img) img_release_decoded_image(bg->u.img);
	mem_free(bg);
}

void g_dummy_draw(struct f_data_c *fd, struct g_object *t, int x, int y)
{
}

void g_tag_destruct(struct g_object *t)
{
	mem_free(t);
}

void g_dummy_mouse(struct f_data_c *fd, struct g_object *a, int x, int y, int b)
{
}

int print_all_textarea = 0;

void g_widget_checkbox(struct graphics_device *dev,int x0,int y0,int x1,int y1,long bg_color,long fg_color)
{
        dev->drv->draw_hline(dev, x0+2, y0+2, x1-1, fg_color);
        dev->drv->draw_hline(dev, x0+2, y1-2, x1-1, fg_color);
        dev->drv->draw_vline(dev, x0, y0+3, y1-2, fg_color);
        dev->drv->draw_vline(dev, x1, y0+3, y1-2, fg_color);
}

void g_widget_select(struct graphics_device *dev,int x0,int y0,int x1,int y1,long bg_color,long fg_color)
{
        dev->drv->draw_hline(dev, x0, y1, x1+1, fg_color);
        dev->drv->draw_vline(dev, x1, y0, y1, fg_color);

        dev->drv->draw_vline(dev, x0+2, y0+2, y1-1, fg_color);
        dev->drv->draw_vline(dev, x0+4, y0+4, y1-3, fg_color);
        dev->drv->draw_vline(dev, x0+6, y0+6, y1-5, fg_color);
        dev->drv->draw_vline(dev, x1-2, y0+2, y1-1, fg_color);
        dev->drv->draw_vline(dev, x1-4, y0+4, y1-3, fg_color);
        dev->drv->draw_vline(dev, x1-6, y0+6, y1-5, fg_color);

}

void g_widget_button(struct graphics_device *dev,int x0,int y0,int x1,int y1,long bg_color,long fg_color)
{
        dev->drv->draw_hline(dev, x0, y1, x1+1, fg_color);
        dev->drv->draw_vline(dev, x1, y0, y1, fg_color);
}

void g_widget_textarea(struct graphics_device *dev,int x0,int y0,int x1,int y1,long bg_color,long fg_color)
{
        dev->drv->draw_hline(dev, x0, y0, x1+1, fg_color);
//        dev->drv->draw_hline(dev, x0, y1, x1+1, fg_color);
        dev->drv->draw_vline(dev, x0, y0, y1+1, fg_color);
//        dev->drv->draw_vline(dev, x1, y0, y1+1, fg_color);
}

void g_draw_box(struct graphics_device *dev,int x0,int y0,int x1,int y1,long bg_color,long fg_color)
{
        dev->drv->draw_hline(dev, x0, y0, x1+1, fg_color);
        dev->drv->draw_hline(dev, x0, y1, x1+1, fg_color);
        dev->drv->draw_vline(dev, x0, y0, y1+1, fg_color);
        dev->drv->draw_vline(dev, x1, y0, y1+1, fg_color);
}



void g_text_draw(struct f_data_c *fd, struct g_object_text *t, int x, int y)
{
	struct form_control *form;
	struct form_state *fs;
	struct link *link = t->link_num >= 0 ? fd->f_data->links + t->link_num : NULL;
        int    link_selected = (t->link_num>=0 && t->link_num == fd->vs->current_link)?1:0;
        struct graphics_device *dev = fd->ses->term->dev;
	int l;
	int ll;
	int i, j;
	int yy;
	int cur;
	struct line_info *ln, *lnx;
        struct style *inv_style=g_invert_style(t->style);
        long bg_color=dip_get_color_sRGB((t->style->r0*256+t->style->g0)*256+t->style->b0);
        long fg_color=dip_get_color_sRGB((t->style->r1*256+t->style->g1)*256+t->style->b1);
        if (link && ((form = link->form)) && ((fs = find_form_state(fd, form)))) {
                /* HTML Form */
                struct style *form_style=mem_alloc(sizeof(struct style));
                struct style *form_inv_style;
                long form_bg_color,form_fg_color;
                struct rgb form_bg=options_get_rgb("default_form_bg_g");
                struct rgb form_fg=options_get_rgb("default_form_fg_g");
                memcpy(form_style,t->style,sizeof(struct style));
                form_style->r0=form_bg.r;
                form_style->g0=form_bg.g;
                form_style->b0=form_bg.b;
                form_style->r1=form_fg.r;
                form_style->g1=form_fg.g;
                form_style->b1=form_fg.b;
                form_inv_style=g_invert_style(form_style);
                form_bg_color=dip_get_color_sRGB((form_style->r0*256+form_style->g0)*256+form_style->b0);
                form_fg_color=dip_get_color_sRGB((form_style->r1*256+form_style->g1)*256+form_style->b1);

                switch (form->type) {
			case FC_RADIO:
				g_print_text(drv, fd->ses->term->dev, x, y, t->style, fs->state ? SYMBOL_RADIO_PRESSED : SYMBOL_RADIO_DEPRESSED, NULL);
                                /*g_widget_checkbox(dev,x,y,x+t->xw-1,y+t->yw-1,bg_color,fg_color);*/
                                goto end_form_draw;
			case FC_CHECKBOX:
				g_print_text(drv, fd->ses->term->dev, x, y, t->style, fs->state ? SYMBOL_CHECKBOX_CHECKED : SYMBOL_CHECKBOX_UNCHECKED, NULL);
                                /*g_widget_checkbox(dev,x,y,x+t->xw-1,y+t->yw-1,bg_color,fg_color);*/
                                goto end_form_draw;
			case FC_SELECT:
				fixup_select_state(form, fs);
				l = 0;
                                g_print_text(drv, fd->ses->term->dev, x + l, y, form_style, " ", &l);
                                if (fs->state < form->nvalues) g_print_text(drv, fd->ses->term->dev, x+l, y, form_style, form->labels[fs->state], &l);
                                while (l < t->xw) g_print_text(drv, fd->ses->term->dev, x + l, y, form_style, " ", &l);
                                g_widget_select(dev,x,y,x+t->xw-1,y+t->yw-1,bg_color,form_fg_color);
                                goto end_form_draw;
		        case FC_SUBMIT:
		        case FC_RESET:
                        case FC_BUTTON:
				l = 0;
                                g_print_text(drv, fd->ses->term->dev, x, y, form_style, t->text, &l);
                                g_widget_button(dev,x,y,x+t->xw-1,y+t->yw-1,bg_color,form_fg_color);
                                goto end_form_draw;
			case FC_TEXT:
			case FC_PASSWORD:
			case FC_FILE:
				/*
				if (fs->state >= fs->vpos + form->size) fs->vpos = fs->state - form->size + 1;
				if (fs->state < fs->vpos) fs->vpos = fs->state;
				*/
				while (fs->vpos < strlen(fs->value) && utf8_diff(fs->value + fs->state, fs->value + fs->vpos) >= form->size) {
					unsigned char *p = fs->value + fs->vpos;
					FWD_UTF_8(p);
					fs->vpos = p - fs->value;
				}
				while (fs->vpos > fs->state) {
					unsigned char *p = fs->value + fs->vpos;
					BACK_UTF_8(p, fs->value);
					fs->vpos = p - fs->value;
				}
				l = 0;
				i = 0;
				ll = strlen(fs->value);
				while (l < t->xw) {
					struct style *st = form_style;
					int sm = 0;
					unsigned char tx[11];
					if (fs->state == fs->vpos + i && t->link_num == fd->vs->current_link && fd->ses->locked_link) {
						st = form_inv_style;
						sm = 1;
					}
					tx[1] = 0;
					if (fs->vpos + i >= ll) tx[0] = ' ', tx[1] = 0, i++;
					else {
						unsigned char *p = fs->value + fs->vpos + i;
						unsigned char *pp = p;
						FWD_UTF_8(p);
						if (p - pp > 10) {
							i++;
							goto xy;
						}
						memcpy(tx, pp, p - pp);
						tx[p - pp] = 0;
						i += strlen(tx);
						if (form->type == FC_PASSWORD) xy:tx[0] = '*';
					}
					g_print_text(drv, fd->ses->term->dev, x + l, y, st, tx, &l);
                                }
                                g_widget_textarea(dev,x,y,x+t->xw-1,y+t->yw-1,bg_color,form_fg_color);
                                goto end_form;
			case FC_TEXTAREA:
				cur = _area_cursor(form, fs);
				if (!(lnx = format_text(fs->value, form->cols, form->wrap))) break;
				ln = lnx;
				yy = y - t->link_order * t->style->height;
				for (j = 0; j < fs->vypos; j++) if (ln->st) ln++;
				for (j = 0; j < form->rows; j++) {
					unsigned char *pp = ln->st;
					int xx = fs->vpos;
					while (pp < ln->en && xx > 0) {
						FWD_UTF_8(pp);
						xx--;
					}
					if (cur >= 0 && cur < form->cols && t->link_num == fd->vs->current_link && fd->ses->locked_link && fd->active) {
						unsigned char tx[11];
						int xx = x;

						if (print_all_textarea || j == t->link_order) while (xx < x + t->xw) {
							struct style *st = form_style;
							unsigned char *ppp = pp;
                                                        if (ln->st && pp < ln->en) {
								FWD_UTF_8(pp);
								memcpy(tx, ppp, pp - ppp);
								tx[pp - ppp] = 0;
							} else {
								tx[0] = ' ';
								tx[1] = 0;
							}
							if (!cur) {
								st = form_inv_style;
							}
							g_print_text(drv, fd->ses->term->dev, xx, yy + j * t->style->height, st, tx, &xx);
							cur--;
						} else cur -= form->cols;
					} else {
						if (print_all_textarea || j == t->link_order) {
							int aa;
							unsigned char *a;
							struct rect old;
                                                        if (ln->st && pp < ln->en) a = memacpy(pp, ln->en - pp);
                                                        else a = stracpy("");
							for (aa = 0; aa < form->cols; aa += 1) add_to_strn(&a, " ");
							restrict_clip_area(fd->ses->term->dev, &old, x, 0, x + t->xw, fd->ses->term->dev->size.y2);
							g_print_text(drv, fd->ses->term->dev, x, yy + j * t->style->height, form_style, a, NULL);
							drv->set_clip_area(fd->ses->term->dev, &old);
							mem_free(a);
						}
						cur -= form->cols;
					}
					if (ln->st) ln++;
                                }
				mem_free(lnx);
                                g_widget_textarea(dev,x,yy,x+t->xw-1,yy+form->rows*t->style->height-1,bg_color,form_fg_color);
                                goto end_form;
		}
        end_form_draw:
                if(link_selected)
                        g_draw_box(dev,x,y,x+t->xw-1,y+t->yw-1,bg_color,fg_color);
        end_form:
                mem_free(form_style);
                g_free_style(form_inv_style);
                g_free_style(inv_style);
                return;
        }

        /* Underline active link */
        if(link_selected){
				/* ysbox test
                t->style->flags = !t->style->flags;
                inv_style->flags = !inv_style->flags; */
				struct style *tempstyle = t->style;
				t->style = inv_style;
				inv_style = tempstyle;
        }

        if (!highlight_positions || !n_highlight_positions){
		prn:
                        if(!t->selected)
                                g_print_text(drv, fd->ses->term->dev, x, y, t->style, t->text, NULL);
                        else {
/* Mouse selection code starts here */
                                unsigned char *cur=t->text;
                                int xx=x;
                                unsigned char *ch;
                                int ll;

                                ch=init_str();ll=0;
                                while(cur<t->selection_start)
                                        add_chr_to_str(&ch,&ll,*(cur++));
                                g_print_text(drv, fd->ses->term->dev, xx, y, t->style, ch, &xx);
                                mem_free(ch);

                                ch=init_str();ll=0;
                                while(cur<t->selection_end)
                                        add_chr_to_str(&ch,&ll,*(cur++));
                                g_print_text(drv, fd->ses->term->dev, xx, y, inv_style, ch, &xx);
                                mem_free(ch);

                                ch=init_str();ll=0;
                                while(*cur)
                                        add_chr_to_str(&ch,&ll,*(cur++));
                                g_print_text(drv, fd->ses->term->dev, xx, y, t->style, ch, &xx);
                                mem_free(ch);
/* Mouse selection code ends here */
                        }
        }
        else {
                int tlen = strlen(t->text);
                int found;
		int start = t->srch_pos;
		int end = t->srch_pos + tlen;
		unsigned char *mask;
		unsigned char *tx;
		int txl;
		int pmask;
		int ii;
#define B_EQUAL(t, m) ((t) + highlight_position_size > start && (t) < end)
#define B_ABOVE(t, m) ((t) >= end)
		BIN_SEARCH(highlight_positions, n_highlight_positions, B_EQUAL, B_ABOVE, *, found);
		if (found == -1) goto prn;
		while (found > 0 && B_EQUAL(highlight_positions[found - 1], *)) found--;
		if (!(mask = mem_calloc(tlen))) goto prn;
		while (found < n_highlight_positions && !B_ABOVE(highlight_positions[found], *)) {
			int pos = highlight_positions[found] - t->srch_pos;
			int ii = 0;
			for (ii = 0; ii < highlight_position_size; ii++) {
				if (pos >= 0 && pos < tlen) mask[pos] = 1;
				pos++;
			}
			found++;
		}
		tx = init_str();;
		txl = 0;
		pmask = -1;
		for (ii = 0; ii < tlen; ii++) {
			if (mask[ii] != pmask) {
				g_print_text(drv, fd->ses->term->dev, x, y, pmask ? inv_style : t->style, tx, &x);
				mem_free(tx);
				tx = init_str();
				txl = 0;
                        }
			add_chr_to_str(&tx, &txl, t->text[ii]);
			pmask = mask[ii];
                }
		g_print_text(drv, fd->ses->term->dev, x, y, pmask ? inv_style : t->style, tx, &x);
		mem_free(tx);
		mem_free(mask);
        }

        /* Restore original underlining */
        if(link_selected){
				/* ysbox test
                t->style->flags = !t->style->flags;
                inv_style->flags = !inv_style->flags; */
  				struct style *tempstyle = t->style;
				t->style = inv_style;
				inv_style = tempstyle;
        }

        g_free_style(inv_style);
}


void g_text_destruct(struct g_object_text *t)
{
	release_image_map(t->map);
	g_free_style(t->style);
	mem_free(t);
}

void g_line_draw(struct f_data_c *fd, struct g_object_line *l, int xx, int yy)
{
	struct graphics_device *dev = fd->ses->term->dev;
	int i;
	int x = 0;
	for (i = 0; i < l->n_entries; i++) {
		struct g_object *o = (struct g_object *)l->entries[i];
		if (o->x > x) g_draw_background(dev, l->bg, xx + x, yy, o->x - x, l->yw);
		if (o->y > 0) g_draw_background(dev, l->bg, xx + o->x, yy, o->xw, o->y);
		if (o->y + o->yw < l->yw) g_draw_background(dev, l->bg, xx + o->x, yy + o->y + o->yw, o->xw, l->yw - o->y - o->yw);
		o->draw(fd, o, xx + o->x, yy + o->y);
		x = o->x + o->xw;
	}
	if (x < l->xw) g_draw_background(dev, l->bg, xx + x, yy, l->xw - x, l->yw);
}

void g_line_destruct(struct g_object_line *l)
{
	int i;
	for (i = 0; i < l->n_entries; i++) l->entries[i]->destruct(l->entries[i]);
	mem_free(l);
}

void g_line_get_list(struct g_object_line *l, void (*f)(struct g_object *parent, struct g_object *child))
{
	int i;
	for (i = 0; i < l->n_entries; i++) f((struct g_object *)l, l->entries[i]);
}

#define OBJ_EQ(a, b)	(*(a)).y <= (b) && (*(a)).y + (*(a)).yw > (b)
#define OBJ_ABOVE(a, b)	(*(a)).y > (b)

static inline struct g_object **g_find_line(struct g_object **a, int n, int p)
{
	int res = -1;
	BIN_SEARCH(a, n, OBJ_EQ, OBJ_ABOVE, p, res);
	if (res == -1) return NULL;
	return &a[res];
}

void g_area_draw(struct f_data_c *fd, struct g_object_area *a, int xx, int yy)
{
	struct g_object **i;
	int rx = root_x, ry = root_y;
	int y1 = fd->ses->term->dev->clip.y1 - yy;
	int y2 = fd->ses->term->dev->clip.y2 - yy - 1;
	struct g_object **l1;
	struct g_object **l2;
	if (fd->ses->term->dev->clip.y1 == fd->ses->term->dev->clip.y2 || fd->ses->term->dev->clip.x1 == fd->ses->term->dev->clip.x2) return;
	l1 = g_find_line((struct g_object **)&a->lines, a->n_lines, y1);
	l2 = g_find_line((struct g_object **)&a->lines, a->n_lines, y2);
	root_x = xx, root_y = yy;
	if (!l1) {
		if (y1 > a->yw) return;
		else l1 = (struct g_object **)&a->lines[0];
	}
	if (!l2) {
		if (y2 < 0) return;
		else l2 = (struct g_object **)&a->lines[a->n_lines - 1];
	}
	for (i = l1; i <= l2; i++) {
		struct g_object *o = *i;
		o->draw(fd, o, xx + o->x, yy + o->y);
	}
	/* !!! FIXME: floating objects */
	root_x = rx, root_y = ry;
}

void g_area_destruct(struct g_object_area *a)
{
	int i;
	g_release_background(a->bg);
	for (i = 0; i < a->n_lfo; i++) a->lfo[i]->destruct(a->lfo[i]);
	mem_free(a->lfo);
	for (i = 0; i < a->n_rfo; i++) a->rfo[i]->destruct(a->rfo[i]);
	mem_free(a->rfo);
	for (i = 0; i < a->n_lines; i++) a->lines[i]->destruct(a->lines[i]);
	mem_free(a);
}

void g_area_get_list(struct g_object_area *a, void (*f)(struct g_object *parent, struct g_object *child))
{
	int i;
	for (i = 0; i < a->n_lfo; i++) f((struct g_object *)a, a->lfo[i]);
	for (i = 0; i < a->n_rfo; i++) f((struct g_object *)a, a->rfo[i]);
	for (i = 0; i < a->n_lines; i++) f((struct g_object *)a, (struct g_object *)a->lines[i]);
}

/*
 * dsize - size of scrollbar
 * total - total data
 * vsize - visible data
 * vpos - position of visible data
 */

void get_scrollbar_pos(int dsize, int total, int vsize, int vpos, int *start, int *end)
{
	int ssize;
	if (!total) {
		*start = *end = 0;
		return;
	}
	ssize = dsize * vsize / total;
	if (ssize < G_SCROLL_BAR_MIN_SIZE) ssize = G_SCROLL_BAR_MIN_SIZE;
	if (total == vsize) {
		*start = 0; *end = dsize;
		return;
	}
	*start = (dsize - ssize) * vpos / (total - vsize);
	*end = *start + ssize;
	if (*start > dsize) *start = dsize;
	if (*start < 0) *start = 0;
	if (*end > dsize) *end = dsize;
	if (*end < 0) *end = 0;
	/*
	else {
		*start = vpos * dsize / total;
		*end = (vpos + vsize) * dsize / total;
	}
	if (*end > dsize) *end = dsize;
	*/
}

long scroll_bar_frame_color;
long scroll_bar_area_color;
long scroll_bar_bar_color;

void draw_vscroll_bar(struct graphics_device *dev, int x, int y, int yw, int total, int view, int pos)
{
	int spos, epos;
	drv->draw_hline(dev, x, y, x + G_SCROLL_BAR_WIDTH, scroll_bar_frame_color);
	drv->draw_vline(dev, x, y, y + yw, scroll_bar_frame_color);
	drv->draw_vline(dev, x + G_SCROLL_BAR_WIDTH - 1, y, y + yw, scroll_bar_frame_color);
	drv->draw_hline(dev, x, y + yw - 1, x + G_SCROLL_BAR_WIDTH, scroll_bar_frame_color);
	drv->draw_vline(dev, x + 1, y + 1, y + yw - 1, scroll_bar_area_color);
	drv->draw_vline(dev, x + G_SCROLL_BAR_WIDTH - 2, y + 1, y + yw - 1, scroll_bar_area_color);
	get_scrollbar_pos(yw - 4, total, view, pos, &spos, &epos);
	drv->fill_area(dev, x + 2, y + 1, x + G_SCROLL_BAR_WIDTH - 2, y + 2 + spos, scroll_bar_area_color);
	drv->fill_area(dev, x + 2, y + 2 + spos, x + G_SCROLL_BAR_WIDTH - 2, y + 2 + epos, scroll_bar_bar_color);
	drv->fill_area(dev, x + 2, y + 2 + epos, x + G_SCROLL_BAR_WIDTH - 2, y + yw - 1, scroll_bar_area_color);
}

void draw_hscroll_bar(struct graphics_device *dev, int x, int y, int xw, int total, int view, int pos)
{
	int spos, epos;
	drv->draw_vline(dev, x, y, y + G_SCROLL_BAR_WIDTH, scroll_bar_frame_color);
	drv->draw_hline(dev, x, y, x + xw, scroll_bar_frame_color);
	drv->draw_hline(dev, x, y + G_SCROLL_BAR_WIDTH - 1, x + xw, scroll_bar_frame_color);
	drv->draw_vline(dev, x + xw - 1, y, y + G_SCROLL_BAR_WIDTH, scroll_bar_frame_color);
	drv->draw_hline(dev, x + 1, y + 1, x + xw - 1, scroll_bar_area_color);
	drv->draw_hline(dev, x + 1, y + G_SCROLL_BAR_WIDTH - 2, x + xw - 1, scroll_bar_area_color);
	get_scrollbar_pos(xw - 4, total, view, pos, &spos, &epos);
	drv->fill_area(dev, x + 1, y + 2, x + 2 + spos, y + G_SCROLL_BAR_WIDTH - 2, scroll_bar_area_color);
	drv->fill_area(dev, x + 2 + spos, y + 2, x + 2 + epos, y + G_SCROLL_BAR_WIDTH - 2, scroll_bar_bar_color);
	drv->fill_area(dev, x + 2 + epos, y + 2, x + xw - 1, y + G_SCROLL_BAR_WIDTH - 2, scroll_bar_area_color);
}

void g_get_search(struct f_data *f, unsigned char *s)
{
	int i;
	if (!s || !*s) return;
	if (f->last_search && !strcmp(f->last_search, s)) return;
	if (f->search_positions) mem_free(f->search_positions), f->search_positions = DUMMY, f->n_search_positions = 0;
	if (f->last_search) mem_free(f->last_search);
	if (!(f->last_search = stracpy(s))) return;
	f->last_search_len = strlen(s);
	for (i = 0; i <= f->srch_string_size - f->last_search_len; i++) {
		int ii;
		if (srch_cmp(f->srch_string[i], s[0])) aiee__killing_interrupt_handler: continue;
		for (ii = 1; ii < f->last_search_len; ii++)
			if (srch_cmp(f->srch_string[i + ii], s[ii])) goto aiee__killing_interrupt_handler;
		if (!(f->n_search_positions & (ALLOC_GR - 1))) {
			if (!(f->search_positions = mem_realloc(f->search_positions, (f->n_search_positions + ALLOC_GR) * sizeof(int)))) return;
		}
		f->search_positions[f->n_search_positions++] = i;
	}
}

void draw_graphical_doc(struct terminal *t, struct f_data_c *scr, int active)
{
	int i;
	int r = 0;
	struct rect old;
	struct view_state *vs = scr->vs;
	struct rect_set *rs1, *rs2;
	int xw = scr->xw;
	int yw = scr->yw;
	int vx, vy;

        if (active) {
		if (scr->ses->search_word && scr->ses->search_word[0]) {
			g_get_search_data(scr->f_data);
			g_get_search(scr->f_data, scr->ses->search_word);
			highlight_positions = scr->f_data->search_positions;
			n_highlight_positions = scr->f_data->n_search_positions;
			highlight_position_size = scr->f_data->last_search_len;
		}
	}

	if (vs->view_pos > scr->f_data->y - scr->yw + scr->f_data->hsb * G_SCROLL_BAR_WIDTH) vs->view_pos = scr->f_data->y - scr->yw + scr->f_data->hsb * G_SCROLL_BAR_WIDTH;
	if (vs->view_pos < 0) vs->view_pos = 0;
	if (vs->view_posx > scr->f_data->x - scr->xw + scr->f_data->vsb * G_SCROLL_BAR_WIDTH) vs->view_posx = scr->f_data->x - scr->xw + scr->f_data->vsb * G_SCROLL_BAR_WIDTH;
	if (vs->view_posx < 0) vs->view_posx = 0;
	vx = vs->view_posx;
	vy = vs->view_pos;
	restrict_clip_area(t->dev, &old, scr->xp, scr->yp, scr->xp + xw, scr->yp + yw);
	if (scr->f_data->vsb) draw_vscroll_bar(t->dev, scr->xp + xw - G_SCROLL_BAR_WIDTH, scr->yp, yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH, scr->f_data->y, yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH, vs->view_pos);
	if (scr->f_data->hsb) draw_hscroll_bar(t->dev, scr->xp, scr->yp + yw - G_SCROLL_BAR_WIDTH, xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->f_data->x, xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, vs->view_posx);
	if (scr->f_data->vsb && scr->f_data->hsb) drv->fill_area(t->dev, scr->xp + xw - G_SCROLL_BAR_WIDTH, scr->yp + yw - G_SCROLL_BAR_WIDTH, scr->xp + xw, scr->yp + yw, scroll_bar_frame_color);
	restrict_clip_area(t->dev, NULL, scr->xp, scr->yp, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
	/*debug("buu: %d %d %d, %d %d %d", scr->xl, vx, xw, scr->yl, vy, yw);*/
	if (drv->flags & GD_DONT_USE_SCROLL) goto rrr;
	if (scr->xl == -1 || scr->yl == -1) goto rrr;
	if (scr->xl - vx > xw || vx - scr->xl > xw ||
	    scr->yl - vy > yw || vy - scr->yl > yw) {
		goto rrr;
	}
	rs1 = rs2 = NULL;
	if (scr->xl != vx)
		r |= drv->hscroll(t->dev, &rs1, scr->xl - vx);
	
	if (scr->yl != vy)
		r |= drv->vscroll(t->dev, &rs2, scr->yl - vy);
	
	if (rs1 || rs2) for (i = 0; i < 2; i++) {
		struct rect_set *rs = !i ? rs1 : rs2;
		if (rs) {
			int j;
			for (j = 0; j < rs->m; j++) {
				struct rect *r = &rs->r[j];
				struct rect clip1;
				/*fprintf(stderr, "scroll: %d,%d %d,%d\n", r->x1, r->y1, r->x2, r->y2);*/
				restrict_clip_area(t->dev, &clip1, r->x1, r->y1, r->x2, r->y2);
				scr->f_data->root->draw(scr, scr->f_data->root, scr->xp - vs->view_posx, scr->yp - vs->view_pos);
				drv->set_clip_area(t->dev, &clip1);
			}
			mem_free(rs);
		}
	}

	
	if (r) {
		struct rect clip1;
		if (scr->xl < vx)  {
			if (scr->yl < vy) {
				restrict_clip_area(t->dev, &clip1, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH - (vx - scr->xl), scr->yp, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH - (vy - scr->yl));
			} else {
				restrict_clip_area(t->dev, &clip1, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH - (vx - scr->xl), scr->yp + (scr->yl - vy), scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
			}
		} else {
			if (scr->yl < vy) {
				restrict_clip_area(t->dev, &clip1, scr->xp, scr->yp, scr->xp + (scr->xl - vx), scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH - (vy - scr->yl));
			} else {
				restrict_clip_area(t->dev, &clip1, scr->xp, scr->yp + (scr->yl - vy), scr->xp + (scr->xl - vx), scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
			}
		}
		scr->f_data->root->draw(scr, scr->f_data->root, scr->xp - vs->view_posx, scr->yp - vs->view_pos);
		drv->set_clip_area(t->dev, &clip1);
		if (scr->yl < vy) {
			restrict_clip_area(t->dev, NULL, scr->xp, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH - (vy - scr->yl), scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
		} else {
			restrict_clip_area(t->dev, NULL, scr->xp, scr->yp, scr->xp + xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + (scr->yl - vy));
		}
		scr->f_data->root->draw(scr, scr->f_data->root, scr->xp - vs->view_posx, scr->yp - vs->view_pos);
	}

	goto eee;
	rrr:
	scr->f_data->root->draw(scr, scr->f_data->root, scr->xp - vs->view_posx, scr->yp - vs->view_pos);
	eee:
	scr->xl = vx;
	scr->yl = vy;
	drv->set_clip_area(t->dev, &old);

	highlight_positions = NULL;
	n_highlight_positions = 0;
	highlight_position_size = 0;
}

int g_forward_mouse(struct f_data_c *fd, struct g_object *a, int x, int y, int b)
{
	if (x >= a->x && x < a->x + a->xw && y >= a->y && y < a->y + a->yw) {
		a->mouse_event(fd, a, x - a->x, y - a->y, b);
		return 1;
	}
	return 0;
}

struct draw_data {
	struct f_data_c *fd;
	struct g_object *o;
};

void draw_one_object_fn(struct terminal *t, struct draw_data *d)
{
	struct rect clip;
	struct f_data_c *scr = d->fd;
	struct g_object *o = d->o;
	int x, y;
	restrict_clip_area(t->dev, &clip, scr->xp, scr->yp, scr->xp + scr->xw - scr->f_data->vsb * G_SCROLL_BAR_WIDTH, scr->yp + scr->yw - scr->f_data->hsb * G_SCROLL_BAR_WIDTH);
	get_object_pos(o, &x, &y);
	o->draw(scr, o, scr->xp - scr->vs->view_posx + x, scr->yp - scr->vs->view_pos + y);
	drv->set_clip_area(t->dev, &clip);
}

void draw_one_object(struct f_data_c *fd, struct g_object *o)
{
	struct draw_data d;
	d.fd = fd;
	d.o = o;
        draw_to_window(fd->ses->win, (void (*)(struct terminal *, void *))draw_one_object_fn, &d);
}

void g_area_mouse(struct f_data_c *fd, struct g_object_area *a, int x, int y, int b)
{
	int i;
        int catched=0;

        for (i = 0; i < a->n_lines && !catched; i++)
                catched=g_forward_mouse(fd, (struct g_object *)a->lines[i], x, y, b);

        /* Seems to never happen ???
        if(!catched)
        	printf("Not catched in area\n");
        */
}

void g_line_mouse(struct f_data_c *fd, struct g_object_line *a, int x, int y, int b)
{
	int i;
        int catched=0;

        for (i = 0; i < a->n_entries && !catched; i++)
                catched=g_forward_mouse(fd, (struct g_object *)a->entries[i], x, y, b);

        if (!catched &&
            b == (B_UP | B_RIGHT))
                do_background_menu(fd->ses->term, NULL, fd->ses);
}

struct f_data *ffff;

void get_parents_sub(struct g_object *p, struct g_object *c)
{
	c->parent = p;
	if (c->get_list) c->get_list(c, get_parents_sub);
	if (c->destruct == g_tag_destruct) {
		int x = 0, y = 0;
		struct g_object *o;
		c->y -= c->parent->yw;
		for (o = c; o; o = o->parent) x += o->x, y += o->y;
		html_tag(ffff, ((struct g_object_tag *)c)->name, x, y);
        }
	if (c->type == G_OBJECT_TEXT || c->type == G_OBJECT_IMAGE) {
		int l = ((struct g_object_text *)c)->link_num;
		if (l >= 0) {
			struct link *link = &ffff->links[l];
			int x = 0, y = 0;
			struct g_object *o;
			for (o = c; o; o = o->parent) x += o->x, y += o->y;
			if (x < link->r.x1) link->r.x1 = x;
			if (y < link->r.y1) link->r.y1 = y;
			if (x + c->xw > link->r.x2) link->r.x2 = x + c->xw;
			if (y + c->yw > link->r.y2) link->r.y2 = y + c->yw;
			link->obj = c;
		}
	}
}

void get_parents(struct f_data *f, struct g_object *a)
{
	ffff = f;
	a->parent = NULL;
	if (a->get_list) a->get_list(a, get_parents_sub);
}

void get_object_pos(struct g_object *o, int *x, int *y)
{
	*x = *y = 0;
	while (o) {
		*x += o->x;
		*y += o->y;
		o = o->parent;
	}
}

/* if set_position is 1 sets cursor position in FIELD/AREA elements */
void g_set_current_link(struct f_data_c *fd, struct g_object_text *a, int x, int y, int set_position)
{
        if (a->map) {
		int i;
		for (i = 0; i < a->map->n_areas; i++) {
			if (is_in_area(&a->map->area[i], x, y) && a->map->area[i].link_num >= 0) {
				fd->vs->current_link = a->map->area[i].link_num;
				return;
			}
		}
	}
	fd->vs->current_link = -1;
	if (a->link_num >= 0) {
		fd->vs->current_link = a->link_num;
		/* if links is a field, set cursor position */
                if (set_position&&a->link_num>=0&&a->link_num<fd->f_data->nlinks) /* valid link */
		{
			struct link *l=&fd->f_data->links[a->link_num];
			struct form_state *fs;
			int xx,yy;
			
			if (!l->form)return;
			if (l->type==L_AREA)
			{
				struct line_info *ln;
				if (!(fs=find_form_state(fd,l->form)))return;

				if (g_char_width(a->style,' ')) {
					xx=x/g_char_width(a->style,' ');
				} else xx=x;
				xx+=fs->vpos;
				xx=xx<0?0:xx;
				yy=a->link_order;
				yy+=fs->vypos;
				if ((ln = format_text(fs->value, l->form->cols, l->form->wrap))) {
					int a;
					for (a = 0; ln[a].st; a++) if (a==yy){
						int bla=utf8_diff(ln[a].en,ln[a].st);
						
						fs->state=ln[a].st-fs->value;
						fs->state = utf8_add(fs->value + fs->state, xx<bla?xx:bla) - fs->value;
						break;
					}
					mem_free(ln);
				}
				return;
			}
			if (l->type!=L_FIELD||!(fs=find_form_state(fd,l->form)))return;
			if (g_char_width(a->style,' ')) {
				xx=x/g_char_width(a->style,' ');
			} else xx=x;
			fs->state=utf8_add(fs->value + (fs->vpos > strlen(fs->value) ? strlen(fs->value) : fs->vpos), (xx<0?0:xx)) - fs->value;
                }
        }
}

void g_text_mouse(struct f_data_c *fd, struct g_object_text *a, int x, int y, int b)
{
	int e;
	g_set_current_link(fd, a, x, y, (b == (B_UP | B_LEFT)));

#ifdef JS
	if (fd->vs&&fd->f_data&&fd->vs->current_link>=0&&fd->vs->current_link<fd->f_data->nlinks)
	{
		/* fd->vs->current links is a valid link */

		struct link *l=&(fd->f_data->links[fd->vs->current_link]);

		if (l->js_event&&l->js_event->up_code&&(b&BM_ACT)==B_UP)
			jsint_execute_code(fd,l->js_event->up_code,strlen(l->js_event->up_code),-1,-1,-1);

		if (l->js_event&&l->js_event->down_code&&(b&BM_ACT)==B_DOWN)
			jsint_execute_code(fd,l->js_event->down_code,strlen(l->js_event->down_code),-1,-1,-1);
		
	}
#endif
        if (b == (B_UP | B_LEFT) || b == (B_UP | B_MIDDLE)) {
                /* New tab requested ? */
                int new=(b == (B_UP | B_MIDDLE));

                if (new) /* Speed up --karpov */
                        new = options_get_bool("tabs_new_on_middle_button");

                ismap_x = x;
		ismap_y = y;
		ismap_link = a->ismap;
		e = enter(fd->ses, fd, 1, new);
		ismap_x = 0;
		ismap_y = 0;
		ismap_link = 0;
		if (e) {
			print_all_textarea = 1;
			draw_one_object(fd, (struct g_object *)a);
			print_all_textarea = 0;
		}
		if (e == 2) fd->f_data->locked_on = (struct g_object *)a;
		return;
        }

        if (b == (B_UP | B_RIGHT)){
                if (fd->vs->current_link != -1)
                        link_menu(fd->ses->term, NULL, fd->ses);
                else
                        do_background_menu(fd->ses->term, NULL, fd->ses);
        }
}

void process_sb_event(struct f_data_c *fd, int off, int h)
{
	int spos, epos;
	int w = h ? fd->f_data->hsbsize : fd->f_data->vsbsize;
	get_scrollbar_pos(w - 4, h ? fd->f_data->x : fd->f_data->y, w, h ? fd->vs->view_posx : fd->vs->view_pos, &spos, &epos);
	spos += 2;
	epos += 2;
	/*debug("%d %d %d", spos, epos, off);*/
	if (off >= spos && off < epos) {
		fd->ses->scrolling = 1;
		fd->ses->scrolltype = h;
		fd->ses->scrolloff = off - spos - 1;
		return;
	}
	if (off < spos) if (h) fd->vs->view_posx -= fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH;
			else fd->vs->view_pos -= fd->yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
	else if (h) fd->vs->view_posx += fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH;
		else fd->vs->view_pos += fd->yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
	draw_graphical_doc(fd->ses->term, fd, 1);
}

void process_sb_move(struct f_data_c *fd, int off)
{
	int h = fd->ses->scrolltype;
	int w = h ? fd->f_data->hsbsize : fd->f_data->vsbsize;
	int rpos = off - 2 - fd->ses->scrolloff;
	int st, en;
	get_scrollbar_pos(w - 4, h ? fd->f_data->x : fd->f_data->y, w, h ? fd->vs->view_posx : fd->vs->view_pos, &st, &en);
	if (en - st >= w - 4) return;
	/*
	*(h ? &fd->vs->view_posx : &fd->vs->view_pos) = rpos * (h ? fd->f_data->x : fd->f_data->y) / (w - 4);
	*/
	if (!(w - 4 - (en - st))) return;
	*(h ? &fd->vs->view_posx : &fd->vs->view_pos) = rpos * (h ? fd->f_data->x - w : fd->f_data->y - w) / (w - 4 - (en - st));
	draw_graphical_doc(fd->ses->term, fd, 1);
}

inline int ev_in_rect(struct event *ev, int x1, int y1, int x2, int y2)
{
	return ev->x >= x1 && ev->y >= y1 && ev->x < x2 && ev->y < y2;
}


int is_link_in_view(struct f_data_c *fd, int nl)
{
	struct link *l = &fd->f_data->links[nl];
	int res=fd->vs->view_pos < l->r.y2 && fd->vs->view_pos + fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH > l->r.y1;

        return res;
}

int skip_link(struct f_data_c *fd, int nl)
{
	struct link *l = &fd->f_data->links[nl];
        return !l->where && !l->form;
}

void redraw_link(struct f_data_c *fd, int nl)
{
	struct link *l = &fd->f_data->links[nl];
	struct rect r;
 /*BOOK0000011x000bb*/        memcpy(&r, &l->r, sizeof(struct rect));
	r.x1 += fd->f_data->opt.xp - fd->vs->view_posx;
	r.x2 += fd->f_data->opt.xp - fd->vs->view_posx;
	r.y1 += fd->f_data->opt.yp - fd->vs->view_pos;
	r.y2 += fd->f_data->opt.yp - fd->vs->view_pos;
	t_redraw(fd->ses->term->dev, &r);
}

int lr_link(struct f_data_c *fd, int nl)
{
	struct link *l = &fd->f_data->links[nl];
	int xx = fd->vs->view_posx;
	if (l->r.x2 > fd->vs->view_posx + fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH) fd->vs->view_posx = l->r.x2 - (fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH);
	if (l->r.x1 < fd->vs->view_posx) fd->vs->view_posx = l->r.x1;
	return xx != fd->vs->view_posx;
}

int ud_link(struct f_data_c *fd, int nl)
{
	struct link *l = &fd->f_data->links[nl];
	int yy = fd->vs->view_pos;
    if (l->r.y2 > fd->vs->view_pos + fd->yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH) fd->vs->view_pos = l->r.y2 - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
    if (l->r.y1 < fd->vs->view_pos) fd->vs->view_pos = l->r.y1;
	return yy != fd->vs->view_pos;
}

int g_next_link(struct f_data_c *fd, int dir)
{
	int r = 2;
	int n, pn;
	if (fd->vs->current_link >= 0 && fd->vs->current_link < fd->f_data->nlinks) {
		n = (pn = fd->vs->current_link) + dir;
	} else retry: n = dir > 0 ? 0 : fd->f_data->nlinks - 1, pn = -1;
	again:
	if (n < 0 || n >= fd->f_data->nlinks) {
		if (r == 1) {
			fd->vs->current_link = -1;
			fd->ses->locked_link = 0;
			return 1;
		}
		if (dir < 0) {
			if (!fd->vs->view_pos) return 0;
			fd->vs->view_pos -= fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
			/*if (fd->vs->view_pos < 0) fd->vs->view_pos = 0;*/
		} else {
			if (fd->vs->view_pos >= fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
			fd->vs->view_pos += fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
			/*if (fd->vs->view_pos > fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) fd->vs->view_pos = fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
			if (fd->vs->view_pos < 0) fd->vs->view_pos = 0;*/
		}
		r = 1;
		goto retry;
	}
	if (!is_link_in_view(fd, n) || skip_link(fd, n)) {
		n += dir;
		goto again;
	}
	if (fd->vs->current_link >= 0 && fd->vs->current_link < fd->f_data->nlinks) {
		redraw_link(fd, fd->vs->current_link);
	}
	fd->vs->current_link = n;
	redraw_link(fd, n);
	fd->ses->locked_link = 0;
	if (fd->f_data->links[fd->vs->current_link].type == L_FIELD || fd->f_data->links[fd->vs->current_link].type == L_AREA) {
		if ((fd->f_data->locked_on = fd->f_data->links[fd->vs->current_link].obj)) fd->ses->locked_link = 1;
	}
	set_textarea(fd->ses, fd, dir < 0 ? KBD_DOWN : KBD_UP);
	change_screen_status(fd->ses);
	print_screen_status(fd->ses);
	if (lr_link(fd, fd->vs->current_link)) r = 1;
	return r;
}

#define KBDNAV_UP		0
#define KBDNAV_DOWN		1
#define KBDNAV_LEFT		2
#define KBDNAV_RIGHT	3

#if 0
static double dist_left_right(int y1, int y2, int x1, int x2)
{
	return (double)((x1-y1)+(x2-y2))/2;
}

static double dist_up_down(int a1, int a2, int b1, int b2)
{
	if (b1 <= a1 && b2 >= a2) // wholly contained
		return 0;
	if (b1 >= a1 && b1 <= a1) // wholly contained
		return 0;
	return (double)abs(a1 - b1); // difference between left items
}
#endif

static double dist_left_right(struct link l1, struct link l2)
{
	if (l1.r.x1 == l2.r.x1)	  // probably on same column
		return 0;
	return ((double)(l2.r.x2 - l2.r.x1)/2 - (double)(l1.r.x2 - l2.r.x1)/2);
}

static double dist_up_down(struct link l1, struct link l2)
{
/*	if (l2.r.x1 <= l1.r.x1 && l2.r.x2 >= l1.r.x2)
		return 0;
	if (l1.r.x1 <= l2.r.x1 && l1.r.x2 >= l2.r.x2)
		return 0; */
	if (l1.r.y1 == l2.r.y1)	  // probably on same line
		return 0;
	return ((double)(l2.r.y2 - l2.r.y1)/2 - (double)(l1.r.y2 - l2.r.y1)/2);
}

int g_next_link_kbdnav(struct f_data_c *fd, int dir)
{
	int startlink, endlink, steplink, targetlink;
	int r = 2;
	int i;
	double minX, minY;

	printf("\n\nEntering g_next_link_kbdnav with dir=%d ! --------------\n\n", dir);

	if (fd->f_data->nlinks < 1)
		return r;

	if (!is_link_in_view(fd, fd->vs->current_link))
		fd->vs->current_link = -1;

kbdnav_retry:
	targetlink = -1;
	minX = INT_MAX;
	minY = INT_MAX;

	switch (dir) {
		case KBDNAV_UP:
		case KBDNAV_LEFT:
			startlink = (fd->vs->current_link == -1) ? fd->f_data->nlinks - 1 : fd->vs->current_link - 1;
			endlink = -1;
			steplink = -1;
			break;
		case KBDNAV_DOWN:
		case KBDNAV_RIGHT:
			startlink = (fd->vs->current_link == -1) ? 0 : fd->vs->current_link + 1;
			endlink = fd->f_data->nlinks;
			steplink = 1;
			break;
	}

	printf("startlink: %d, endlink: %d, steplink: %d\n", startlink, endlink, steplink);

	for (i = startlink; i != endlink; i += steplink) {
		double x, y;

		if (i == fd->vs->current_link)
			continue;

		if (skip_link(fd, i)) {
			continue;
		}

		if (fd->vs->current_link == -1 && !is_link_in_view(fd, i)) {
			continue;
		}
		
		if (r == 2 && (fd->vs->current_link == -1 || !is_link_in_view(fd, fd->vs->current_link))) {
			targetlink = i;
			break;
		}
		x = dist_left_right(fd->f_data->links[fd->vs->current_link], fd->f_data->links[i]);
		y = dist_up_down(fd->f_data->links[fd->vs->current_link], fd->f_data->links[i]);
		switch (dir) {
			case KBDNAV_UP:
				//x = dist_up_down(fd->f_data->links[fd->vs->current_link].r.x1, fd->f_data->links[fd->vs->current_link].r.x2, fd->f_data->links[i].r.x1, fd->f_data->links[i].r.x2)/4;
				//y = fd->f_data->links[i].r.y1 - fd->f_data->links[fd->vs->current_link].r.y1;
				if (!( fd->f_data->links[i].r.y1 < fd->f_data->links[fd->vs->current_link].r.y1 && (x*x + 3*y*y < minX + 3*minY) ))
					continue;
				break;
			case KBDNAV_DOWN:
				//x = dist_up_down(fd->f_data->links[fd->vs->current_link].r.x1, fd->f_data->links[fd->vs->current_link].r.x2, fd->f_data->links[i].r.x1, fd->f_data->links[i].r.x2)/4;
				//y = fd->f_data->links[i].r.y1 - fd->f_data->links[fd->vs->current_link].r.y1;
				if (!( fd->f_data->links[i].r.y1 > fd->f_data->links[fd->vs->current_link].r.y1 && (x*x + 3*y*y < minX + 3*minY) ))
					continue;
				break;
			case KBDNAV_LEFT:
				//x = fd->f_data->links[fd->vs->current_link].r.x1 - fd->f_data->links[i].r.x1;
				//y = dist_left_right(fd->f_data->links[fd->vs->current_link].r.y1, fd->f_data->links[fd->vs->current_link].r.y2, fd->f_data->links[i].r.y1, fd->f_data->links[i].r.y2)*4;
		        if (!( fd->f_data->links[i].r.x1 < fd->f_data->links[fd->vs->current_link].r.x1 && (y*y < minY || y*y == minY && x*x < minX) ))
					continue;
				break;
			case KBDNAV_RIGHT:
				//x = fd->f_data->links[fd->vs->current_link].r.x1 - fd->f_data->links[i].r.x1;
				//y = dist_left_right(fd->f_data->links[fd->vs->current_link].r.y1, fd->f_data->links[fd->vs->current_link].r.y2, fd->f_data->links[i].r.y1, fd->f_data->links[i].r.y2)*4;
		        if (!( fd->f_data->links[i].r.x2 > fd->f_data->links[fd->vs->current_link].r.x2 && (y*y < minY || y*y == minY && x*x < minX) ))
					continue;
				break;
		}

        minY = y*y;
        minX = x*x;
        targetlink = i;
	}

	printf("targetlink=%d, minX=%f, minY=%f\n", targetlink, minX, minY);

	if (targetlink == -1 && r == 2) {
		/* no link found */
		switch (dir) {
			case KBDNAV_UP:
				if (!fd->vs->view_pos) return 0;
				fd->vs->view_pos -= fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
				break;
			case KBDNAV_DOWN:
				if (fd->vs->view_pos >= fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_pos += fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
				break;
			case KBDNAV_LEFT:
				if (!fd->vs->view_posx) return 0;
				fd->vs->view_posx -= fd->f_data->opt.xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH;
				break;
			case KBDNAV_RIGHT:
				if (fd->vs->view_posx >= fd->f_data->x - fd->xw + fd->f_data->vsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_posx += fd->f_data->opt.xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH;
				break;
		}

		r = 1;
		goto kbdnav_retry;
	}

	if (fd->vs->current_link >= 0 && fd->vs->current_link < fd->f_data->nlinks) {
		redraw_link(fd, fd->vs->current_link);
	}
	fd->vs->current_link = targetlink;
	redraw_link(fd, targetlink);
	fd->ses->locked_link = 0;
	if (fd->f_data->links[fd->vs->current_link].type == L_FIELD || fd->f_data->links[fd->vs->current_link].type == L_AREA) {
		if ((fd->f_data->locked_on = fd->f_data->links[fd->vs->current_link].obj)) fd->ses->locked_link = 1;
	}
	switch (dir) {
		case KBDNAV_UP:
			set_textarea(fd->ses, fd, KBD_UP);
			break;
		case KBDNAV_LEFT:
			set_textarea(fd->ses, fd, KBD_DOWN);
			break;
		case KBDNAV_DOWN:
			set_textarea(fd->ses, fd, KBD_LEFT);
			break;
		case KBDNAV_RIGHT:
			set_textarea(fd->ses, fd, KBD_RIGHT);
			break;
	}
	change_screen_status(fd->ses);
	print_screen_status(fd->ses);
	if (lr_link(fd, fd->vs->current_link)) r = 1;
	if (ud_link(fd, fd->vs->current_link)) r = 1;
	return r;
}

int g_frame_ev(struct session *ses, struct f_data_c *fd, struct event *ev)
{
	int x, y;
	if (!fd->f_data) return 0;
	switch (ev->ev) {
		case EV_MOUSE:
                        /* Pass middle button press to textarea - we need it for pasting */
                        if ((ev->b & BM_ACT) == B_DOWN && (ev->b & BM_BUTT) == B_MIDDLE){

                                if (ses->locked_link &&
                                    fd->vs->current_link >= 0 &&
                                    (fd->f_data->links[fd->vs->current_link].type == L_FIELD ||
                                     fd->f_data->links[fd->vs->current_link].type == L_AREA)) {

                                        if (field_op(ses, fd, &fd->f_data->links[fd->vs->current_link], ev, 0)) {

                                                if (fd->f_data->locked_on) {
                                                        print_all_textarea = 1;
                                                        draw_one_object(fd, fd->f_data->locked_on);
                                                        print_all_textarea = 0;
                                                        return 2;
                                                }
                                                return 1;
                                        }
                                } else if(fd->vs->current_link >=0 &&
                                          fd->f_data->links[fd->vs->current_link].where &&
                                          options_get_bool("tabs_new_on_middle_button"))
                                        goto enter;
                        }
                        if ((ev->b & BM_BUTT) == B_WHEELUP) goto up;
			if ((ev->b & BM_BUTT) == B_WHEELDOWN) goto down;
			if ((ev->b & BM_BUTT) == B_WHEELUP1) goto up1;
			if ((ev->b & BM_BUTT) == B_WHEELDOWN1) goto down1;
			if ((ev->b & BM_BUTT) == B_WHEELLEFT) goto left;
			if ((ev->b & BM_BUTT) == B_WHEELRIGHT) goto right;
			if ((ev->b & BM_BUTT) == B_WHEELLEFT1) goto left1;
			if ((ev->b & BM_BUTT) == B_WHEELRIGHT1) goto right1;
			x = ev->x;
			y = ev->y;
			if ((ev->b & BM_ACT) == B_MOVE) ses->scrolling = 0;
			if (ses->scrolling == 1) process_sb_move(fd, ses->scrolltype ? ev->x : ev->y);
			if (ses->scrolling == 2) {
				fd->vs->view_posx = -ev->x + ses->scrolltype;
				fd->vs->view_pos = -ev->y + ses->scrolloff;
                                //draw_fd(fd);
                                draw_graphical_doc(fd->ses->term, fd, 1);
				break;
			}
			if (ses->scrolling) {
				if ((ev->b & BM_ACT) == B_UP) {
					ses->scrolling = 0;
				}
				break;
			}
			if ((ev->b & BM_ACT) == B_DOWN && (ev->b & BM_BUTT) == B_MIDDLE) {
				scrll:
				ses->scrolltype = ev->x + fd->vs->view_posx;
				ses->scrolloff = ev->y + fd->vs->view_pos;
				ses->scrolling = 2;
				break;
			}
			if ((ev->b & BM_ACT) == B_DOWN && fd->f_data->vsb && ev_in_rect(ev, fd->xw - G_SCROLL_BAR_WIDTH, 0, fd->xw, fd->yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH)) {
				process_sb_event(fd, ev->y, 0);
				break;
			}
			if ((ev->b & BM_ACT) == B_DOWN && fd->f_data->hsb && ev_in_rect(ev, 0, fd->yw - G_SCROLL_BAR_WIDTH, fd->xw - fd->f_data->vsb * G_SCROLL_BAR_WIDTH, fd->yw)) {
				process_sb_event(fd, ev->x, 1);
				break;
			}
			if (fd->f_data->vsb && ev_in_rect(ev, fd->xw - G_SCROLL_BAR_WIDTH, 0, fd->xw, fd->yw)) return 0;
			if (fd->f_data->hsb && ev_in_rect(ev, 0, fd->yw - G_SCROLL_BAR_WIDTH, fd->xw, fd->yw)) return 0;

/* Mouse selection code starts here */
                        if (fd->selection){ /* Selection structure exist - selection is activated */
                                mouse_selection(fd,ev->x+fd->vs->view_posx,ev->y+fd->vs->view_pos,1);
                                if (!((ev->b & BM_ACT) == B_DRAG && (ev->b & BM_BUTT) == B_LEFT))
                                        mouse_selection(fd,ev->x+fd->vs->view_posx,ev->y+fd->vs->view_pos,2);
                                break;
                        } else  /* Start selection on left mouse drag */
                                if ((ev->b & BM_ACT) == B_DRAG && (ev->b & BM_BUTT) == B_LEFT) {
                                        mouse_selection(fd,ev->x+fd->vs->view_posx,ev->y+fd->vs->view_pos,0);
			}
/* Mouse selection code ends here */

                enter:
                        previous_link=fd->vs->current_link;

                        fd->vs->current_link = -1;
 /*BOOK0000010x000bb*/			fd->f_data->root->mouse_event(fd, fd->f_data->root, ev->x + fd->vs->view_posx, ev->y + fd->vs->view_pos, ev->b);

                        if (previous_link != fd->vs->current_link){
                                if(previous_link >= 0 &&
                                   previous_link < fd->f_data->nlinks)
                                        redraw_link(fd, previous_link);
                                if(fd->vs->current_link >= 0 &&
                                        fd->vs->current_link < fd->f_data->nlinks)
                                        redraw_link(fd, fd->vs->current_link);
                        }

                        if (previous_link!=fd->vs->current_link)
                                change_screen_status(ses);
			print_screen_status(ses);

#ifdef JS
			/* process onmouseover/onmouseout handlers */

			if (previous_link!=fd->vs->current_link)
			{
				struct link* lnk=NULL;

			if (previous_link>=0&&previous_link<fd->f_data->nlinks)lnk=&(fd->f_data->links[previous_link]);
				if (lnk&&lnk->js_event&&lnk->js_event->out_code)
					jsint_execute_code(fd,lnk->js_event->out_code,strlen(lnk->js_event->out_code),-1,-1,-1);
				lnk=NULL;
				if (fd->vs->current_link>=0&&fd->vs->current_link<fd->f_data->nlinks)lnk=&(fd->f_data->links[fd->vs->current_link]);
				if (lnk&&lnk->js_event&&lnk->js_event->over_code)
					jsint_execute_code(fd,lnk->js_event->over_code,strlen(lnk->js_event->over_code),-1,-1,-1);
			}

                        /* ???
                         if ((ev->b & BM_ACT) == B_DOWN && (ev->b & BM_BUTT) == B_RIGHT && fd->vs->current_link == -1) goto scrll;
                         */
#endif
			break;
		case EV_KBD:
			if (ses->locked_link && fd->vs->current_link >= 0 && (fd->f_data->links[fd->vs->current_link].type == L_FIELD || fd->f_data->links[fd->vs->current_link].type == L_AREA)) {
				if (field_op(ses, fd, &fd->f_data->links[fd->vs->current_link], ev, 0)) {
					if (fd->f_data->locked_on) {
						print_all_textarea = 1;
						draw_one_object(fd, fd->f_data->locked_on);
						print_all_textarea = 0;
						return 2;
					}
					return 1;
				}
				if (ev->x == KBD_ENTER) return enter(ses, fd, 0, 0);
			}

                        if (((options_get_bool("keyboard_navigation") || ev->y == KBD_CTRL) && ev->x == KBD_RIGHT && ev->y != KBD_ALT) || ev->x == KBD_ENTER) {
				struct link *l;
                                /* New tab requested */
                                int new=(ev->y == KBD_CTRL && ev->x == KBD_ENTER && options_get_bool("tabs_new_on_ctrl_enter"));

                                if (fd->vs->current_link >= 0 && fd->vs->current_link < fd->f_data->nlinks) {
					l = &fd->f_data->links[fd->vs->current_link];
					set_window_ptr(ses->win, fd->f_data->opt.xp + l->r.x1 - fd->vs->view_posx, fd->f_data->opt.yp + l->r.y1 - fd->vs->view_pos);
                                } else {
					set_window_ptr(ses->win, fd->f_data->opt.xp, fd->f_data->opt.yp);
				}
				return enter(ses, fd, 0, new);
 			}


                        if (ev->x == KBD_PAGE_DOWN || (ev->x == ' ' && (!ev->y || ev->y== KBD_CTRL)) || (upcase(ev->x) == 'F' && ev->y & KBD_CTRL)) {
				if (fd->vs->view_pos == fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_pos += fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
				fd->vs->current_link = -1;
				return 3;
			}
			if (ev->x == KBD_PAGE_UP || (upcase(ev->x) == 'B' && (!ev->y || ev->y == KBD_CTRL)) || (upcase(ev->x) == 'B' && ev->y & KBD_CTRL)) {
				if (!fd->vs->view_pos) return 0;
				fd->vs->view_pos -= fd->f_data->opt.yw - fd->f_data->hsb * G_SCROLL_BAR_WIDTH;
				fd->vs->current_link = -1;
				return 3;
			}
			if (0) {
				down:
				if (fd->vs->view_pos == fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_pos += 64;
				return 3;
			}
			if (0) {
				up:
				if (!fd->vs->view_pos) return 0;
				fd->vs->view_pos -= 64;
				return 3;
			}
			if (ev->x == KBD_DEL || (upcase(ev->x) == 'N' && ev->y == KBD_CTRL)) {
				if (fd->vs->view_pos == fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_pos += 32;
				return 3;
			}
			if (ev->x == KBD_INS || (upcase(ev->x) == 'P' && ev->y == KBD_CTRL)) {
				if (!fd->vs->view_pos) return 0;
				fd->vs->view_pos -= 32;
				return 3;
			}
			if ((ev->x == KBD_DOWN) && !(options_get_bool("keyboard_navigation") || ev->y == KBD_CTRL)) {
				/* ysbox: new kbdnav */
				return g_next_link_kbdnav(fd, KBDNAV_DOWN);
				down1:
				if (fd->vs->view_pos == fd->f_data->y - fd->yw + fd->f_data->hsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_pos += 8;
				return 3;
			}
			if ((ev->x == KBD_UP) && !(options_get_bool("keyboard_navigation") || ev->y == KBD_CTRL)) {
				/* ysbox: new kbdnav */
				return g_next_link_kbdnav(fd, KBDNAV_UP);
				up1:
				if (!fd->vs->view_pos) return 0;
				fd->vs->view_pos -= 8;
				return 3;
			}
			if (ev->x == KBD_DOWN) {
				return g_next_link(fd, 1);
			}
			if (ev->x == KBD_UP) {
				return g_next_link(fd, -1);
			}
			if (ev->x == '[') {
				left:
				if (!fd->vs->view_posx) return 0;
				fd->vs->view_posx -= 64;
				return 3;
			}
			if (ev->x == ']') {
				right:
				if (fd->vs->view_posx == fd->f_data->x - fd->xw + fd->f_data->vsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_posx += 64;
				return 3;
			}
			if (ev->x == KBD_LEFT && !options_get_bool("keyboard_navigation") && !ev->y) {
				/* ysbox: new kbdnav */
				return g_next_link_kbdnav(fd, KBDNAV_LEFT);
				left1:
				if (!fd->vs->view_posx) return 0;
				fd->vs->view_posx -= 16;
				return 3;
			}
			if (ev->x == KBD_RIGHT && !options_get_bool("keyboard_navigation") && !ev->y) {
				/* ysbox: new kbdnav */
				return g_next_link_kbdnav(fd, KBDNAV_RIGHT);
				right1:
				if (fd->vs->view_posx == fd->f_data->x - fd->xw + fd->f_data->vsb * G_SCROLL_BAR_WIDTH) return 0;
				fd->vs->view_posx += 16;
				return 3;
			}
			if (ev->x == KBD_HOME || (upcase(ev->x) == 'A' && ev->y & KBD_CTRL)) {
				fd->vs->view_pos = 0;
				fd->vs->current_link = -1;
				return 3;
			}
			if (ev->x == KBD_END || (upcase(ev->x) == 'E' && ev->y & KBD_CTRL)) {
				fd->vs->view_pos = fd->f_data->y;
				fd->vs->current_link = -1;
				return 3;
			}
			if (upcase(ev->x) == 'F' && !(ev->y & KBD_ALT)) {
				set_frame(ses, fd, 0);
				return 2;
			}
			if (upcase(ev->x) == 'D' && !(ev->y & KBD_ALT)) {
				if (!anonymous) frm_download(ses, fd);
				return 2;
			}
			if (ev->x == '/') {
				search_dlg(ses, fd, 0);
				return 2;
			}
			if (ev->x == '?') {
				search_back_dlg(ses, fd, 0);
				return 2;
			}
			if (ev->x == 'n' && !(ev->y & KBD_ALT)) {
				find_next(ses, fd, 0);
				return 2;
			}
			if (ev->x == 'N' && !(ev->y & KBD_ALT)) {
				find_next_back(ses, fd, 0);
				return 2;
                        }
                        if (upcase(ev->x) == 'K' && !(ev->y & KBD_ALT)) {
                                options_set_bool("keyboard_navigation",!options_get_bool("keyboard_navigation"));
                                print_screen_status(ses);
                                return 2;
                        }
                        break;
	}
	return 0;
}

void draw_title(struct f_data_c *f)
{
	unsigned char *title = stracpy(!drv->set_title ? f->f_data && f->f_data->title ? f->f_data->title : (unsigned char *)"" : f->rq && f->rq->url ? f->rq->url : (unsigned char *)"");
	int b, z, w, x;
	struct graphics_device *dev = f->ses->term->dev;
	if(options_get_bool("hide_menus")) {
		mem_free(title);
		return;
	}
	if (drv->set_title && strchr(title, POST_CHAR)) *strchr(title, POST_CHAR) = 0;
	w = g_text_width(bfu_style_bw_b, title);
        x=0;

        /* Menubar drawing

         Don't forget to add/delete corresponding actions in view.c / send_event func / ;-)) */

        /* Back */
        z = 0;
        if(options_get_int("toolbar_button_visibility_back"))
                g_print_text(drv, dev, x, 0, bfu_style_bw_system, BUTTON_BACK, &z);
        f->ses->back_size = z;
        x+=z;

        /* History */
        z = 0;
        if(options_get_int("toolbar_button_visibility_history"))
                g_print_text(drv, dev, x, 0, bfu_style_bw_system, BUTTON_HISTORY, &z);
        f->ses->history_size = z;
        x+=z;

        /* Forward */
        z = 0;
        if(options_get_int("toolbar_button_visibility_forward"))
                g_print_text(drv, dev, x, 0, bfu_style_bw_system, BUTTON_FORWARD, &z);
	f->ses->forward_size = z;
        x+=z;

        /* Reload */
        z = 0;
        if(options_get_int("toolbar_button_visibility_reload"))
                g_print_text(drv, dev, x, 0, bfu_style_bw_system, BUTTON_RELOAD, &z);
	f->ses->reload_size = z;
        x+=z;

        /* Bookmarks */
        z = 0;
        if(options_get_int("toolbar_button_visibility_bookmarks"))
                g_print_text(drv, dev, x, 0, bfu_style_bw_system, BUTTON_BOOKMARKS, &z);
	f->ses->bookmarks_size = z;
        x+=z;

        /* Home */
        z = 0;
        if(options_get_int("toolbar_button_visibility_home"))
                g_print_text(drv, dev, x, 0, bfu_style_bw_system, BUTTON_HOME, &z);
        f->ses->home_size = z;
        x+=z;

        /* Stop */
        z = 0;
        if(options_get_int("toolbar_button_visibility_stop"))
                g_print_text(drv, dev, x, 0, bfu_style_bw_system, BUTTON_STOP, &z);
        f->ses->stop_size = z;
        x+=z;

        /* End menubar */

        b = (dev->size.x2 - w) - 16;
        z=x;
        if (b < z) b = z;
	drv->fill_area(dev, z, 0, b, G_BFU_FONT_SIZE, bfu_bg_color);
	g_print_text(drv, dev, b, 0, bfu_style_bw_b, title, &b);
	drv->fill_area(dev, b, 0, dev->size.x2, G_BFU_FONT_SIZE, bfu_bg_color);
	mem_free(title);

        /* Draw graphical tabs */
#define TAB_SPACE 5
        if (options_get_bool("tabs_show")) {
                struct terminal *term = f->ses->term;
                int number = number_of_tabs(term);
                int tab_width = term->dev->size.x2/number;
                int tab;
                int msglen;
                unsigned char *msg;
                int y1 = term->dev->size.y2 - 2 * G_BFU_FONT_SIZE;
                int y2 = term->dev->size.y2 - G_BFU_FONT_SIZE;
                struct rect saved_clip;

                if(number==1 &&
                   !options_get_bool("tabs_show_if_single"))
                        goto tabs_end;

                for (tab = 0; tab < number; tab++){
                        struct window *win = get_tab_by_number(term,tab);
                        int x1=tab*tab_width;
                        int x2=(tab+1)*tab_width;
                        int is_active = tab==term->current_tab;
                        int xx=x1+TAB_SPACE;

                        if(win->data &&
                           current_frame(win->data) &&
                           current_frame(win->data)->f_data &&
                           current_frame(win->data)->f_data->title &&
                           strlen(current_frame(win->data)->f_data->title))
                                msg = current_frame(win->data)->f_data->title;
                        else
                                msg = "Untitled";

                        restrict_clip_area(term->dev,
                                           &saved_clip,
                                           x1, y1,
                                           x2, y2);
                        term->dev->drv->fill_area(term->dev,
                                                  x1, y1,
                                                  x1+TAB_SPACE, y2,
                                                  bfu_bg_color);
                        g_print_text(term->dev->drv, term->dev, x1+TAB_SPACE, y1,
                                     is_active ? bfu_style_bw_b : bfu_style_bw,
                                     msg, &xx);
                        term->dev->drv->fill_area(term->dev,
                                                  (xx > x2-TAB_SPACE) ? x2-TAB_SPACE : xx, y1,
                                                  x2, y2,
                                                  bfu_bg_color);
                        term->dev->drv->draw_hline(term->dev, x1, (is_active ? y2-1 : y1), x2, bfu_fg_color);

                        if(is_active){
                                term->dev->drv->draw_vline(term->dev, x1, y1, y2, bfu_fg_color);
                                term->dev->drv->draw_vline(term->dev, x2-1, y1, y2, bfu_fg_color);
                        }
                        term->dev->drv->set_clip_area(term->dev, &saved_clip);
                }
                term->dev->drv->fill_area(term->dev,
                                          number*tab_width, y1,
                                          term->dev->size.x2, y2,
                                          bfu_bg_color);

        tabs_end:
				w=w;
        }

}

struct f_data *srch_f_data;

void get_searched_sub(struct g_object *p, struct g_object *c)
{
	if (c->type == G_OBJECT_TEXT) {
		struct g_object_text *t = (struct g_object_text *)c;
		int pos = srch_f_data->srch_string_size;
		t->srch_pos = pos;
		add_to_str(&srch_f_data->srch_string, &srch_f_data->srch_string_size, t->text);
	}
	if (c->get_list) c->get_list(c, get_searched_sub);
	if (c->type == G_OBJECT_LINE) {
		if (srch_f_data->srch_string_size && srch_f_data->srch_string[srch_f_data->srch_string_size - 1] != ' ')
			add_to_str(&srch_f_data->srch_string, &srch_f_data->srch_string_size, " ");
	}
}

void g_get_search_data(struct f_data *f)
{
	srch_f_data = f;
	if (f->srch_string) return;
	f->srch_string = init_str();
	f->srch_string_size = 0;
	if (f->root && f->root->get_list) f->root->get_list(f->root, get_searched_sub);
	while (f->srch_string_size && f->srch_string[f->srch_string_size - 1] == ' ') {
		f->srch_string[--f->srch_string_size] = 0;
	}
}

unsigned char *search_word;

int find_refline;
int find_direction;

int find_opt_yy;
int find_opt_y;
int find_opt_yw;
int find_opt_x;
int find_opt_xw;

void find_next_sub(struct g_object *p, struct g_object *c)
{
	if (c->type == G_OBJECT_TEXT) {
		struct g_object_text *t = (struct g_object_text *)c;
		int start = t->srch_pos;
		int end = t->srch_pos + strlen(t->text);
		int found;
		BIN_SEARCH(highlight_positions, n_highlight_positions, B_EQUAL, B_ABOVE, *, found);
		if (found != -1) {
			int x, y, yy;
			get_object_pos(c, &x, &y);
			y += t->yw / 2;
			yy = y;
			if (yy < find_refline) yy += MAXINT / 2;
			if (find_direction < 0) yy = MAXINT - yy;
			if (find_opt_yy == -1 || yy > find_opt_yy) {
				int i, l, ll;
				find_opt_yy = yy;
				find_opt_y = y;
				find_opt_yw = t->style->height;
				find_opt_x = x;
				find_opt_xw = t->xw;
				l = strlen(t->text);
				ll = strlen(search_word);
				for (i = 0; i <= l - ll; i++) {
					int j;
					unsigned char *tt;
					for (j = 0; j < ll; j++) if (srch_cmp(t->text[i + j], search_word[j])) goto no_ch;
					tt = memacpy(t->text, i);
					find_opt_x += g_text_width(t->style, tt);
					find_opt_xw = g_text_width(t->style, search_word);
					mem_free(tt);
					goto fnd;
					no_ch:;
				}
				fnd:;
				/*debug("-%s-%s-: %d %d", t->text, search_word, find_opt_x, find_opt_xw);*/
			}
		}
	}
	if (c->get_list) c->get_list(c, find_next_sub);
}

void g_find_next_str(struct f_data *f)
{
	find_opt_yy = -1;
	if (f->root && f->root->get_list) f->root->get_list(f->root, find_next_sub);
}

void g_find_next(struct f_data_c *f, int a)
{
	g_get_search_data(f->f_data);
	g_get_search(f->f_data, f->ses->search_word);
	search_word = f->ses->search_word;
	if (!f->f_data->n_search_positions) msg_box(f->ses->term, NULL, TXT(T_SEARCH), AL_CENTER, TXT(T_SEARCH_STRING_NOT_FOUND), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);

	highlight_positions = f->f_data->search_positions;
	n_highlight_positions = f->f_data->n_search_positions;
	highlight_position_size = f->f_data->last_search_len;

	if ((!a && f->ses->search_direction == -1) ||
	     (a && f->ses->search_direction == 1)) find_refline = f->vs->view_pos;
	else find_refline = f->vs->view_pos + f->yw - f->f_data->hsb * G_SCROLL_BAR_WIDTH;
	find_direction = -f->ses->search_direction;

	g_find_next_str(f->f_data);

	highlight_positions = NULL;
	n_highlight_positions = highlight_position_size = 0;

	if (find_opt_yy == -1) goto d;
	if (!a || find_opt_y < f->vs->view_pos || find_opt_y + find_opt_yw >= f->vs->view_pos + f->yw - f->f_data->hsb * G_SCROLL_BAR_WIDTH) {
		f->vs->view_pos = find_opt_y - (f->yw - f->f_data->hsb * G_SCROLL_BAR_WIDTH) / 2;
	}
	if (find_opt_x < f->vs->view_posx || find_opt_x + find_opt_xw >= f->vs->view_posx + f->xw - f->f_data->vsb * G_SCROLL_BAR_WIDTH) {
		f->vs->view_posx = find_opt_x + find_opt_xw / 2 - (f->xw - f->f_data->vsb * G_SCROLL_BAR_WIDTH) / 2;
	}

	d:draw_fd(f);
}

void init_grview()
{
	int i, w = g_text_width(bfu_style_wb_mono, " ");
	for (i = 32; i < 128; i++) {
		unsigned char a[2];
		a[0] = i, a[1] = 0;
		if (g_text_width(bfu_style_wb_mono, a) != w) internal("Monospaced font is not monospaced (error at char %d, width %d, wanted width %d)", i, (int)g_text_width(bfu_style_wb_mono, a), w);
	}
	scroll_bar_frame_color = dip_get_color_sRGB(options_get_rgb_int("scrollbar_frame_color"));
	scroll_bar_area_color = dip_get_color_sRGB(options_get_rgb_int("scrollbar_area_color"));
	scroll_bar_bar_color = dip_get_color_sRGB(options_get_rgb_int("scrollbar_bar_color"));
}

/*
 Recursive execution of user-defined code for object tree
 It calls fn(g_object (object, int x, int y, void *data);
 where 'data' is user-defined 'fn_data'
 */

void g_object_do_recursive(struct g_object *obj, int x0, int y0,void (*fn)(struct g_object *, int, int, void *), void *fn_data)
{
        struct g_object_area  *area =(struct g_object_area  *)obj;
        struct g_object_text  *text =(struct g_object_text  *)obj;
        struct g_object_tag   *tag  =(struct g_object_tag   *)obj;
        struct g_object_line  *line =(struct g_object_line  *)obj;
        struct g_object_image *image=(struct g_object_image *)obj;
        struct g_object_table *table=(struct g_object_table *)obj;
        int x=obj->x;
        int y=obj->y;
        int wx=obj->xw;
        int wy=obj->yw;

        switch(obj->type){
        case(G_OBJECT_AREA):
                {
                        int line=0;
                        while(line<area->n_lines){
                                struct g_object_line *obj_line=area->lines[line];
                                g_object_do_recursive((struct g_object*)obj_line,x0+obj_line->x,y0+obj_line->y,fn,fn_data);
                                line++;
                        }
                        return;
                }
        case(G_OBJECT_LINE):
                {
                        int entry=0;
                        while(entry<line->n_entries){
                                struct g_object *obj_entry=line->entries[entry];
                                g_object_do_recursive((struct g_object*)obj_entry,x0+obj_entry->x,y0+obj_entry->y,fn,fn_data);
                                entry++;
                        }
                        return;
                }
        case(G_OBJECT_TABLE):
                {
                        struct table *t=table->t;
                        int i, j;
                        for (j = 0; j < t->y; j++) for (i = 0; i < t->x; i++) {
                                struct table_cell *c = CELL(t, i, j);
                                struct g_object *obj_cell=(struct g_object*)c->root;
                                if (obj_cell)
                                        g_object_do_recursive(obj_cell,x0+obj_cell->x,y0+obj_cell->y,fn,fn_data);

                        }
                        return;
                }
        case(G_OBJECT_TEXT):
        case(G_OBJECT_IMAGE):
        case(G_OBJECT_TAG):
                fn(obj,x0,y0,fn_data);
                return;
        }
}

void g_goto_link(struct session *ses, int num)
{
	struct f_data_c *f = current_frame(ses);
  struct link *l;
	if (!f || !f->f_data || !f->vs || !f->f_data->links || !f->ses) return;
	if (num < -1 || num >= f->f_data->nlinks) return;
	f->vs->current_link = num;
  if (num > -1) {
    lr_link(f, num);
    l = &f->f_data->links[num];
    // ensure we remain in view
    if (l->r.y2 > f->vs->view_pos + f->yw - f->f_data->hsb * G_SCROLL_BAR_WIDTH) f->vs->view_pos = l->r.y2 - f->yw + f->f_data->hsb * G_SCROLL_BAR_WIDTH;
    if (l->r.y1 < f->vs->view_pos) f->vs->view_pos = l->r.y1;
    redraw_link(f, num);
    f->ses->locked_link = 0;
	  if (f->f_data->links[f->vs->current_link].type == L_FIELD || f->f_data->links[f->vs->current_link].type == L_AREA) {
		  if ((f->f_data->locked_on = f->f_data->links[f->vs->current_link].obj)) f->ses->locked_link = 1;
    }
    set_textarea(ses, f, KBD_UP);
	}
	f->active = 1;
	draw_to_window(ses->win, (void (*)(struct terminal *, void *))draw_doc_c, f);
	change_screen_status(ses);
	print_screen_status(ses);
}

#endif
