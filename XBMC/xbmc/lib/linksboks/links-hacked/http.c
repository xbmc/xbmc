/* http.c
 * HTTP protocol client implementation
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

struct http_connection_info {
	int bl_flags;
	int http10;
	int close;
	int length;
	int version;
	int chunk_remaining;
};

#define LEN_CHUNKED -2 /* == we get data in unknown number of chunks */
#define LEN_FINISHED 0

#define CHUNK_DATA_END	-3
#define CHUNK_ZERO_SIZE	-2
#define CHUNK_SIZE	-1

/* Returns a string pointer with value of the item.
 * The string must be destroyed after usage with mem_free.
 */
unsigned char *parse_http_header(unsigned char *head, unsigned char *item, unsigned char **ptr)
{
	unsigned char *i, *f, *g, *h;
	if (!head) return NULL;
	h = NULL;
	for (f = head; *f; f++) {
		if (*f != 10) continue;
		f++;
		for (i = item; *i && *f; i++, f++)
			if (upcase(*i) != upcase(*f)) goto cont;
		if (!*f) break;
		if (f[0] == ':') {
			while (f[1] == ' ') f++;
			for (g = ++f; *g >= ' '; g++) ;
			while (g > f && g[-1] == ' ') g--;
			if (h) mem_free(h);
			if ((h = mem_alloc(g - f + 1))) {
				memcpy(h, f, g - f);
				h[g - f] = 0;
				if (ptr) {
					*ptr = f;
					break;
				}
				return h;
			}
		}
		cont:;
		f--;
	}
	return h;
}

unsigned char *parse_header_param(unsigned char *x, unsigned char *e)
{
	int le = strlen(e);
	int lp;
	unsigned char *y = x;
	a:
	if (!(y = strchr(y, ';'))) return NULL;
	while (*y && (*y == ';' || *y <= ' ')) y++;
	if (strlen(y) < le) return NULL;
	if (casecmp(y, e, le)) goto a;
	y += le;
	while (*y && (*y <= ' ' || *y == '=')) y++;
	if (!*y) return stracpy("");
	lp = 0;
	while (y[lp] >= ' ' && y[lp] != ';') lp++;
	return memacpy(y, lp);
}

static int get_http_code(unsigned char *head, int *code, int *version)
{
	while (head[0] == ' ') head++;
	if (upcase(head[0]) != 'H' || upcase(head[1]) != 'T' || upcase(head[2]) != 'T' ||
	    upcase(head[3]) != 'P') return -1;
	if (head[4] == '/' && head[5] >= '0' && head[5] <= '9'
	 && head[6] == '.' && head[7] >= '0' && head[7] <= '9' && head[8] <= ' ') {
		*version = (head[5] - '0') * 10 + head[7] - '0';
	} else *version = 0;
	for (head += 4; *head > ' '; head++) ;
	if (*head++ != ' ') return -1;
	if (head[0] < '1' || head [0] > '9' || head[1] < '0' || head[1] > '9' ||
	    head[2] < '0' || head [2] > '9') return -1;
	*code = (head[0]-'0')*100 + (head[1]-'0')*10 + head[2]-'0';
	return 0;
}

unsigned char *buggy_servers[] = { "mod_czech/3.1.0", "Purveyor", "Netscape-Enterprise", NULL };

int check_http_server_bugs(unsigned char *url, struct http_connection_info *info, unsigned char *head)
{
	unsigned char *server, **s;
	if (!options_get_bool("http_bugs_allow_blacklist") || info->http10) return 0;
	if (!(server = parse_http_header(head, "Server", NULL))) return 0;
	for (s = buggy_servers; *s; s++) if (strstr(server, *s)) goto bug;
	mem_free(server);
	return 0;
	bug:
	mem_free(server);
	if ((server = get_host_name(url))) {
		add_blacklist_entry(server, BL_HTTP10);
		mem_free(server);
		return 1;
	}
	return 0;	
}

void http_end_request(struct connection *c)
{
	if (c->state == S_OKAY) {
		if (c->cache) {
			truncate_entry(c->cache, c->from, 1);
			c->cache->incomplete = 0;
		}
	}
	if (c->info && !((struct http_connection_info *)c->info)->close 
#ifdef HAVE_SSL
	&& (!c->ssl) /* We won't keep alive ssl connections */
#endif
	&& (!options_get_bool("http_bugs_post_no_keepalive") || !strchr(c->url, POST_CHAR))) {
		add_keepalive_socket(c, HTTP_KEEPALIVE_TIMEOUT);
	} else abort_connection(c);
}

void http_send_header(struct connection *);

void http_func(struct connection *c)
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
		make_connection(c, p, &c->sock1, http_send_header);
	} else http_send_header(c);
}

void proxy_func(struct connection *c)
{
	http_func(c);
}

void http_get_header(struct connection *);

void http_send_header(struct connection *c)
{
	struct http_connection_info *info;
	int http10 = options_get_bool("http_bugs_http10");
	struct cache_entry *e = NULL;
	unsigned char *hdr;
	unsigned char *h, *u, *uu;
	int l = 0;
	unsigned char *post;
	unsigned char *host;
	unsigned char *host_data;

	find_in_cache(c->url, &c->cache);

        host = (upcase(c->url[0]) != 'P') ? c->url : get_url_data(c->url);

        set_timeout(c);
	if (!(info = mem_alloc(sizeof(struct http_connection_info)))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	memset(info, 0, sizeof(struct http_connection_info));
	c->info = info;
	if ((h = get_host_name(host))) {
		info->bl_flags = get_blacklist_flags(h);
		mem_free(h);
	}
	if (info->bl_flags & BL_HTTP10) http10 = 1;
	info->http10 = http10;
	post = strchr(c->url, POST_CHAR);
	if (post) post++;
	if (!(hdr = init_str())) {
		setcstate(c, S_OUT_OF_MEM);
		http_end_request(c);
		return;
	}
	if (!post) add_to_str(&hdr, &l, "GET ");
	else {
		add_to_str(&hdr, &l, "POST ");
		c->unrestartable = 2;
	}
	if (upcase(c->url[0]) != 'P') add_to_str(&hdr, &l, "/");
	if (!(u = get_url_data(c->url))) {
		setcstate(c, S_BAD_URL);
		http_end_request(c);
		return;
	}
	if (!post) uu = stracpy(u);
	else uu = memacpy(u, post - u - 1);
	while (strchr(uu, ' ')) {
		unsigned char *sp = strchr(uu, ' ');
		unsigned char *nu = mem_alloc(strlen(uu) + 3);
		if (!nu) break;
		memcpy(nu, uu, sp - uu);
		nu[sp - uu] = 0;
		strcat(nu, "%20");
		strcat(nu, sp + 1);
		mem_free(uu);
		uu = nu;
	}
        add_to_str(&hdr, &l, uu);
	mem_free(uu);
	if (!http10) add_to_str(&hdr, &l, " HTTP/1.1\r\n");
	else add_to_str(&hdr, &l, " HTTP/1.0\r\n");
	if ((h = get_host_name(host))) {
		add_to_str(&hdr, &l, "Host: ");
		add_to_str(&hdr, &l, h);
		mem_free(h);
		if ((h = get_port_str(host))) {
			add_to_str(&hdr, &l, ":");
			add_to_str(&hdr, &l, h);
			mem_free(h);
		}
		add_to_str(&hdr, &l, "\r\n");
	}

        /* Proxy auth block */
        if (upcase(c->url[0]) == 'P'){
		unsigned char *user = options_get("http_proxy_user");

                if(user && user[0]){
                        unsigned char *proxy_auth = straconcat(user, ":", options_get("http_proxy_password"), NULL);

			if (proxy_auth) {
				unsigned char *proxy_auth64 = base64_encode(proxy_auth);

				if (proxy_auth64) {
					add_to_str(&hdr, &l, "Proxy-Authorization: Basic ");
					add_to_str(&hdr, &l, proxy_auth64);
					add_to_str(&hdr, &l, "\r\n");
					mem_free(proxy_auth64);
				}
				mem_free(proxy_auth);
			}
		}
	}

        add_to_str(&hdr, &l, "User-Agent: ");
        if (!options_get("http_fake_useragent")){
		add_to_str(&hdr, &l, "LinksBoks/");
		add_to_str(&hdr, &l, LINKSBOKS_VERSION_STRING);
		add_to_str(&hdr, &l, " (");
		add_to_str(&hdr, &l, system_name);
		if (!F && !list_empty(terminals)) {
			struct terminal *t = terminals.prev;
			add_to_str(&hdr, &l, "; ");
			add_num_to_str(&hdr, &l, t->x);
			add_to_str(&hdr, &l, "x");
			add_num_to_str(&hdr, &l, t->y);
		}
#ifdef G
		if (F && drv) {
			add_to_str(&hdr, &l, "; ");
			add_to_str(&hdr, &l, drv->name);
		}
#endif
		add_to_str(&hdr, &l, ")\r\n");
	}
	else {
		add_to_str(&hdr, &l, options_get("http_fake_useragent"));
		add_to_str(&hdr, &l, "\r\n");
	}
	switch (options_get_int("http_referer"))
	{
		case REFERER_FAKE:
		add_to_str(&hdr, &l, "Referer: ");
		add_to_str(&hdr, &l, options_get("http_referer_fake_referer"));
		add_to_str(&hdr, &l, "\r\n");
		break;
		
		case REFERER_SAME_URL:
		add_to_str(&hdr, &l, "Referer: ");
		if (!post) add_to_str(&hdr, &l, c->url);
		else add_bytes_to_str(&hdr, &l, c->url, post - c->url - 1);
		add_to_str(&hdr, &l, "\r\n");
		break;

		case REFERER_REAL:
		{
			unsigned char *post2;
			if (!(c->prev_url))break;   /* no referrer */

			post2 = strchr(c->prev_url, POST_CHAR);
			add_to_str(&hdr, &l, "Referer: ");
			if (!post2) add_to_str(&hdr, &l, c->prev_url);
			else add_bytes_to_str(&hdr, &l, c->prev_url, post2 - c->prev_url);
			add_to_str(&hdr, &l, "\r\n");
		}
		break;
	}
	
	add_to_str(&hdr, &l, "Accept: */*\r\n");

        /* Accept-Encoding */
#if defined(HAVE_BZLIB_H) || defined(HAVE_ZLIB_H)
	add_to_str(&hdr, &l, "Accept-Encoding: ");

#ifdef HAVE_BZLIB_H
	add_to_str(&hdr, &l, "bzip2");
#endif

#ifdef HAVE_ZLIB_H
#ifdef HAVE_BZLIB_H
	add_to_str(&hdr, &l, ", ");
#endif
	add_to_str(&hdr, &l, "gzip");
#endif
	add_to_str(&hdr, &l, "\r\n");
#endif



        /* Accept-Charset: */
        if (!(info->bl_flags & BL_NO_CHARSET) && !options_get_bool("http_bugs_no_accept_charset")){
                unsigned char *accept_charset=options_get("http_accept_charset");
                if(accept_charset && *accept_charset){
                        add_to_str(&hdr, &l, "Accept-Charset: ");
                        add_to_str(&hdr, &l, accept_charset);
                        add_to_str(&hdr, &l, "\r\n");
                }
        }

        /* Accept-Language: */
        {
                unsigned char *accept_language=options_get("http_accept_language");
                if(accept_language && *accept_language){
                        add_to_str(&hdr, &l, "Accept-Language: ");
                        add_to_str(&hdr, &l, accept_language);
                        add_to_str(&hdr, &l, "\r\n");
                }
        }

        if (!http10) {
		if (upcase(c->url[0]) != 'P') add_to_str(&hdr, &l, "Connection: ");
		else add_to_str(&hdr, &l, "Proxy-Connection: ");
		if (!post || !options_get_bool("http_bugs_post_no_keepalive")) add_to_str(&hdr, &l, "Keep-Alive\r\n");
		else add_to_str(&hdr, &l, "close\r\n");
	}

        if ((e = c->cache)) {
		if (!e->incomplete && e->head && c->no_cache <= NC_IF_MOD) {
			unsigned char *m;

			if (e->last_modified)
				m = stracpy(e->last_modified);
			else {
				m = parse_http_header(e->head, "Date", NULL);
				if (!m)
					m = parse_http_header(e->head, "Expires", NULL);
			}
			if (m){
				add_to_str(&hdr, &l, "If-Modified-Since: ");
				add_to_str(&hdr, &l, m);
				add_to_str(&hdr, &l, "\r\n");
				mem_free(m);
			}
		}
	}
	if (c->no_cache >= NC_PR_NO_CACHE) add_to_str(&hdr, &l, "Pragma: no-cache\r\nCache-Control: no-cache\r\n");
	if (c->from) {
		add_to_str(&hdr, &l, "Range: bytes=");
		add_num_to_str(&hdr, &l, c->from);
		add_to_str(&hdr, &l, "-\r\n");
	}
	host_data = find_auth(host);
	if (host_data) {
		add_to_str(&hdr, &l, "Authorization: Basic ");
		add_to_str(&hdr, &l, host_data);
		add_to_str(&hdr, &l, "\r\n");
		mem_free(host_data);
	}

        if (post) {
		unsigned char *pd = strchr(post, '\n');
		if (pd) {
			add_to_str(&hdr, &l, "Content-Type: ");
			add_bytes_to_str(&hdr, &l, post, pd - post);
			add_to_str(&hdr, &l, "\r\n");
			post = pd + 1;
		}
		add_to_str(&hdr, &l, "Content-Length: ");
		add_num_to_str(&hdr, &l, strlen(post) / 2);
		add_to_str(&hdr, &l, "\r\n");
	}
	send_cookies(&hdr, &l, host);
	add_to_str(&hdr, &l, "\r\n");
	if (post) {
		while (post[0] && post[1]) {
			int h1, h2;
			h1 = post[0] <= '9' ? post[0] - '0' : post[0] >= 'A' ? upcase(post[0]) - 'A' + 10 : 0;
			if (h1 < 0 || h1 >= 16) h1 = 0;
			h2 = post[1] <= '9' ? post[1] - '0' : post[1] >= 'A' ? upcase(post[1]) - 'A' + 10 : 0;
			if (h2 < 0 || h2 >= 16) h2 = 0;
			add_chr_to_str(&hdr, &l, h1 * 16 + h2);
			post += 2;
		}
	}
	write_to_socket(c, c->sock1, hdr, strlen(hdr), http_get_header);
	mem_free(hdr);
	setcstate(c, S_SENT);
}

int is_line_in_buffer(struct read_buffer *rb)
{
	int l;
	for (l = 0; l < rb->len; l++) {
		if (rb->data[l] == 10) return l + 1;
		if (l < rb->len - 1 && rb->data[l] == 13 && rb->data[l + 1] == 10) return l + 2;
		if (l == rb->len - 1 && rb->data[l] == 13) return 0;
		if (rb->data[l] < ' ') return -1;
	}
	return 0;
}



/** @func	uncompress_data(struct connection *conn, unsigned char *data,
		int len, int *dlen)
 * @brief	This function uncompress data blocks (if they were compressed).
 * @param	conn	standard structure
 * @param	data	block of data
 * @param	len	length of the block
 * @param	new_len	number of uncompressed bytes (length of returned block
			of data)
 * @ret		unsigned char *	address of uncompressed block
 * @remark	In this function, value of either info->chunk_remaining or
 *		info->length is being changed (it depends on if chunked mode is
 *		used or not).
 *		Note that the function is still a little esotheric for me. Don't
 *		take it lightly and don't mess with it without grave reason! If
 *		you dare to touch this without testing the changes on slashdot
 *		and cvsweb (including revision history), don't dare to send me
 *		any patches! ;) --pasky
 */
static unsigned char *uncompress_data(struct connection *conn, unsigned char *data, int len, int *new_len)
{
	struct http_connection_info *info = conn->info;
	/* Number of uncompressed bytes that could be safely get from
	 * read_encoded().  We can't want to read too much, because if gzread
	 * "clears" the buffer next time it will return -1. */
	int ret = 0;
	int r = 0, *length_of_block;
	/* If true, all stuff was written to pipe and only uncompression is
	 * wanted now. */
	int finishing = 0;
	unsigned char *output = DUMMY;

	length_of_block = (info->length == LEN_CHUNKED ? &info->chunk_remaining
						       : &info->length);
	if (!*length_of_block) finishing = 1;

	if (conn->content_encoding == ENCODING_NONE) {
		*new_len = len;
		if (*length_of_block > 0) *length_of_block -= len;
		return data;
	}

	*new_len = 0; /* new_len must be zero if we would ever return NULL */

	if (conn->stream_pipes[0] == -1) {
		if (c_pipe(conn->stream_pipes) < 0) return NULL;
		if (set_nonblocking_fd(conn->stream_pipes[0]) < 0) return NULL;
		if (set_nonblocking_fd(conn->stream_pipes[1]) < 0) return NULL;
	}

	while (r == ret) {
		if (!finishing) {
			int written = write(conn->stream_pipes[1], data, len);

			/* When we're writing zero bytes already, we want to
			 * zero ret properly for now, so that we'll go out for
			 * some more data. Otherwise, read_encoded() will yield
			 * zero bytes, but ret will be on its original value,
			 * causing us to close the stream, and that's disaster
			 * when more data are about to come yet. */
			if (written > 0 || (!written && !len)) {
				ret = written;
				data += ret;
				len -= ret;
				if (*length_of_block > 0)
					*length_of_block -= ret;
				if (!info->length)
					finishing = 1;
			}

			if (len) {
				/* We assume that this is because full pipe. */
				/* FIXME: We should probably handle errors as
				 * well. --pasky */
				ret = PIPE_BUF; /* pipe capacity */
			}
		}
		/* finishing could be changed above ;) */
		if (finishing) {
			/* Granularity of the final decompression. When we were
			 * taking the decompressed content, we only took the
			 * amount which we inserted there. When finishing, we
			 * have to drain the rest from the beast. */
			/* TODO: We should probably double the ret before trying
			 * to read as well..? Would maybe make the progressive
			 * displaying feeling better? --pasky */
			ret = 65536;
		}
		if (ret < PIPE_BUF) {
			/* Not enough data, try in next round. */
			return output;
		}

                if (!conn->stream) {
                        conn->stream = open_encoded(conn->stream_pipes[0],
                                                    conn->content_encoding);
                        if (!conn->stream) return NULL;
                }

		output = (unsigned char *) mem_realloc(output, *new_len + ret);
		if (!output) break;

		r = read_encoded(conn->stream, output + *new_len, ret);
		if (r > 0) *new_len += r;
	}

	if (r < 0 && output) {
		mem_free(output);
		output = NULL;
	}

	uncompress_shutdown(conn);
	return output;
}

void uncompress_shutdown(struct connection *conn)
{
	if (conn->stream) {
		close_encoded(conn->stream);
		conn->stream = NULL;
	}
	if (conn->stream_pipes[1] >= 0)
		close(conn->stream_pipes[1]);
	conn->stream_pipes[0] = conn->stream_pipes[1] = -1;
}



void read_http_data(struct connection *c, struct read_buffer *rb)
{
	struct http_connection_info *info = c->info;
	set_timeout(c);
	if (rb->close == 2) {
		if (c->content_encoding && info->length == -1) {
			/* Flush uncompression first. */
			info->length = 0;
		} else {
		thats_all_folks:
				setcstate(c, S_OKAY);
				http_end_request(c);
				return;
		}
	}
	if (info->length != LEN_CHUNKED) {
                /*
                int l = rb->len;
		if (info->length >= 0 && info->length < l) l = info->length;
		c->received += l;
		if (add_fragment(c->cache, c->from, rb->data, l) == 1) c->tries = 0;
		if (info->length >= 0) info->length -= l;
		c->from += l;
		kill_buffer_data(rb, l);
		if (!info->length && !rb->close) {
			setcstate(c, S_OKAY);
			http_end_request(c);
			return;
                }
                */
		unsigned char *data;
		int data_len;
		int len = rb->len;

		if (info->length >= 0 && info->length < len) {
			/* We won't read more than we have to go. */
			len = info->length;
		}

		c->received += len;

		data = uncompress_data(c, rb->data, len, &data_len);

		if (add_fragment(c->cache, c->from, data, data_len) == 1)
			c->tries = 0;

		if (data && data != rb->data) mem_free(data);

		c->from += data_len;

		kill_buffer_data(rb, len);

		if (!info->length && !rb->close)
			goto thats_all_folks;
	} else {
        next_chunk:
		if (info->chunk_remaining == CHUNK_DATA_END) {
                        int l;
			if ((l = is_line_in_buffer(rb))) {
                                if (l == -1) {
					setcstate(c, S_HTTP_ERROR);
                                        abort_connection(c);
                                        return;
                                }
                                kill_buffer_data(rb, l);
				if (l <= 2) {
                                        setcstate(c, S_OKAY);
                                        http_end_request(c);
                                        return;
				}
                                goto next_chunk;
                        }
                } else if (info->chunk_remaining == -1) {
                        int l;
                        if ((l = is_line_in_buffer(rb))) {
                                unsigned char *de;
                                int n = 0;	/* warning, go away */
                                if (l != -1) n = strtol(rb->data, (char **)&de, 16);
                                if (l == -1 || de == rb->data) {
                                        setcstate(c, S_HTTP_ERROR);
                                        abort_connection(c);
                                        return;
                                }
				kill_buffer_data(rb, l);
                                if (!(info->chunk_remaining = n)) info->chunk_remaining = -2;
                                goto next_chunk;
                        }
                } else {
                        /*
                         int l = info->chunk_remaining;
                         if (l > rb->len) l = rb->len;
                         c->received += l;
                         if (add_fragment(c->cache, c->from, rb->data, l) == 1) c->tries = 0;
                         info->chunk_remaining -= l;
                         c->from += l;
                         kill_buffer_data(rb, l);
                         if (!info->chunk_remaining && rb->len >= 1) {
                         if (rb->data[0] == 10) kill_buffer_data(rb, 1);
                         else {
                         if (rb->data[0] != 13 || (rb->len >= 2 && rb->data[1] != 10)) {
                         setcstate(c, S_HTTP_ERROR);
                         abort_connection(c);
                         return;
                         }
                         if (rb->len < 2) goto read_more;
                         kill_buffer_data(rb, 2);
                         }
                         info->chunk_remaining = -1;
                         goto next_chunk;
                         }
                         */
                        unsigned char *data;
                        int data_len;
                        int len;
                        int zero = 0;

                        zero = (info->chunk_remaining == CHUNK_ZERO_SIZE);
                        if (zero) info->chunk_remaining = 0;
                        len = info->chunk_remaining;

			/* Maybe everything neccessary didn't come yet.. */
                        if (len > rb->len) len = rb->len;
                        c->received += len;

                        data = uncompress_data(c, rb->data, len, &data_len);

                        if (add_fragment(c->cache, c->from,
                                         data, data_len) == 1)
                                c->tries = 0;

                        if (data && data != rb->data) mem_free(data);

			c->from += data_len;

                        kill_buffer_data(rb, len);

			if (zero) {
				/* Last chunk has zero length, so this is last
				 * chunk, we finished decompression just now
				 * and now we can happily finish reading this
				 * stuff. */
				info->chunk_remaining = CHUNK_DATA_END;
				goto next_chunk;
                        }

                        if (!info->chunk_remaining && rb->len > 0) {
                                /* Eat newline succeeding each chunk. */
                                if (rb->data[0] == 10) {
					kill_buffer_data(rb, 1);
				} else {
					if (rb->data[0] != 13
					    || (rb->len >= 2
						&& rb->data[1] != 10)) {
						setcstate(c, S_HTTP_ERROR);
						abort_connection(c);
						return;
					}
                                        if (rb->len < 2) goto read_more;
                                        kill_buffer_data(rb, 2);
                                }
				info->chunk_remaining = CHUNK_SIZE;
				goto next_chunk;
                        }
		}
	}
read_more:
	read_from_socket(c, c->sock1, rb, read_http_data);
	setcstate(c, S_TRANS);
}

int get_header(struct read_buffer *rb)
{
        int i;
	for (i = 0; i < rb->len; i++) {
		unsigned char a = rb->data[i];
		if (/*a < ' ' && a != 10 && a != 13*/!a) return -1;
		if (i < rb->len - 1 && a == 10 && rb->data[i + 1] == 10) return i + 2;
		if (i < rb->len - 3 && a == 13) {
			if (rb->data[i + 1] != 10) return -1;
			if (rb->data[i + 2] == 13) {
				if (rb->data[i + 3] != 10) return -1;
				return i + 4;
			}
		}
	}
	return 0;
}

void http_got_header(struct connection *c, struct read_buffer *rb)
{
	int cf;
	int state = c->state != S_PROC ? S_GETH : S_PROC;
	unsigned char *head;
	unsigned char *cookie, *ch;
	int a, h, version;
	unsigned char *d;
	struct cache_entry *e;
	struct http_connection_info *info;
	unsigned char *host = upcase(c->url[0]) != 'P' ? c->url : get_url_data(c->url);
	set_timeout(c);
	info = c->info;
	if (rb->close == 2) {
		unsigned char *h;
		if (!c->tries && (h = get_host_name(host))) {
			if (info->bl_flags & BL_NO_CHARSET) {
				del_blacklist_entry(h, BL_NO_CHARSET);
			} else {
				add_blacklist_entry(h, BL_NO_CHARSET);
				c->tries = -1;
			}
			mem_free(h);
		}
		setcstate(c, S_CANT_READ);
		retry_connection(c);
		return;
	}
	rb->close = 0;
	again:
	if ((a = get_header(rb)) == -1) {
		setcstate(c, S_HTTP_ERROR);
		abort_connection(c);
		return;
	}
	if (!a) {
		read_from_socket(c, c->sock1, rb, http_got_header);
		setcstate(c, state);
		return;
	}
	if (get_http_code(rb->data, &h, &version) || h == 101) {
		setcstate(c, S_HTTP_ERROR);
		abort_connection(c);
		return;
	}
	if (!(head = mem_alloc(a + 1))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	memcpy(head, rb->data, a); head[a] = 0;
	if (check_http_server_bugs(host, c->info, head)) {
		mem_free(head);
		setcstate(c, S_RESTART);
		retry_connection(c);
		return;
	}
	ch = head;
	while ((cookie = parse_http_header(ch, "Set-Cookie", &ch))) {
		unsigned char *host = upcase(c->url[0]) != 'P' ? c->url : get_url_data(c->url);
		set_cookie(NULL, host, cookie);
		mem_free(cookie);
	}
	if (h == 100) {
		mem_free(head);
		state = S_PROC;
		kill_buffer_data(rb, a);
		goto again;
	}
	if (h < 200) {
		mem_free(head);
		setcstate(c, S_HTTP_ERROR);
		abort_connection(c);
		return;
	}
	if (h == 304) {
		mem_free(head);
		setcstate(c, S_OKAY);
		http_end_request(c);
		return;
	}
	if (get_cache_entry(c->url, &e)) {
		mem_free(head);
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	if (e->head) mem_free(e->head);
	e->head = head;
	if ((d = parse_http_header(head, "Expires", NULL))) {
		time_t t = parse_http_date(d);
		if (t && e->expire_time != 1) e->expire_time = t;
		mem_free(d);
       }
	if ((d = parse_http_header(head, "Pragma", NULL))) {
		if (!casecmp(d, "no-cache", 8)) e->expire_time = 1;
		mem_free(d);
	}
	if ((d = parse_http_header(head, "Cache-Control", NULL))) {
		if (!casecmp(d, "no-cache", 8)) e->expire_time = 1;
		if (!casecmp(d, "max-age=", 8)) {
			if (e->expire_time != 1) e->expire_time = time(NULL) + atoi(d + 8);
		}
		mem_free(d);
	}
#ifdef HAVE_SSL
	if (c->ssl) {
		int l = 0;
		if (e->ssl_info) mem_free(e->ssl_info);
		e->ssl_info = init_str();
		add_num_to_str(&e->ssl_info, &l, SSL_get_cipher_bits(c->ssl, NULL));
		add_to_str(&e->ssl_info, &l, "-bit ");
		add_to_str(&e->ssl_info, &l, SSL_get_cipher_version(c->ssl));
		add_to_str(&e->ssl_info, &l, " ");
		add_to_str(&e->ssl_info, &l, (unsigned  char *)SSL_get_cipher_name(c->ssl));
	}
#endif
	if (h == 204) {
		setcstate(c, S_OKAY);
		http_end_request(c);
		return;
	}
	if (h == 301 || h == 302 || h == 303) {
		if ((d = parse_http_header(e->head, "Location", NULL))) {
			if (e->redirect) mem_free(e->redirect);
			e->redirect = d;
			e->redirect_get = h == 303;
		}
	}
 	if (h == 401) {
		d = parse_http_header(e->head, "WWW-Authenticate", NULL);
		if (d) {
			if (!strncasecmp(d, "Basic", 5)) {
				unsigned char *realm = get_http_header_param(d, "realm");

				if (realm) {
					if (add_auth_entry(host, realm) > 0) {
                                            need_auth=1;
                                        }
					mem_free(realm);
				}
			}
			mem_free(d);
		}
  	}

	kill_buffer_data(rb, a);
	c->cache = e;
	info->close = 0;
	info->length = -1;
	info->version = version;
	if ((d = parse_http_header(e->head, "Connection", NULL)) || (d = parse_http_header(e->head, "Proxy-Connection", NULL))) {
		if (!_stricmp(d, "close")) info->close = 1;
		mem_free(d);
	} else if (version < 11) info->close = 1;
	cf = c->from;
	c->from = 0;
	if ((d = parse_http_header(e->head, "Content-Range", NULL))) {
		if (strlen(d) > 6) {
			d[5] = 0;
			if (!(_stricmp(d, "bytes")) && d[6] >= '0' && d[6] <= '9') {
				int f = strtol(d + 6, NULL, 10);
				if (f >= 0) c->from = f;
			}
		}
		mem_free(d);
	}
	if (cf && !c->from && !c->unrestartable) c->unrestartable = 1;
	if (c->from > cf || c->from < 0) {
		setcstate(c, S_HTTP_ERROR);
		abort_connection(c);
		return;
	}
	if ((d = parse_http_header(e->head, "Content-Length", NULL))) {
		unsigned char *ep;
		int l = strtol(d, (char **)&ep, 10);
		if (!*ep && l >= 0) {
			if (!info->close || version >= 11) info->length = l;
			c->est_length = c->from + l;
		}
		mem_free(d);
	}
	if ((d = parse_http_header(e->head, "Accept-Ranges", NULL))) {
		if (!_stricmp(d, "none") && !c->unrestartable) c->unrestartable = 1;
		mem_free(d);
	} else if (!c->unrestartable && !c->from) c->unrestartable = 1;
	if ((d = parse_http_header(e->head, "Transfer-Encoding", NULL))) {
		if (!_stricmp(d, "chunked")) {
			info->length = -2;
			info->chunk_remaining = -1;
		}
		mem_free(d);
	}
	if (!info->close && info->length == -1) info->close = 1;
	if ((d = parse_http_header(e->head, "Last-Modified", NULL))) {
		if (e->last_modified && _stricmp(e->last_modified, d)) {
			delete_entry_content(e);
			if (c->from) {
				c->from = 0;
				mem_free(d);
				setcstate(c, S_MODIFIED);
				retry_connection(c);
				return;
			}
		}
		if (!e->last_modified) e->last_modified = d;
		else mem_free(d);
	}
	if (!e->last_modified && (d = parse_http_header(e->head, "Date", NULL)))
		e->last_modified = d;
	if (info->length == -1 || (version < 11 && info->close)) rb->close = 1;

	d = parse_http_header(e->head, "Content-Type", NULL);
	if (d) {
		if (!strncmp(d, "text", 4)) {
			mem_free(d);
			d = parse_http_header(e->head, "Content-Encoding", NULL);
			if (d) {
#ifdef HAVE_ZLIB_H
				if (!_stricmp(d, "gzip") || !_stricmp(d, "x-gzip"))
					c->content_encoding = ENCODING_GZIP;
#endif
#ifdef HAVE_BZLIB_H
				if (!_stricmp(d, "bzip2") || !_stricmp(d, "x-bzip2"))
					c->content_encoding = ENCODING_BZIP2;
#endif
				mem_free(d);
			}
		} else {
			mem_free(d);
		}
	}
	if (c->content_encoding != ENCODING_NONE) {
		if (e->encoding_info) mem_free(e->encoding_info);
		e->encoding_info = stracpy(encoding_names[c->content_encoding]);
	}


        read_http_data(c, rb);
}

void http_get_header(struct connection *c)
{
	struct read_buffer *rb;
	set_timeout(c);
	if (!(rb = alloc_read_buffer(c))) return;
	rb->close = 1;
	read_from_socket(c, c->sock1, rb, http_got_header);
}
