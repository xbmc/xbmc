/* ftp.c
 * ftp:// processing
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

int fast_ftp = 0;

#define FTP_BUF	16384

struct ftp_connection_info {
	int pending_commands;
	int opc;
	int dir;
	int rest_sent;
	int conn_st;
	int d;
	int dpos;
	int buf_pos;
	unsigned char ftp_buffer[FTP_BUF];
	unsigned char cmdbuf[1];
};

void ftp_login(struct connection *);
void ftp_logged(struct connection *);
void ftp_sent_passwd(struct connection *);
void ftp_got_reply(struct connection *, struct read_buffer *);
void ftp_got_info(struct connection *, struct read_buffer *);
void ftp_got_user_info(struct connection *, struct read_buffer *);
void ftp_dummy_info(struct connection *, struct read_buffer *);
void ftp_pass_info(struct connection *, struct read_buffer *);
void ftp_send_retr_req(struct connection *, int);
struct ftp_connection_info *add_file_cmd_to_str(struct connection *);
void ftp_retr_1(struct connection *);
void ftp_retr_file(struct connection *, struct read_buffer *);
void ftp_got_final_response(struct connection *, struct read_buffer *);
void got_something_from_data_connection(struct connection *);
void ftp_end_request(struct connection *);

int get_ftp_response(struct connection *c, struct read_buffer *rb, int part)
{
	int l;
	set_timeout(c);
	again:
	for (l = 0; l < rb->len; l++) if (rb->data[l] == 10) {
		unsigned char *e;
		int k = strtoul(rb->data, (char **)&e, 10);
		if (e != rb->data + 3 || k < 100) return -1;
		if (*e == '-') {
			int i;
			for (i = 0; i < rb->len - 5; i++) {
				if (rb->data[i] == 10 && !memcmp(rb->data+i+1, rb->data, 3) && rb->data[i+4] == ' ') {
					for (i++; i < rb->len; i++) if (rb->data[i] == 10) goto ok;
					return 0;
				}
			}
			return 0;
			ok:
			l = i;
		}
		if (!part && k >= 100 && k < 200) {
			kill_buffer_data(rb, l + 1);
			goto again;
		}
		if (part == 2) return k;
		kill_buffer_data(rb, l + 1);
		return k;
	}
	return 0;
}

void ftp_func(struct connection *c)
{
	/*setcstate(c, S_CONN);*/
	set_timeout(c);
	if (get_keepalive_socket(c)) {
		int p;
		if ((p = get_port(c->url)) == -1) {
			setcstate(c, S_INTERNAL);
			abort_connection(c);
			return;
		}
		make_connection(c, p, &c->sock1, ftp_login);
	} else ftp_send_retr_req(c, S_SENT);
}

void ftp_login(struct connection *c)
{
	unsigned char *login;
	unsigned char *u;
	int logl = 0;
	if (!(login = init_str())) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	add_to_str(&login, &logl, "USER ");
	if ((u = get_user_name(c->url)) && *u) add_to_str(&login, &logl, u);
	else add_to_str(&login, &logl, "anonymous");
	if (u) mem_free(u);
	if (fast_ftp) {
		struct ftp_connection_info *fi;
		add_to_str(&login, &logl, "\r\nPASS ");
		if ((u = get_pass(c->url)) && *u) add_to_str(&login, &logl, u);
		else add_to_str(&login, &logl, options_get("ftp_anonymous_password"));
		if (u) mem_free(u);
		add_to_str(&login, &logl, "\r\n");
		if (!(fi = add_file_cmd_to_str(c))) {
			mem_free(login);
			return;
		}
		add_to_str(&login, &logl, fi->cmdbuf);
	} else add_to_str(&login, &logl, "\r\n");
	write_to_socket(c, c->sock1, login, strlen(login), ftp_logged);
	mem_free(login);
	setcstate(c, S_SENT);
}

void ftp_logged(struct connection *c)
{
	struct read_buffer *rb;
	if (!(rb = alloc_read_buffer(c))) return;
	read_from_socket(c, c->sock1, rb, ftp_got_info);
}

void ftp_got_info(struct connection *c, struct read_buffer *rb)
{
	int g = get_ftp_response(c, rb, 0);
	if (g == -1) { setcstate(c, S_FTP_ERROR); abort_connection(c); return; }
	if (!g) { read_from_socket(c, c->sock1, rb, ftp_got_info); return; }
	if (g >= 400) { setcstate(c, S_FTP_UNAVAIL); retry_connection(c); return; }
	ftp_got_user_info(c, rb);
}

void ftp_got_user_info(struct connection *c, struct read_buffer *rb)
{
	int g = get_ftp_response(c, rb, 0);
	if (g == -1) { setcstate(c, S_FTP_ERROR); abort_connection(c); return; }
	if (!g) { read_from_socket(c, c->sock1, rb, ftp_got_user_info); return; }
	if (g >= 530 && g < 540) { setcstate(c, S_FTP_LOGIN); retry_connection(c); return; }
	if (g >= 400) { setcstate(c, S_FTP_UNAVAIL); retry_connection(c); return; }
	if (g >= 200 && g < 300) {
		if (fast_ftp) ftp_dummy_info(c, rb);
		else ftp_send_retr_req(c, S_GETH);
	} else {
		if (fast_ftp) ftp_pass_info(c, rb);
		else {
			unsigned char *login;
			unsigned char *u;
			int logl = 0;
			if (!(login = init_str())) {
				setcstate(c, S_OUT_OF_MEM);
				abort_connection(c);
				return;
			}
			add_to_str(&login, &logl, "PASS ");
			if ((u = get_pass(c->url)) && *u) add_to_str(&login, &logl, u);
			else add_to_str(&login, &logl, options_get("ftp_anonymous_password"));
			if (u) mem_free(u);
			add_to_str(&login, &logl, "\r\n");
			write_to_socket(c, c->sock1, login, strlen(login), ftp_sent_passwd);
			mem_free(login);
			setcstate(c, S_LOGIN);
		}
	}
}

void ftp_dummy_info(struct connection *c, struct read_buffer *rb)
{
	int g = get_ftp_response(c, rb, 0);
	if (g == -1) { setcstate(c, S_FTP_ERROR); abort_connection(c); return; }
	if (!g) { read_from_socket(c, c->sock1, rb, ftp_dummy_info); return; }
	ftp_retr_file(c, rb);
}

void ftp_sent_passwd(struct connection *c)
{
	struct read_buffer *rb;
	if (!(rb = alloc_read_buffer(c))) return;
	read_from_socket(c, c->sock1, rb, ftp_pass_info);
}

void ftp_pass_info(struct connection *c, struct read_buffer *rb)
{
	int g = get_ftp_response(c, rb, 0);
	if (g == -1) { setcstate(c, S_FTP_ERROR); abort_connection(c); return; }
	if (!g) { read_from_socket(c, c->sock1, rb, ftp_pass_info); setcstate(c, S_LOGIN); return; }
	if (g >= 530 && g < 540) { setcstate(c, S_FTP_LOGIN); abort_connection(c); return; }
	if (g >= 400) { setcstate(c, S_FTP_UNAVAIL); abort_connection(c); return; }
	if (fast_ftp) ftp_retr_file(c, rb);
	else ftp_send_retr_req(c, S_GETH);
}

struct ftp_connection_info *add_file_cmd_to_str(struct connection *c)
{
	unsigned char *d = get_url_data(c->url);
	unsigned char *de;
	unsigned char pc[6];
	int ps;
	struct ftp_connection_info *inf, *inf2;
	unsigned char *s;
	int l;
	if (!(inf = mem_alloc(sizeof(struct ftp_connection_info)))) {
		oom:
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return NULL;
	}
	memset(inf, 0, sizeof(struct ftp_connection_info));
	l = 0;
	if (!(s = init_str())) {
		mem_free(inf);
		goto oom;
	}
	c->info = inf;
	if ((ps = get_pasv_socket(c, c->sock1, &c->sock2, pc))) return NULL;
	if (!d) {
		internal("get_url_data failed");
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return NULL;
	}
	if (!(de = strchr(d, POST_CHAR))) de = d + strlen(d);
	if (d == de || de[-1] == '/') {
		inf->dir = 1;
		inf->pending_commands = 4;
		add_to_str(&s, &l, "TYPE A\r\nPORT ");
		add_num_to_str(&s, &l, pc[0]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[1]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[2]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[3]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[4]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[5]);
		add_to_str(&s, &l, "\r\nCWD /");
		add_bytes_to_str(&s, &l, d, de - d);
		add_to_str(&s, &l, "\r\nLIST\r\n");
		c->from = 0;
	} else {
		inf->dir = 0;
		inf->pending_commands = 3;
		add_to_str(&s, &l, "TYPE I\r\nPORT ");
		add_num_to_str(&s, &l, pc[0]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[1]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[2]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[3]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[4]);
		add_chr_to_str(&s, &l, ',');
		add_num_to_str(&s, &l, pc[5]);
		if (c->from) {
			add_to_str(&s, &l, "\r\nREST ");
			add_num_to_str(&s, &l, c->from);
			inf->rest_sent = 1;
			inf->pending_commands++;
		}
		add_to_str(&s, &l, "\r\nRETR /");
		add_bytes_to_str(&s, &l, d, de - d);
		add_to_str(&s, &l, "\r\n");
	}
	inf->opc = inf->pending_commands;
	if ((inf2 = mem_realloc(inf, sizeof(struct ftp_connection_info) + l + 1)))
		strcpy((inf = inf2)->cmdbuf, s);
	mem_free(s);
	c->info = inf;
	return inf;
}


void ftp_send_retr_req(struct connection *c, int state)
{
	struct ftp_connection_info *fi;
	unsigned char *login;
	int logl = 0;
	if (!(login = init_str())) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	if (!c->info && !(fi = add_file_cmd_to_str(c))) {
		mem_free(login);
		return;
	} else fi = c->info;
	if (fast_ftp) a:add_to_str(&login, &logl, fi->cmdbuf);
	else {
		unsigned char *nl = strchr(fi->cmdbuf, '\n');
		if (!nl) goto a;
		nl++;
		add_bytes_to_str(&login, &logl, fi->cmdbuf, nl - fi->cmdbuf);
		memmove(fi->cmdbuf, nl, strlen(nl) + 1);
	}
	write_to_socket(c, c->sock1, login, strlen(login), ftp_retr_1);
	mem_free(login);
	setcstate(c, state);
}

void ftp_retr_1(struct connection *c)
{
	struct read_buffer *rb;
	if (!(rb = alloc_read_buffer(c))) return;
	read_from_socket(c, c->sock1, rb, ftp_retr_file);
}

void ftp_retr_file(struct connection *c, struct read_buffer *rb)
{
	int g;
	struct ftp_connection_info *inf = c->info;
	if (0) {
		rep:
		if (!fast_ftp) {
			ftp_send_retr_req(c, S_GETH);
			return;
		}
	}
	if (inf->pending_commands > 1) {
		g = get_ftp_response(c, rb, 0);
		if (g == -1) { setcstate(c, S_FTP_ERROR); abort_connection(c); return; }
		if (!g) { read_from_socket(c, c->sock1, rb, ftp_retr_file); setcstate(c, S_GETH); return; }
		inf->pending_commands--;
		switch (inf->opc - inf->pending_commands) {
			case 1:		/* TYPE */
				goto rep;
			case 2:		/* PORT */
				if (g >= 400) { setcstate(c, S_FTP_PORT); abort_connection(c); return; }
				goto rep;
			case 3:		/* REST / CWD */
				if (g >= 400) {
					if (!inf->dir) c->from = 0;
					else { setcstate(c, S_FTP_NO_FILE); abort_connection(c); return; }
				}
				goto rep;
		}
		internal("WHAT???");
	}
	g = get_ftp_response(c, rb, 2);
	if (!g) { read_from_socket(c, c->sock1, rb, ftp_retr_file); setcstate(c, S_GETH); return; }
	if (g >= 100 && g < 200) {
		int est;
		unsigned char *d = rb->data;
		int i, p = 0;
		for (i = 0; i < rb->len && d[i] != 10; i++) if (d[i] == '(') p = i;
		if (!p || p == rb->len - 1) goto nol;
		p++;
		if (d[p] < '0' || d[p] > '9') goto nol;
		for (i = p; i < rb->len; i++) if (d[i] < '0' || d[i] > '9') goto quak;
		goto nol;
		quak:
		for (; i < rb->len; i++) if (d[i] != ' ') break;
		if (i + 4 > rb->len) goto nol;
		if (casecmp(&d[i], "byte", 4)) goto nol;
		est = strtol(&d[p], NULL, 10);
		if (est && !c->from) c->est_length = est; /* !!! FIXME: when I add appedning to downloaded file */
		nol:;
	}
	set_handlers(c->sock2, (void (*)(void *))got_something_from_data_connection, NULL, NULL, c);
	/*read_from_socket(c, c->sock1, rb, ftp_got_final_response);*/
	ftp_got_final_response(c, rb);
}

void ftp_got_final_response(struct connection *c, struct read_buffer *rb)
{
	struct ftp_connection_info *inf = c->info;
	int g = get_ftp_response(c, rb, 0);
	if (g == -1) { setcstate(c, S_FTP_ERROR); abort_connection(c); return; }
	if (!g) { read_from_socket(c, c->sock1, rb, ftp_got_final_response); if (c->state != S_TRANS) setcstate(c, S_GETH); return; }
	if (g == 550) {
		if (!c->cache && get_cache_entry(c->url, &c->cache)) {
			setcstate(c, S_OUT_OF_MEM);
			abort_connection(c);
			return;
		}
		if (c->cache->redirect) mem_free(c->cache->redirect);
		c->cache->redirect = stracpy(c->url);
		c->cache->redirect_get = 1;
		add_to_strn(&c->cache->redirect, "/");
		c->cache->incomplete = 0;
		/*setcstate(c, S_FTP_NO_FILE);*/
		setcstate(c, S_OKAY);
		abort_connection(c);
		return;
	}
	if (g >= 400) { setcstate(c, S_FTP_FILE_ERROR); abort_connection(c); return; }
	if (inf->conn_st == 2) {
		setcstate(c, S_OKAY);
		ftp_end_request(c);
	} else {
		inf->conn_st = 1;
		if (c->state != S_TRANS) setcstate(c, S_GETH);
	}
}

#define accept_char(x) ((upcase((x)) >= 'A' && upcase((x)) <= 'Z') || ((x) >= '0' && (x) <= '9') || (x) == ' ' || (x) == '-' || (x) == '_' || (x) == '.' || (x) == ':' || (x) == ';')

void add_conv_str(unsigned char **s, int *l, unsigned char *b, int ll)
{
	for (; ll; ll--, b++) {
		if (accept_char(*b)) add_chr_to_str(s, l, *b);
		else {
			add_to_str(s, l, "&#");
			add_num_to_str(s, l, (int)*b);
			add_chr_to_str(s, l, ';');
		}
	}
}

int ftp_process_dirlist(struct cache_entry *ce, int *pos, int *d, unsigned char *bf, int ln, int fin, int *tr)
{
	unsigned char *str, *buf;
	int sl;
	int ret = 0;
	int p;
	int len;
	again:
	buf = bf + ret;
	len = ln - ret;
	for (p = 0; p < len; p++) if (buf[p] == '\n') goto lb;
	if (p && (fin || len >= FTP_BUF)) {
		ret += p;
		goto pl;
	}
	return ret;
	lb:
	ret += p + 1;
	if (p && buf[p - 1] == '\r') p--;
	pl:
	str = init_str();
	sl = 0;
	/*add_to_str(&str, &sl, "   ");*/
	if (*d && *d < p && WHITECHAR(buf[*d - 1])) {
		int ee;
		ppp:
		for (ee = *d; ee <= p - 4; ee++)
			if (!memcmp(buf + ee, " -> ", 4)) goto syml;
		ee = p;
		syml:
		add_conv_str(&str, &sl, buf, *d);
		add_to_str(&str, &sl, "<a href=\"");
		add_conv_str(&str, &sl, buf + *d, ee - *d);
		if (buf[0] == 'd') add_chr_to_str(&str, &sl, '/');
		add_to_str(&str, &sl, "\">");
		add_conv_str(&str, &sl, buf + *d, ee - *d);
		add_to_str(&str, &sl, "</a>");
		add_conv_str(&str, &sl, buf + ee, p - ee);
	} else {
		int pp;
		if (p > 5 && !casecmp(buf, "total", 5)) goto raw;
		for (pp = p - 1; pp >= 0; pp--) if (!WHITECHAR(buf[pp])) break;
		if (pp < 0) goto raw;
		for (; pp >= 0; pp--) if (WHITECHAR(buf[pp]) && (pp < 3 || memcmp(buf + pp - 3, " -> ", 4)) && (pp > p - 4 || memcmp(buf + pp, " -> ", 4))) break;
		*d = pp + 1;
		goto ppp;
		raw:
		add_conv_str(&str, &sl, buf, p);
	}
	add_chr_to_str(&str, &sl, '\n');
	if (add_fragment(ce, *pos, str, sl)) *tr = 0;
	*pos += sl;
	mem_free(str);
	goto again;
}

void got_something_from_data_connection(struct connection *c)
{
	struct ftp_connection_info *inf = c->info;
	int l;
	set_timeout(c);
	if (!inf->d) {
		int ns;
		inf->d = 1;
		set_handlers(c->sock2, NULL, NULL, NULL, NULL);
		if ((ns = accept(c->sock2, NULL, NULL)) == -1) goto e;
		close(c->sock2);
		c->sock2 = ns;
		set_handlers(ns, (void (*)(void *))got_something_from_data_connection, NULL, NULL, c);
		return;
	}
	if (!c->cache && get_cache_entry(c->url, &c->cache)) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	if (inf->dir && !c->from) {
		unsigned char *ud;
		static unsigned char ftp_head[] = "<html><head><title>/";
		static unsigned char ftp_head2[] = "</title></head><body><h2>Directory /";
		static unsigned char ftp_head3[] = "</h2><pre>";
#define A(s) add_fragment(c->cache, c->from, s, strlen(s)), c->from += strlen(s)
		A(ftp_head);
		ud = stracpy(get_url_data(c->url));
		if (strchr(ud, POST_CHAR)) *strchr(ud, POST_CHAR) = 0;
		if (ud) A(ud);
		A(ftp_head2);
		if (ud) A(ud);
		A(ftp_head3);
		if (ud) mem_free(ud);
		if (!c->cache->head) c->cache->head = stracpy("\r\n");
		add_to_strn(&c->cache->head, "Content-Type: text/html\r\n");
#undef A
	}
	if ((l = read(c->sock2, inf->ftp_buffer + inf->buf_pos, FTP_BUF - inf->buf_pos)) == -1) {
		e:
		setcstate(c, -errno);
		retry_connection(c);
		return;
	}
	if (l > 0) {
		if (!inf->dir) {
			c->received += l;
			if (add_fragment(c->cache, c->from, inf->ftp_buffer, l) == 1) c->tries = 0;
			c->from += l;
		} else {
			int m;
			c->received += l;
			m = ftp_process_dirlist(c->cache, &c->from, &inf->dpos, inf->ftp_buffer, l + inf->buf_pos, 0, &c->tries);
			memmove(inf->ftp_buffer, inf->ftp_buffer + m, inf->buf_pos + l - m);
			inf->buf_pos += l - m;
		}
		setcstate(c, S_TRANS);
		return;
	}
	ftp_process_dirlist(c->cache, &c->from, &inf->dpos, inf->ftp_buffer, inf->buf_pos, 1, &c->tries);
	set_handlers(c->sock2, NULL, NULL, NULL, NULL);
	close_socket(&c->sock2);
	if (inf->conn_st == 1) {
		setcstate(c, S_OKAY);
		ftp_end_request(c);
	} else {
		inf->conn_st = 2;
		setcstate(c, S_TRANS);
	}
}

void ftp_end_request(struct connection *c)
{
	if (c->state == S_OKAY) {
		if (c->cache) {
			truncate_entry(c->cache, c->from, 1);
			c->cache->incomplete = 0;
		}
	}
	add_keepalive_socket(c, FTP_KEEPALIVE_TIMEOUT);
}

