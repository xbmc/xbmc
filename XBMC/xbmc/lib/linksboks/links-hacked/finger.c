/* finger.c
 * finger:// processing
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

void finger_send_request(struct connection *);
void finger_sent_request(struct connection *);
void finger_get_response(struct connection *, struct read_buffer *);
void finger_end_request(struct connection *);

void finger_func(struct connection *c)
{
	int p;
	set_timeout(c);
	if ((p = get_port(c->url)) == -1) {
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	c->from = 0;
	make_connection(c, p, &c->sock1, finger_send_request);
}

void finger_send_request(struct connection *c)
{
	unsigned char *req = init_str();
	int rl = 0;
	unsigned char *user;
	add_to_str(&req, &rl, "/W");
	if ((user = get_user_name(c->url))) {
		add_to_str(&req, &rl, " ");
		add_to_str(&req, &rl, user);
		mem_free(user);
	}
	add_to_str(&req, &rl, "\r\n");
	write_to_socket(c, c->sock1, req, rl, finger_sent_request);
	mem_free(req);
	setcstate(c, S_SENT);
}

void finger_sent_request(struct connection *c)
{
	struct read_buffer *rb;
	set_timeout(c);
	if (!(rb = alloc_read_buffer(c))) return;
	rb->close = 1;
	read_from_socket(c, c->sock1, rb, finger_get_response);
}

void finger_get_response(struct connection *c, struct read_buffer *rb)
{
	struct cache_entry *e;
	int l;
	set_timeout(c);
	if (get_cache_entry(c->url, &e)) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	c->cache = e;
	if (rb->close == 2) {
		setcstate(c, S_OKAY);
		finger_end_request(c);
		return;
	}
	l = rb->len;
	c->received += l;
	if (add_fragment(c->cache, c->from, rb->data, l) == 1) c->tries = 0;
	c->from += l;
	kill_buffer_data(rb, l);
	read_from_socket(c, c->sock1, rb, finger_get_response);
	setcstate(c, S_TRANS);
}

void finger_end_request(struct connection *c)
{
	if (c->state == S_OKAY) {
		if (c->cache) {
			truncate_entry(c->cache, c->from, 1);
			c->cache->incomplete = 0;
		}
	}
	abort_connection(c);
}
