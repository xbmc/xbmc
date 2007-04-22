#include "cfg.h"

#ifdef GRDRV_ATHEOS

#include <atheos/threads.h>
#include <gui/view.h>
#include <gui/window.h>
#include <gui/desktop.h>
#include <gui/bitmap.h>
#include <util/locker.h>
#include <util/application.h>

extern "C" {
#include "links.h"
}

#ifdef debug
#undef debug
#endif
#define debug(x)
#define fprintf(x, y)

extern struct graphics_driver atheos_driver;

using namespace os;

class LinksApplication : public Application {
	public:
	LinksApplication():Application("application/x-vnd.links"){}
	virtual bool OkToQuit(){return false;}
};

class LinksView;

class LinksWindow : public Window {
	public:
	LinksWindow(Rect r);
	~LinksWindow();
	virtual void FrameSized(const Point &d);
	virtual bool OkToQuit();
	int resized;
	LinksView *view;
};

class LinksView : public View {
	public:
	LinksView(LinksWindow *w);
	~LinksView();
	virtual void Paint(const Rect &r);
	virtual void MouseDown(const Point &p, uint32 b);
	virtual void MouseUp(const Point &p, uint32 b, Message *m);
	virtual void MouseMove(const Point &p, int c, uint32 b, Message *m);
	virtual void KeyDown(const char *s, const char *rs, uint32 q);
	virtual void WheelMove(const point &d);
	LinksWindow *win;
	struct graphics_device *dev;
	void d_flush();
	int flushing;
	int last_x, last_y;
};

#define lv(dev) ((LinksView *)(dev)->driver_data)

#define lock_dev(dev) do { if (lv(dev)->win->Lock()) return; } while (0)
#define lock_dev0(dev) do { if (lv(dev)->win->Lock()) return 0; } while (0)
#define unlock_dev(dev) do { lv(dev)->win->Unlock(); } while (0)

LinksApplication *ath_links_app;
Locker *ath_lock = NULL;

int msg_pipe[2];

thread_id ath_app_thread_id;

#define rpipe (msg_pipe[0])
#define wpipe (msg_pipe[1])

#define small_color (sizeof(Color32_s) <= sizeof(long))
#define get_color32(c, rgb) Color32_s c((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, 255)

color_space ath_cs_desktop, ath_cs_bmp;

int ath_x_size, ath_y_size;

int ath_win_x_size, ath_win_y_size;
int ath_win_x_pos, ath_win_y_pos;

LinksWindow::LinksWindow(Rect r):Window(r, "links_wnd", "Links")
{
	fprintf(stderr, "LINKSWINDOW\n");
	resized = 0;
	view = NULL;
}

LinksWindow::~LinksWindow()
{
	view = NULL;
	fprintf(stderr, "~LINKSWINDOW\n");
}

void LinksWindow::FrameSized(const Point &d)
{
	resized = 1;
}

bool LinksWindow::OkToQuit()
{
	ath_lock->Lock();
	Lock();
	if (view) if (view->dev) view->dev->keyboard_handler(view->dev, KBD_CTRL_C, 0);
	Unlock();
	ath_lock->Unlock();
	write(wpipe, " ", 1);
	/*fprintf(stderr, "key: :%s: :%s: %d %d\n", s, rs, q, c);*/
	return false;
}

void do_flush(void *p_dev)
{
	struct graphics_device *dev = (struct graphics_device *)p_dev;
	LinksView *v = lv(dev);
	v->win->Lock();
	v->win->Flush();
	v->win->Unlock();
	v->flushing = 0;
}

LinksView::LinksView(LinksWindow *w):View(w->GetBounds(), "Links", CF_FOLLOW_ALL, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE)
{
	fprintf(stderr, "LINKSVIEW\n");
	(win = w)->AddChild(this);
	w->SetFocusChild(this);
	w->view = this;
	flushing = 0;
	last_x = last_y = 0;
}

LinksView::~LinksView()
{
	win->view = NULL;
	fprintf(stderr, "~LINKSVIEW\n");
}

void LinksView::d_flush()
{
	if (flushing) return;
	register_bottom_half(do_flush, this->dev);
	flushing = 1;
}

#undef select

int ath_select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
	int v;
	if (ath_lock) ath_lock->Unlock();
	v = select(n, r, w, e, t);
	if (ath_lock) {
		ath_lock->Lock();
		check_bottom_halves();
	}
	return v;
}

void ath_get_event(void *dummy)
{
	char dummy_buffer[256];
	read(rpipe, dummy_buffer, 256);
	fprintf(stderr, "GETE\n");
}

void ath_get_size(struct graphics_device *dev)
{
	Rect r = lv(dev)->GetBounds();
	dev->size.x1 = dev->size.y1 = 0;
	dev->size.x2 = (int)r.Width() + 1;
	dev->size.y2 = (int)r.Height() + 1;
}

void LinksView::Paint(const Rect &r)
{
	struct rect rr;
	win->Unlock();
	ath_lock->Lock();
	win->Lock();
	rr.x1 = (int)r.left;
	rr.x2 = (int)r.right + 1;
	rr.y1 = (int)r.top;
	rr.y2 = (int)r.bottom + 1;
	/*fprintf(stderr, "paint: %d %d %d %d\n", rr.x1, rr.x2, rr.y1, rr.y2);*/
	if (dev) {
		if (!win->resized) dev->redraw_handler(dev, &rr);
		else {
			ath_get_size(dev);
			win->resized = 0;
			dev->resize_handler(dev);
		}
	}
	check_bottom_halves();
	ath_lock->Unlock();
	write(wpipe, " ", 1);
}


void LinksView::MouseDown(const Point &p, uint32 b)
{
	win->Unlock();
	ath_lock->Lock();
	win->Lock();
	if (dev) dev->mouse_handler(dev, last_x = (int)p.x, last_y = (int)p.y, B_DOWN | (b == 2 ? B_RIGHT : b == 3 ? B_MIDDLE : B_LEFT));
	ath_lock->Unlock();
	write(wpipe, " ", 1);
}

void LinksView::MouseUp(const Point &p, uint32 b, Message *m)
{
	win->Unlock();
	ath_lock->Lock();
	win->Lock();
	if (dev) dev->mouse_handler(dev, last_x = (int)p.x, last_y = (int)p.y, B_UP | (b == 2 ? B_RIGHT : b == 3 ? B_MIDDLE : B_LEFT));
	ath_lock->Unlock();
	write(wpipe, " ", 1);
}

void LinksView::MouseMove(const Point &p, int c, uint32 b, Message *m)
{
	win->Unlock();
	ath_lock->Lock();
	win->Lock();
	if (dev) dev->mouse_handler(dev, last_x = (int)p.x, last_y = (int)p.y, !b ? B_MOVE : B_DRAG | (b & 1 ? B_LEFT : b & 2 ? B_RIGHT : b & 4 ? B_MIDDLE : B_LEFT));
	ath_lock->Unlock();
	write(wpipe, " ", 1);
}

void LinksView::WheelMove(const point &d)
{
	win->Unlock();
	ath_lock->Lock();
	win->Lock();
	if (d.y) if (dev) dev->mouse_handler(dev, last_x, last_y, B_MOVE | (d.y > 0 ? B_WHEELDOWN : B_WHEELUP));
	if (d.x) if (dev) dev->mouse_handler(dev, last_x, last_y, B_MOVE | (d.x < 0 ? B_WHEELLEFT : B_WHEELRIGHT));
	ath_lock->Unlock();
	write(wpipe, " ", 1);
}

void LinksView::KeyDown(const char *s, const char *rs, uint32 q)
{
	int c;
	unsigned char *ss = q & (QUAL_CTRL | QUAL_ALT) ? (unsigned char *)rs : (unsigned char *)s;
	win->Unlock();
	ath_lock->Lock();
	win->Lock();
	GET_UTF_8(ss, c);
	switch (c) {
		case VK_BACKSPACE: c = KBD_BS; break;
		case VK_ENTER: c = KBD_ENTER; break;
		case VK_SPACE: c = ' '; break;
		case VK_TAB: c = KBD_TAB; break;
		case VK_ESCAPE: c = KBD_ESC; break;
		case VK_LEFT_ARROW: c = KBD_LEFT; break;
		case VK_RIGHT_ARROW: c = KBD_RIGHT; break;
		case VK_UP_ARROW: c = KBD_UP; break;
		case VK_DOWN_ARROW: c = KBD_DOWN; break;
		case VK_INSERT: c = KBD_INS; break;
		case VK_DELETE: c = KBD_DEL; break;
		case VK_HOME: c = KBD_HOME; break;
		case VK_END: c = KBD_END; break;
		case VK_PAGE_UP: c = KBD_PAGE_UP; break;
		case VK_PAGE_DOWN: c = KBD_PAGE_DOWN; break;
		default: if (c < 32) c = 0;
			 else q &= ~QUAL_SHIFT;
			 break;
	}
	if (c) if (dev) dev->keyboard_handler(dev, c, (q & QUAL_SHIFT ? KBD_SHIFT : 0) | (q & QUAL_CTRL ? KBD_CTRL : 0) | (q & QUAL_ALT ? KBD_ALT : 0));
	ath_lock->Unlock();
	write(wpipe, " ", 1);
	/*fprintf(stderr, "key: :%s: :%s: %d %d\n", s, rs, q, c);*/
}

unsigned char *ath_get_driver_param(void)
{
	return NULL;
}

uint32 ath_app_thread(void *p)
{
	ath_links_app->Run();
	delete ath_links_app;
	return 0;
}

unsigned char *ath_init_driver(unsigned char *param, unsigned char *display)
{
	Desktop *d;
	ath_links_app = new LinksApplication();
	if (!ath_links_app) {
		return stracpy((unsigned char *)"Unable to allocate Application object.\n");
	}
	ath_lock = new Locker("links_sem", false, false);
	if (!ath_lock || ath_lock->Lock()) {
		delete ath_links_app;
		return stracpy((unsigned char *)"Could not create lock.\n");
	}
	if (c_pipe(msg_pipe)) {
		delete ath_lock; ath_lock = NULL;
		delete ath_links_app;
		return stracpy((unsigned char *)"Could not create pipe.\n");
	}
	fcntl(rpipe, F_SETFL, O_NONBLOCK);
	fcntl(wpipe, F_SETFL, O_NONBLOCK);
	set_handlers(rpipe, ath_get_event, NULL, NULL, NULL);
	ath_app_thread_id = spawn_thread("links_app", ath_app_thread, 0, 0, NULL);
	resume_thread(ath_app_thread_id);
	if ((d = new Desktop)) {
		ath_cs_desktop = d->GetColorSpace();
		ath_x_size = d->GetResolution().x;
		ath_y_size = d->GetResolution().y;
		delete d;
	} else {
		ath_cs_desktop = CS_NO_COLOR_SPACE;
		ath_x_size = 640;
		ath_y_size = 480;
	}
	ath_win_y_size = ath_y_size * 9 / 10;
	ath_win_x_size = ath_win_y_size;
	/*
	fprintf(stderr, "%d %d\n", ath_x_size, ath_y_size);
	fprintf(stderr, "%d %d\n", ath_win_x_size, ath_win_y_size);
	*/
	ath_win_y_pos = (ath_y_size - ath_win_y_size) / 2;
	ath_win_x_pos = ath_x_size - ath_win_x_size - ath_win_y_pos;
	if (/*ath_cs_desktop == CS_RGB32 ||*/ ath_cs_desktop == CS_RGB24 || ath_cs_desktop == CS_RGB16 || ath_cs_desktop == CS_RGB15) 
		ath_cs_bmp = ath_cs_desktop;
	else if (ath_cs_desktop == CS_RGB32 || ath_cs_desktop == CS_RGBA32) ath_cs_bmp = CS_RGB24;
	else ath_cs_bmp = CS_RGB15;
	switch (ath_cs_bmp) {
		case CS_RGB24:
			atheos_driver.depth = 0xc3;
			break;
		case CS_RGB16:
			atheos_driver.depth = 0x82;
			break;
		case CS_RGB15:
			atheos_driver.depth = 0x7a;
			break;
		default:
			internal((unsigned char *)"unknown depth");
	}
	return NULL;
}

void ath_shutdown_driver()
{
	debug((unsigned char *)"D");
	close(rpipe);
	close(wpipe);
	set_handlers(rpipe, NULL, NULL, NULL, NULL);
	ath_lock->Unlock();
	debug((unsigned char *)"DD");
	ath_links_app->PostMessage(M_TERMINATE);
	debug((unsigned char *)"E");
	/*delete ath_lock; ath_lock = NULL;*/
	debug((unsigned char *)"F");
}

struct graphics_device *ath_init_device()
{
	LinksView *view;
	LinksWindow *win;
	struct graphics_device *dev = (struct graphics_device *)mem_calloc(sizeof(struct graphics_device));
	if (!dev) return NULL;
	dev->drv = &atheos_driver;
	debug((unsigned char *)"1");
	win = new LinksWindow(Rect(ath_win_x_pos, ath_win_y_pos, ath_win_x_pos + ath_win_x_size, ath_win_y_pos + ath_win_y_size));
	debug((unsigned char *)"2");
	if (!win) {
		mem_free(dev);
		return NULL;
	}
	debug((unsigned char *)"3");
	view = new LinksView(win);
	if (!view) {
		delete win;
		mem_free(dev);
		return NULL;
	}
	view->dev = dev;
	dev->driver_data = view;
	ath_get_size(dev);
	memcpy(&dev->clip, &dev->size, sizeof(struct rect));
	debug((unsigned char *)"4");
	win->Show();
	win->MakeFocus();
	debug((unsigned char *)"5");
	return dev;
}

void ath_shutdown_device(struct graphics_device *dev)
{
	LinksWindow *win = lv(dev)->win;
	unregister_bottom_half(do_flush, dev);
	lv(dev)->dev = NULL;
	win->PostMessage(M_TERMINATE);
	mem_free(dev);
}

void ath_set_title(struct graphics_device *dev, unsigned char *title)
{
	LinksWindow *win = lv(dev)->win;
	lock_dev(dev);
	win->SetTitle((string)(char *)title);
	lv(dev)->d_flush();
	unlock_dev(dev);
}

int ath_get_filled_bitmap(struct bitmap *bmp, long color)
{
	internal((unsigned char *)"nedopsano");
	return 0;
}

int ath_get_empty_bitmap(struct bitmap *bmp)
{
	fprintf(stderr, "bmp\n");
	Bitmap *b = new Bitmap(bmp->x, bmp->y, ath_cs_bmp, Bitmap::SHARE_FRAMEBUFFER);
	if (!b) {
		bmp->data = NULL;
		return 0;
	}
	bmp->data = b->LockRaster();
	bmp->skip = b->GetBytesPerRow();
	bmp->flags = b;
	return 0;
}

void ath_register_bitmap(struct bitmap *bmp)
{
	Bitmap *b = (Bitmap *)bmp->flags;
	b->UnlockRaster();
}

void *ath_prepare_strip(struct bitmap *bmp, int top, int lines)
{
	fprintf(stderr, "preps\n");
	Bitmap *b = (Bitmap *)bmp->flags;
	bmp->data = b->LockRaster();
	bmp->skip = b->GetBytesPerRow();
	return ((char *)bmp->data) + bmp->skip * top;
}

void ath_commit_strip(struct bitmap *bmp, int top, int lines)
{
	Bitmap *b = (Bitmap *)bmp->flags;
	b->UnlockRaster();
}

void ath_unregister_bitmap(struct bitmap *bmp)
{
	fprintf(stderr, "unb\n");
	Bitmap *b = (Bitmap *)bmp->flags;
	delete b;
}

void ath_draw_bitmap(struct graphics_device *dev, struct bitmap *bmp, int x, int y)
{
	fprintf(stderr, "drawb\n");
	Bitmap *b = (Bitmap *)bmp->flags;
	lock_dev(dev);
	lv(dev)->DrawBitmap(b, b->GetBounds(), Rect(x, y, x + bmp->x - 1, y + bmp->y - 1));
	lv(dev)->d_flush();
	unlock_dev(dev);
}

void ath_draw_bitmaps(struct graphics_device *dev, struct bitmap **bmp, int n, int x, int y)
{
	LinksView *lvv = lv(dev);
	lock_dev(dev);
	while (n--) {
		Bitmap *b = (Bitmap *)(*bmp)->flags;
		lvv->DrawBitmap(b, b->GetBounds(), Rect(x, y, x + (*bmp)->x, y + (*bmp)->y));
		x += (*bmp)->x;
		bmp++;
	}
	lv(dev)->d_flush();
	unlock_dev(dev);
}

long ath_get_color(int rgb)
{
	if (small_color) {
		get_color32(c, rgb);
		return *(long *)(void *)&c;
	} else return rgb & 0xffffff;
}

void ath_fill_area(struct graphics_device *dev, int x1, int y1, int x2, int y2, long color)
{
	fprintf(stderr, "fill\n");
	if (x1 >= x2 || y1 >= y2) return;
	lock_dev(dev);
	if (small_color)
		lv(dev)->FillRect(Rect(x1, y1, x2 - 1, y2 - 1), *(Color32_s *)(void *)&color);
	else
		lv(dev)->FillRect(Rect(x1, y1, x2 - 1, y2 - 1), get_color32(, color));
	lv(dev)->d_flush();
	unlock_dev(dev);
}

void ath_draw_hline(struct graphics_device *dev, int x1, int y, int x2, long color)
{
	fprintf(stderr, "hline\n");
	if (x1 >= x2) return;
	lock_dev(dev);
	if (small_color)
		lv(dev)->SetFgColor(*(Color32_s *)(void *)&color);
	else
		lv(dev)->SetFgColor(get_color32(, color));
	lv(dev)->DrawLine(Point(IPoint(x1, y)), Point(IPoint(x2 - 1, y)));
	lv(dev)->d_flush();
	unlock_dev(dev);
}

void ath_draw_vline(struct graphics_device *dev, int x, int y1, int y2, long color)
{
	fprintf(stderr, "vline\n");
	if (y1 >= y2) return;
	lock_dev(dev);
	if (small_color)
		lv(dev)->SetFgColor(*(Color32_s *)(void *)&color);
	else
		lv(dev)->SetFgColor(get_color32(, color));
	lv(dev)->DrawLine(Point(IPoint(x, y1)), Point(IPoint(x, y2 - 1)));
	lv(dev)->d_flush();
	unlock_dev(dev);
}

int ath_hscroll(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	fprintf(stderr, "hscroll\n");
	if (dev->clip.x1 >= dev->clip.x2 || dev->clip.y1 >= dev->clip.y2) return 0;
	if (sc <= dev->clip.x1 - dev->clip.x2) return 1;
	if (sc >= dev->clip.x2 - dev->clip.x1) return 1;
	lock_dev0(dev);
	if (sc > 0) lv(dev)->ScrollRect(Rect(dev->clip.x1, dev->clip.y1, dev->clip.x2 - sc - 1, dev->clip.y2 - 1), Rect(dev->clip.x1 + sc, dev->clip.y1, dev->clip.x2 - 1, dev->clip.y2 - 1));
	else lv(dev)->ScrollRect(Rect(dev->clip.x1 - sc, dev->clip.y1, dev->clip.x2 - 1, dev->clip.y2 - 1), Rect(dev->clip.x1, dev->clip.y1, dev->clip.x2 + sc - 1, dev->clip.y2 - 1));
	lv(dev)->d_flush();
	unlock_dev(dev);
	return 1;
}

int ath_vscroll(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	fprintf(stderr, "vscroll\n");
	if (!sc || dev->clip.x1 >= dev->clip.x2 || dev->clip.y1 >= dev->clip.y2) return 0;
	if (sc <= dev->clip.y1 - dev->clip.y2) return 1;
	if (sc >= dev->clip.y2 - dev->clip.y1) return 1;
	lock_dev0(dev);
	if (sc > 0) lv(dev)->ScrollRect(Rect(dev->clip.x1, dev->clip.y1, dev->clip.x2 - 1, dev->clip.y2 - sc - 1), Rect(dev->clip.x1, dev->clip.y1 + sc, dev->clip.x2 - 1, dev->clip.y2 - 1));
	else lv(dev)->ScrollRect(Rect(dev->clip.x1, dev->clip.y1 - sc, dev->clip.x2 - 1, dev->clip.y2 - 1), Rect(dev->clip.x1, dev->clip.y1, dev->clip.x2 - 1, dev->clip.y2 + sc - 1));
	lv(dev)->d_flush();
	unlock_dev(dev);
	return 1;
}

void ath_set_clip_area(struct graphics_device *dev, struct rect *r)
{
	fprintf(stderr, "setc\n");
	memcpy(&dev->clip, r, sizeof(struct rect));
	lock_dev(dev);
	lv(dev)->SetDrawingRegion(Region(IRect(r->x1, r->y1, r->x2 - 1, r->y2 - 1)));
	unlock_dev(dev);
}

void atn_put_to_clipboard(struct graphics_device *gd, char *string,int length)
{
}

void atn_request_clipboard(struct grphics_device *gd)
{
}

char *atn_get_from_clipboard(struct graphics_device *gd)
{
        return NULL;
}

struct graphics_driver atheos_driver = {
	(unsigned char *)"atheos",
	ath_init_driver,
	ath_init_device,
	ath_shutdown_device,
	ath_shutdown_driver,
	ath_get_driver_param,
	ath_get_empty_bitmap,
	ath_get_filled_bitmap,
	ath_register_bitmap,
	ath_prepare_strip,
	ath_commit_strip,
	ath_unregister_bitmap,
	ath_draw_bitmap,
	ath_draw_bitmaps,
	ath_get_color,
	ath_fill_area,
	ath_draw_hline,
	ath_draw_vline,
	ath_hscroll,
	ath_vscroll,
	ath_set_clip_area,
	dummy_block,
	dummy_unblock,
	ath_set_title,
        ath_put_to_clipboard,
        atn_request_clipboard,
        atn_get_from_clipboard,
        0,				/* depth */
	0, 0,				/* size */
	0,				/* flags */
};

#endif
