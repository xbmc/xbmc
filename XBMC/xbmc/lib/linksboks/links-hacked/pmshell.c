/* pmshell.c
 * PMShell graphics driver
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "cfg.h"

#if 0
#define debug_call(x) printf x; fflush(stdout);
#else
#define debug_call(x)
#endif

#ifdef GRDRV_PMSHELL

#include "links.h"

extern struct graphics_driver pmshell_driver;

#define INCL_DOS
#define INCL_GPI
#define INCL_WIN
#include <os2.h>

#include <process.h>

#include <sys/builtin.h>
#include <sys/fmutex.h>
_fmutex pm_mutex;
#define pm_lock_init	_fmutex_create(&pm_mutex, 0)
#define pm_lock		_fmutex_request(&pm_mutex, _FMR_IGNINT)
#define pm_unlock	_fmutex_release(&pm_mutex)

unsigned char *pm_class_name = "links";
unsigned char *pm_msg_class_name = "links.msg";

ULONG pm_frame = (FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER | FCF_MINMAX | FCF_SHELLPOSITION | FCF_TASKLIST);

ULONG pm_msg_frame = 0;

#define E_KEY		1
#define E_MOUSE		2
#define E_REDRAW	3
#define E_RESIZE	4

struct pm_event {
	struct pm_event *next;
	struct pm_event *prev;
	int type;
	int x1, y1, x2, y2;
};

struct pm_window {
	struct pm_window *next;
	struct pm_window *prev;
	int x, y;
	int in;
	struct pm_window *nxt;
	struct pm_window **prv;
	HPS ps;
	HWND h;
	HWND hc;
	struct graphics_device *dev;
	int button;
	unsigned lastpos;
	struct list_head queue;
};

#define WIN_HASH	64

struct pm_window *pm_windows[WIN_HASH];

void pm_hash_window(struct pm_window *win)
{
	int pos = win->hc & (WIN_HASH - 1);
	win->prv = &pm_windows[pos];
	if ((win->nxt = pm_windows[pos])) pm_windows[pos]->prv = &win->nxt;
	pm_windows[pos] = win;
}

void pm_unhash_window(struct pm_window *win)
{
	if (win->nxt) win->nxt->prv = win->prv;
	*win->prv = win->nxt;
}

static inline struct pm_window *pm_lookup_window(HWND h)
{
	struct pm_window *win;
	for (win = pm_windows[h & (WIN_HASH - 1)]; win && win->hc != h; win = win->nxt) ;
	return win;
}

#define pm_win(dev) ((struct pm_window *)dev->driver_data)

int pm_pipe[2];

HEV pm_sem;
ULONG pm_sem_dummy;

#define pm_wait	do {					\
	DosWaitEventSem(pm_sem, SEM_INDEFINITE_WAIT);	\
	DosResetEventSem(pm_sem, &pm_sem_dummy);	\
} while (0)

#define pm_signal DosPostEventSem(pm_sem)

unsigned char *pm_not_ses = "Not in a pmshell session.\n";
unsigned char *pm_status;

HAB hab_disp;
HAB hab;
HMQ hmq;
HWND hwnd_msg;
HPS hps_msg;
HDC hdc_mem;
HPS hps_mem;

int pm_cp;

struct list_head pm_event_windows = { &pm_event_windows, &pm_event_windows };

void pm_send_event(struct pm_window *win, int t, int x1, int y1, int x2, int y2)
{
	/* must be called with pm_lock */
	struct pm_event *ev;
	if ((ev = malloc(sizeof(struct pm_event)))) {
		ev->type = t;
		ev->x1 = x1, ev->y1 = y1;
		ev->x2 = x2, ev->y2 = y2;
		if (!win->in) {
			if (list_empty(pm_event_windows)) write(pm_pipe[1], "x", 1);
			add_to_list(pm_event_windows, win);
			win->in = 1;
		}
		add_to_list(win->queue, ev);
	}
}

void pm_send_mouse_event(struct pm_window *win, int x1, int y1, int b)
{
	if (!list_empty(win->queue)) {
		struct pm_event *last = win->queue.next;
		if (last->type == E_MOUSE && last->x2 == b) {
			last->x1 = x1;
			last->y1 = y1;
			return;
		}
	}
	pm_send_event(win, E_MOUSE, x1, y1, b, 0);
}

void pm_cancel_event(struct pm_window *win, int t, struct pm_event **pev)
{
	struct pm_event *ev;
	if (pev) *pev = NULL;
	foreachback(ev, win->queue) if (ev->type == t) {
		if (pev) *pev = ev;
		else {
			del_from_list(ev);
			free(ev);
		}
		return;
	}
}

void pm_resize(struct pm_window *win, RECTL *r)
{
	struct pm_event *ev;
	win->x = r->xRight;
	win->y = r->yTop;
	pm_cancel_event(win, E_REDRAW, NULL);
	pm_cancel_event(win, E_RESIZE, &ev);
	if (ev) {
		ev->x2 = r->xRight;
		ev->y2 = r->yTop;
	} else pm_send_event(win, E_RESIZE, 0, 0, r->xRight, r->yTop);
}

void pm_redraw(struct pm_window *win, RECTL *r)
{
	struct pm_event *ev;
	pm_cancel_event(win, E_RESIZE, &ev);
	if (ev) return;
	pm_cancel_event(win, E_REDRAW, &ev);
	if (ev) {
		if (r->xLeft < ev->x1) ev->x1 = r->xLeft;
		if (r->xRight > ev->x2) ev->x2 = r->xRight;
		if (win->y - r->yTop < ev->y1) ev->y1 = win->y - r->yTop;
		if (win->y - r->yBottom > ev->y2) ev->y2 = win->y - r->yBottom;
		return;
	}
	pm_send_event(win, E_REDRAW, r->xLeft, win->y - r->yTop, r->xRight, win->y - r->yBottom);
}

#define N_VK	0x42

struct os2_key pm_vk_table[N_VK] = {
	0, 0, 0, 0, 0, 0, 0, 0, KBD_CTRL_C, 0, KBD_BS, 0, KBD_TAB, 0, KBD_TAB, KBD_SHIFT,
	KBD_ENTER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KBD_ESC, 0,
	' ', 0, KBD_PAGE_UP, 0, KBD_PAGE_DOWN, 0, KBD_END, 0, KBD_HOME, 0, KBD_LEFT, 0, KBD_UP, 0, KBD_RIGHT, 0, 
	KBD_DOWN, 0, 0, 0, KBD_INS, 0, KBD_DEL, 0, 0, 0, 0, 0, KBD_ENTER, 0, 0, 0,
	KBD_F1, 0, KBD_F2, 0, KBD_F3, 0, KBD_F4, 0, KBD_F5, 0, KBD_F6, 0, KBD_F7, 0, KBD_F8, 0,
	KBD_F9, 0, KBD_F10, 0, KBD_F11, 0, KBD_F12, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, KBD_DEL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
};

MRESULT EXPENTRY pm_window_proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
	int k_usch, k_usvk, k_fsflags;
	int key, flags;
	struct pm_window *win;
	RECTL wr, ur;
	/*fprintf(stderr, "%08x %08x %08x\n", (int)msg, (int)mp1, (int)mp2);*/
	switch (msg) {
		case WM_PAINT:
			pm_lock;
			WinQueryUpdateRect(hwnd, &ur);
			WinQueryWindowRect(hwnd, &wr);
			WinValidateRect(hwnd, &ur, FALSE);
			if (!(win = pm_lookup_window(hwnd))) {
				pm_unlock;
				return 0;
			}
			if (wr.xRight != win->x || wr.yTop != win->y) pm_resize(win, &wr);
			else pm_redraw(win, &ur);
			pm_unlock;
			return 0;
		case WM_CLOSE:
		case WM_QUIT:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) {
				pm_unlock;
				return 0;
			}
			pm_send_event(win, E_KEY, KBD_CTRL_C, 0, 0, 0);
			pm_unlock;
			return 0;
		case WM_CHAR:
			k_fsflags = (int)mp1;
			if (k_fsflags & (KC_KEYUP | KC_DEADKEY | KC_INVALIDCOMP)) return 0;
			k_usch = (int)mp2 & 0xffff;
			k_usvk = ((int)mp2 >> 16) & 0xffff;
			/*printf("%08x, %08x\n", mp1, mp2);fflush(stdout);*/
			flags = (k_fsflags & KC_SHIFT ? KBD_SHIFT : 0) | (k_fsflags & KC_CTRL ? KBD_CTRL : 0) | (k_fsflags & KC_ALT ? KBD_ALT : 0);
			if (k_fsflags & KC_VIRTUALKEY) {
				if (k_usvk < N_VK && (key = pm_vk_table[k_usvk].x)) {
					flags |= pm_vk_table[k_usvk].y;
					if (key == KBD_CTRL_C) flags &= ~KBD_CTRL;
					goto s;
				}
			}
			if (k_usch & 0xff) {
				key = k_usch & 0xff;
				if (!(flags & KBD_CTRL)) {
					if (key == 0x0d) key = KBD_ENTER;
					if (key == 0x08) key = KBD_BS;
					if (key == 0x09) key = KBD_TAB;
					if (key == 0x1b) key = KBD_ESC;
				}
				if (key < ' ') key += '@', flags |= KBD_CTRL;
			} else key = os2xtd[k_usch >> 8].x, flags |= os2xtd[k_usch >> 8].y;
			if ((key & 0xdf) == 'C' && (flags & KBD_CTRL)) key = KBD_CTRL_C, flags &= ~KBD_CTRL;
			s:
			if (!key) return 0;
			if (key >= 0) flags &= ~KBD_SHIFT;
			if (key >= 0x80 && pm_cp) {
				if ((key = cp2u(key, pm_cp)) < 0) return 0;
			}
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) {
				pm_unlock;
				return 0;
			}
			pm_send_event(win, E_KEY, key, flags, 0, 0);
			pm_unlock;
			return 0;
		case WM_BUTTON1DOWN:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) { pm_unlock; break; }
			win->button |= 1 << B_LEFT;
			win->lastpos = (unsigned)mp1;
			pm_send_event(win, E_MOUSE, (unsigned)mp1 & 0xffff, win->y - ((unsigned)mp1 >> 16), B_DOWN | B_LEFT, 0);
			pm_unlock;
			break;
		case WM_BUTTON2DOWN:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) { pm_unlock; break; }
			win->button |= 1 << B_RIGHT;
			win->lastpos = (unsigned)mp1;
			pm_send_event(win, E_MOUSE, (unsigned)mp1 & 0xffff, win->y - ((unsigned)mp1 >> 16), B_DOWN | B_RIGHT, 0);
			pm_unlock;
			break;
		case WM_BUTTON3DOWN:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) { pm_unlock; break; }
			win->button |= 1 << B_MIDDLE;
			win->lastpos = (unsigned)mp1;
			pm_send_event(win, E_MOUSE, (unsigned)mp1 & 0xffff, win->y - ((unsigned)mp1 >> 16), B_DOWN | B_MIDDLE, 0);
			pm_unlock;
			break;
		case WM_BUTTON1UP:
		case WM_BUTTON1MOTIONEND:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) { pm_unlock; break; }
			if (win->button & (1 << B_LEFT)) pm_send_event(win, E_MOUSE, (unsigned)mp1 & 0xffff, win->y - ((unsigned)mp1 >> 16), B_UP | B_LEFT, 0);
			win->button &= ~(1 << B_LEFT);
			win->lastpos = (unsigned)mp1;
			pm_unlock;
			break;
		case WM_BUTTON2UP:
		case WM_BUTTON2MOTIONEND:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) { pm_unlock; break; }
			if (win->button & (1 << B_RIGHT)) pm_send_event(win, E_MOUSE, (unsigned)mp1 & 0xffff, win->y - ((unsigned)mp1 >> 16), B_UP | B_RIGHT, 0);
			win->button &= ~(1 << B_RIGHT);
			win->lastpos = (unsigned)mp1;
			pm_unlock;
			break;
		case WM_BUTTON3UP:
		case WM_BUTTON3MOTIONEND:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) { pm_unlock; break; }
			if (win->button & (1 << B_MIDDLE)) pm_send_event(win, E_MOUSE, (unsigned)mp1 & 0xffff, win->y - ((unsigned)mp1 >> 16), B_UP | B_MIDDLE, 0);
			win->button &= ~(1 << B_MIDDLE);
			win->lastpos = (unsigned)mp1;
			pm_unlock;
			break;
		case WM_MOUSEMOVE:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) { pm_unlock; break; }
			win->lastpos = (unsigned)mp1;
			pm_send_mouse_event(win, (unsigned)mp1 & 0xffff, win->y - ((unsigned)mp1 >> 16), (win->button ? B_DRAG : B_MOVE) | (win->button & (1 << B_LEFT) ? B_LEFT : win->button & (1 << B_MIDDLE) ? B_MIDDLE : win->button & (1 << B_RIGHT) ? B_RIGHT : 0));
			pm_unlock;
			break;
		case WM_VSCROLL:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) { pm_unlock; break; }
			if ((unsigned)mp2 == SB_LINEUP << 16 || (unsigned)mp2 == SB_LINEDOWN << 16) pm_send_event(win, E_MOUSE, win->lastpos & 0xffff, win->y - (win->lastpos >> 16), ((unsigned)mp2 == SB_LINEUP << 16 ? B_WHEELUP1 : B_WHEELDOWN1) | B_MOVE, 0);
			pm_unlock;
			break;
		case WM_HSCROLL:
			pm_lock;
			if (!(win = pm_lookup_window(hwnd))) { pm_unlock; break; }
			if ((unsigned)mp2 == SB_LINELEFT << 16 || (unsigned)mp2 == SB_LINERIGHT << 16) pm_send_event(win, E_MOUSE, win->lastpos & 0xffff, win->y - (win->lastpos >> 16), ((unsigned)mp2 == SB_LINELEFT << 16 ? B_WHEELLEFT1 : B_WHEELRIGHT1) | B_MOVE, 0);
			pm_unlock;
			break;
	}
	return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

int pm_thread_shutdown;

#define MSG_CREATE_WINDOW	1
#define MSG_DELETE_WINDOW	2
#define MSG_SET_WINDOW_TITLE	3
#define MSG_SHUTDOWN_THREAD	4

struct title_set {
	struct pm_window *win;
	unsigned char *text;
};

MRESULT EXPENTRY pm_msg_proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
	struct title_set *w;
	if (msg == WM_USER) switch ((int)mp1) {
		struct pm_window *win;
		case MSG_CREATE_WINDOW:
			win = mp2;
			win->h = WinCreateStdWindow(HWND_DESKTOP, WS_VISIBLE, &pm_frame, pm_class_name, "Links", 0, 0, 0, &win->hc);
			WinSetWindowPos(win->h, 0, 0, 0, 0, 0, SWP_ACTIVATE);
			pm_lock;
			pm_signal;
			return 0;
		case MSG_DELETE_WINDOW:
			win = mp2;
			WinDestroyWindow(win->h);
			pm_lock;
			pm_signal;
			return 0;
		case MSG_SET_WINDOW_TITLE:
			w = mp2;
			WinSetWindowText(w->win->h, w->text);
			pm_lock;
			pm_signal;
			return 0;
		case MSG_SHUTDOWN_THREAD:
			pm_thread_shutdown = 1;
			return 0;
	}
	return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

void pm_send_msg(int msg, void *param)
{
	WinPostMsg(hwnd_msg, WM_USER, (MPARAM)msg, (MPARAM)param);
	pm_wait;
}

void pm_dispatcher(void *p)
{
	QMSG msg;
	pm_status = NULL;
	/*DosSetPriority(PRTYS_THREAD, PRTYC_FOREGROUNDSERVER, 1, 0);*/
	DosSetPriority(PRTYS_THREAD, PRTYC_NOCHANGE, 1, 0);
	if ((hab_disp = WinInitialize(0)) == NULLHANDLE) {
		pm_status = "WinInitialize failed in pm thread.\n";
		goto fail;
	}
	if ((hmq = WinCreateMsgQueue(hab_disp, 0)) == NULLHANDLE) {
		ERRORID e = WinGetLastError(hab_disp);
		if ((e & 0xffff) == PMERR_NOT_IN_A_PM_SESSION) pm_status = pm_not_ses;
		else pm_status = "WinCreateMsgQueue failed in pm thread.\n";
		goto fail1;
	}
	if ((pm_cp = WinQueryCp(hmq))) {
		unsigned char a[64];
		snprint(a, 64, pm_cp);
		if ((pm_cp = get_cp_index(a)) < 0 || is_cp_special(pm_cp)) pm_cp = 0;
	}
	/*{
		ULONG cp_list[100];
		int n, i;
		debug("WinQueryCp: %d", WinQueryCp(hmq));
		n = WinQueryCpList(hab_disp, 100, cp_list);
		debug("%d", n);
		for (i = 0; i < n; i++) fprintf(stderr, "%d, ", cp_list[i]);
	}*/
	if (WinRegisterClass(hab_disp, pm_class_name, pm_window_proc, CS_SIZEREDRAW, 0) == FALSE) {
		pm_status = "WinRegisterClass failed for window.\n";
		goto fail2;
	}
	if (WinRegisterClass(hab_disp, pm_msg_class_name, pm_msg_proc, 0, 0) == FALSE) {
		pm_status = "WinRegisterClass failed for msg window.\n";
		goto fail3;
	}
	if ((hwnd_msg = WinCreateStdWindow(HWND_DESKTOP, 0, &pm_msg_frame, pm_msg_class_name, NULL, 0, 0, 0, NULL)) == NULLHANDLE) {
		pm_status = "Could not create msg window.\n";
		goto fail4;
	}
	if ((hps_msg = WinGetPS(hwnd_msg)) == NULLHANDLE) {
		pm_status = "Could not get msg window ps.\n";
		goto fail5;
	}
	pm_signal;
	while (!pm_thread_shutdown) {
		WinGetMsg(hab_disp, &msg, 0L, 0, 0);
		WinDispatchMsg(hab_disp, &msg);
	}

	
		WinReleasePS(hps_msg);
	fail5:	WinDestroyWindow(hwnd_msg);
	fail4:
	fail3:
	fail2:	WinDestroyMsgQueue(hmq);
	fail1:	WinTerminate(hab);
	fail:
	pm_signal;
	return;
}

void pm_pipe_error(void *p)
{
	error("exception on pm pipe");
	set_handlers(pm_pipe[0], NULL, NULL, NULL, NULL);
}

void pm_handler(void *p)
{
	unsigned char c;
	struct pm_window *win = NULL;
	struct pm_event *ev = NULL;
	pm_lock;
	if (!list_empty(pm_event_windows)) {
		win = pm_event_windows.prev;
		if (!list_empty(win->queue)) {
			ev = win->queue.prev;
			del_from_list(ev);
		}
		if (list_empty(win->queue)) {
			del_from_list(win);
			win->in = 0;
		}
	}
	if (list_empty(pm_event_windows)) {
		if (read(pm_pipe[0], &c, 1) != 1) pm_pipe_error(NULL);
	}
	pm_unlock;
	if (!ev) return;
	switch (ev->type) {
		struct rect r;
		case E_KEY:
			if (win->dev->keyboard_handler)
				win->dev->keyboard_handler(win->dev, ev->x1, ev->y1);
			break;
		case E_MOUSE:
			if (win->dev->mouse_handler)
				win->dev->mouse_handler(win->dev, ev->x1, ev->y1, ev->x2);
			break;
		case E_REDRAW:
			if (win->dev->redraw_handler) {
				r.x1 = ev->x1; r.y1 = ev->y1;
				r.x2 = ev->x2; r.y2 = ev->y2;
				win->dev->redraw_handler(win->dev, &r);
			}
			break;
		case E_RESIZE:
			win->dev->size.x2 = ev->x2;
			win->dev->size.y2 = ev->y2;
			if (win->dev->resize_handler) {
				win->dev->resize_handler(win->dev);
			}
	}
	free(ev);
}

BITMAPINFO *pm_bitmapinfo;

int pm_bitmap_count;

int pm_child_pid;

void pm_sigcld(void *p)
{
	int st;
	int w;
	if (!(w = waitpid(pm_child_pid, &st, WNOHANG))) return;
	if (w > 0) exit(st);
	else exit(1);
}

int pm_sin, pm_sout, pm_serr, pm_ip[2], pm_op[2], pm_ep[2];
int pm_conS_OKAY = 0;
	
void pm_setup_console()
{
	if (pm_conS_OKAY) goto fail9;
	if ((pm_sin = dup(0)) < 0) goto fail;
	if ((pm_sout = dup(1)) < 0) goto fail1;
	if ((pm_serr = dup(2)) < 0) goto fail2;
	if (c_pipe(pm_ip)) goto fail3;
	if (c_pipe(pm_op)) goto fail4;
	if (c_pipe(pm_ep)) goto fail5;
	if (dup2(pm_ip[0], 0) != 0) goto fail6;
	if (dup2(pm_op[1], 1) != 1) goto fail7;
	if (dup2(pm_ep[1], 2) != 2) goto fail8;
	close(pm_ip[0]);
	close(pm_op[1]);
	close(pm_ep[1]);
	pm_conS_OKAY = 1;
	return;
fail9:	dup2(pm_serr, 2);
fail8:	dup2(pm_sout, 1);
fail7:	dup2(pm_sin, 0);
fail6:	close(pm_ep[0]);
	close(pm_ep[1]);
fail5:	close(pm_op[0]);
	close(pm_op[1]);
fail4:	close(pm_ip[0]);
	close(pm_ip[1]);
fail3:	close(pm_serr);
fail2:	close(pm_sout);
fail1:	close(pm_sin);
fail:	pm_conS_OKAY = 0;
}

void pm_do_console()
{
#define CONS_BUF 20
	unsigned char buffer[CONS_BUF];
	fd_set s;
	int m = pm_op[0] > pm_ep[0] ? pm_op[0] : pm_ep[0];
	if (pm_sin > m) m = pm_sin;
	m++;
	if (!pm_conS_OKAY) return;
	while (1) {
		int r, w;
		FD_ZERO(&s);
		/*FD_SET(pm_sin, &s);*/
		FD_SET(pm_op[0], &s);
		FD_SET(pm_ep[0], &s);
		if (select(m, &s, NULL, NULL, NULL) <= 0) goto br;
#define SEL_CHK(ih, oh)							\
	if (FD_ISSET(ih, &s)) {						\
		if ((r = read(ih, buffer, CONS_BUF)) <= 0) goto br;	\
		do {							\
			if ((w = write(oh, buffer, r)) <= 0) goto br;	\
			r -= w;						\
		} while (r > 0);					\
	}

		/*SEL_CHK(pm_sin, pm_ip[1]);*/
		SEL_CHK(pm_op[0], pm_sout);
		SEL_CHK(pm_ep[0], pm_serr);
	}

	br:;
}

unsigned char *pm_get_driver_param(void)
{
	return NULL;
}

unsigned char *pm_init_driver(unsigned char *param, unsigned char *display)
{
	unsigned char *s;
	pm_bitmap_count = 0;
	if ((hab = WinInitialize(0)) == 0) {
		s = "WinInitialize failed.\n";
		goto r1;
	}
	if (DosCreateEventSem(NULL, &pm_sem, 0, 0)) {
		s = "Could not create event semaphore.\n";
		goto r2;
	}
	if (c_pipe(pm_pipe)) {
		s = "Could not create pipe.\n";
		goto r3;
	}
	fcntl(pm_pipe[1], F_SETFL, O_NONBLOCK);
	memset(pm_windows, 0, sizeof(struct pm_window *) * WIN_HASH);
	pm_lock_init;
	pm_thread_shutdown = 0;
	if (_beginthread(pm_dispatcher, NULL, 65536, NULL) == -1) {
		s = "Could not start thread.\n";
		goto r4;
	}
	pm_wait;
	if (pm_status) {
		int pid;
		char **arg;
		if (pm_status != pm_not_ses) goto f;
		if (!(arg = mem_alloc((g_argc + 1) * sizeof(char *)))) goto f;
		memcpy(arg, g_argv, g_argc * sizeof(char *));
		arg[g_argc] = NULL;
		pm_child_pid = -1;
		install_signal_handler(SIGCLD, pm_sigcld, NULL, 1);
		pm_setup_console();
		pm_child_pid = pid = spawnvp(P_PM, path_to_exe, arg);
		mem_free(arg);
		if (pid < 0) {
			pm_setup_console();
			goto f;
		}
		pm_do_console();
		pm_setup_console();
		while (1) select(1, NULL, NULL, NULL, NULL);
		f:
		s = pm_status;
		goto r4;
	}
	{
#define N_FORMATS	100
		int i, pm_bitcount;
		LONG formats[N_FORMATS];
		memset(formats, 0, N_FORMATS * sizeof(LONG));
		if (GpiQueryDeviceBitmapFormats(hps_msg, N_FORMATS, formats) == FALSE) goto std_form;
		for (i = 0; i + 1 < N_FORMATS; i += 2) if (formats[i] == 1) switch (formats[i+1]) {
	/* !!! FIXME: tady by se mely pridat dalsi formaty, ale neznam je */
			/*
			case 15:
				pmshell_driver.depth = 0x7a;
				pm_bitcount = 15;
				goto e;
			*/
			case 16:
				pmshell_driver.depth = 0x82;
				pm_bitcount = 16;
				goto e;
			case 24:
			std_form:
				pmshell_driver.depth = 0xc3;
				pm_bitcount = 24;
				goto e;
		}
		goto std_form;
		e:;
		if ((pm_bitmapinfo = mem_calloc(sizeof(BITMAPINFOHEADER)))) {
			pm_bitmapinfo->cbFix = sizeof(BITMAPINFOHEADER);
			pm_bitmapinfo->cPlanes = 1;
			pm_bitmapinfo->cBitCount = pm_bitcount;
		}
	}
	{
		SIZEL sizl = { 0, 0 };
		PSZ data[4] = { "DISPLAY", NULL, NULL, NULL };
		hdc_mem = DevOpenDC(hab, OD_MEMORY, "*", 4L, (PDEVOPENDATA)data, NULLHANDLE);
		hps_mem = GpiCreatePS(hab, hdc_mem, &sizl, GPIA_ASSOC | PU_PELS | GPIT_MICRO);
	}
	set_handlers(pm_pipe[0], pm_handler, NULL, pm_pipe_error, NULL);
	return NULL;
	r4:
	close(pm_pipe[0]);
	close(pm_pipe[1]);
	r3:
	DosCloseEventSem(pm_sem);
	r2:
	WinTerminate(hab);
	r1:
	return stracpy(s);
}

struct graphics_device *pm_init_device(void)
{
	RECTL rect;
	struct graphics_device *dev;
	struct pm_window *win;
	if (!(win = mem_alloc(sizeof(struct pm_window)))) goto r1;
	win->button = 0;
	init_list(win->queue);
	win->in = 0;
	pm_send_msg(MSG_CREATE_WINDOW, win);
	if (win->h == NULLHANDLE) goto r2;
	if ((win->ps = WinGetPS(win->hc)) == NULLHANDLE) goto r3;
	if (!(dev = mem_calloc(sizeof(struct graphics_device)))) goto r4;
	dev->driver_data = win;
	win->dev = dev;
	if (WinQueryWindowRect(win->hc, &rect) == TRUE) {
		win->x = dev->size.x2 = rect.xRight;
		win->y = dev->size.y2 = rect.yTop;
	} else dev->size.x2 = dev->size.y2 = 0;
	dev->clip.x1 = dev->clip.y1 = 0;
	dev->clip.x2 = dev->size.x2;
	dev->clip.y2 = dev->size.y2;
	dev->drv = &pmshell_driver;
	GpiCreateLogColorTable(win->ps, 0, LCOLF_RGB, 0, 0, NULL);
	pm_hash_window(win);
	pm_unlock;
	return dev;

	r4:	WinReleasePS(win->ps);
	r3:	pm_unlock;
		pm_send_msg(MSG_DELETE_WINDOW, win);
	r2:	if (win->in) del_from_list(win);
		pm_unlock;
		mem_free(win);
	r1:
	return NULL;
	
}

void pm_shutdown_device(struct graphics_device *dev)
{
	struct pm_window *win = pm_win(dev);
	WinReleasePS(win->ps);
	pm_send_msg(MSG_DELETE_WINDOW, win);
	pm_unhash_window(win);
	if (win->in) del_from_list(win);
	pm_unlock;
	while (!list_empty(win->queue)) {
		struct pm_event *ev = win->queue.next;
		del_from_list(ev);
		free(ev);
	}
	mem_free(win);
	mem_free(dev);
}

void pm_shutdown_driver()
{
	pm_send_msg(MSG_SHUTDOWN_THREAD, NULL);
	GpiDestroyPS(hps_mem);
	DevCloseDC(hdc_mem);
	if (pm_bitmapinfo) mem_free(pm_bitmapinfo);
	set_handlers(pm_pipe[0], NULL, NULL, NULL, NULL);
	close(pm_pipe[0]);
	close(pm_pipe[1]);
	DosCloseEventSem(pm_sem);
	WinTerminate(hab);
	if (pm_bitmap_count) internal("pm_shutdown_driver: %d bitmaps leaked", pm_bitmap_count);
}

void pm_set_window_title(struct graphics_device *dev, unsigned char *title)
{
	static int idx = -1;
	struct conv_table *ct = get_translation_table(idx >= 0 ? idx : (idx = get_cp_index("utf-8")), pm_cp);
	struct title_set w;
	w.win = pm_win(dev);
	w.text = convert_string(ct, title, strlen(title), NULL);
	pm_send_msg(MSG_SET_WINDOW_TITLE, &w);
	pm_unlock;
	mem_free(w.text);
}

int pm_get_filled_bitmap(struct bitmap *bmp, long color)
{
	/* Mikulas jestlize plati ze get_color u pmshell nic nedela (jen oanduje
	 * 0xffffff), tak tady zavolej (*get_color_fn)(color) z dither.c a ta
	 * vrati long a, a ty udelas (void *)(&a) a budes mit ty bajty co chces
	 */
	internal("nedopsano");
	return 0;
}

int pm_get_empty_bitmap(struct bitmap *bmp)
{
	debug_call(("get_empty_bitmap (%dx%d)\n", bmp->x, bmp->y));
	bmp->skip = -((bmp->x * (pmshell_driver.depth & 7) + 3) & ~3);
	bmp->data = (char *)(bmp->flags = mem_alloc(-bmp->skip * bmp->y)) - bmp->skip * (bmp->y - 1);
	debug_call(("done\n"));
	return 1;
}

void pm_register_bitmap(struct bitmap *bmp)
{
	HBITMAP hbm;
	debug_call(("register_bitmap (%dx%d)\n", bmp->x, bmp->y));
	pm_bitmap_count++;
	pm_bitmapinfo->cx = bmp->x;
	pm_bitmapinfo->cy = bmp->y;
	hbm = GpiCreateBitmap(hps_msg, (PBITMAPINFOHEADER2)pm_bitmapinfo, CBM_INIT, bmp->flags, (PBITMAPINFO2)pm_bitmapinfo);
	mem_free(bmp->flags);
	bmp->flags = (void *)hbm;
	debug_call(("done\n"));
}

void *pm_prepare_strip(struct bitmap *bmp, int top, int lines)
{
	bmp->data = mem_alloc(-bmp->skip * lines);
	return (char *)bmp->data - bmp->skip * (lines - 1);
}


void pm_commit_strip(struct bitmap *bmp, int top, int lines)
{
	LONG s;
	HBITMAP old;
	old = GpiSetBitmap(hps_mem, (HBITMAP)bmp->flags);
	pm_bitmapinfo->cx = bmp->x;
	pm_bitmapinfo->cy = bmp->y;
	s = GpiSetBitmapBits(hps_mem, bmp->y - top - lines, lines, bmp->data, (PBITMAPINFO2)pm_bitmapinfo);
	GpiSetBitmap(hps_mem, old);
	mem_free(bmp->data);
}

void pm_unregister_bitmap(struct bitmap *bmp)
{
	debug_call(("unregister_bitmap (%dx%d)\n", bmp->x, bmp->y));
	pm_bitmap_count--;
	GpiDeleteBitmap((HBITMAP)bmp->flags);
	debug_call(("done\n"));
}

void pm_draw_bitmap(struct graphics_device *dev, struct bitmap *bmp, int x, int y)
{
	POINTL p;
	debug_call(("draw_bitmap (%dx%d -> %x,%x)\n", bmp->x, bmp->y, x, y));
	p.x = x;
	p.y = pm_win(dev)->y - y - bmp->y;
	WinDrawBitmap(pm_win(dev)->ps, (HBITMAP)bmp->flags, NULL, &p, 0, 1, DBM_NORMAL);
	debug_call(("done\n"));
}

void pm_draw_bitmaps(struct graphics_device *dev, struct bitmap **bmp, int n, int x, int y)
{
	HPS ps = pm_win(dev)->ps;
	POINTL p;
	debug_call(("draw_bitmaps\n"));
	p.x = x;
	p.y = pm_win(dev)->y - y - (*bmp)->y;
	while (n--) {
		WinDrawBitmap(ps, (HBITMAP)(*bmp)->flags, NULL, &p, 0xffffffff, 0, DBM_NORMAL);
		p.x += (*bmp++)->x;
	}
	debug_call(("done\n"));
}

long pm_get_color(int rgb)
{
	return rgb & 0xffffff;
}

void pm_fill_area(struct graphics_device *dev, int x1, int y1, int x2, int y2, long color)
{
	RECTL r;
	debug_call(("fill_area (%d,%d)->(%d,%d)\n", x1, y1, x2, y2));
	r.xLeft = x1;
	r.yBottom = pm_win(dev)->y - y2;
	r.xRight = x2;
	r.yTop = pm_win(dev)->y - y1;
	WinFillRect(pm_win(dev)->ps, &r, color);
	debug_call(("done\n"));
}

void pm_draw_hline(struct graphics_device *dev, int x1, int y, int x2, long color)
{
	HPS ps = pm_win(dev)->ps;
	POINTL p;
	debug_call(("draw_hline (%d,%d)->(%d)\n", x1, y, x2));
	if (x1 >= x2) {
		debug_call(("done\n"));
		return;
	}
	GpiSetColor(ps, color);
	p.x = x1;
	p.y = pm_win(dev)->y - y - 1;
	GpiMove(ps, &p);
	p.x = x2 - 1;
	GpiLine(ps, &p);
	debug_call(("done\n"));
}

void pm_draw_vline(struct graphics_device *dev, int x, int y1, int y2, long color)
{
	HPS ps = pm_win(dev)->ps;
	POINTL p;
	debug_call(("draw_vline (%d,%d)->(%d)\n", x, y1, y2));
	if (y1 >= y2) {
		debug_call(("done\n"));
		return;
	}
	GpiSetColor(ps, color);
	p.x = x;
	p.y = pm_win(dev)->y - y1 - 1;
	GpiMove(ps, &p);
	p.y = pm_win(dev)->y - y2;
	GpiLine(ps, &p);
	debug_call(("done\n"));
}

void pm_hscroll_redraws(struct pm_window *win, struct rect *r, int dir)
{
	struct pm_event *e;
	pm_cancel_event(win, E_REDRAW, &e);
	if (!e) return;
	if (dir > 0) {
		if (e->x2 > r->x1 && e->x2 < r->x2) {
			e->x2 += dir;
			if (e->x2 > r->x2) e->x2 = r->x2;
		}
	} else if (dir < 0) {
		if (e->x1 > r->x1 && e->x1 < r->x2) {
			e->x1 += dir;
			if (e->x1 < r->x1) e->x1 = r->x1;
		}
	}
}

int pm_hscroll(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	RECTL r;

	debug_call(("hscroll (%d)\n", sc));
	ignore=NULL;
	r.xLeft = dev->clip.x1;
	r.yBottom = pm_win(dev)->y - dev->clip.y2;
	r.xRight = dev->clip.x2;
	r.yTop = pm_win(dev)->y - dev->clip.y1;
	pm_lock;
	WinScrollWindow(pm_win(dev)->hc, sc, 0, &r, &r, NULLHANDLE, NULL, SW_INVALIDATERGN);
	pm_hscroll_redraws(pm_win(dev), &dev->clip, sc);
	pm_unlock;
	debug_call(("done\n"));
	return 0;
}

void pm_vscroll_redraws(struct pm_window *win, struct rect *r, int dir)
{
	struct pm_event *e;
	pm_cancel_event(win, E_REDRAW, &e);
	if (!e) return;
	if (dir > 0) {
		if (e->y2 > r->y1 && e->y2 < r->y2) {
			e->y2 += dir;
			if (e->y2 > r->y2) e->y2 = r->y2;
		}
	} else if (dir < 0) {
		if (e->y1 > r->y1 && e->y1 < r->y2) {
			e->y1 += dir;
			if (e->y1 < r->y1) e->y1 = r->y1;
		}
	}
}

int pm_vscroll(struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	RECTL r;

	debug_call(("vscroll (%d)\n", sc));
	ignore=NULL;
	r.xLeft = dev->clip.x1;
	r.yBottom = pm_win(dev)->y - dev->clip.y2;
	r.xRight = dev->clip.x2;
	r.yTop = pm_win(dev)->y - dev->clip.y1;
	pm_lock;
	WinScrollWindow(pm_win(dev)->hc, 0, -sc, &r, &r, NULLHANDLE, NULL, SW_INVALIDATERGN);
	pm_vscroll_redraws(pm_win(dev), &dev->clip, sc);
	pm_unlock;
	debug_call(("done\n"));
	return 0;
}

void pm_set_clip_area(struct graphics_device *dev, struct rect *rr)
{
	HPS ps = pm_win(dev)->ps;
	HRGN rg, org;
	RECTL r;
	debug_call(("set_clip_area (%d,%d)x(%d,%d)\n", rr->x1, rr->y1, rr->x2, rr->y2));
	memcpy(&dev->clip, rr, sizeof(struct rect));
	if (dev->clip.x1 >= dev->clip.x2 || dev->clip.y1 >= dev->clip.y2) dev->clip.x1 = dev->clip.x2 = dev->clip.y1 = dev->clip.y2 = 0;
	r.xLeft = dev->clip.x1;
	r.yBottom = pm_win(dev)->y - dev->clip.y2;
	r.xRight = dev->clip.x2;
	r.yTop = pm_win(dev)->y - dev->clip.y1;
	if ((rg = GpiCreateRegion(ps, 1, &r)) == RGN_ERROR) return;
	if (GpiSetClipRegion(ps, rg, &org) == RGN_ERROR) org = rg;
	GpiDestroyRegion(ps, org);
	debug_call(("done\n"));
}

void pm_put_to_clipboard(struct graphics_device *gd, char *string,int length)
{
}

void pm_request_clipboard(struct grphics_device *gd)
{
}

char *pm_get_from_clipboard(struct graphics_device *gd)
{
        return NULL;
}


struct graphics_driver pmshell_driver = {
	"pmshell",
	pm_init_driver,
	pm_init_device,
	pm_shutdown_device,
	pm_shutdown_driver,
	pm_get_driver_param,
	pm_get_empty_bitmap,
	pm_get_filled_bitmap,
	pm_register_bitmap,
	pm_prepare_strip,
	pm_commit_strip,
	pm_unregister_bitmap,
	pm_draw_bitmap,
	pm_draw_bitmaps,
	pm_get_color,
	pm_fill_area,
	pm_draw_hline,
	pm_draw_vline,
	pm_hscroll,
	pm_vscroll,
	pm_set_clip_area,
	dummy_block,
	dummy_unblock,
	pm_set_window_title,
        pm_put_to_clipboard,
        pm_request_clipboard,
        pm_get_from_clipboard.
        0,			/* depth */
	0, 0,			/* x, y */
	0,			/* flags */
};

#endif
