/* sched.c
 * Links internal scheduler
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

tcount connection_count = 0;

int active_connections = 0;

struct list_head queue = {&queue, &queue};

struct h_conn {
	struct h_conn *next;
	struct h_conn *prev;
	unsigned char *host;
	int conn;
};

struct list_head h_conns = {&h_conns, &h_conns};

struct k_conn {
	struct k_conn *next;
	struct k_conn *prev;
	void (*protocol)(struct connection *);
	unsigned char *host;
	int port;
	int conn;
	ttime timeout;
	ttime add_time;
};

struct list_head keepalive_connections = {&keepalive_connections, &keepalive_connections};

void no_owner()
{
	internal("connection has no owner");
}

long connect_info(int type)
{
	int i = 0;
	struct connection *ce;
	struct k_conn *cee;
	switch (type) {
		case CI_FILES:
			foreach(ce, queue) i++;
			return i;
		case CI_CONNECTING:
			foreach(ce, queue) i += ce->state > S_WAIT && ce->state < S_TRANS;
			return i;
		case CI_TRANSFER:
			foreach(ce, queue) i += ce->state == S_TRANS;
			return i;
		case CI_KEEP:
			foreach(cee, keepalive_connections) i++;
			return i;
		case CI_LIST:
			return (long) &queue;
		default:
			internal("cache_info: bad request");
	}
	return 0;
}

int connection_disappeared(struct connection *c, tcount count)
{
	struct connection *d;
	foreach(d, queue) if (c == d && count == d->count) return 0;
	return 1;
}

struct h_conn *is_host_on_list(struct connection *c)
{
	unsigned char *ho;
	struct h_conn *h;
	if (!(ho = get_host_name(c->url))) return NULL;
	foreach(h, h_conns) if (!strcmp(h->host, ho)) {
		mem_free(ho);
		return h;
	}
	mem_free(ho);
	return NULL;
}

int st_r = 0;

void stat_timer(struct connection *c)
{
	ttime a;
	struct remaining_info *r = &c->prg;
	r->loaded = c->received;
	if ((r->size = c->est_length) < (r->pos = c->from) && r->size != -1)
		r->size = c->from;
	r->dis_b += a = get_time() - r->last_time;
	while (r->dis_b >= SPD_DISP_TIME * CURRENT_SPD_SEC) {
		r->cur_loaded -= r->data_in_secs[0];
		memmove(r->data_in_secs, r->data_in_secs + 1, sizeof(int) * (CURRENT_SPD_SEC - 1));
		r->data_in_secs[CURRENT_SPD_SEC - 1] = 0;
		r->dis_b -= SPD_DISP_TIME;
	}
	r->data_in_secs[CURRENT_SPD_SEC - 1] += r->loaded - r->last_loaded;
	r->cur_loaded += r->loaded - r->last_loaded;
	r->last_loaded = r->loaded;
	r->last_time += a;
	r->elapsed += a;
	r->timer = install_timer(SPD_DISP_TIME, (void (*)(void *))stat_timer, c);
	if (!st_r) send_connection_info(c);
}

void setcstate(struct connection *c, int state)
{
	struct status *stat;
	if (c->state < 0 && state >= 0) c->prev_error = c->state;
	if ((c->state = state) == S_TRANS) {
		struct remaining_info *r = &c->prg;
		if (r->timer == -1) {
			tcount count = c->count;
			if (!r->valid) {
				memset(r, 0, sizeof(struct remaining_info));
				r->valid = 1;
			}
			r->last_time = get_time();
			r->last_loaded = r->loaded;
			st_r = 1;
			stat_timer(c);
			st_r = 0;
			if (connection_disappeared(c, count)) return;
		}
	} else {
		struct remaining_info *r = &c->prg;
		if (r->timer != -1) kill_timer(r->timer), r->timer = -1;
	}
	foreach(stat, c->statuss) {
		stat->state = state;
		stat->prev_error = c->prev_error;
	}
	if (state >= 0) send_connection_info(c);
}

struct k_conn *is_host_on_keepalive_list(struct connection *c)
{
	unsigned char *ho;
	int po;
	void (*ph)(struct connection *);
	struct k_conn *h;
	if ((po = get_port(c->url)) == -1) return NULL;
	if (!(ph = get_protocol_handle(c->url))) return NULL;
	if (!(ho = get_host_and_pass(c->url))) return NULL;
	foreach(h, keepalive_connections)
		if (h->protocol == ph && h->port == po && !strcmp(h->host, ho)) {
			mem_free(ho);
			return h;
		}
	mem_free(ho);
	return NULL;
}

int get_keepalive_socket(struct connection *c)
{
	struct k_conn *k;
	int cc;
	if (!(k = is_host_on_keepalive_list(c))) return -1;
	cc = k->conn;
	del_from_list(k);
	mem_free(k->host);
	mem_free(k);
	c->sock1 = cc;
	return 0;
}

void check_keepalive_connections();

void abort_all_keepalive_connections()
{
	struct k_conn *k;
	foreach(k, keepalive_connections) mem_free(k->host), close(k->conn);
	free_list(keepalive_connections);
	check_keepalive_connections();
}

void free_connection_data(struct connection *c)
{
	struct h_conn *h;
	if (c->sock1 != -1) set_handlers(c->sock1, NULL, NULL, NULL, NULL);
	if (c->sock2 != -1) set_handlers(c->sock2, NULL, NULL, NULL, NULL);
	close_socket(&c->sock2);
	if (!c->running) {
		internal("connection already suspended");
	}

        uncompress_shutdown(c);

        c->running = 0;
	if (c->dnsquery) kill_dns_request(&c->dnsquery);
	if (c->buffer) {
		mem_free(c->buffer);
		c->buffer = NULL;
	}
	if (c->info) {
		mem_free(c->info);
		c->info = NULL;
	}
	if (c->timer != -1) kill_timer(c->timer), c->timer = -1;
	if (--active_connections < 0) {
		internal("active connections underflow");
		active_connections = 0;
	}
	if (c->state != S_WAIT) {
		if ((h = is_host_on_list(c))) {
			if (!--h->conn) {
				del_from_list(h);
				mem_free(h->host);
				mem_free(h);
			}
		} else internal("suspending connection that is not on the list (state %d)", c->state);
	}
}

void send_connection_info(struct connection *c)
{
	int st = c->state;
	tcount count = c->count;
	struct status *stat = c->statuss.next;
	while ((void *)stat != &c->statuss) {
		stat->ce = c->cache;
		stat = stat->next;
		if (stat->prev->end) stat->prev->end(stat->prev, stat->prev->data);
		if (st >= 0 && connection_disappeared(c, count)) return;
	}
}

void del_connection(struct connection *c)
{
	del_from_list(c);
	send_connection_info(c);
	mem_free(c->url);
	if (c->prev_url)mem_free(c->prev_url);
	mem_free(c);
}

#ifdef DEBUG
void check_queue_bugs();
#endif

void add_keepalive_socket(struct connection *c, ttime timeout)
{
	struct k_conn *k;
	free_connection_data(c);
	if (c->sock1 == -1) {
		internal("keepalive connection not connected");
		goto del;
	}
	if (!(k = mem_alloc(sizeof(struct k_conn)))) goto close;
	if ((k->port = get_port(c->url)) == -1 || !(k->protocol = get_protocol_handle(c->url)) || !(k->host = get_host_and_pass(c->url))) {
		mem_free(k);
		del_connection(c);
		goto close;
	}
	k->conn = c->sock1;
	k->timeout = timeout;
	k->add_time = get_time();
	add_to_list(keepalive_connections, k);
	del:
	del_connection(c);
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half((void (*)(void *))check_queue, NULL);
	return;
	close:
	close(c->sock1);
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half((void (*)(void *))check_queue, NULL);
}

void del_keepalive_socket(struct k_conn *kc)
{
	del_from_list(kc);
	close(kc->conn);
	mem_free(kc->host);
	mem_free(kc);
}

int keepalive_timeout = -1;

void check_keepalive_connections();

void keepalive_timer(void *x)
{
	keepalive_timeout = -1;
	check_keepalive_connections();
}

void check_keepalive_connections()
{
	struct k_conn *kc;
	ttime ct = get_time();
	int p = 0;
	if (keepalive_timeout != -1) kill_timer(keepalive_timeout), keepalive_timeout = -1;
	foreach(kc, keepalive_connections) if (can_read(kc->conn) || ct - kc->add_time > kc->timeout) {
		kc = kc->prev;
		del_keepalive_socket(kc->next);
	} else p++;
	for (; p > MAX_KEEPALIVE_CONNECTIONS; p--)
		if (!list_empty(keepalive_connections))
			del_keepalive_socket(keepalive_connections.prev);
		else internal("keepalive list empty");
	if (!list_empty(keepalive_connections)) keepalive_timeout = install_timer(KEEPALIVE_CHECK_TIME, keepalive_timer, NULL);
}

void add_to_queue(struct connection *c)
{
	struct connection *cc;
	int pri = getpri(c);
	foreach(cc, queue) if (getpri(cc) > pri) break;
	add_at_pos(cc->prev, c);
}

void sort_queue()
{
	struct connection *c, *n;
	int swp;
	do {
		swp = 0;
		foreach(c, queue) if ((void *)c->next != &queue) {
			if (getpri(c->next) < getpri(c)) {
				n = c->next;
				del_from_list(c);
				add_at_pos(n, c);
				swp = 1;
			}
		}
	} while (swp);
}

void interrupt_connection(struct connection *c)
{
#ifdef HAVE_SSL
	if (c->ssl == (void *)-1) c->ssl = 0;
	if(c->ssl) {
		SSL_free(c->ssl);
		c->ssl=NULL;
	}
#endif
	close_socket(&c->sock1);
	free_connection_data(c);
}

void suspend_connection(struct connection *c)
{
	interrupt_connection(c);
	setcstate(c, S_WAIT);
}

int try_to_suspend_connection(struct connection *c, unsigned char *ho)
{
	int pri = getpri(c);
	struct connection *d;
	foreachback(d, queue) {
		if (getpri(d) <= pri) return -1;
		if (d->state == S_WAIT) continue;
		if (d->unrestartable == 2 && getpri(d) < PRI_CANCEL) continue;
		if (ho) {
			unsigned char *h;
			if (!(h = get_host_name(d->url))) continue;
			if (strcmp(h, ho)) {
				mem_free(h);
				continue;
			}
			mem_free(h);
		}
		suspend_connection(d);
		return 0;
	}
	return -1;
}

void run_connection(struct connection *c)
{
	struct h_conn *hc;
	void (*func)(struct connection *);
	if (c->running) {
		internal("connection already running");
		return;
	}
	if (!(func = get_protocol_handle(c->url))) {
		setcstate(c, S_BAD_URL);
		del_connection(c);
		return;
	}
	if (!(hc = is_host_on_list(c))) {
		if (!(hc = mem_alloc(sizeof(struct h_conn)))) {
			setcstate(c, S_OUT_OF_MEM);
			del_connection(c);
			return;
		}
		if (!(hc->host = get_host_name(c->url))) {
			setcstate(c, S_BAD_URL);
			del_connection(c);
			mem_free(hc);
			return;
		}
		hc->conn = 0;
		add_to_list(h_conns, hc);
	}
	hc->conn++;
	active_connections++;
	c->running = 1;
	func(c);
}

void retry_connection(struct connection *c)
{
        int max_tries=options_get_int("network_max_tries");

        interrupt_connection(c);
        if (c->unrestartable >= 2 || (++c->tries >= (max_tries ? max_tries : 1000))) {
		/*send_connection_info(c);*/
		del_connection(c);
#ifdef DEBUG
		check_queue_bugs();
#endif
		register_bottom_half((void (*)(void *))check_queue, NULL);
	} else {
		c->prev_error = c->state;
		run_connection(c);
	}
}

void abort_connection(struct connection *c)
{
	if (c->running) interrupt_connection(c);
	/*send_connection_info(c);*/
	del_connection(c);
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half((void (*)(void *))check_queue, NULL);
}

int try_connection(struct connection *c)
{
	struct h_conn *hc = NULL;
	if ((hc = is_host_on_list(c))) {
		if (hc->conn >= options_get_int("network_max_connections_to_host")) {
			if (try_to_suspend_connection(c, hc->host)) return 0;
			else return -1;
		}
	}
	if (active_connections >= options_get_int("network_max_connections")) {
		if (try_to_suspend_connection(c, NULL)) return 0;
		else return -1;
	}
	run_connection(c);
	return 1;
}

#ifdef DEBUG
void check_queue_bugs()
{
	struct connection *d;
	int p = 0, ps = 0;
	int cc;
	again:
	cc = 0;
	foreach(d, queue) {
		int q = getpri(d);
		cc += d->running;
		if (q < p) if (!ps) {
			internal("queue is not sorted");
			sort_queue();
			ps = 1;
			goto again;
		} else {
			internal("queue is not sorted even after sort_queue!");
			break;
		} else p = q;
		if (d->state < 0) {
			internal("interrupted connection on queue (conn %s, state %d)", d->url, d->state);
			d = d->prev;
			abort_connection(d->next);
		}
	}
	if (cc != active_connections) {
		internal("bad number of active connections (counted %d, stored %d)", cc, active_connections);
		active_connections = cc;
	}
}
#endif

void check_queue()
{
	struct connection *c;
	again:
	c = queue.next;
#ifdef DEBUG
	check_queue_bugs();
#endif
	check_keepalive_connections();
	while (c != (struct connection *)&queue) {
		struct connection *d;
		int cp = getpri(c);
		for (d = c; d != (struct connection *)&queue && getpri(d) == cp;) {
			struct connection *dd = d; d = d->next;
			if (!dd->state) if (is_host_on_keepalive_list(dd)) {
				if (try_connection(dd)) goto again;
			}
		}
		for (d = c; d != (struct connection *)&queue && getpri(d) == cp;) {
			struct connection *dd = d; d = d->next;
			if (!dd->state) {
				if (try_connection(dd)) goto again;
			}
		}
		c = d;
	}
	again2:
	foreachback(c, queue) {
		if (getpri(c) < PRI_CANCEL) break;
		if (c->state == S_WAIT) {
			setcstate(c, S_INTERRUPTED);
			del_connection(c);
			goto again2;
		}
	}
#ifdef DEBUG
	check_queue_bugs();
#endif
}

#ifdef HAVE_LUA
unsigned char *script_hook_get_proxy(unsigned char *url)
{
	lua_State *L = lua_state;
	char *ret = NULL;
	int err;

	lua_getglobal(L, "proxy_for_hook");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return NULL;
	}

	lua_pushstring(L, url);
	if (prepare_lua(NULL))
		return NULL;
	err = lua_call(L, 1, 1);
	finish_lua();
	if (err)
		return NULL;

	if (lua_isstring(L, -1))
		ret = stracpy((unsigned char *)lua_tostring(L, -1));
	else if (!lua_isnil(L, -1))
		alert_lua_error("proxy_hook must return a string or nil");
	lua_pop(L, 1);
	return ret;
}
#endif

unsigned char *get_proxy(unsigned char *url)
{
	int l = strlen(url);
	unsigned char *proxy = NULL;
	unsigned char *u;
        unsigned char *tmp=NULL;
#ifdef HAVE_LUA
        tmp=script_hook_get_proxy(url);
#endif
        if(tmp==NULL){
                if (options_get("http_proxy") &&
                    l >= 7 && !casecmp(url, "http://", 7))
                        proxy = options_get("http_proxy");

                if (options_get("ftp_proxy") &&
                    l >= 6 && !casecmp(url, "ftp://", 6))
                        proxy = options_get("ftp_proxy");
        } else {
                if (tmp)
                        if (*tmp) proxy = tmp;  /* !"" from script_hook_get_proxy() */
        }
        if (!(u = mem_alloc(l + 1 + (proxy ? strlen(proxy) + 9 : 0)))) return NULL;
        if (proxy) strcpy(u, "proxy://"), strcat(u, proxy), strcat(u, "/");
        else *u = 0;
        if (tmp) mem_free(tmp);
        strcat(u, url);
        return u;
}

/* prev_url is a pointer to previous url or NULL */
/* prev_url will NOT be deallocated */
int load_url(unsigned char *url, unsigned char * prev_url, struct status *stat, int pri, int no_cache)
{
	struct cache_entry *e = NULL;
	struct connection *c;
	unsigned char *u;
	if (stat) stat->c = NULL, stat->ce = NULL, stat->pri = pri;
#ifdef DEBUG
	foreach(c, queue) {
		struct status *st;
		foreach (st, c->statuss) {
			if (st == stat) {
				internal("status already assigned to '%s'", c->url);
				stat->state = S_INTERNAL;
				if (stat->end) stat->end(stat, stat->data);
				return 0;
			}
		}
	}
#endif
	if (stat) stat->state = S_OUT_OF_MEM, stat->prev_error = 0;
	if (no_cache <= NC_CACHE && !find_in_cache(url, &e) && !e->incomplete) {
                if (options_get_bool("cache_aggressive") ||
                    !e->expire_time ||
                    (e->expire_time && e->expire_time >= time(NULL))){
                        if (stat) {
                                stat->ce = e;
                                stat->state = S_OKAY;
                                if (stat->end) stat->end(stat, stat->data);
                        }
                        return 0;
                }
        }
        if (!(u = get_proxy(url))) {
		if (stat) stat->end(stat, stat->data);
		return -1;
	}
	foreach(c, queue) if (!c->detached && !strcmp(c->url, u)) {
		mem_free(u);
		if (getpri(c) > pri) {
			del_from_list(c);
			c->pri[pri]++;
			add_to_queue(c);
			register_bottom_half((void (*)(void *))check_queue, NULL);
		} else c->pri[pri]++;
		if (stat) {
			stat->prg = &c->prg;
			stat->c = c;
			stat->ce = c->cache;
			add_to_list(c->statuss, stat);
			setcstate(c, c->state);
		}
#ifdef DEBUG
		check_queue_bugs();
#endif
		return 0;
	}
	if (!(c = mem_alloc(sizeof(struct connection)))) {
		stat->end(stat, stat->data);
		mem_free(u);
		return -1;
	}
	memset(c, 0, sizeof(struct connection));
	c->count = connection_count++;
	c->url = u;
	c->prev_url = stracpy(prev_url);
	c->running = 0;
	c->prev_error = 0;
	c->from = no_cache >= NC_RELOAD || !e || e->frag.next == &e->frag || ((struct fragment *)e->frag.next)->offset ? 0 : ((struct fragment *)e->frag.next)->length;
	memset(c->pri, 0, sizeof c->pri);
	c->pri[pri] = 1;
	c->no_cache = no_cache;
	c->sock1 = c->sock2 = -1;
	c->content_encoding = ENCODING_NONE;
	c->stream_pipes[0] = c->stream_pipes[1] = -1;
	c->dnsquery = NULL;
	c->info = NULL;
	c->buffer = NULL;
	c->cache = NULL;
	c->tries = 0;
	init_list(c->statuss);
	c->est_length = -1;
	c->unrestartable = 0;
	c->prg.timer = -1;
	c->timer = -1;
	if (stat) {
		stat->prg = &c->prg;
		stat->c = c;
		stat->ce = NULL;
		add_to_list(c->statuss, stat);
	}
	add_to_queue(c);
	setcstate(c, S_WAIT);
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half((void (*)(void *))check_queue, NULL);
	return 0;
}

void change_connection(struct status *oldstat, struct status *newstat, int newpri)
{		/* !!! FIXME: one object in more connections */
	struct connection *c;
	int oldpri;
	if (!oldstat) {
		internal("change_connection: oldstat == NULL");
		return;
	}
	oldpri = oldstat->pri;
	if (oldstat->state < 0) {
		if (newstat) {
			newstat->ce = oldstat->ce;
			newstat->state = oldstat->state;
			newstat->prev_error = oldstat->prev_error;
			if (newstat->end) newstat->end(newstat, newstat->data);
		}
		return;
	}
#ifdef DEBUG
	check_queue_bugs();
#endif
	c = oldstat->c;
	if (--c->pri[oldpri] < 0) {
		internal("priority counter underflow");
		c->pri[oldpri] = 0;
	}
	c->pri[newpri]++;
	del_from_list(oldstat);
	oldstat->state = S_INTERRUPTED;
	if (newstat) {
		newstat->prg = &c->prg;
		add_to_list(c->statuss, newstat);
		newstat->state = c->state;
		newstat->prev_error = c->prev_error;
		newstat->pri = newpri;
		newstat->c = c;
		newstat->ce = c->cache;
	}
	if (c->detached && !newstat) {
		setcstate(c, S_INTERRUPTED);
		abort_connection(c);
	}
	sort_queue();
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half((void (*)(void *))check_queue, NULL);
}

void detach_connection(struct status *stat, int pos)
{
	struct connection *c;
	int i, l;
	if (stat->state < 0) return;
	c = stat->c;
	if (c->detached) goto detach_done;
	if (!c->cache) return;
	if (c->est_length == -1) l = c->from;
	else l = c->est_length;
	if (l < options_get_int("cache_memory_size") * MAX_CACHED_OBJECT) return;
	l = 0;
	for (i = 0; i < PRI_CANCEL; i++) l += c->pri[i];
	if (!l) internal("detaching free connection");
	if (l != 1 || c->cache->refcount) return;
	shrink_memory(SH_CHECK_QUOTA);
	/*debug("detached");*/
	c->cache->url[0] = 0;
	c->detached = 1;
	detach_done:
	free_entry_to(c->cache, pos);
}

void connection_timeout(struct connection *c)
{
	c->timer = -1;
	setcstate(c, S_TIMEOUT);
	if (c->dnsquery) abort_connection(c);
	else retry_connection(c);
}

void connection_timeout_1(struct connection *c)
{
        c->timer = install_timer((c->unrestartable
                                  ? options_get_int("network_unrestartable_receive_timeout")
                                  : options_get_int("network_receive_timeout")) * 500,
                                 (void (*)(void *))connection_timeout,
                                 c);
}

void set_timeout(struct connection *c)
{
	if (c->timer != -1) kill_timer(c->timer);
        c->timer = install_timer((c->unrestartable
                                  ? options_get_int("network_unrestartable_receive_timeout")
                                  : options_get_int("network_receive_timeout")) * 500,
                                 (void (*)(void *))connection_timeout_1,
                                 c);
}

void reset_timeout(struct connection *c)
{
	if (c->timer != -1) kill_timer(c->timer), c->timer = -1;
}

void abort_all_connections()
{
	while(queue.next != &queue) {
		setcstate(queue.next, S_INTERRUPTED);
		abort_connection(queue.next);
	}
	abort_all_keepalive_connections();
}

void abort_background_connections()
{
	int i = 0;
	while (1) {
		int j;
		struct connection *c = (void *)&queue;
		for (j = 0; j <= i; j++) if ((c = c->next) == (void *)&queue) return;
		if (getpri(c) >= PRI_CANCEL) {
			setcstate(c, S_INTERRUPTED);
			abort_connection(c);
		} else i++;
	}
}

int is_entry_used(struct cache_entry *e)
{
	struct connection *c;
	foreach(c, queue) if (c->cache == e) return 1;
	return 0;
}

struct blacklist_entry {
	struct blacklist_entry *next;
	struct blacklist_entry *prev;
	int flags;
	unsigned char host[1];
};

struct list_head blacklist = { &blacklist, &blacklist };

void add_blacklist_entry(unsigned char *host, int flags)
{
	struct blacklist_entry *b;
	foreach(b, blacklist) if (!_stricmp(host, b->host)) {
		b->flags |= flags;
		return;
	}
	if (!(b = mem_alloc(sizeof(struct blacklist_entry) + strlen(host) + 1))) return;
	b->flags = flags;
	strcpy(b->host, host);
	add_to_list(blacklist, b);
}

void del_blacklist_entry(unsigned char *host, int flags)
{
	struct blacklist_entry *b;
	foreach(b, blacklist) if (!_stricmp(host, b->host)) {
		b->flags &= ~flags;
		if (!b->flags) {
			del_from_list(b);
			mem_free(b);
		}
		return;
	}
}

int get_blacklist_flags(unsigned char *host)
{
	struct blacklist_entry *b;
	foreach(b, blacklist) if (!_stricmp(host, b->host)) return b->flags;
	return 0;
}

void free_blacklist()
{
	free_list(blacklist);
}

struct s_msg_dsc msg_dsc[] = {
	{S_WAIT,		TXT(T_WAITING_IN_QUEUE)},
	{S_DNS,			TXT(T_LOOKING_UP_HOST)},
	{S_CONN,		TXT(T_MAKING_CONNECTION)},
	{S_SSL_NEG,		TXT(T_SSL_NEGOTIATION)},
	{S_SENT,		TXT(T_REQUEST_SENT)},
	{S_LOGIN,		TXT(T_LOGGING_IN)},
	{S_GETH,		TXT(T_GETTING_HEADERS)},
	{S_PROC,		TXT(T_SERVER_IS_PROCESSING_REQUEST)},
	{S_TRANS,		TXT(T_TRANSFERRING)},

	{S_WAIT_REDIR,		TXT(T_WAITING_FOR_REDIRECT_CONFIRMATION)},
	{S_OKAY,			TXT(T_OK)},
	{S_INTERRUPTED,		TXT(T_INTERRUPTED)},
	{S_EXCEPT,		TXT(T_SOCKET_EXCEPTION)},
	{S_INTERNAL,		TXT(T_INTERNAL_ERROR)},
	{S_OUT_OF_MEM,		TXT(T_OUT_OF_MEMORY)},
	{S_NO_DNS,		TXT(T_HOST_NOT_FOUND)},
	{S_CANT_WRITE,		TXT(T_ERROR_WRITING_TO_SOCKET)},
	{S_CANT_READ,		TXT(T_ERROR_READING_FROM_SOCKET)},
	{S_MODIFIED,		TXT(T_DATA_MODIFIED)},
	{S_BAD_URL,		TXT(T_BAD_URL_SYNTAX)},
	{S_TIMEOUT,		TXT(T_RECEIVE_TIMEOUT)},
	{S_RESTART,		TXT(T_REQUEST_MUST_BE_RESTARTED)},
	{S_STATE,		TXT(T_CANT_GET_SOCKET_STATE)},

	{S_HTTP_ERROR,		TXT(T_BAD_HTTP_RESPONSE)},
	{S_HTTP_100,		TXT(T_HTTP_100)},
	{S_HTTP_204,		TXT(T_NO_CONTENT)},

	{S_FILE_TYPE,		TXT(T_UNKNOWN_FILE_TYPE)},
	{S_FILE_ERROR,		TXT(T_ERROR_OPENING_FILE)},

	{S_FTP_ERROR,		TXT(T_BAD_FTP_RESPONSE)},
	{S_FTP_UNAVAIL,		TXT(T_FTP_SERVICE_UNAVAILABLE)},
	{S_FTP_LOGIN,		TXT(T_BAD_FTP_LOGIN)},
	{S_FTP_PORT,		TXT(T_FTP_PORT_COMMAND_FAILED)},
	{S_FTP_NO_FILE,		TXT(T_FILE_NOT_FOUND)},
	{S_FTP_FILE_ERROR,	TXT(T_FTP_FILE_ERROR)},
#ifdef HAVE_SSL
	{S_SSL_ERROR,		TXT(T_SSL_ERROR)},
#else
	{S_NO_SSL,		TXT(T_NO_SSL)},
#endif
	{0,		NULL}
};
