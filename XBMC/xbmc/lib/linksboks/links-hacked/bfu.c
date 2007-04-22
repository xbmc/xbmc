/* bfu.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

int menu_font_size;

struct memory_list *getml(void *p, ...)
{
	struct memory_list *ml;
	va_list ap;
	int n = 0;
	void *q = p;
	va_start(ap, p);
	while (q) n++, q = va_arg(ap, void *);
	if (!(ml = mem_alloc(sizeof(struct memory_list) + n * sizeof(void *)))) {
		va_end(ap);
		return NULL;
	}
	ml->n = n;
	n = 0;
	q = p;
	va_end(ap);
	va_start(ap, p);
	while (q) ml->p[n++] = q, q = va_arg(ap, void *);
	va_end(ap);
	return ml;
}

void add_to_ml(struct memory_list **ml, ...)
{
	struct memory_list *nml;
	va_list ap;
	int n = 0;
	void *q;
	if (!*ml) {
		if (!(*ml = mem_alloc(sizeof(struct memory_list)))) return;
		(*ml)->n = 0;
	}
	va_start(ap, ml);
	while ((q = va_arg(ap, void *))) n++;
	if (!(nml = mem_realloc(*ml, sizeof(struct memory_list) + (n + (*ml)->n) * sizeof(void *)))) {
		va_end(ap);
		return;
	}
	va_end(ap);
	va_start(ap, ml);
	while ((q = va_arg(ap, void *))) nml->p[nml->n++] = q;
	*ml = nml;
	va_end(ap);
}

void freeml(struct memory_list *ml)
{
	int i;
	if (!ml) return;
	for (i = 0; i < ml->n; i++) mem_free(ml->p[i]);
	mem_free(ml);
}

#ifndef G
#define txtlen strlen
#else
#define txtlen(x) (F ? g_text_width(bfu_style_wb, x) : strlen(x))
#endif

#ifdef G

int is_in_str(unsigned char *str, int u)
{
	while (*str) {
		int v;
		GET_UTF_8(str, v);
		if (u >= 'a' && u <= 'z') u -= 'a' - 'A';
		if (v >= 'a' && v <= 'z') v -= 'a' - 'A';
		if (u == v) return 1;
	}
	return 0;
}

#endif

#ifdef G
struct style *bfu_style_wb, *bfu_style_wb_b, *bfu_style_bw, *bfu_style_bw_b, *bfu_style_bw_u;
struct style *bfu_style_bw_mono;
struct style *bfu_style_wb_mono, *bfu_style_wb_mono_u;
struct style *bfu_style_bw_system, *bfu_style_wb_system;

long bfu_fg_color, bfu_bg_color, bfu_shadow_color;

int G_DIALOG_FIELD_WIDTH;

void init_bfu()
{
	if (!F) return;
	bfu_bg_color = dip_get_color_sRGB(options_get_rgb_int("menu_bg_color"));
	bfu_fg_color = dip_get_color_sRGB(options_get_rgb_int("menu_fg_color"));
	bfu_shadow_color = dip_get_color_sRGB(options_get_rgb_int("menu_shadow_color"));

        menu_font_size = options_get_int("menu_font_size");

        bfu_style_wb   = g_get_style(options_get_rgb_int("menu_bg_color"),
                                     options_get_rgb_int("menu_fg_color"),
                                     options_get_int("menu_font_size"),
                                     options_get("menu_font"), 0);
        bfu_style_wb_b = g_get_style(options_get_rgb_int("menu_bg_color"),
                                     options_get_rgb_int("menu_fg_color"),
                                     options_get_int("menu_font_size"),
                                     options_get("menu_bold_font"), 0);
        bfu_style_bw   = g_get_style(options_get_rgb_int("menu_fg_color"),
                                     options_get_rgb_int("menu_bg_color"),
                                     options_get_int("menu_font_size"),
                                     options_get("menu_font"), 0);
        bfu_style_bw_b = g_get_style(options_get_rgb_int("menu_fg_color"),
                                     options_get_rgb_int("menu_bg_color"),
                                     options_get_int("menu_font_size"),
                                     options_get("menu_bold_font"), 0);
        bfu_style_bw_u = g_get_style(options_get_rgb_int("menu_fg_color"),
                                     options_get_rgb_int("menu_bg_color"),
                                     options_get_int("menu_font_size"),
                                     options_get("menu_font"), FF_UNDERLINE);

        bfu_style_bw_mono   = g_get_style(options_get_rgb_int("menu_fg_color"),
                                          options_get_rgb_int("menu_bg_color"),
                                          options_get_int("menu_font_size"),
                                          options_get("menu_mono_font"), 0);
        bfu_style_wb_mono   = g_get_style(options_get_rgb_int("menu_bg_color"),
                                          options_get_rgb_int("menu_fg_color"),
                                          options_get_int("menu_font_size"),
                                          options_get("menu_mono_font"), 0);
        bfu_style_wb_mono_u = g_get_style(options_get_rgb_int("menu_bg_color"),
                                          options_get_rgb_int("menu_fg_color"),
                                          options_get_int("menu_font_size"),
                                          options_get("menu_mono_font"), FF_UNDERLINE);

        bfu_style_bw_system   = g_get_style(options_get_rgb_int("menu_fg_color"),
                                            options_get_rgb_int("menu_bg_color"),
                                            options_get_int("menu_font_size"),
                                            options_get("menu_system_font"), 0);
        bfu_style_wb_system   = g_get_style(options_get_rgb_int("menu_bg_color"),
                                            options_get_rgb_int("menu_fg_color"),
                                            options_get_int("menu_font_size"),
                                            options_get("menu_system_font"), 0);


        G_DIALOG_FIELD_WIDTH = g_char_width(bfu_style_wb_mono, ' ');

}

void shutdown_bfu()
{
	if (!F) return;
	g_free_style(bfu_style_wb);
	g_free_style(bfu_style_wb_b);
	g_free_style(bfu_style_bw);
	g_free_style(bfu_style_bw_b);
	g_free_style(bfu_style_bw_u);
	g_free_style(bfu_style_bw_mono);
	g_free_style(bfu_style_wb_mono);
	g_free_style(bfu_style_wb_mono_u);
	g_free_style(bfu_style_bw_system);
	g_free_style(bfu_style_wb_system);
}

#else

void init_bfu() {}
void shutdown_bfu() {}

#endif

unsigned char m_bar = 0;

void menu_func(struct window *, struct event *, int);
void mainmenu_func(struct window *, struct event *, int);
void dialog_func(struct window *, struct event *, int);

void do_menu_selected(struct terminal *term, struct menu_item *items, void *data, int selected)
{
	struct menu *menu;
	if ((menu = mem_alloc(sizeof(struct menu)))) {
		menu->selected = selected;
		menu->view = 0;
		menu->items = items;
		menu->data = data;
#ifdef G
		if (F) {
			int n, i;
			for (n = 0; items[n].text; n++) ;
			if (!(menu->hktxt1 = mem_calloc(n * sizeof(unsigned char *)))) {
				mem_free(menu);
				goto b;
			}
			if (!(menu->hktxt2 = mem_calloc(n * sizeof(unsigned char *)))) {
				mem_free(menu->hktxt1);
				mem_free(menu);
				goto b;
			}
			if (!(menu->hktxt3 = mem_calloc(n * sizeof(unsigned char *)))) {
				mem_free(menu->hktxt2);
				mem_free(menu->hktxt1);
				mem_free(menu);
				goto b;
			}
			for (i = 0; i < n; i++) {
				unsigned char *txt = _(items[i].text, term);
				unsigned char *ext = _(items[i].hotkey, term);
				unsigned char *txt2, *txt3 = txt;
				if (ext != M_BAR) while (*txt3) {
					int u;
					txt2 = txt3;
					GET_UTF_8(txt3, u);
					if (is_in_str(ext, u)) {
						menu->hktxt1[i] = memacpy(txt, txt2 - txt);
						menu->hktxt2[i] = memacpy(txt2, txt3 - txt2);
						menu->hktxt3[i] = stracpy(txt3);
						goto x;
					}
				}
				menu->hktxt1[i] = stracpy(txt);
				menu->hktxt2[i] = stracpy("");
				menu->hktxt3[i] = stracpy("");
				x:;
			}
		}
#endif
		add_window(term, menu_func, menu);
	} else
#ifdef G
	b:
#endif
	if (items->free_i) {
		int i;
		for (i = 0; items[i].text; i++) {
			if (items[i].free_i & 2) mem_free(items[i].text);
			if (items[i].free_i & 4) mem_free(items[i].rtext);
		}
		mem_free(items);
	}
}

void do_menu(struct terminal *term, struct menu_item *items, void *data)
{
	do_menu_selected(term, items, data, 0);
}

void select_menu(struct terminal *term, struct menu *menu)
{
	struct menu_item *it = &menu->items[menu->selected];
	void (*func)(struct terminal *, void *, void *) = it->func;
	void *data1 = it->data;
	void *data2 = menu->data;
	if (menu->selected < 0 || menu->selected >= menu->ni || it->hotkey == M_BAR) return;
	if (!it->in_m) {
		struct window *win, *win1;
		for (win = term->windows.next; (void *)win != &term->windows && (win->handler == menu_func || win->handler == mainmenu_func); win1 = win->next, delete_window(win), win = win1) ;
	}
	func(term, data1, data2);
}

void count_menu_size(struct terminal *term, struct menu *menu)
{
	int sx = term->x;
	int sy = term->y;
	int mx = gf_val(4, 2 * (G_MENU_LEFT_BORDER + G_MENU_LEFT_INNER_BORDER));
	int my;
	for (my = 0; menu->items[my].text; my++) {
		int s = txtlen(_(menu->items[my].text, term)) + txtlen(_(menu->items[my].rtext, term)) + gf_val(MENU_HOTKEY_SPACE, G_MENU_HOTKEY_SPACE) * (_(menu->items[my].rtext, term)[0] != 0) + gf_val(4, 2 * (G_MENU_LEFT_BORDER + G_MENU_LEFT_INNER_BORDER));
		if (s > mx) mx = s;
	}
	menu->ni = my;
	my = gf_val(my, my * G_BFU_FONT_SIZE);
	my += gf_val(2, 2 * G_MENU_TOP_BORDER);
	if (mx > sx) mx = sx;
	if (my > sy) my = sy;
#ifdef G
	if (F) {
		my -= 2 * G_MENU_TOP_BORDER;
		my -= my % G_BFU_FONT_SIZE;
		my += 2 * G_MENU_TOP_BORDER;
	}
#endif
	menu->nview = gf_val(my - 2, (my - 2 * G_MENU_TOP_BORDER) / G_BFU_FONT_SIZE);
	menu->xw = mx;
	menu->yw = my;
	if ((menu->x = menu->xp) < 0) menu->x = 0;
	if ((menu->y = menu->yp) < 0) menu->y = 0;
	if (menu->x + mx > sx) menu->x = sx - mx;
	if (menu->y + my > sy) menu->y = sy - my;
#ifdef G
	if (F) set_window_pos(menu->win, menu->x, menu->y, menu->x + menu->xw + G_SHADOW_WIDTH, menu->y + menu->yw + G_SHADOW_HEIGHT);
#endif
}

void scroll_menu(struct menu *menu, int d)
{
	int c = 0;
	int w = menu->nview;
	int scr_i = SCROLL_ITEMS > (w-1)/2 ? (w-1)/2 : SCROLL_ITEMS;
	if (scr_i < 0) scr_i = 0;
	if (w < 0) w = 0;
	menu->selected += d;
	while (1) {
		if (c++ > menu->ni) {
			menu->selected = -1;
			menu->view = 0;
			return;
		}
		if (menu->selected < 0) menu->selected = menu->ni-1;
		if (menu->selected >= menu->ni) menu->selected = 0;
		if (menu->ni && menu->items[menu->selected].hotkey != M_BAR) break;
                menu->selected += (d>0) ? 1 : -1 ;
	}
	if (menu->selected < menu->view + scr_i) menu->view = menu->selected - scr_i;
	if (menu->selected >= menu->view + w - scr_i - 1) menu->view = menu->selected - w + scr_i + 1;
	if (menu->view > menu->ni - w) menu->view = menu->ni - w;
	if (menu->view < 0) menu->view = 0;
}

void display_menu_txt(struct terminal *term, struct menu *menu)
{
	int p, s;
	fill_area(term, menu->x+1, menu->y+1, menu->xw-2, menu->yw-2, COLOR_MENU);
	draw_frame(term, menu->x, menu->y, menu->xw, menu->yw, COLOR_MENU_FRAME, 1);
	set_window_ptr(menu->win, menu->x, menu->y);
	for (p = menu->view, s = menu->y + 1; p < menu->ni && p < menu->view + menu->yw - 2; p++, s++) {
		int x;
		int h = 0;
		unsigned char c;
		unsigned char *tmptext = _(menu->items[p].text, term);
		int co = p == menu->selected ? h = 1, COLOR_MENU_SELECTED : COLOR_MENU;
		if (h) {
			set_cursor(term, menu->x+1, s, term->x - 1, term->y - 1);
			/*set_window_ptr(menu->win, menu->x+3, s+1);*/
			set_window_ptr(menu->win, menu->x+menu->xw, s);
			fill_area(term, menu->x+1, s, menu->xw-2, 1, co);
		}
		if (menu->items[p].hotkey != M_BAR || (tmptext[0])) {
			int l = strlen(_(menu->items[p].rtext, term));
			for (x = l - 1; x >= 0 && menu->xw - 4 >= l - x && (c = _(menu->items[p].rtext, term)[x]); x--)
				set_char(term, menu->x + menu->xw - 2 - l + x, s, c | co);
			for (x = 0; x < menu->xw - 4 && (c = tmptext[x]); x++)
				set_char(term, menu->x + x + 2, s, !h && strchr(_(menu->items[p].hotkey, term), upcase(c)) ? h = 1, COLOR_MENU_HOTKEY | c : co | c);
		} else {
			set_char(term, menu->x, s, COLOR_MENU_FRAME | ATTR_FRAME | 0xc3);
			fill_area(term, menu->x+1, s, menu->xw-2, 1, COLOR_MENU_FRAME | ATTR_FRAME | 0xc4);
			set_char(term, menu->x+menu->xw-1, s, COLOR_MENU_FRAME | ATTR_FRAME | 0xb4);
                }
        }
}

int menu_oldview = -1;
int menu_oldsel = -1;

#ifdef G

int menu_ptr_set;

void display_menu_item_gfx(struct terminal *term, struct menu *menu, int it)
{
	struct menu_item *item = &menu->items[it];
	struct graphics_device *dev = term->dev;
	int y;
	if (it < menu->view || it >= menu->ni || it >= menu->view + menu->nview) return;
	y = menu->y + G_MENU_TOP_BORDER + (it - menu->view) * G_BFU_FONT_SIZE;
	if (item->hotkey == M_BAR && !_(item->text, term)[0]) {
		drv->fill_area(dev, menu->x + (G_MENU_LEFT_BORDER - 1) / 2 + 1, y, menu->x + menu->xw - (G_MENU_LEFT_BORDER + 1) / 2, y + (G_BFU_FONT_SIZE - 1) / 2, bfu_bg_color);
		drv->draw_hline(dev, menu->x + (G_MENU_LEFT_BORDER - 1) / 2 + 1, y + (G_BFU_FONT_SIZE - 1) / 2, menu->x + menu->xw - G_MENU_LEFT_BORDER / 2, bfu_fg_color);
		drv->fill_area(dev, menu->x + (G_MENU_LEFT_BORDER - 1) / 2 + 1, y + (G_BFU_FONT_SIZE - 1) / 2 + 1, menu->x + menu->xw - (G_MENU_LEFT_BORDER + 1) / 2, y + G_BFU_FONT_SIZE, bfu_bg_color);
	} else {
		int p;
		struct rect r;
		unsigned char *rtext = _(item->rtext, term);
		if (it != menu->selected) {
			drv->fill_area(dev, menu->x + (G_MENU_LEFT_BORDER - 1) / 2 + 1, y, menu->x + G_MENU_LEFT_BORDER + G_MENU_LEFT_INNER_BORDER, y + G_BFU_FONT_SIZE, bfu_bg_color);
		} else {
			menu->xl1 = menu->x;
			menu->yl1 = y;
			menu->xl2 = menu->x + menu->xw;
			menu->yl2 = y + G_BFU_FONT_SIZE;
			menu_ptr_set = 1;
			set_window_ptr(menu->win, menu->x + menu->xw, y);
			drv->fill_area(dev, menu->x + (G_MENU_LEFT_BORDER - 1) / 2 + 1, y, menu->x + G_MENU_LEFT_BORDER, y + G_BFU_FONT_SIZE, bfu_bg_color);
			drv->fill_area(dev, menu->x + menu->xw - G_MENU_LEFT_BORDER, y, menu->x + menu->xw - (G_MENU_LEFT_BORDER + 1) / 2, y + G_BFU_FONT_SIZE, bfu_bg_color);
			drv->fill_area(dev, menu->x + G_MENU_LEFT_BORDER, y, menu->x + G_MENU_LEFT_BORDER + G_MENU_LEFT_INNER_BORDER, y + G_BFU_FONT_SIZE, bfu_fg_color);
		}
		restrict_clip_area(dev, &r, menu->x + G_MENU_LEFT_BORDER + G_MENU_LEFT_INNER_BORDER, y, menu->x + menu->xw - G_MENU_LEFT_BORDER - G_MENU_LEFT_INNER_BORDER, y + G_BFU_FONT_SIZE);
		if (it == menu->selected) {
			p = menu->x + G_MENU_LEFT_BORDER + G_MENU_LEFT_INNER_BORDER;
			g_print_text(drv, dev, p, y, bfu_style_wb, menu->hktxt1[it], &p);
			g_print_text(drv, dev, p, y, bfu_style_wb, menu->hktxt2[it], &p);
			g_print_text(drv, dev, p, y, bfu_style_wb, menu->hktxt3[it], &p);
		} else {
			p = menu->x + G_MENU_LEFT_BORDER + G_MENU_LEFT_INNER_BORDER;
			g_print_text(drv, dev, p, y, bfu_style_bw, menu->hktxt1[it], &p);
			g_print_text(drv, dev, p, y, bfu_style_bw_u, menu->hktxt2[it], &p);
			g_print_text(drv, dev, p, y, bfu_style_bw, menu->hktxt3[it], &p);
		}
		if (!*rtext) {
			drv->set_clip_area(dev, &r);
			if (p > menu->x + menu->xw - G_MENU_LEFT_BORDER - G_MENU_LEFT_INNER_BORDER) p = menu->x + menu->xw - G_MENU_LEFT_BORDER - G_MENU_LEFT_INNER_BORDER;
			if (it != menu->selected)
				drv->fill_area(dev, p, y, menu->x + menu->xw - (G_MENU_LEFT_BORDER + 1) / 2, y + G_BFU_FONT_SIZE, bfu_bg_color);
			else
				drv->fill_area(dev, p, y, menu->x + menu->xw - G_MENU_LEFT_BORDER, y + G_BFU_FONT_SIZE, bfu_fg_color);
		} else {
			int s = menu->x + menu->xw - G_MENU_LEFT_BORDER - G_MENU_LEFT_INNER_BORDER - g_text_width(bfu_style_wb, rtext);
			if (s < p) s = p;
			drv->fill_area(dev, p, y, s, y + G_BFU_FONT_SIZE, it != menu->selected ? bfu_bg_color : bfu_fg_color);
			g_print_text(drv, dev, s, y, it != menu->selected ? bfu_style_bw : bfu_style_wb, rtext, NULL);
			drv->set_clip_area(dev, &r);
			if (it != menu->selected)
				drv->fill_area(dev, menu->x + menu->xw - G_MENU_LEFT_BORDER - G_MENU_LEFT_INNER_BORDER, y, menu->x + menu->xw - (G_MENU_LEFT_BORDER + 1) / 2, y + G_BFU_FONT_SIZE, bfu_bg_color);
			else
				drv->fill_area(dev, menu->x + menu->xw - G_MENU_LEFT_BORDER - G_MENU_LEFT_INNER_BORDER, y, menu->x + menu->xw - G_MENU_LEFT_BORDER, y + G_BFU_FONT_SIZE, bfu_fg_color);
		}
	}
}

void display_menu_gfx(struct terminal *term, struct menu *menu)
{
	int p;
	struct graphics_device *dev = term->dev;
	if (menu_oldview == menu->view) {
		if (menu_oldsel >= 0 && menu_oldsel < menu->ni && menu_oldsel < menu->view + menu->nview) display_menu_item_gfx(term, menu, menu_oldsel);
		if (menu->selected >= 0 && menu->selected < menu->ni && menu->selected < menu->view + menu->nview) display_menu_item_gfx(term, menu, menu->selected);
		return;
	}
#define PX1 (menu->x + (G_MENU_LEFT_BORDER - 1) / 2)
#define PX2 (menu->x + menu->xw - (G_MENU_LEFT_BORDER + 1) / 2)
#define PY1 (menu->y + (G_MENU_TOP_BORDER - 1) / 2)
#define PY2 (menu->y + menu->yw - (G_MENU_TOP_BORDER + 1) / 2)
	drv->fill_area(dev, menu->x, menu->y, menu->x + menu->xw, PY1, bfu_bg_color);
	drv->fill_area(dev, menu->x, PY1, PX1, PY2 + 1, bfu_bg_color);
	drv->fill_area(dev, PX2 + 1, PY1, menu->x + menu->xw, PY2 + 1, bfu_bg_color);
	drv->fill_area(dev, menu->x, PY2 + 1, menu->x + menu->xw, menu->y + menu->yw, bfu_bg_color);
	drv->draw_hline(dev, PX1, PY1, PX2 + 1, bfu_fg_color);
	drv->draw_hline(dev, PX1, PY2, PX2 + 1, bfu_fg_color);
	drv->draw_vline(dev, PX1, PY1 + 1, PY2, bfu_fg_color);
	drv->draw_vline(dev, PX2, PY1 + 1, PY2, bfu_fg_color);
	drv->fill_area(dev, PX1 + 1, PY1 + 1, PX2, menu->y + G_MENU_TOP_BORDER, bfu_bg_color);
	drv->fill_area(dev, PX1 + 1, menu->y + menu->yw - G_MENU_TOP_BORDER, PX2, PY2, bfu_bg_color);

        /* Simple menu borders and shadows */
        drv->draw_hline(dev, menu->x, menu->y, menu->x+menu->xw, bfu_fg_color);
        drv->draw_hline(dev, menu->x, menu->y+menu->yw, menu->x+menu->xw, bfu_fg_color);
        drv->draw_vline(dev, menu->x, menu->y, menu->y+menu->yw, bfu_fg_color);
        drv->draw_vline(dev, menu->x+menu->xw, menu->y, menu->y+menu->yw, bfu_fg_color);
        drv->fill_area(dev, menu->x + G_SHADOW_WIDTH, menu->y + menu->yw , menu->x + menu->xw + G_SHADOW_WIDTH, menu->y + menu->yw + G_SHADOW_HEIGHT, bfu_shadow_color);
	drv->fill_area(dev, menu->x + menu->xw, menu->y + G_SHADOW_HEIGHT, menu->x + menu->xw + G_SHADOW_WIDTH, menu->y + menu->yw + G_SHADOW_HEIGHT, bfu_shadow_color);

        menu->xl1 = menu->yl1 = menu->xl2 = menu->yl2 = 0;
	menu_ptr_set = 0;
	for (p = menu->view; p < menu->ni && p < menu->view + menu->nview; p++) display_menu_item_gfx(term, menu, p);
	if (!menu_ptr_set) set_window_ptr(menu->win, menu->x, menu->y);
}

#endif
#define DIST 5

int step_pageupdown(struct menu *menu, int dir)
{
        int i = menu->selected - 1 + dir;
        int found = 0;
        int step = dir;

        for (; i >= 0 && i < menu->ni ; i+=dir) {
                if (menu->items[i].hotkey == M_BAR) {
                        found = 1;
                        break;
                }
        }

        if (found) {
                step = i + 1 - menu->selected;
        } else {
                step = DIST * dir;
        }

        if (dir<0) {
                if (menu->selected + step < 0)
                        step = -menu->selected;

                if (step < -DIST) step = DIST;

                if (step > 0)
                        step = menu->selected - menu->ni - 1;
        } else {
                if (menu->selected + step >= menu->ni)
                        step = menu->ni - menu->selected - 1;

                if (step > DIST) step = DIST;

                if (step >= menu->ni)
                        step = menu->ni - 1;
        }

        return step;
}

#undef DIST

void menu_func(struct window *win, struct event *ev, int fwd)
{
	int s = 0;
	int xp, yp;
	struct menu *menu = win->data;
	struct window *w1;
	menu->win = win;
        switch (ev->ev) {
		case EV_INIT:
		case EV_RESIZE:
			get_parent_ptr(win, &menu->xp, &menu->yp);
			count_menu_size(win->term, menu);
			goto xxx;
		case EV_REDRAW:
                        /* Hey, do we need this code? --karpov
                        get_parent_ptr(win, &xp, &yp);
			if (xp != menu->xp || yp != menu->yp) {
				menu->xp = xp;
				menu->yp = yp;
				count_menu_size(win->term, menu);
                        }
                        */
			xxx:
			menu->selected--;
			scroll_menu(menu, 1);
			draw_to_window(win, (void (*)(struct terminal *, void *))gf_val(display_menu_txt, display_menu_gfx), menu);
			break;
		case EV_MOUSE:
                        if ((ev->b & BM_BUTT) == B_WHEELUP )
                                goto pageup;

                        if ((ev->b & BM_BUTT) == B_WHEELDOWN )
                                goto pagedown;

                        if ((ev->b & BM_ACT) == B_MOVE) break;
                        if (ev->x < menu->x || ev->x >= menu->x+menu->xw || ev->y < menu->y || ev->y >= menu->y+menu->yw) {
				int f = 1;
				for (w1 = win; (void *)w1 != &win->term->windows; w1 = w1->next) {
					struct menu *m1;
					if (w1->handler == mainmenu_func) {
#ifdef G
						struct mainmenu *m2 = w1->data;
						if (F && !f && ev->x >= m2->xl1 && ev->x < m2->xl2 && ev->y >= m2->yl1 && ev->y < m2->yl2) goto bbb;
#endif
						if (ev->y < gf_val(1, G_BFU_FONT_SIZE)) {
							del:delete_window_ev(win, ev);
							goto bbb;
						}
						break;
					}
					if (w1->handler != menu_func) break;
					m1 = w1->data;
#ifdef G
					if (F && !f && ev->x >= m1->xl1 && ev->x < m1->xl2 && ev->y >= m1->yl1 && ev->y < m1->yl2) goto bbb;
#endif
					if (ev->x > m1->x && ev->x < m1->x+m1->xw-1 && ev->y > m1->y && ev->y < m1->y+m1->yw-1) goto del;
					f--;
				}
				if ((ev->b & BM_ACT) == B_DOWN) goto del;
				bbb:;
			} else {
				if (!(ev->x < menu->x || ev->x >= menu->x+menu->xw || ev->y < menu->y + gf_val(1, G_MENU_TOP_BORDER) || ev->y >= menu->y + menu->yw - gf_val(1, G_MENU_TOP_BORDER))) {
					int s = gf_val(ev->y - menu->y-1 + menu->view, (ev->y - menu->y - G_MENU_TOP_BORDER) / G_BFU_FONT_SIZE + menu->view);
					if (s >= 0 && s < menu->ni && menu->items[s].hotkey != M_BAR) {
						menu_oldview = menu->view;
						menu_oldsel = menu->selected;
						menu->selected = s;
						scroll_menu(menu, 0);
						draw_to_window(win, (void (*)(struct terminal *, void *))gf_val(display_menu_txt, display_menu_gfx), menu);
						menu_oldview = menu_oldsel = -1;
						if ((ev->b & BM_ACT) == B_UP || menu->items[s].in_m) select_menu(win->term, menu);
					}
				}
			}
			break;
		case EV_KBD:
			if (ev->x == KBD_LEFT || ev->x == KBD_RIGHT) {
				if ((void *)win->next != &win->term->windows && win->next->handler == mainmenu_func) goto mm;
				/*for (w1 = win; (void *)w1 != &win->term->windows; w1 = w1->next) {
					if (w1->handler == mainmenu_func) goto mm;
					if (w1->handler != menu_func) break;
				}*/
				if (ev->x == KBD_RIGHT) goto enter;
				delete_window(win);
				break;
			}
			if ((ev->x >= KBD_F1 && ev->x <= KBD_F12) || ev->y == KBD_ALT) {
				mm:
				delete_window_ev(win, ev);
				break;
			}
			if (ev->x == KBD_ESC) {
				delete_window_ev(win, (void *)win->next != &win->term->windows && win->next->handler == mainmenu_func ? ev : NULL);
				break;
			}
			menu_oldview = menu->view;
			menu_oldsel = menu->selected;
			if (ev->x == KBD_UP) scroll_menu(menu, -1);
			if (ev->x == KBD_DOWN) scroll_menu(menu, 1);
			if (ev->x == KBD_HOME) menu->selected = -1, scroll_menu(menu, 1);
			if (ev->x == KBD_END) menu->selected = menu->ni, scroll_menu(menu, -1);

                        if (ev->x == KBD_PAGE_UP){
                        pageup:
                                scroll_menu(menu, step_pageupdown(menu,-1));
                        }

                        if (ev->x == KBD_PAGE_DOWN){
                        pagedown:
                                scroll_menu(menu, step_pageupdown(menu,1));
                        }

                        if (ev->x > ' ' && ev->x < 256) {
				int i;
				for (i = 0; i < menu->ni; i++)
					if (strchr(_(menu->items[i].hotkey, win->term), upcase(ev->x))) {
						menu->selected = i;
						scroll_menu(menu, 0);
						s = 1;
					}
			}
			draw_to_window(win, (void (*)(struct terminal *, void *))gf_val(display_menu_txt, display_menu_gfx), menu);
			if (s || ev->x == KBD_ENTER || ev->x == ' ') {
				enter:
				menu_oldview = menu_oldsel = -1;
				select_menu(win->term, menu);
			}
			menu_oldview = menu_oldsel = -1;
			break;
		case EV_ABORT:
#ifdef G
			if (F) {
				int i;
				for (i = 0; menu->items[i].text; i++) {
					mem_free(menu->hktxt1[i]);
					mem_free(menu->hktxt2[i]);
					mem_free(menu->hktxt3[i]);
				}
				mem_free(menu->hktxt1);
				mem_free(menu->hktxt2);
				mem_free(menu->hktxt3);
			}
#endif
			if (menu->items->free_i) {
				int i;
				for (i = 0; menu->items[i].text; i++) {
					if (menu->items[i].free_i & 2) mem_free(menu->items[i].text);
					if (menu->items[i].free_i & 4) mem_free(menu->items[i].rtext);
				}
				mem_free(menu->items);
			}
			break;
	}
}

void do_mainmenu(struct terminal *term, struct menu_item *items, void *data, int sel)
{
	struct mainmenu *menu;
	if ((menu = mem_alloc(sizeof(struct mainmenu)))) {
		menu->selected = sel == -1 ? 0 : sel;
		menu->items = items;
		menu->data = data;
		add_window(term, mainmenu_func, menu);
		if (sel != -1) {
			struct event ev = {EV_KBD, KBD_ENTER, 0, 0};
                        struct window *first_win=term->windows.next;
                        struct window *win=first_win->type ? get_tab_by_number(term,term->current_tab) : first_win;

			win->handler(win, &ev, 0);
		}
	}
}

void display_mainmenu(struct terminal *term, struct mainmenu *menu)
{
	/* No term on Xbox
	if (!F) {
		int i;
		int p = 2;
		fill_area(term, 0, 0, term->x, 1, COLOR_MAINMENU | ' ');
		for (i = 0; menu->items[i].text; i++) {
			int s = 0;
			int j;
			unsigned char c;
			unsigned char *tmptext = _(menu->items[i].text, term);
			int co = i == menu->selected ? s = 1, COLOR_MAINMENU_SELECTED : COLOR_MAINMENU;
			if (s) {
				fill_area(term, p, 0, 2, 1, co);
				fill_area(term, p+strlen(tmptext)+2, 0, 2, 1, co);
				menu->sp = p;
				set_cursor(term, p, 0, term->x - 1, term->y - 1);
				set_window_ptr(menu->win, p, 1);
			}
			p += 2;
			for (j = 0; (c = tmptext[j]); j++, p++)
				set_char(term, p, 0, (!s && strchr(_(menu->items[i].hotkey, term), upcase(c)) ? s = 1, COLOR_MAINMENU_HOTKEY : co) | c);
			p += 2;
		}
		menu->ni = i;
#ifdef G
	} else {
	*/
		struct graphics_device *dev = term->dev;
		int i, p;
		drv->fill_area(dev, 0, 0, p = G_MAINMENU_LEFT_BORDER, G_BFU_FONT_SIZE, bfu_bg_color);
		for (i = 0; menu->items[i].text; i++) {
			int s = i == menu->selected;
			unsigned char *text = _(menu->items[i].text, term);
			if (s) {
				menu->xl1 = p;
				menu->yl1 = 0;
				set_window_ptr(menu->win, p, G_BFU_FONT_SIZE);
			}
			drv->fill_area(dev, p, 0, p + G_MAINMENU_BORDER, G_BFU_FONT_SIZE, s ? bfu_fg_color : bfu_bg_color);
			p += G_MAINMENU_BORDER;
			g_print_text(drv, dev, p, 0, s ? bfu_style_wb : bfu_style_bw, text, &p);
			drv->fill_area(dev, p, 0, p + G_MAINMENU_BORDER, G_BFU_FONT_SIZE, s ? bfu_fg_color : bfu_bg_color);
			p += G_MAINMENU_BORDER;
			if (s) {
				menu->xl2 = p;
				menu->yl2 = G_BFU_FONT_SIZE;
			}
		}
		drv->fill_area(dev, p, 0, term->x, G_BFU_FONT_SIZE, bfu_bg_color);
		menu->ni = i;
/*#endif
	} */
}

void select_mainmenu(struct terminal *term, struct mainmenu *menu)
{
	struct menu_item *it = &menu->items[menu->selected];
	if (menu->selected < 0 || menu->selected >= menu->ni || it->hotkey == M_BAR) return;
	if (!it->in_m) {
		struct window *win;
		for (win = term->windows.next; (void *)win != &term->windows && (win->handler == menu_func || win->handler == mainmenu_func); delete_window(win)) ;
	}
	it->func(term, it->data, menu->data);
}

void mainmenu_func(struct window *win, struct event *ev, int fwd)
{
	int s = 0;
	struct mainmenu *menu = win->data;
	menu->win = win;
	switch(ev->ev) {
		case EV_INIT:
		case EV_RESIZE:
#ifdef G
			if (F) set_window_pos(win, 0, 0, win->term->x, G_BFU_FONT_SIZE);
#endif
		case EV_REDRAW:
			draw_to_window(win, (void (*)(struct terminal *, void *))display_mainmenu, menu);
			break;
		case EV_MOUSE:
			if ((ev->b & BM_ACT) == B_MOVE) break;
			if ((ev->b & BM_ACT) == B_DOWN && ev->y >= gf_val(1, G_BFU_FONT_SIZE)) delete_window_ev(win, ev);
			else if (ev->y < gf_val(1, G_BFU_FONT_SIZE)) {
				int i;
				int p = gf_val(2, G_MAINMENU_LEFT_BORDER);
				for (i = 0; i < menu->ni; i++) {
					int o = p;
					unsigned char *tmptext = _(menu->items[i].text, win->term);
					p += txtlen(tmptext) + gf_val(4, 2 * G_MAINMENU_BORDER);
					if (ev->x >= o && ev->x < p) {
						menu->selected = i;
						draw_to_window(win, (void (*)(struct terminal *, void *))display_mainmenu, menu);
						if ((ev->b & BM_ACT) == B_UP || menu->items[s].in_m) select_mainmenu(win->term, menu);
						break;
					}
				}
			}
			break;
		case EV_KBD:
			if (ev->x == ' ' || ev->x == KBD_ENTER || ev->x == KBD_DOWN || ev->x == KBD_UP || ev->x == KBD_PAGE_DOWN || ev->x == KBD_PAGE_UP) {
				select_mainmenu(win->term, menu);
				break;
			}
			if (ev->x == KBD_LEFT) {
				if (!menu->selected--) menu->selected = menu->ni - 1;
				s = 1;
			}
			if (ev->x == KBD_RIGHT) {
				if (++menu->selected >= menu->ni) menu->selected = 0;
				s = 1;
			}
			if (ev->x == KBD_HOME) {
				menu->selected = 0;
				s = 1;
			}
			if (ev->x == KBD_END) {
				menu->selected = menu->ni - 1;
				s = 1;
			}
			if ((ev->x == KBD_LEFT || ev->x == KBD_RIGHT) && fwd) {
				draw_to_window(win, (void (*)(struct terminal *, void *))display_mainmenu, menu);
				select_mainmenu(win->term, menu);
				break;
			}
			if (ev->x > ' ' && ev->x < 256) {
				int i;
				s = 1;
				for (i = 0; i < menu->ni; i++)
					if (strchr(_(menu->items[i].hotkey, win->term), upcase(ev->x))) {
						menu->selected = i;
						s = 2;
					}
			} else if (!s) {
				delete_window_ev(win, ev->x != KBD_ESC ? ev : NULL);
				break;
			}
			draw_to_window(win, (void (*)(struct terminal *, void *))display_mainmenu, menu);
			if (s == 2) select_mainmenu(win->term, menu);
			break;
		case EV_ABORT:
			break;
	}
}

struct menu_item *new_menu(int free_i)
{
	struct menu_item *mi;
	if (!(mi = mem_alloc(sizeof(struct menu_item)))) return NULL;
	memset(mi, 0, sizeof(struct menu_item));
	mi->free_i = free_i;
	return mi;
}

void add_to_menu(struct menu_item **mi, unsigned char *text, unsigned char *rtext, unsigned char *hotkey, void (*func)(struct terminal *, void *, void *), void *data, int in_m)
{
	struct menu_item *mii;
	int n;
	for (n = 0; (*mi)[n].text; n++) ;
	if (!(mii = mem_realloc(*mi, (n + 2) * sizeof(struct menu_item)))) return;
	*mi = mii;
	memcpy(mii + n + 1, mii + n, sizeof(struct menu_item));
	mii[n].text = text;
	mii[n].rtext = rtext;
	mii[n].hotkey = hotkey;
	mii[n].func = func;
	mii[n].data = data;
	mii[n].in_m = in_m;
}

void do_dialog(struct terminal *term, struct dialog *dlg, struct memory_list *ml)
{
	struct dialog_data *dd;
	struct dialog_item *d;
	int n = 0;
	for (d = dlg->items; d->type != D_END; d++) n++;
	if (!(dd = mem_calloc(sizeof(struct dialog_data) + sizeof(struct dialog_item_data) * n))) return;
	dd->dlg = dlg;
	dd->n = n;
	dd->ml = ml;
	add_window(term, dialog_func, dd);
}

void display_dlg_item(struct dialog_data *dlg, struct dialog_item_data *di, int sel)
{
	struct terminal *term = dlg->win->term;
	if (!F) switch (di->item->type) {
		int co;
		unsigned char *text;
		case D_CHECKBOX:
			if (di->item->gid)
			{
				if (di->checked) print_text(term, di->x, di->y, 3, "[X]", COLOR_DIALOG_CHECKBOX);
				else print_text(term, di->x, di->y, 3, "[ ]", COLOR_DIALOG_CHECKBOX);
			}
			else
			{
				if (di->checked) print_text(term, di->x, di->y, 3, "[X]", COLOR_DIALOG_CHECKBOX);
				else print_text(term, di->x, di->y, 3, "[ ]", COLOR_DIALOG_CHECKBOX);
			}
			if (sel) {
				set_cursor(term, di->x + 1, di->y, di->x + 1, di->y);
				set_window_ptr(dlg->win, di->x, di->y);
			}
			break;
		case D_FIELD:
		case D_FIELD_PASS:
			if (di->vpos + di->l <= di->cpos) di->vpos = di->cpos - di->l + 1;
			if (di->vpos > di->cpos) di->vpos = di->cpos;
			if (di->vpos < 0) di->vpos = 0;
			fill_area(term, di->x, di->y, di->l, 1, COLOR_DIALOG_FIELD);
                        if (di->item->type == D_FIELD) {
                            print_text(term, di->x, di->y, strlen(di->cdata + di->vpos) <= di->l ? strlen(di->cdata + di->vpos) : di->l, di->cdata + di->vpos, COLOR_DIALOG_FIELD_TEXT);
                        } else {
                            fill_area(term, di->x, di->y,
                                      strlen(di->cdata + di->vpos) <= di->l ? strlen(di->cdata + di->vpos) : di->l, 1,
                                      COLOR_DIALOG_FIELD_TEXT | '*');
                        }
			if (sel) {
				set_cursor(term, di->x + di->cpos - di->vpos, di->y, di->x + di->cpos - di->vpos, di->y);
				set_window_ptr(dlg->win, di->x, di->y);
			}
			break;
		case D_BUTTON:
			co = sel ? COLOR_DIALOG_BUTTON_SELECTED : COLOR_DIALOG_BUTTON;
			text = _(di->item->text, term);
			print_text(term, di->x, di->y, 2, "[ ", co);
			print_text(term, di->x + 2, di->y, strlen(text), text, co);
			print_text(term, di->x + 2 + strlen(text), di->y, 2, " ]", co);
			if (sel) {
				set_cursor(term, di->x + 2, di->y, di->x + 2, di->y);
				set_window_ptr(dlg->win, di->x, di->y);
			}
			break;
		default:
			internal("display_dlg_item: unknown item: %d", di->item->type);
#ifdef G
	} else {
		struct rect rr;
		struct graphics_device *dev = term->dev;
		if (!dlg->s) restrict_clip_area(dev, &rr, dlg->rr.x1, dlg->rr.y1, dlg->rr.x2, dlg->rr.y2);
		switch (di->item->type) {
			int p, pp;
			struct style *st;
			unsigned char *text, *text2, *text3,*text_pass;
			struct rect r;
			case D_CHECKBOX:
				p = di->x;
				if (di->checked) {
					if (!sel) g_print_text(drv, dev, di->x, di->y, bfu_style_bw, di->item->gid?(G_DIALOG_RADIO_L G_DIALOG_RADIO_X G_DIALOG_RADIO_R):(G_DIALOG_CHECKBOX_L G_DIALOG_CHECKBOX_X G_DIALOG_CHECKBOX_R), &p);
					else {
						g_print_text(drv, dev, di->x, di->y, bfu_style_bw, di->item->gid?G_DIALOG_RADIO_L:G_DIALOG_CHECKBOX_L, &p);
						g_print_text(drv, dev, p, di->y, bfu_style_bw_u, di->item->gid?G_DIALOG_RADIO_X:G_DIALOG_CHECKBOX_X, &p);
						g_print_text(drv, dev, p, di->y, bfu_style_bw, di->item->gid?G_DIALOG_RADIO_R:G_DIALOG_CHECKBOX_R, &p);
					}
				} else {
					int s = g_text_width(bfu_style_bw, di->item->gid?G_DIALOG_RADIO_X:G_DIALOG_CHECKBOX_X);
					g_print_text(drv, dev, di->x, di->y, bfu_style_bw, di->item->gid?G_DIALOG_RADIO_L:G_DIALOG_CHECKBOX_L, &p);
					if (!sel) drv->fill_area(dev, p, di->y, p + s, di->y + G_BFU_FONT_SIZE, bfu_bg_color), p += s;
					else {
						restrict_clip_area(dev, &r, p, di->y, p + s, di->y + G_BFU_FONT_SIZE);
						g_print_text(drv, dev, p, di->y, bfu_style_bw_u, "          ", NULL);
						p += s;
						drv->set_clip_area(dev, &r);
					}
					g_print_text(drv, dev, p, di->y, bfu_style_bw, di->item->gid?G_DIALOG_RADIO_R:G_DIALOG_CHECKBOX_R, &p);
				}
				di->l = p - di->x;
				if (sel) set_window_ptr(dlg->win, di->x, di->y + G_BFU_FONT_SIZE);
				if (dlg->s) exclude_from_set(&dlg->s, di->x, di->y, p, di->y + G_BFU_FONT_SIZE);
				break;
			case D_FIELD:
			case D_FIELD_PASS:
				if (!(text = memacpy(di->cdata, di->cpos))) break;
                                text_pass=mem_alloc(di->cpos+1);
                                for(pp=0;pp<di->cpos;pp++) text_pass[pp]='*';text_pass[pp]='\0';
                                if (*(text2 = text3 = di->cdata + di->cpos)) {
					GET_UTF_8(text3, p);
					text2 = memacpy(text2, text3 - text2);
				} else {
					text2 = stracpy(" ");
					text3 = "";
				}
				if (!text2) {
					mem_free(text);
					break;
				}
				p = g_text_width(bfu_style_wb_mono, text);
				pp = g_text_width(bfu_style_wb_mono, text2);
				if (di->vpos + di->l < p + pp) di->vpos = p + pp - di->l;
				if (di->vpos > p) di->vpos = p;
				if (di->vpos < 0) di->vpos = 0;

				if (dlg->s) exclude_from_set(&dlg->s, di->x, di->y, di->x + di->l, di->y + G_BFU_FONT_SIZE);
				restrict_clip_area(dev, &r, di->x, di->y, di->x + di->l, di->y + G_BFU_FONT_SIZE);
				p = di->x - di->vpos;
				if (di->item->type == D_FIELD) {
                                    g_print_text(drv, dev, p, di->y, bfu_style_wb_mono, text, &p);
                                } else {
                                    g_print_text(drv, dev, p, di->y, bfu_style_wb_mono, text_pass, &p);
                                }
                                mem_free(text_pass);
                                g_print_text(drv, dev, p, di->y, sel ? bfu_style_wb_mono_u : bfu_style_wb_mono, text2, &p);
				g_print_text(drv, dev, p, di->y, bfu_style_wb_mono, text3, &p);
				drv->fill_area(dev, p, di->y, di->x + di->l, di->y + G_BFU_FONT_SIZE, bfu_fg_color);
				drv->set_clip_area(dev, &r);
				mem_free(text);
				mem_free(text2);
				if (sel) {
					set_window_ptr(dlg->win, di->x, di->y);
				}

				break;
			case D_BUTTON:
//				st = sel ? bfu_style_wb_b : bfu_style_bw;
				st = sel ? bfu_style_wb : bfu_style_bw;
                                text = _(di->item->text, term);
				if ((text2 = mem_alloc(strlen(text) + 5))) {
					strcpy(text2, G_DIALOG_BUTTON_L);
					strcpy(text2 + 2, text);
					strcat(text2, G_DIALOG_BUTTON_R);
					di->l = 0;
					g_print_text(drv, dev, di->x, di->y, st, text2, &di->l);
					mem_free(text2);
					if (dlg->s) exclude_from_set(&dlg->s, di->x, di->y, di->x + di->l, di->y + G_BFU_FONT_SIZE);
				}
				if (sel) set_window_ptr(dlg->win, di->x, di->y + G_BFU_FONT_SIZE);
				break;
			default:
				internal("display_dlg_item: unknown item: %d", di->item->type);
		}
		if (!dlg->s) drv->set_clip_area(dev, &rr);
#endif
	}
}

struct dspd {
	struct dialog_data *dlg;
	struct dialog_item_data *di;
	int sel;
};

void u_display_dlg_item(struct terminal *term, void *p)
{
	struct dspd *d = p;
	display_dlg_item(d->dlg, d->di, d->sel);
}

void x_display_dlg_item(struct dialog_data *dlg, struct dialog_item_data *di, int sel)
{
	struct dspd dspd;
	dspd.dlg = dlg, dspd.di = di, dspd.sel = sel;
	draw_to_window(dlg->win, u_display_dlg_item, &dspd);
}

void dlg_select_item(struct dialog_data *dlg, struct dialog_item_data *di)
{
	if (di->item->type == D_CHECKBOX) {
		if (!di->item->gid) di -> checked = *(int *)di->cdata = !*(int *)di->cdata;
		else {
			int i;
			for (i = 0; i < dlg->n; i++) {
				if (dlg->items[i].item->type == D_CHECKBOX && dlg->items[i].item->gid == di->item->gid) {
					*(int *)dlg->items[i].cdata = di->item->gnum;
					dlg->items[i].checked = 0;
					x_display_dlg_item(dlg, &dlg->items[i], 0);
				}
			}
			di->checked = 1;
		}
		x_display_dlg_item(dlg, di, 1);
	}
	else if (di->item->type == D_BUTTON) di->item->fn(dlg, di);
}

void dlg_set_history(struct dialog_item_data *di)
{
	unsigned char *s = "";
	int l;
	if ((void *)di->cur_hist != &di->history) s = di->cur_hist->d;
	if ((l = strlen(s)) > di->item->dlen) l = di->item->dlen - 1;
	memcpy(di->cdata, s, l);
	di->cdata[l] = 0;
	di->cpos = l;
}

int dlg_mouse(struct dialog_data *dlg, struct dialog_item_data *di, struct event *ev)
{
	switch (di->item->type) {
		case D_BUTTON:
			if (gf_val(ev->y != di->y, ev->y < di->y || ev->y >= di->y + G_BFU_FONT_SIZE) || ev->x < di->x || ev->x >= di->x + gf_val(strlen(_(di->item->text, dlg->win->term)) + 4, di->l)) return 0;
			if (dlg->selected != di - dlg->items) {
				x_display_dlg_item(dlg, &dlg->items[dlg->selected], 0);
				dlg->selected = di - dlg->items;
				x_display_dlg_item(dlg, di, 1);
			}
			if ((ev->b & BM_ACT) == B_UP) dlg_select_item(dlg, di);
			return 1;
		case D_FIELD:
		case D_FIELD_PASS:
			if (gf_val(ev->y != di->y, ev->y < di->y || ev->y >= di->y + G_BFU_FONT_SIZE) || ev->x < di->x || ev->x >= di->x + di->l) return 0;
			if (!F) {
				if ((di->cpos = di->vpos + ev->x - di->x) > strlen(di->cdata)) di->cpos = strlen(di->cdata);
#ifdef G
			} else {
				int p, u;
				unsigned char *t = di->cdata;
				p = di->x - di->vpos;
				while (1) {
					di->cpos = t - di->cdata;
					if (!*t) break;
					GET_UTF_8(t, u);
					if (!u) continue;
					p += g_char_width(bfu_style_wb_mono, u);
					if (p > ev->x) break;
				}
#endif
			}
			if (dlg->selected != di - dlg->items) {
				x_display_dlg_item(dlg, &dlg->items[dlg->selected], 0);
				dlg->selected = di - dlg->items;
				x_display_dlg_item(dlg, di, 1);
			} else x_display_dlg_item(dlg, di, 1);
			return 1;
		case D_CHECKBOX:
			if (gf_val(ev->y != di->y, ev->y < di->y || ev->y >= di->y + G_BFU_FONT_SIZE) || ev->x < di->x || ev->x >= di->x + gf_val(3, di->l)) return 0;
			if (dlg->selected != di - dlg->items) {
				x_display_dlg_item(dlg, &dlg->items[dlg->selected], 0);
				dlg->selected = di - dlg->items;
				x_display_dlg_item(dlg, di, 1);
			}
			if ((ev->b & BM_ACT) == B_UP) dlg_select_item(dlg, di);
			return 1;
	}
	return 0;
}

void redraw_dialog_items(struct terminal *term, struct dialog_data *dlg)
{
	int i;
	for (i = 0; i < dlg->n; i++) display_dlg_item(dlg, &dlg->items[i], i == dlg->selected);
}

void redraw_dialog(struct terminal *term, struct dialog_data *dlg)
{
#ifdef G
	int i;
#endif
	dlg->dlg->fn(dlg);
	redraw_dialog_items(term, dlg);
#ifdef G
	if (F) {
		drv->set_clip_area(term->dev, &dlg->r);
		for (i = 0; i < dlg->s->m; i++) if (is_rect_valid(&dlg->s->r[i]))
			drv->fill_area(term->dev, dlg->s->r[i].x1, dlg->s->r[i].y1, dlg->s->r[i].x2, dlg->s->r[i].y2, bfu_bg_color);
		mem_free(dlg->s);
		dlg->s = NULL;
	}
#endif
}

void tab_compl(struct terminal *term, unsigned char *item, struct window *win)
{
	struct event ev = {EV_REDRAW, 0, 0, 0};
	struct dialog_item_data *di = &((struct dialog_data*)win->data)->items[((struct dialog_data*)win->data)->selected];
	int l = strlen(item);
	if (l >= di->item->dlen) l = di->item->dlen - 1;
	memcpy(di->cdata, item, l);
	di->cdata[l] = 0;
	di->cpos = l;
	di->vpos = 0;
	ev.x = term->x;
	ev.y = term->y;
	dialog_func(win, &ev, 0);
}

void do_tab_compl(struct terminal *term, struct list_head *history, struct window *win)
{
	unsigned char *cdata = ((struct dialog_data*)win->data)->items[((struct dialog_data*)win->data)->selected].cdata;
	int l = strlen(cdata), n = 0;
	struct history_item *hi;
	struct menu_item *items = DUMMY, *i;
	foreach(hi, *history) if (!strncmp(cdata, hi->d, l)) {
		if (!(n & (ALLOC_GR - 1))) {
			if (!(i = mem_realloc(items, (n + ALLOC_GR + 1) * sizeof(struct menu_item)))) {
				mem_free(items);
				return;
			}
			items = i;
		}
		items[n].text = hi->d;
		items[n].rtext = "";
		items[n].hotkey = "";
		items[n].func = (void(*)(struct terminal *, void *, void *))tab_compl;
		items[n].rtext = "";
		items[n].data = hi->d;
		items[n].in_m = 0;
		items[n].free_i = 1;
		n++;
	}
	if (n == 1) {
		tab_compl(term, items->data, win);
		mem_free(items);
		return;
	}
	if (n) {
		memset(&items[n], 0, sizeof(struct menu_item));
		do_menu_selected(term, items, win, n - 1);
	}
}

void dialog_func(struct window *win, struct event *ev, int fwd)
{
	int i;
	struct terminal *term = win->term;
	struct dialog_data *dlg = win->data;
	struct dialog_item_data *di;

	dlg->win = win;

	/* Use nonstandard event handlers */
	if (dlg->dlg->handle_event && ((dlg->dlg->handle_event)(dlg, ev) == EVENT_PROCESSED) ) {
		return;
	}
	
	switch (ev->ev) {
		case EV_INIT:
			for (i = 0; i < dlg->n; i++) {
				struct dialog_item_data *di = &dlg->items[i];
				memset(di, 0, sizeof(struct dialog_item_data));
				di->item = &dlg->dlg->items[i];
				if ((di->cdata = mem_alloc(di->item->dlen)))
					memcpy(di->cdata, di->item->data, di->item->dlen);
				if (di->item->type == D_CHECKBOX) {
					if (di->item->gid) {
						if (*(int *)di->cdata == di->item->gnum) di->checked = 1;
					} else if (*(int *)di->cdata) di->checked = 1;
				} 
				init_list(di->history);
				di->cur_hist = (struct history_item *)&di->history;
				if (di->item->type == D_FIELD || di->item->type == D_FIELD_PASS) {
					if (di->item->history) {
						struct history_item *j;
						/*int l = di->item->dlen;*/
						foreach(j, di->item->history->items) {
							struct history_item *hi;
							if (!(hi = mem_alloc(sizeof(struct history_item) + strlen(j->d) + 1))) continue;
							strcpy(hi->d, j->d);
							add_to_list(di->history, hi);
						}
					}
					di->cpos = strlen(di->cdata);
				}
			}
			dlg->selected = 0;
		case EV_RESIZE:
				/* this must be really called twice !!! */
			draw_to_window(dlg->win, (void (*)(struct terminal *, void *))redraw_dialog, dlg);
		case EV_REDRAW:
			draw_to_window(dlg->win, (void (*)(struct terminal *, void *))redraw_dialog, dlg);
			break;
		case EV_MOUSE:
			if ((ev->b & BM_ACT) == B_MOVE) break;
			for (i = 0; i < dlg->n; i++) if (dlg_mouse(dlg, &dlg->items[i], ev)) break;
#ifdef G
                        if ( F && (ev->b & BM_ACT) == B_DOWN && (ev->b & BM_BUTT) == B_MIDDLE)
                                drv->request_clipboard(term->dev);
#endif
                        break;
		case EV_KBD:
			di = &dlg->items[dlg->selected];
			if (di->item->type == D_FIELD || di->item->type == D_FIELD_PASS) {
				if (ev->x == KBD_UP && (void *)di->cur_hist->prev != &di->history) {
					di->cur_hist = di->cur_hist->prev;
					dlg_set_history(di);
					goto dsp_f;
				}
				if (ev->x == KBD_DOWN && (void *)di->cur_hist != &di->history) {
					di->cur_hist = di->cur_hist->next;
					dlg_set_history(di);
					goto dsp_f;
				}
				if (ev->x == KBD_RIGHT) {
					if (di->cpos < strlen(di->cdata)) {
						if (!F) di->cpos++;
#ifdef G
						else {
							int u;
							unsigned char *p = di->cdata + di->cpos;
							GET_UTF_8(p, u);
							di->cpos = p - di->cdata;
						}
#endif
					}
					goto dsp_f;
				}
				if (ev->x == KBD_LEFT) {
					if (di->cpos > 0) {
						if (!F) di->cpos--;
#ifdef G
						else {
							unsigned char *p = di->cdata + di->cpos;
							BACK_UTF_8(p, di->cdata);
							di->cpos = p - di->cdata;
						}
#endif
					}
					goto dsp_f;
				}
				if (ev->x == KBD_HOME) {
					di->cpos = 0;
					goto dsp_f;
				}
				if (ev->x == KBD_END) {
					di->cpos = strlen(di->cdata);
					goto dsp_f;
				}
				if (ev->x >= ' ' && !ev->y) {
					unsigned char *u;
					unsigned char p[2] = { 0, 0 };
					if (!F) p[0] = ev->x, u = p;
#ifdef G
					else {
						u = encode_utf_8(ev->x);
					}
#endif
					if (strlen(di->cdata) < di->item->dlen - strlen(u)) {
						memmove(di->cdata + di->cpos + strlen(u), di->cdata + di->cpos, strlen(di->cdata) - di->cpos + 1);
						memcpy(&di->cdata[di->cpos], u, strlen(u));
						di->cpos += strlen(u);
					}
					goto dsp_f;
				}
                                if (ev->x == KBD_BS) {
                                        if (di->cpos) {
                                                int delta=1;
#ifdef G
                                                unsigned char *p = di->cdata + di->cpos;
                                                BACK_UTF_8(p, di->cdata);
                                                delta = di->cdata+di->cpos-p;
#endif
						memmove(di->cdata + di->cpos - delta, di->cdata + di->cpos, strlen(di->cdata) - di->cpos + delta);
						di->cpos-=delta;
					}
					goto dsp_f;
				}
				if (ev->x == KBD_DEL) {
                                        if (di->cpos < strlen(di->cdata)){
                                                int delta=1;
#ifdef G
                                                unsigned char *p = di->cdata + di->cpos;
                                                FWD_UTF_8(p);
                                                delta = p-di->cdata-di->cpos;
#endif
                                                memmove(di->cdata + di->cpos, di->cdata + di->cpos + delta, strlen(di->cdata) - di->cpos + delta);
                                        }
                                        goto dsp_f;
				}
				if (upcase(ev->x) == 'U' && ev->y == KBD_CTRL) {
					memmove(di->cdata, di->cdata + di->cpos, strlen(di->cdata + di->cpos) + 1);
					di->cpos = 0;
					goto dsp_f;
				}
				/* Copy to clipboard */
                                if ((ev->x == KBD_INS && ev->y == KBD_CTRL) ||
                                    (upcase(ev->x) == 'Z' && ev->y == KBD_CTRL)) {
                                        struct conv_table *conv = get_translation_table(get_cp_index("UTF-8"),get_cp_index(options_get("text_selection_clipboard_charset")));
                                        unsigned char *txt=convert_string(conv, di->cdata, strlen(di->cdata), NULL);
                                        if (!F)
                                                set_clipboard_text(txt);
#ifdef G
                                        else
                                                drv->put_to_clipboard(term->dev,txt,strlen(txt));
#endif
                                        mem_free(txt);
                                        break;	/* We don't need to redraw */
				}
				/* FIXME -- why keyboard shortcuts with shift don't works??? */
				/* Cut to clipboard */
                                if ((ev->x == KBD_DEL && ev->y == KBD_SHIFT) ||
                                    (upcase(ev->x) == 'X' && ev->y == KBD_CTRL)) {
                                        struct conv_table *conv = get_translation_table(get_cp_index("UTF-8"),get_cp_index(options_get("text_selection_clipboard_charset")));
                                        unsigned char *txt = convert_string(conv, di->cdata, strlen(di->cdata), NULL);

                                        if (!F)
                                                set_clipboard_text(txt);
#ifdef G
                                        else
                                                drv->put_to_clipboard(term->dev,txt,strlen(txt));
#endif
                                        mem_free(txt);
					di->cdata[0] = 0;
					di->cpos = 0;
					goto dsp_f;
                                }
				/* Paste from clipboard */
                                if ((ev->x == KBD_INS && ev->y == KBD_SHIFT) ||
                                    (upcase(ev->x) == 'V' && ev->y == KBD_CTRL)) {
                                        if(!F)
                                                goto paste;
#ifdef G
                                        else
                                                drv->request_clipboard(term->dev);
#endif
                                }
                                if ( F && ev->x == KBD_PASTE ) {
                                        struct conv_table *conv = get_translation_table(get_cp_index(options_get("text_selection_clipboard_charset")),get_cp_index("UTF-8"));
                                        unsigned char *text;
                                        unsigned char *clipboard;

                                paste:
                                        if(!F)
                                                text = get_clipboard_text();
#ifdef G
                                        else
                                                text = drv->get_from_clipboard(term->dev);
#endif
                                        clipboard = convert_string(conv, text, strlen(text), NULL);
                                        mem_free(text);

                                        if (clipboard) {
                                            int max_len = di->item->dlen;
                                            int orig_index = di->cpos;
                                            int added_len = strlen(clipboard);

                                            if(max_len - orig_index - added_len > 0)
                                                memmove(di->cdata + orig_index + added_len, di->cdata + orig_index, max_len - orig_index - added_len);
                                            memcpy(di->cdata + orig_index, clipboard,(max_len-orig_index > added_len)?added_len:(max_len-orig_index));
                                            di->cdata[max_len - 1] = 0;
                                            di->cpos = orig_index + added_len;
                                            mem_free(clipboard);
                                        }
					goto dsp_f;
                                }
				if (upcase(ev->x) == 'W' && ev->y & KBD_CTRL) {
					do_tab_compl(term, &di->history, win);
					goto dsp_f;
				}
				goto gh;
				dsp_f:
				x_display_dlg_item(dlg, di, 1);
				break;
			}
			if ((ev->x == KBD_ENTER && di->item->type == D_BUTTON) || ev->x == ' ') {
				dlg_select_item(dlg, di);
				break;
			}
			gh:
			if (ev->x > ' ') for (i = 0; i < dlg->n; i++)
				if (dlg->dlg->items[i].type == D_BUTTON && upcase(_(dlg->dlg->items[i].text, term)[0]) == upcase(ev->x)) {
					sel:
					if (dlg->selected != i) {
						x_display_dlg_item(dlg, &dlg->items[dlg->selected], 0);
						x_display_dlg_item(dlg, &dlg->items[i], 1);
						dlg->selected = i;
					}
					dlg_select_item(dlg, &dlg->items[i]);
					goto bla;
			}
			if (ev->x == KBD_ENTER) for (i = 0; i < dlg->n; i++)
				if (dlg->dlg->items[i].type == D_BUTTON && dlg->dlg->items[i].gid & B_ENTER) goto sel;
			if (ev->x == KBD_ESC) for (i = 0; i < dlg->n; i++)
				if (dlg->dlg->items[i].type == D_BUTTON && dlg->dlg->items[i].gid & B_ESC) goto sel;
			if (((ev->x == KBD_TAB && !ev->y) || ev->x == KBD_DOWN || ev->x == KBD_RIGHT) && dlg->n > 1) {
				x_display_dlg_item(dlg, &dlg->items[dlg->selected], 0);
				if ((++dlg->selected) >= dlg->n) dlg->selected = 0;
				x_display_dlg_item(dlg, &dlg->items[dlg->selected], 1);
				break;
			}
			if (((ev->x == KBD_TAB && ev->y) || ev->x == KBD_UP || ev->x == KBD_LEFT) && dlg->n > 1) {
				x_display_dlg_item(dlg, &dlg->items[dlg->selected], 0);
				if ((--dlg->selected) < 0) dlg->selected = dlg->n - 1;
				x_display_dlg_item(dlg, &dlg->items[dlg->selected], 1);
				break;
			}
			break;
		case EV_ABORT:
		/* Moved this line up so that the dlg would have access to its 
		   member vars before they get freed. */ 
			if (dlg->dlg->abort) dlg->dlg->abort(dlg);
			for (i = 0; i < dlg->n; i++) {
				struct dialog_item_data *di = &dlg->items[i];
				if (di->cdata) mem_free(di->cdata);
				free_list(di->history);
			}
			freeml(dlg->ml);
	}
	bla:;
}

/* gid and gnum are 100 times greater than boundaries (e.g. if gid==1 boundary is 0.01) */
int check_float(struct dialog_data *dlg, struct dialog_item_data *di)
{
	unsigned char *end;
	double d = strtod(di->cdata, (char **)&end);
	if (!*di->cdata || *end) {
		msg_box(dlg->win->term, NULL, TXT(T_BAD_NUMBER), AL_CENTER, TXT(T_NUMBER_EXPECTED), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		return 1;
	}
	if (100*d < di->item->gid || 100*d > di->item->gnum) {
		msg_box(dlg->win->term, NULL, TXT(T_BAD_NUMBER), AL_CENTER, TXT(T_NUMBER_OUT_OF_RANGE), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		return 1;
	}
	return 0;
}

int check_number(struct dialog_data *dlg, struct dialog_item_data *di)
{
	unsigned char *end;
	long l = strtol(di->cdata, (char **)&end, 10);
	if (!*di->cdata || *end) {
		msg_box(dlg->win->term, NULL, TXT(T_BAD_NUMBER), AL_CENTER, TXT(T_NUMBER_EXPECTED), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		return 1;
	}
	if (l < di->item->gid || l > di->item->gnum) {
		msg_box(dlg->win->term, NULL, TXT(T_BAD_NUMBER), AL_CENTER, TXT(T_NUMBER_OUT_OF_RANGE), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		return 1;
	}
	return 0;
}

int check_hex_number(struct dialog_data *dlg, struct dialog_item_data *di)
{
	unsigned char *end;
	long l = strtol(di->cdata, (char **)&end, 16);
	if (!*di->cdata || *end) {
		msg_box(dlg->win->term, NULL, TXT(T_BAD_NUMBER), AL_CENTER, TXT(T_NUMBER_EXPECTED), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		return 1;
	}
	if (l < di->item->gid || l > di->item->gnum) {
		msg_box(dlg->win->term, NULL, TXT(T_BAD_NUMBER), AL_CENTER, TXT(T_NUMBER_OUT_OF_RANGE), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		return 1;
	}
	return 0;
}

int check_nonempty(struct dialog_data *dlg, struct dialog_item_data *di)
{
	unsigned char *p;
	for (p = di->cdata; *p; p++) if (*p > ' ') return 0;
	msg_box(dlg->win->term, NULL, TXT(T_BAD_STRING), AL_CENTER, TXT(T_EMPTY_STRING_NOT_ALLOWED), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
	return 1;
}

int cancel_dialog(struct dialog_data *dlg, struct dialog_item_data *di)
{
	delete_window(dlg->win);
	return 0;
}

int check_dialog(struct dialog_data *dlg)
{
	int i;
	for (i = 0; i < dlg->n; i++)
		if (dlg->dlg->items[i].type == D_CHECKBOX || dlg->dlg->items[i].type == D_FIELD || dlg->dlg->items[i].type == D_FIELD_PASS)
			if (dlg->dlg->items[i].fn && dlg->dlg->items[i].fn(dlg, &dlg->items[i])) {
				dlg->selected = i;
				draw_to_window(dlg->win, (void (*)(struct terminal *, void *))redraw_dialog_items, dlg);
				return 1;
			}
	return 0;
}

int ok_dialog(struct dialog_data *dlg, struct dialog_item_data *di)
{
	int i;
	void (*fn)(void *) = dlg->dlg->refresh;
	void *data = dlg->dlg->refresh_data;
	if (check_dialog(dlg)) return 1;
	for (i = 0; i < dlg->n; i++)
		memcpy(dlg->dlg->items[i].data, dlg->items[i].cdata, dlg->dlg->items[i].dlen);
	if (fn) fn(data);
	i = cancel_dialog(dlg, di);
	return i;
}

void center_dlg(struct dialog_data *dlg)
{
	dlg->x = (dlg->win->term->x - dlg->xw) / 2;
	dlg->y = (dlg->win->term->y - dlg->yw) / 2;
}

void draw_dlg(struct dialog_data *dlg)
{
	if (!F) {
		int i;
		struct terminal *term = dlg->win->term;
		fill_area(term, dlg->x, dlg->y, dlg->xw, dlg->yw, COLOR_DIALOG);
		draw_frame(term, dlg->x + DIALOG_LEFT_BORDER, dlg->y + DIALOG_TOP_BORDER, dlg->xw - 2 * DIALOG_LEFT_BORDER, dlg->yw - 2 * DIALOG_TOP_BORDER, COLOR_DIALOG_FRAME, DIALOG_FRAME);
		i = strlen(_(dlg->dlg->title, term));
		print_text(term, (dlg->xw - i) / 2 + dlg->x - 1, dlg->y + DIALOG_TOP_BORDER, 1, " ", COLOR_DIALOG_TITLE);
		print_text(term, (dlg->xw - i) / 2 + dlg->x, dlg->y + DIALOG_TOP_BORDER, i, _(dlg->dlg->title, term), COLOR_DIALOG_TITLE);
		print_text(term, (dlg->xw - i) / 2 + dlg->x + i, dlg->y + DIALOG_TOP_BORDER, 1, " ", COLOR_DIALOG_TITLE);
#ifdef G
	} else {
		struct graphics_device *dev = dlg->win->term->dev;
		struct rect r;
		struct rect rt;
		unsigned char *text = _(dlg->dlg->title, dlg->win->term);
//		int xtl = txtlen(text);
                int xtl = g_text_width(bfu_style_wb, text);
                int tl = xtl + 2 * G_DIALOG_TITLE_BORDER;
		int TEXT_X, TEXT_Y;
		if (tl > dlg->xw - 2 * G_DIALOG_LEFT_BORDER - 2 * G_DIALOG_VLINE_SPACE) tl = dlg->xw - 2 * G_DIALOG_LEFT_BORDER - 2 * G_DIALOG_VLINE_SPACE;
		TEXT_X = dlg->x + (dlg->xw - tl) / 2;
		TEXT_Y = dlg->y + G_DIALOG_TOP_BORDER + (G_DIALOG_HLINE_SPACE + 1) / 2 - G_BFU_FONT_SIZE / 2;
		if (TEXT_Y < dlg->y) TEXT_Y = dlg->y;
		set_window_pos(dlg->win, dlg->x, dlg->y, dlg->x + dlg->xw + G_SHADOW_WIDTH, dlg->y + dlg->yw + G_SHADOW_HEIGHT);

		restrict_clip_area(dev, &r, TEXT_X, TEXT_Y, TEXT_X + tl, TEXT_Y + G_BFU_FONT_SIZE);
		rt.x1 = TEXT_X;
		rt.x2 = TEXT_X + tl;
		rt.y1 = TEXT_Y;
		rt.y2 = TEXT_Y + G_BFU_FONT_SIZE;
		if (xtl > tl) g_print_text(drv, dev, TEXT_X, TEXT_Y, bfu_style_wb, text, NULL);
		else {
			drv->fill_area(dev, TEXT_X, TEXT_Y, TEXT_X + (tl - xtl) / 2, TEXT_Y + G_BFU_FONT_SIZE, bfu_fg_color);
			g_print_text(drv, dev, TEXT_X + (tl - xtl) / 2, TEXT_Y, bfu_style_wb, text, NULL);
			drv->fill_area(dev, TEXT_X + (tl - xtl) / 2 + xtl, TEXT_Y, TEXT_X + tl, TEXT_Y + G_BFU_FONT_SIZE, bfu_fg_color);
		}
		drv->set_clip_area(dev, &r);

		drv->draw_hline(dev, dlg->x + G_DIALOG_LEFT_BORDER, dlg->y + G_DIALOG_TOP_BORDER, TEXT_X, bfu_fg_color);
		drv->draw_hline(dev, dlg->x + G_DIALOG_LEFT_BORDER + G_DIALOG_VLINE_SPACE, dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE, TEXT_X, bfu_fg_color);
		drv->draw_hline(dev, TEXT_X + tl, dlg->y + G_DIALOG_TOP_BORDER, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER, bfu_fg_color);
		drv->draw_hline(dev, TEXT_X + tl, dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - G_DIALOG_VLINE_SPACE, bfu_fg_color);
		drv->draw_hline(dev, dlg->x + G_DIALOG_LEFT_BORDER, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - 1, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER, bfu_fg_color);
		drv->draw_hline(dev, dlg->x + G_DIALOG_LEFT_BORDER + G_DIALOG_VLINE_SPACE, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - G_DIALOG_HLINE_SPACE - 1, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - G_DIALOG_VLINE_SPACE, bfu_fg_color);

		drv->draw_vline(dev, dlg->x + G_DIALOG_LEFT_BORDER, dlg->y + G_DIALOG_TOP_BORDER + 1, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - 1, bfu_fg_color);
		drv->draw_vline(dev, dlg->x + G_DIALOG_LEFT_BORDER + G_DIALOG_VLINE_SPACE, dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE + 1, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - G_DIALOG_HLINE_SPACE - 1, bfu_fg_color);
		drv->draw_vline(dev, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - 1, dlg->y + G_DIALOG_TOP_BORDER + 1, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - 1, bfu_fg_color);
		drv->draw_vline(dev, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - G_DIALOG_VLINE_SPACE - 1, dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE + 1, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - G_DIALOG_HLINE_SPACE - 1, bfu_fg_color);

		drv->fill_area(dev, dlg->x, dlg->y, TEXT_X, dlg->y + G_DIALOG_TOP_BORDER, bfu_bg_color);
		drv->fill_area(dev, TEXT_X, dlg->y, TEXT_X + tl, TEXT_Y, bfu_bg_color);
		drv->fill_area(dev, TEXT_X + tl, dlg->y, dlg->x + dlg->xw, dlg->y + G_DIALOG_TOP_BORDER, bfu_bg_color);
		drv->fill_area(dev, dlg->x, dlg->y + G_DIALOG_TOP_BORDER, dlg->x + G_DIALOG_LEFT_BORDER, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER, bfu_bg_color);
		drv->fill_area(dev, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER, dlg->y + G_DIALOG_TOP_BORDER, dlg->x + dlg->xw, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER, bfu_bg_color);
		drv->fill_area(dev, dlg->x, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER, dlg->x + dlg->xw, dlg->y + dlg->yw, bfu_bg_color);

		drv->fill_area(dev, dlg->x + G_DIALOG_LEFT_BORDER + 1, dlg->y + G_DIALOG_TOP_BORDER + 1, TEXT_X, dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE, bfu_bg_color);
		drv->fill_area(dev, TEXT_X + tl, dlg->y + G_DIALOG_TOP_BORDER + 1, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - 1, dlg->y + G_DIALOG_TOP_BORDER + + G_DIALOG_HLINE_SPACE, bfu_bg_color);
		drv->fill_area(dev, dlg->x + G_DIALOG_LEFT_BORDER + 1, dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE, dlg->x + G_DIALOG_LEFT_BORDER + G_DIALOG_VLINE_SPACE, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - G_DIALOG_HLINE_SPACE, bfu_bg_color);
		drv->fill_area(dev, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - G_DIALOG_VLINE_SPACE, dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - 1, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - G_DIALOG_HLINE_SPACE, bfu_bg_color);
		drv->fill_area(dev, dlg->x + G_DIALOG_LEFT_BORDER + 1, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - G_DIALOG_HLINE_SPACE, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - 1, dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - 1, bfu_bg_color);

                /* Simple dialog borders and shadows */
                drv->draw_hline(dev, dlg->x, dlg->y, dlg->x+dlg->xw, bfu_fg_color);
                drv->draw_hline(dev, dlg->x, dlg->y+dlg->yw, dlg->x+dlg->xw, bfu_fg_color);
                drv->draw_vline(dev, dlg->x, dlg->y, dlg->y+dlg->yw, bfu_fg_color);
                drv->draw_vline(dev, dlg->x+dlg->xw, dlg->y, dlg->y+dlg->yw, bfu_fg_color);
                drv->fill_area(dev, dlg->x+G_SHADOW_WIDTH, dlg->y + dlg->yw, dlg->x + dlg->xw+G_SHADOW_WIDTH, dlg->y + dlg->yw+G_SHADOW_HEIGHT, bfu_shadow_color);
		drv->fill_area(dev, dlg->x+dlg->xw, dlg->y + G_SHADOW_HEIGHT, dlg->x + dlg->xw+G_SHADOW_WIDTH, dlg->y + dlg->yw+G_SHADOW_HEIGHT, bfu_shadow_color);

		/*
		drv->fill_area(dev, dlg->x + G_DIALOG_LEFT_BORDER + G_DIALOG_VLINE_SPACE + 1, dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE + 1, TEXT_X, TEXT_Y + G_BFU_FONT_SIZE, bfu_bg_color);
		drv->fill_area(dev, TEXT_X + tl, dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE + 1, dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - G_DIALOG_VLINE_SPACE - 1, TEXT_Y + G_BFU_FONT_SIZE, bfu_bg_color);
		*/

                dlg->s = init_rect_set();
		dlg->rr.x1 = dlg->x + G_DIALOG_LEFT_BORDER + G_DIALOG_VLINE_SPACE + 1;
		dlg->rr.x2 = dlg->x + dlg->xw - G_DIALOG_LEFT_BORDER - G_DIALOG_VLINE_SPACE - 1;
		dlg->rr.y1 = dlg->y + G_DIALOG_TOP_BORDER + G_DIALOG_HLINE_SPACE + 1;
		dlg->rr.y2 = dlg->y + dlg->yw - G_DIALOG_TOP_BORDER - G_DIALOG_HLINE_SPACE - 1;
		add_to_rect_set(&dlg->s, &dlg->rr);
		exclude_rect_from_set(&dlg->s, &rt);
		restrict_clip_area(dev, &dlg->r, dlg->rr.x1, dlg->rr.y1, dlg->rr.x2, dlg->rr.y2);

#endif
	}
}

void max_text_width(struct terminal *term, unsigned char *text, int *width, int align)
{
	text = _(text, term);
	do {
		int c = 0;
		while (*text && *text != '\n') {
			if (!F) text++, c++;
#ifdef G
			else {
				int u;
				GET_UTF_8(text, u);
				c += g_char_width(align & AL_MONO ? bfu_style_wb_mono : bfu_style_wb, u);
			}
#endif
		}
		if (c > *width) *width = c;
	} while (*(text++));
}

void min_text_width(struct terminal *term, unsigned char *text, int *width, int align)
{
	text = _(text, term);
	do {
		int c = 0;
		while (*text && *text != '\n' && *text != ' ') {
			if (!F) text++, c++;
#ifdef G
			else {
				int u;
				GET_UTF_8(text, u);
				c += g_char_width(align & AL_MONO ? bfu_style_wb_mono : bfu_style_wb, u);
			}
#endif
		}
		if (c > *width) *width = c;
	} while (*(text++));
}

void dlg_format_text(struct dialog_data *dlg, struct terminal *term, unsigned char *text, int x, int *y, int w, int *rw, int co, int align)
{
#ifdef G
	unsigned char *tx2;
#endif
	text = _(text, dlg->win->term);
	if (!F) do {
		unsigned char *tx;
		unsigned char *tt = text;
		int s;
		int xx = x;
		do {
			while (*text && *text != '\n' && *text != ' ') {
				text++, xx++;
			}
			tx = ++text;
			xx++;
			if (*(text - 1) != ' ') break;
			while (*tx && *tx != '\n' && *tx != ' ') tx++;
		} while (tx - text + xx - x <= w);
		s = (align & AL_MASK) == AL_CENTER ? (w - (xx - 1 - x)) / 2 : 0;
		if (s < 0) s = 0;
		while (tt < text - 1) {
			if (s >= w) {
				s = 0, (*y)++;
				if (rw) *rw = w;
				rw = NULL;
			}
			if (term) set_char(term, x + s, *y, co | *tt);
			s++, tt++;
		}
		if (rw && xx - 1 - x > *rw) *rw = xx - 1 - x;
		(*y)++;
	} while (*(text - 1));
#ifdef G
	else if ((tx2 = strchr(text, '\n'))) {
		unsigned char *txt = stracpy(text);
		unsigned char *tx1 = txt;
		tx2 += txt - text;
		do {
			*tx2 = 0;
			dlg_format_text(dlg, term, tx1, x, y, w, rw, co, align);
			tx1 = tx2 + 1;
		} while ((tx2 = strchr(tx1, '\n')));
		dlg_format_text(dlg, term, tx1, x, y, w, rw, co, align);
		mem_free(txt);
	} else {
		int www;
		unsigned char *txt;
		struct wrap_struct ww;
		int r;
		ww.style = align & AL_MONO ? bfu_style_bw_mono : bfu_style_bw;
		ww.width = w;
		new_ln:
		ww.text = text;
		ww.pos = 0;
		ww.last_wrap = NULL;
		ww.last_wrap_obj = NULL;
		r = g_wrap_text(&ww);
		if (!r) {
			txt = memacpy(text, ww.last_wrap - text);
			www = g_text_width(ww.style, txt);
			if (!term) mem_free(txt);
			text = ww.last_wrap;
			if (*text == ' ') text++;
			else if (term) add_to_strn(&txt, "-");
		} else {
			www = ww.pos;
			txt = text;
		}
		if (term) {
			int xx = (align & AL_MASK) == AL_CENTER ? x + (w - www) / 2 : x;
			g_print_text(drv, dlg->win->term->dev, xx, *y, ww.style, txt, NULL);
			if (dlg->s) exclude_from_set(&dlg->s, xx, *y, xx + www, *y + G_BFU_FONT_SIZE);
			if (!r) mem_free(txt);
		}
		if (www > w) www = w;
		if (rw && www > *rw) *rw = www;
		*y += G_BFU_FONT_SIZE;
		if (!r) goto new_ln;
	}
#endif
}

void max_buttons_width(struct terminal *term, struct dialog_item_data *butt, int n, int *width)
{
	int w = gf_val(-2, -G_DIALOG_BUTTON_SPACE);
	int i;
	for (i = 0; i < n; i++) w += txtlen(_((butt++)->item->text, term)) + gf_val(6, G_DIALOG_BUTTON_SPACE + txtlen(G_DIALOG_BUTTON_L) + txtlen(G_DIALOG_BUTTON_R));
	if (w > *width) *width = w;
}

void min_buttons_width(struct terminal *term, struct dialog_item_data *butt, int n, int *width)
{
	int i;
	for (i = 0; i < n; i++) {
		int w = txtlen(_((butt++)->item->text, term)) + gf_val(4, txtlen(G_DIALOG_BUTTON_L G_DIALOG_BUTTON_R));
		if (w > *width) *width = w;
	}
}

void dlg_format_buttons(struct dialog_data *dlg, struct terminal *term, struct dialog_item_data *butt, int n, int x, int *y, int w, int *rw, int align)
{
	int i1 = 0;
	while (i1 < n) {
		int i2 = i1 + 1;
		int mw;
		while (i2 < n) {
			mw = 0;
			max_buttons_width(dlg->win->term, butt + i1, i2 - i1 + 1, &mw);
			if (mw <= w) i2++;
			else break;
		}
		mw = 0;
		max_buttons_width(dlg->win->term, butt + i1, i2 - i1, &mw);
		if (rw && mw > *rw) if ((*rw = mw) > w) *rw = w;
		if (term) {
			int i;
			int p = x + ((align & AL_MASK) == AL_CENTER ? (w - mw) / 2 : 0);
			for (i = i1; i < i2; i++) {
				butt[i].x = p;
				butt[i].y = *y;
				p += (butt[i].l = txtlen(_(butt[i].item->text, dlg->win->term)) + gf_val(4, txtlen(G_DIALOG_BUTTON_L G_DIALOG_BUTTON_R))) + gf_val(2, G_DIALOG_BUTTON_SPACE);
			}
		}
		*y += gf_val(2, G_BFU_FONT_SIZE * 2);
		i1 = i2;
	}
}

void dlg_format_checkbox(struct dialog_data *dlg, struct terminal *term, struct dialog_item_data *chkb, int x, int *y, int w, int *rw, unsigned char *text)
{
	int k = gf_val(4, txtlen(G_DIALOG_CHECKBOX_L G_DIALOG_CHECKBOX_X G_DIALOG_CHECKBOX_R) + G_DIALOG_CHECKBOX_SPACE);
	if (term) {
		chkb->x = x;
		chkb->y = *y;
	}
	if (rw) *rw -= k;
	dlg_format_text(dlg, term, text, x + k, y, w - k, rw, COLOR_DIALOG_CHECKBOX_TEXT, AL_LEFT);
	if (rw) *rw += k;
}

void dlg_format_checkboxes(struct dialog_data *dlg, struct terminal *term, struct dialog_item_data *chkb, int n, int x, int *y, int w, int *rw, unsigned char **texts)
{
	while (n) {
		dlg_format_checkbox(dlg, term, chkb, x, y, w, rw, texts[0]);
		texts++; chkb++; n--;
	}
}

void checkboxes_width(struct terminal *term, unsigned char **texts, int *w, void (*fn)(struct terminal *, unsigned char *, int *, int))
{
	int k = gf_val(4, txtlen(G_DIALOG_CHECKBOX_L G_DIALOG_CHECKBOX_X G_DIALOG_CHECKBOX_R) + G_DIALOG_CHECKBOX_SPACE);
	while (texts[0]) {
		*w -= k;
		fn(term, _(texts[0], term), w, 0);
		*w += k;
		texts++;
	}
}

void dlg_format_field(struct dialog_data *dlg, struct terminal *term, struct dialog_item_data *item, int x, int *y, int w, int *rw, int align)
{
	if (term) {
		item->x = x;
		item->y = *y;
		item->l = w;
	}
	/*if ((item->l = w) > item->item->dlen - 1) item->l = item->item->dlen - 1;*/
	if (rw && item->l > *rw) if ((*rw = item->l) > w) *rw = w;
	(*y) += gf_val(1, G_BFU_FONT_SIZE);
}

/* Layout for generic boxes */
void dlg_format_box(struct terminal *term, struct terminal *t2, struct dialog_item_data *item, int x, int *y, int w, int *rw, int align) {
	item->x = x;
	item->y = *y;
	item->l = w;
	if (rw && item->l > *rw) if ((*rw = item->l) > w) *rw = w;
	(*y) += item->item->gid;
}

void max_group_width(struct terminal *term, unsigned char **texts, struct dialog_item_data *item, int n, int *w)
{
	int ww = 0;
	while (n--) {
		int wx = item->item->type == D_CHECKBOX ? gf_val(4, txtlen(G_DIALOG_CHECKBOX_L G_DIALOG_CHECKBOX_X G_DIALOG_CHECKBOX_R) + G_DIALOG_CHECKBOX_SPACE) :
			item->item->type == D_BUTTON ? txtlen(_(item->item->text, term)) + (gf_val(4, txtlen(G_DIALOG_BUTTON_L G_DIALOG_BUTTON_R))) :
			gf_val(item->item->dlen + 1, (item->item->dlen + 1) * G_DIALOG_FIELD_WIDTH);
		wx += txtlen(_(texts[0], term));
		if (n) gf_val(wx++, wx += G_DIALOG_GROUP_SPACE);
		ww += wx;
		texts++;
		item++;
	}
	if (ww > *w) *w = ww;
}

void min_group_width(struct terminal *term, unsigned char **texts, struct dialog_item_data *item, int n, int *w)
{
	while (n--) {
		int wx = item->item->type == D_CHECKBOX ? gf_val(4, txtlen(G_DIALOG_CHECKBOX_L G_DIALOG_CHECKBOX_X G_DIALOG_CHECKBOX_R) + G_DIALOG_CHECKBOX_SPACE) :
			item->item->type == D_BUTTON ? txtlen(_(item->item->text, term)) + (gf_val(4, txtlen(G_DIALOG_BUTTON_L G_DIALOG_BUTTON_R))) :
			gf_val(item->item->dlen + 1, (item->item->dlen + 1) * G_DIALOG_FIELD_WIDTH);
		wx += txtlen(_(texts[0], term));
		if (wx > *w) *w = wx;
		texts++;
		item++;
	}
}

void dlg_format_group(struct dialog_data *dlg, struct terminal *term, unsigned char **texts, struct dialog_item_data *item, int n, int x, int *y, int w, int *rw)
{
	int nx = 0;
	while (n--) {
		int wx = item->item->type == D_CHECKBOX ? gf_val(3, txtlen(G_DIALOG_CHECKBOX_L G_DIALOG_CHECKBOX_X G_DIALOG_CHECKBOX_R)) :
			item->item->type == D_BUTTON ? txtlen(_(item->item->text, dlg->win->term)) + (gf_val(4, txtlen(G_DIALOG_BUTTON_L G_DIALOG_BUTTON_R))) :
			gf_val(item->item->dlen, item->item->dlen * G_DIALOG_FIELD_WIDTH);
		int sl;
		if (_(texts[0], dlg->win->term)[0]) sl = txtlen(_(texts[0], dlg->win->term)) + gf_val(1, G_DIALOG_GROUP_TEXT_SPACE);
		else sl = 0;
		wx += sl;
		if (nx && nx + wx > w) {
			nx = 0;
			(*y) += gf_val(2, G_BFU_FONT_SIZE * 2);
		}
		if (term) {
			if (!F) print_text(term, x + nx + 4 * (item->item->type == D_CHECKBOX), *y, strlen(_(texts[0], dlg->win->term)), _(texts[0], dlg->win->term), COLOR_DIALOG_TEXT);
#ifdef G
			else {
				int l, ll;
				l = ll = x + nx + (item->item->type == D_CHECKBOX ? txtlen(G_DIALOG_CHECKBOX_L G_DIALOG_CHECKBOX_X G_DIALOG_CHECKBOX_R) + G_DIALOG_GROUP_TEXT_SPACE : 0);
				g_print_text(drv, term->dev, ll, *y, bfu_style_bw, _(texts[0], dlg->win->term), &ll);
				exclude_from_set(&dlg->s, l, *y, ll, *y + G_BFU_FONT_SIZE);
			}
#endif
			item->x = x + nx + sl * (item->item->type != D_CHECKBOX);
			item->y = *y;
			if (item->item->type == D_FIELD || item->item->type == D_FIELD_PASS) item->l = gf_val(item->item->dlen, item->item->dlen * G_DIALOG_FIELD_WIDTH);
		}
		if (rw && nx + wx > *rw) if ((*rw = nx + wx) > w) *rw = w;
		nx += wx + gf_val(1, G_DIALOG_GROUP_SPACE);
		texts++;
		item++;
	}
	(*y) += gf_val(1, G_BFU_FONT_SIZE);
}

#define LL gf_val(1, G_BFU_FONT_SIZE)

void checkbox_list_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	checkboxes_width(term, dlg->dlg->udata, &max, max_text_width);
	checkboxes_width(term, dlg->dlg->udata, &min, min_text_width);
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;
	dlg_format_checkboxes(dlg, NULL, dlg->items, dlg->n - 2, 0, &y, w, &rw, dlg->dlg->udata);
	y += LL;
	dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + LL;
	dlg_format_checkboxes(dlg, term, dlg->items, dlg->n - 2, dlg->x + DIALOG_LB, &y, w, NULL, dlg->dlg->udata);
	y += LL;
	dlg_format_buttons(dlg, term, dlg->items + dlg->n - 2, 2, dlg->x + DIALOG_LB, &y, w, &rw, AL_CENTER);
}

void group_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	max_group_width(term, dlg->dlg->udata, dlg->items, dlg->n - 2, &max);
	min_group_width(term, dlg->dlg->udata, dlg->items, dlg->n - 2, &min);
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	dlg_format_group(dlg, NULL, dlg->dlg->udata, dlg->items, dlg->n - 2, 0, &y, w, &rw);
	y += LL;
	dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + LL;
	dlg_format_group(dlg, term, dlg->dlg->udata, dlg->items, dlg->n - 2, dlg->x + DIALOG_LB, &y, w, NULL);
	y += LL;
	dlg_format_buttons(dlg, term, dlg->items + dlg->n - 2, 2, dlg->x + DIALOG_LB, &y, w, &rw, AL_CENTER);
}

void msg_box_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	unsigned char **ptr;
	unsigned char *text = init_str();
	int textl = 0;
	for (ptr = dlg->dlg->udata; *ptr; ptr++) add_to_str(&text, &textl, _(*ptr, term));
	max_text_width(term, text, &max, dlg->dlg->align);
	min_text_width(term, text, &min, dlg->dlg->align);
	max_buttons_width(term, dlg->items, dlg->n, &max);
	min_buttons_width(term, dlg->items, dlg->n, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	dlg_format_text(dlg, NULL, text, 0, &y, w, &rw, COLOR_DIALOG_TEXT, dlg->dlg->align);
	y += LL;
	dlg_format_buttons(dlg, NULL, dlg->items, dlg->n, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + LL;
	dlg_format_text(dlg, term, text, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, dlg->dlg->align);
	y += LL;
	dlg_format_buttons(dlg, term, dlg->items, dlg->n, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
	mem_free(text);
}

int msg_box_button(struct dialog_data *dlg, struct dialog_item_data *di)
{
	void (*fn)(void *) = (void (*)(void *))di->item->udata;
	void *data = dlg->dlg->udata2;
	/*struct dialog *dl = dlg->dlg;*/
	if (fn) fn(data);
	cancel_dialog(dlg, di);
	return 0;
}

#ifdef __XBOX__
int LinksBoks_MsgBox(struct terminal *term, struct dialog *dlg, struct memory_list *ml);
#endif

void msg_box(struct terminal *term, struct memory_list *ml, unsigned char *title, int align, /*unsigned char *text, void *data, int n,*/ ...)
{
	struct dialog *dlg;
	int i;
	int n;
	unsigned char *text;
	unsigned char **udata;
	void *udata2;
	int udatan;
	va_list ap;
	va_start(ap, align);
	udata = DUMMY;
	udatan = 0;
	do {
		unsigned char **udata_;
		text = va_arg(ap, unsigned char *);
		na_kovarne__to_je_narez:
		if (!(udata_ = mem_realloc(udata, ++udatan * sizeof(unsigned char *)))) {
			mem_free(udata);
			va_end(ap);
			return;
		}
		udata = udata_;
		udata[udatan - 1] = text;
		if (text && !(align & AL_EXTD_TEXT)) {
			text = NULL;
			goto na_kovarne__to_je_narez;
		}
	} while (text);
	udata2 = va_arg(ap, void *);
	n = va_arg(ap, int);
	if (!(dlg = mem_alloc(sizeof(struct dialog) + (n + 1) * sizeof(struct dialog_item)))) {
		mem_free(udata);
		va_end(ap);
		return;
	}
	memset(dlg, 0, sizeof(struct dialog) + (n + 1) * sizeof(struct dialog_item));
	dlg->title = title;
	dlg->fn = msg_box_fn;
	dlg->udata = udata;
	dlg->udata2 = udata2;
	dlg->align = align;
	for (i = 0; i < n; i++) {
		unsigned char *m;
		void (*fn)(void *);
		int flags;
		m = va_arg(ap, unsigned char *);
		fn = va_arg(ap, void *);
		flags = va_arg(ap, int);
		if (!m) {
			i--, n--;
			continue;
		}
		dlg->items[i].type = D_BUTTON;
		dlg->items[i].gid = flags;
		dlg->items[i].fn = msg_box_button;
		dlg->items[i].dlen = 0;
		dlg->items[i].text = m;
		dlg->items[i].udata = fn;
	}
	va_end(ap);
	dlg->items[i].type = D_END;
	add_to_ml(&ml, dlg, udata, NULL);
#ifdef __XBOX__
	if(LinksBoks_MsgBox(term, dlg, ml))
    return; /* dialog was accepted by the external handler */
#endif
	do_dialog(term, dlg, ml);
}

/* Trim starting and ending chars from a string.
 * WARNING: string is modified, pointer to new start of the
 * string is returned. if len != NULL, it is set to length of
 * trimmed string.
 */
inline unsigned char *
trim_chars(unsigned char *s, unsigned char c, int *len)
{
	int l = strlen(s);

	while (*s == c) s++, l--;
	while (l && s[l - 1] == c) s[--l] = '\0';

	if (len) *len = l;
	return s;
}

void add_to_history(struct history *h, unsigned char *t)
{
	struct history_item *hi, *hs;
	int l;
	if (!h || !t || !*t) return;

        t = trim_chars(t, ' ', &l);
        if(!l) return;

        l = strlen(t) + 1;
	if (!(hi = mem_alloc(sizeof(struct history_item) + l))) return;
	memcpy(hi->d, t, l);
	foreach(hs, h->items) if (!strcmp(hs->d, t)) {
		struct history_item *hd = hs;
		hs = hs->prev;
		del_from_list(hd);
		mem_free(hd);
		h->n--;
	}
	add_to_list(h->items, hi);
	h->n++;
	while (h->n > MAX_HISTORY_ITEMS) {
		struct history_item *hd = h->items.prev;
		if ((void *)hd == &h->items) {
			internal("history is empty");
			h->n = 0;
			return;
		}
		del_from_list(hd);
		mem_free(hd);
		h->n--;
	}
}

int input_field_cancel(struct dialog_data *dlg, struct dialog_item_data *di)
{
	void (*fn)(void *) = di->item->udata;
	void *data = dlg->dlg->udata2;
	if (fn) fn(data);
	cancel_dialog(dlg, di);
	return 0;
}

int input_field_ok(struct dialog_data *dlg, struct dialog_item_data *di)
{
	void (*fn)(void *, unsigned char *) = di->item->udata;
	void *data = dlg->dlg->udata2;
	unsigned char *text = dlg->items->cdata;
	if (check_dialog(dlg)) return 1;
	add_to_history(dlg->dlg->items->history, text);
	if (fn) fn(data, text);
	ok_dialog(dlg, di);
	return 0;
}

void input_field_fn(struct dialog_data *dlg)
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
	dlg_format_buttons(dlg, NULL, dlg->items + 1, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_text(dlg, term, dlg->dlg->udata, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_field(dlg, term, dlg->items, dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += LL;
	dlg_format_buttons(dlg, term, dlg->items + 1, 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

void input_field(struct terminal *term, struct memory_list *ml, unsigned char *title, unsigned char *text, unsigned char *okbutton, unsigned char *cancelbutton, void *data, struct history *history, int l, unsigned char *def, int min, int max, int (*check)(struct dialog_data *, struct dialog_item_data *), void (*fn)(void *, unsigned char *), void (*cancelfn)(void *))
{
	struct dialog *dlg;
	unsigned char *field;
	if (!(dlg = mem_alloc(sizeof(struct dialog) + 4 * sizeof(struct dialog_item) + l)))
		return;
	memset(dlg, 0, sizeof(struct dialog) + 4 * sizeof(struct dialog_item) + l);
	*(field = (unsigned char *)dlg + sizeof(struct dialog) + 4 * sizeof(struct dialog_item)) = 0;
	if (def) {
		if (strlen(def) + 1 > l) memcpy(field, def, l - 1);
		else strcpy(field, def);
	}
	dlg->title = title;
	dlg->fn = input_field_fn;
	dlg->udata = text;
	dlg->udata2 = data;
	dlg->items[0].type = D_FIELD;
	dlg->items[0].gid = min;
	dlg->items[0].gnum = max;
	dlg->items[0].fn = check;
	dlg->items[0].history = history;
	dlg->items[0].dlen = l;
	dlg->items[0].data = field;
	dlg->items[1].type = D_BUTTON;
	dlg->items[1].gid = B_ENTER;
	dlg->items[1].fn = input_field_ok;
	dlg->items[1].dlen = 0;
	dlg->items[1].text = okbutton;
	dlg->items[1].udata = fn;
	dlg->items[2].type = D_BUTTON;
	dlg->items[2].gid = B_ESC;
	dlg->items[2].fn = input_field_cancel;
	dlg->items[2].dlen = 0;
	dlg->items[2].text = cancelbutton;
	dlg->items[2].udata = cancelfn;
	dlg->items[3].type = D_END;
	add_to_ml(&ml, dlg, NULL);
	do_dialog(term, dlg, ml);
}

