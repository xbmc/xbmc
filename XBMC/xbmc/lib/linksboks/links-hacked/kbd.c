
/* kbd.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

#define OUT_BUF_SIZE	16384
#define IN_BUF_SIZE	16

#define USE_TWIN_MOUSE	1
#define TW_BUTT_LEFT	1
#define TW_BUTT_MIDDLE	2
#define TW_BUTT_RIGHT	4

//struct itrm {
//	int std_in;
//	int std_out;
//	int sock_in;
//	int sock_out;
//	int ctl_in;
//	int blocked;
//	struct termios t;
//	int flags;
//	unsigned char kqueue[IN_BUF_SIZE];
//	int qlen;
//	int tm;
//	void (*queue_event)(struct itrm *, unsigned char *, int);
//	unsigned char *ev_queue;
//	int eqlen;
//	void *mouse_h;
//	unsigned char *orig_title;
//	void (*free_trm)(struct itrm *);
//};

void free_trm(struct itrm *);
void in_kbd(struct itrm *);
void in_sock(struct itrm *);

struct itrm *ditrm = NULL;

int is_blocked()
{
	return 0;
}

void free_all_itrms()
{
}

void write_ev_queue(struct itrm *itrm)
{
	//int l;
	//if (!itrm->eqlen) internal("event queue empty");
	//if ((l = write(itrm->sock_out, itrm->ev_queue, itrm->eqlen > 128 ? 128 : itrm->eqlen)) == -1) {
	//	itrm->free_trm(itrm);
	//	return;
	//}
	//memmove(itrm->ev_queue, itrm->ev_queue + l, itrm->eqlen -= l);
	//if (!itrm->eqlen) set_handlers(itrm->sock_out, get_handler(itrm->sock_out, H_READ), NULL, get_handler(itrm->sock_out, H_ERROR), get_handler(itrm->sock_out, H_DATA));
}

void queue_event(struct itrm *itrm, unsigned char *data, int len)
{
	//int w = 0;
	//if (!len) return;
	//if (!itrm->eqlen && can_write(itrm->sock_out) && (w = write(itrm->sock_out, data, len)) <= 0) {
	//	/*free_trm(itrm);*/
	//	register_bottom_half((void (*)(void *))itrm->free_trm, itrm);
	//	return;
	//}
	//if (w < len) {
	//	unsigned char *c;
	//	if (!(c = mem_realloc(itrm->ev_queue, itrm->eqlen + len - w))) {
	//		/*free_trm(itrm);*/
	//		register_bottom_half((void (*)(void *))itrm->free_trm, itrm);
	//		return;
	//	}
	//	itrm->ev_queue = c;
	//	memcpy(itrm->ev_queue + itrm->eqlen, data + w, len - w);
	//	itrm->eqlen += len - w;
	//	set_handlers(itrm->sock_out, get_handler(itrm->sock_out, H_READ), (void (*)(void *))write_ev_queue, (void (*)(void *))itrm->free_trm, itrm);
	//}
}

void kbd_ctrl_c()
{
}

void send_init_sequence(int h,int flags)
{
}

void send_term_sequence(int h,int flags)
{
}

void resize_terminal()
{
}

int setraw(int fd, struct termios *p)
{
	return 0;
}

void handle_trm(int std_in, int std_out, int sock_in, int sock_out, int ctl_in, void *init_string, int init_len)
{
	//int x, y;
	//struct itrm *itrm;
	//struct event ev = { EV_INIT, 80, 24, 0 };
	//unsigned char *ts;
	//int xwin;
	//if (get_terminal_size(ctl_in, &x, &y)) {
	//	error("ERROR: could not get terminal size");
	//	return;
	//}
	//if (!(itrm = mem_alloc(sizeof(struct itrm)))) return;
	//itrm->queue_event = queue_event;
	//itrm->free_trm = free_trm;
	//ditrm = itrm;
	//itrm->std_in = std_in;
	//itrm->std_out = std_out;
	//itrm->sock_in = sock_in;
	//itrm->sock_out = sock_out;
	//itrm->ctl_in = ctl_in;
	//itrm->blocked = 0;
	//itrm->qlen = 0;
	//itrm->tm = -1;
	//itrm->ev_queue = DUMMY;
	//itrm->eqlen = 0;
	//if (ctl_in >= 0) setraw(ctl_in, &itrm->t);
	//set_handlers(std_in, (void (*)(void *))in_kbd, NULL, (void (*)(void *))itrm->free_trm, itrm);
	//if (sock_in != std_out) set_handlers(sock_in, (void (*)(void *))in_sock, NULL, (void (*)(void *))itrm->free_trm, itrm);
	//ev.x = x;
	//ev.y = y;
	//handle_terminal_resize(ctl_in, resize_terminal);
	//queue_event(itrm, (char *)&ev, sizeof(struct event));
	//xwin = is_xterm() * ENV_XWIN + can_twterm() * ENV_TWIN + (!!getenv("STY")) * ENV_SCREEN + get_system_env();
	//itrm->flags = 0;
	//if (!(ts = getenv("TERM"))) ts = "";
	//if ((xwin & ENV_TWIN) && !strcmp(ts,"linux")) itrm->flags |= USE_TWIN_MOUSE;
	//if (strlen(ts) >= MAX_TERM_LEN) queue_event(itrm, ts, MAX_TERM_LEN);
	//else {
	//	unsigned char *mm;
	//	int ll = MAX_TERM_LEN - strlen(ts);
	//	queue_event(itrm, ts, strlen(ts));
	//	if (!(mm = mem_alloc(ll))) {
	//		itrm->free_trm(itrm);
	//		return;
	//	}
	//	memset(mm, 0, ll);
	//	queue_event(itrm, mm, ll);
	//	mem_free(mm);
	//}
	//if (!(ts = get_cwd()) && !(ts = stracpy(""))) goto neni_pamet;
	//if (strlen(ts) >= MAX_CWD_LEN) queue_event(itrm, ts, MAX_CWD_LEN);
	//else {
	//	unsigned char *mm;
	//	int ll = MAX_CWD_LEN - strlen(ts);
	//	queue_event(itrm, ts, strlen(ts));
	//	if (!(mm = mem_alloc(ll))) {
	//		itrm->free_trm(itrm);
	//		return;
	//	}
	//	memset(mm, 0, ll);
	//	queue_event(itrm, mm, ll);
	//	mem_free(mm);
	//}
	//mem_free(ts);
	//neni_pamet:
	//queue_event(itrm, (char *)&xwin, sizeof(int));
	//queue_event(itrm, (char *)&init_len, sizeof(int));
	//queue_event(itrm, (char *)init_string, init_len);
	//itrm->mouse_h = handle_mouse(0, (void (*)(void *, unsigned char *, int))queue_event, itrm);
	//itrm->orig_title = get_window_title();
	//set_window_title("Links");
	//send_init_sequence(std_out,itrm->flags);
}

void unblock_itrm_x(void *h)
{
}

int unblock_itrm(int fd)
{
	return 0;
}

void block_itrm(int fd)
{
}

void free_trm(struct itrm *itrm)
{
//#ifndef __XBOX__
//	if (!itrm) return;
//	set_window_title(itrm->orig_title);
//	if (itrm->orig_title) mem_free(itrm->orig_title), itrm->orig_title = NULL;
//	unhandle_terminal_resize(itrm->ctl_in);
//	send_term_sequence(itrm->std_out,itrm->flags);
//	tcsetattr(itrm->ctl_in, TCSANOW, &itrm->t);
//	if (itrm->mouse_h) unhandle_mouse(itrm->mouse_h);
//	set_handlers(itrm->std_in, NULL, NULL, NULL, NULL);
//	set_handlers(itrm->sock_in, NULL, NULL, NULL, NULL);
//	set_handlers(itrm->std_out, NULL, NULL, NULL, NULL);
//	set_handlers(itrm->sock_out, NULL, NULL, NULL, NULL);
//	if (itrm->tm != -1) kill_timer(itrm->tm);
//	mem_free(itrm->ev_queue);
//	mem_free(itrm);
//	if (itrm == ditrm) ditrm = NULL;
//#endif
}

void resize_terminal_x(unsigned char *text)
{
}

void dispatch_special(unsigned char *text)
{
}

unsigned char buf[OUT_BUF_SIZE];

/*#define RD ({ char cc; if (p < c) cc = buf[p++]; else if ((dl = hard_read(itrm->sock_in, &cc, 1)) <= 0) {debug("%d %d", dl, errno);goto fr;} cc; })*/
/*udefine RD ({ char cc; if (p < c) cc = buf[p++]; else if ((hard_read(itrm->sock_in, &cc, 1)) <= 0) goto fr; cc; })*/
#define RD(xx) { unsigned char cc; if (p < c) cc = buf[p++]; else if ((hard_read(itrm->sock_in, &cc, 1)) <= 0) goto fr; xx = cc; }

void in_sock(struct itrm *itrm)
{
	//unsigned char *path, *delete;
	//int pl, dl;
	//char ch;
	//int fg;
	//int c, i, p;
	//if ((c = read(itrm->sock_in, buf, OUT_BUF_SIZE)) <= 0) {
	//	fr:
	//	itrm->free_trm(itrm);
	//	terminate_loop = 1;
	//	return;
	//}
	//qwerty:
	//for (i = 0; i < c; i++) if (!buf[i]) goto ex;
	//if (!is_blocked()) {
	//	want_draw();
	//	hard_write(itrm->std_out, buf, c);
	//	done_draw();
	//}
	//return;
	//ex:
	//if (!is_blocked()) {
	//	want_draw();
	//	hard_write(itrm->std_out, buf, i);
	//	done_draw();
	//}
	//i++;
	//memmove(buf, buf + i, OUT_BUF_SIZE - i);
	//c -= i;
	//p = 0;
	///*fg = RD;*/
	//RD(fg);
	//path = init_str();
	//delete = init_str();
	//pl = dl = 0;
	//while (1) {
	//	RD(ch);
	//	if (!ch) break;
	//	add_chr_to_str(&path, &pl, ch);
	//}
	//while (1) {
	//	RD(ch);
	//	if (!ch) break;
	//	add_chr_to_str(&delete, &dl, ch);
	//}
	//if (!*path) {
	//	dispatch_special(delete);
	//} else {
	//	int blockh;
	//	unsigned char *param;
	//	if (is_blocked() && fg) {
	//		if (*delete) unlink(delete);
	//		goto to_je_ale_hnus;
	//	}
	//	if (!(param = mem_alloc(strlen(path) + strlen(delete) + 3))) goto to_je_ale_hnus;
	//	param[0] = fg;
	//	strcpy(param + 1, path);
	//	strcpy(param + 1 + strlen(path) + 1, delete);
	//	if (fg == 1) block_itrm(itrm->ctl_in);
	//	if ((blockh = start_thread((void (*)(void *, int))exec_thread, param, strlen(path) + strlen(delete) + 3)) == -1) {
	//		if (fg == 1) unblock_itrm(itrm->ctl_in);
	//		mem_free(param);
	//		goto to_je_ale_hnus;
	//	}
	//	mem_free(param);
	//	if (fg == 1) {
	//		set_handlers(blockh, (void (*)(void *))unblock_itrm_x, NULL, (void (*)(void *))unblock_itrm_x, (void *)blockh);
	//		/*block_itrm(itrm->ctl_in);*/
	//	} else {
	//		set_handlers(blockh, close_handle, NULL, close_handle, (void *)blockh);
	//	}
	//}
	//to_je_ale_hnus:
	//mem_free(path);
	//mem_free(delete);
	//memmove(buf, buf + p, OUT_BUF_SIZE - p);
	//c -= p;
	//goto qwerty;
}

int process_queue(struct itrm *);

void kbd_timeout(struct itrm *itrm)
{
	//struct event ev = {EV_KBD, KBD_ESC, 0, 0};
	//itrm->tm = -1;
	//if (can_read(itrm->std_in)) {
	//	in_kbd(itrm);
	//	return;
	//}
	//if (!itrm->qlen) {
	//	internal("timeout on empty queue");
	//	return;
	//}
	//itrm->queue_event(itrm, (char *)&ev, sizeof(struct event));
	//if (--itrm->qlen) memmove(itrm->kqueue, itrm->kqueue+1, itrm->qlen);
	//while (process_queue(itrm)) ;
}

int get_esc_code(char *str, int len, char *code, int *num, int *el)
{
	return 0;
}

int xterm_button = -1;

int process_queue(struct itrm *itrm)
{
	//struct event ev = {EV_KBD, -1, 0, 0};
	//int el = 0;
	//if (!itrm->qlen) goto end;
	//if (itrm->kqueue[0] == '\033') {
	//	if (itrm->qlen < 2) goto ret;
	//	if (itrm->kqueue[1] == '[' || itrm->kqueue[1] == 'O') {
	//		char c;
	//		int v;
	//		if (itrm->qlen >= 4 && itrm->kqueue[2] == '[') {
	//			if (itrm->kqueue[3] < 'A' || itrm->kqueue[3] > 'L') goto ret;
	//			ev.x = KBD_F1 - (itrm->kqueue[3] - 'A');
	//			el = 4;
	//		} else if (get_esc_code(itrm->kqueue, itrm->qlen, &c, &v, &el)) goto ret;
	//		else switch (c) {
	//			case 'A': ev.x = KBD_UP; break;
	//			case 'B': ev.x = KBD_DOWN; break;
	//			case 'C': ev.x = KBD_RIGHT; break;
	//			case 'D': ev.x = KBD_LEFT; break;
	//			case 'F':
	//			case 'e': ev.x = KBD_END; break;
	//			case 'H':
	//			case 0: ev.x = KBD_HOME; break;
	//			case 'I': ev.x = KBD_PAGE_UP; break;
	//			case 'G': ev.x = KBD_PAGE_DOWN; break;

	//			case 'z': switch (v) {
	//				case 247: ev.x = KBD_INS; break;
	//				case 214: ev.x = KBD_HOME; break;
	//				case 220: ev.x = KBD_END; break;
	//				case 216: ev.x = KBD_PAGE_UP; break;
	//				case 222: ev.x = KBD_PAGE_DOWN; break;
	//				case 249: ev.x = KBD_DEL; break;
	//				} break;
	//			case '~': switch (v) {
	//				case 1: ev.x = KBD_HOME; break;
	//				case 2: ev.x = KBD_INS; break;
	//				case 3: ev.x = KBD_DEL; break;
	//				case 4: ev.x = KBD_END; break;
	//				case 5: ev.x = KBD_PAGE_UP; break;
	//				case 6: ev.x = KBD_PAGE_DOWN; break;
	//				case 7: ev.x = KBD_HOME; break;
	//				case 8: ev.x = KBD_END; break;
	//				case 17: ev.x = KBD_F6; break;
	//				case 18: ev.x = KBD_F7; break;
	//				case 19: ev.x = KBD_F8; break;
	//				case 20: ev.x = KBD_F9; break;
	//				case 21: ev.x = KBD_F10; break;
	//				case 23: ev.x = KBD_F11; break;
	//				case 24: ev.x = KBD_F12; break;
	//				} break;
	//			case 'R':
	//					  resize_terminal (); break ;
	//			case 'M': if (itrm->qlen - el < 3) goto ret;
	//				if (v == 5) {
	//					if (xterm_button == -1) xterm_button = 0; /* */
	//					if (itrm->qlen - el < 5) goto ret;
	//					ev.x = (unsigned char)(itrm->kqueue[el+1]) - ' ' - 1 + ((int)((unsigned char)(itrm->kqueue[el+2]) - ' ' - 1) << 7);
	//					if ( ev.x & (1 << 13)) ev.x = 0; /* ev.x |= ~0 << 14; */
	//					ev.y = (unsigned char)(itrm->kqueue[el+3]) - ' ' - 1 + ((int)((unsigned char)(itrm->kqueue[el+4]) - ' ' - 1) << 7);
	//					if ( ev.y & (1 << 13)) ev.y = 0; /* ev.y |= ~0 << 14; */
	//					switch ((itrm->kqueue[el] - ' ') ^ xterm_button) { /* Every event changhes only one bit */
	//					    case TW_BUTT_LEFT:   ev.b = B_LEFT | ( (xterm_button & TW_BUTT_LEFT) ? B_UP : B_DOWN ); break; 
	//					    case TW_BUTT_MIDDLE: ev.b = B_MIDDLE | ( (xterm_button & TW_BUTT_MIDDLE) ? B_UP :  B_DOWN ); break;
	//					    case TW_BUTT_RIGHT:  ev.b = B_RIGHT | ( (xterm_button & TW_BUTT_RIGHT) ? B_UP : B_DOWN ); break;
	//					    case 0: ev.b = B_DRAG;
	//					    /* default : Twin protocol error */
	//					}
	//					xterm_button = itrm->kqueue[el] - ' ';
	//					el += 5;
 //                                       } else {
 //                                               ev.x = itrm->kqueue[el+1] - ' ' - 1;
	//					ev.y = itrm->kqueue[el+2] - ' ' - 1;
	//					/* There are rumours arising from remnants of code dating to
	//					 * the ancient Mikulas' times that bit 4 indicated B_DRAG.
	//					 * However, I didn't find on what terminal it should be ever
	//					 * supposed to work and it conflicts with wheels. So I removed
	//					 * the last remnants of the code as well. --pasky */

 //                                               ev.b = (itrm->kqueue[el] & 7) | B_DOWN;

 //                                               /* smartglasses1 - rxvt wheel: */
	//					if (ev.b == 3 && xterm_button != -1)
 //                                                               ev.b = xterm_button | B_UP;
 //                                               else if (ev.b == 3)
 //                                                       ev.b = B_WHEELUP;
 //                                               else if (ev.b == 4)
 //                                                       ev.b = B_WHEELDOWN;

 //                                               /* xterm wheel: */
	//					if ((itrm->kqueue[el] & 96) == 96)
 //                                                       ev.b = (itrm->kqueue[el] & 1) ? B_WHEELDOWN : B_WHEELUP;

	//					xterm_button = -1;
	//					/* XXX: Eterm/aterm uses rxvt-like reporting, but sends the
	//					 * release sequence for wheel. rxvt itself sends only press
	//					 * sequence. Since we can't reliably guess what we're talking
	//					 * with from $TERM, we will rather support Eterm/aterm, as in
	//					 * rxvt, at least each second wheel up move will work. */
 //                                               if ((ev.b & BM_ACT) == B_DOWN)
 //                                                               xterm_button = ev.b & BM_BUTT;

 //                                               el += 3;
 //                                       }
	//				ev.ev = EV_MOUSE;
	//				break;
	//		}
	//	} else {
	//		el = 2;
	//		if (itrm->kqueue[1] >= ' ') {
	//			ev.x = itrm->kqueue[1];
	//			ev.y = KBD_ALT;
	//			goto l2;
	//		}
	//		if (itrm->kqueue[1] == '\033') {
	//			if (itrm->qlen >= 3 && (itrm->kqueue[2] == '[' || itrm->kqueue[2] == 'O')) el = 1;
	//			ev.x = KBD_ESC;
	//			goto l2;
	//		}
	//	}
	//	goto l1;
	//} else if (itrm->kqueue[0] == 0) {
	//	if (itrm->qlen < 2) goto ret;
	//	if (!(ev.x = os2xtd[itrm->kqueue[1]].x)) ev.x = -1;
	//	ev.y = os2xtd[itrm->kqueue[1]].y;
	//	el = 2;
	//	/*printf("%02x - %02x %02x\n", (int)itrm->kqueue[1], ev.x, ev.y);*/
	//	goto l1;
	//}
	//el = 1;
	//ev.x = itrm->kqueue[0];
	//l2:
	//if (ev.x == 1) ev.x = KBD_HOME;
	//if (ev.x == 2) ev.x = KBD_PAGE_UP;
	//if (ev.x == 4) ev.x = KBD_DEL;
	//if (ev.x == 5) ev.x = KBD_END;
	//if (ev.x == 6) ev.x = KBD_PAGE_DOWN;
	//if (ev.x == 8) ev.x = KBD_BS;
	//if (ev.x == 9) ev.x = KBD_TAB;
	//if (ev.x == 10) ev.x = KBD_ENTER, ev.y = KBD_CTRL;
	//if (ev.x == 13) ev.x = KBD_ENTER;
	//if (ev.x == 127) ev.x = KBD_BS;
	//if (ev.x >= 0 && ev.x < ' ') {
	//	ev.x += 'A' - 1;
	//	ev.y = KBD_CTRL;
	//}
	//l1:
	//if (itrm->qlen < el) {
	//	internal("event queue underflow");
	//	itrm->qlen = el;
	//}
	//if (ev.x != -1) {
	//	itrm->queue_event(itrm, (char *)&ev, sizeof(struct event));
	//	memmove(itrm->kqueue, itrm->kqueue + el, itrm->qlen -= el);
	//} else {
	//	/*printf("%d %d\n", itrm->qlen, el);fflush(stdout);*/
	//	memmove(itrm->kqueue, itrm->kqueue + el, itrm->qlen -= el);
	//}
	//end:
	//if (itrm->qlen < IN_BUF_SIZE && !itrm->blocked) set_handlers(itrm->std_in, (void (*)(void *))in_kbd, NULL, (void (*)(void *))itrm->free_trm, itrm);
	//return el;
	//ret:
	//itrm->tm = install_timer(ESC_TIMEOUT, (void (*)(void *))kbd_timeout, itrm);
	return 0;
}

void in_kbd(struct itrm *itrm)
{
//	int r;
//	if (!can_read(itrm->std_in)) return;
//	if (itrm->tm != -1) kill_timer(itrm->tm), itrm->tm = -1;
//	if (itrm->qlen >= IN_BUF_SIZE) {
//		set_handlers(itrm->std_in, NULL, NULL, (void (*)(void *))itrm->free_trm, itrm);
//		while (process_queue(itrm));
//		return;
//	}
//	if ((r = read(itrm->std_in, itrm->kqueue + itrm->qlen, IN_BUF_SIZE - itrm->qlen)) <= 0) {
//		itrm->free_trm(itrm);
//		return;
//	}
//	if ((itrm->qlen += r) > IN_BUF_SIZE) {
//		error("ERROR: too many bytes read");
//		itrm->qlen = IN_BUF_SIZE;
//	}
//	while (process_queue(itrm));
}
