/* cookies.c
 * Cookies
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL
 */

#include "links.h"

#define ACCEPT_NONE	0
#define ACCEPT_ASK	1
#define ACCEPT_ALL	2

int accept_cookies = ACCEPT_ALL;

int cookies_nosave = 0;

tcount cookie_id = 0;

struct list_head cookies = { &cookies, &cookies };

struct list_head c_domains = { &c_domains, &c_domains };

struct c_server {
	struct c_server *next;
	struct c_server *prev;
	int accept;
	unsigned char server[1];
};

struct list_head c_servers = { &c_servers, &c_servers };

void accept_cookie(struct cookie *);
void delete_cookie(struct cookie *);

void save_cookies();

void free_cookie(struct cookie *c)
{
        if (c->name) mem_free(c->name);
	if (c->value) mem_free(c->value);
	if (c->server) mem_free(c->server);
	if (c->path) mem_free(c->path);
	if (c->domain) mem_free(c->domain);
}

int check_domain_security(unsigned char *server, unsigned char *domain)
{
	int i, j, dl, nd;
	if (domain[0] == '.') domain++;
	dl = strlen(domain);
	if (dl > strlen(server)) return 1;
	for (i = strlen(server) - dl, j = 0; server[i]; i++, j++)
		if (upcase(server[i]) != upcase(domain[j])) return 1;
	nd = 2;
	if (dl > 4 && domain[dl - 4] == '.') {
		unsigned char *tld[] = { "com", "edu", "net", "org", "gov", "mil", "int", NULL };
		for (i = 0; tld[i]; i++) if (!casecmp(tld[i], &domain[dl - 3], 3)) {
			nd = 1;
			break;
		}
	}
	for (i = 0; domain[i]; i++) if (domain[i] == '.') if (!--nd) break;
	if (nd > 0) return 1;
	return 0;
}

/* sezere 1 cookie z retezce str, na zacatku nesmi byt zadne whitechars
 * na konci muze byt strednik nebo 0
 * cookie musi byt ve tvaru nazev=hodnota, kolem rovnase nesmi byt zadne mezery
 * (respektive mezery se budou pocitat do nazvu a do hodnoty)
 */
int set_cookie(struct terminal *term, unsigned char *url, unsigned char *str)
{
	struct cookie *cookie;
	struct c_server *cs;
	unsigned char *p, *q, *s, *server, *date, *document;

        if (accept_cookies == ACCEPT_NONE) return 0;

        for (p = str; *p != ';' && *p; p++); /* if (WHITECHAR(*p)) return 0;*/

        for (q = str; *q != '='; q++) if (!*q || q >= p) return 0;
	if (str == q || q + 1 == p) return 0;
	if (!(cookie = mem_alloc(sizeof(struct cookie)))) return 0;
	document = get_url_data(url);
	server = get_host_name(url);
	cookie->name = memacpy(str, q - str);
	cookie->value = memacpy(q + 1, p - q - 1);
	cookie->server = stracpy(server);
	date = parse_header_param(str, "expires");
	if (date) {
		cookie->expires = parse_http_date(date);
		/*if (! cookie->expires) cookie->expires++;*/ /* no harm and we can use zero then */
		mem_free(date);
	} else
		cookie->expires = 0;
	if (!(cookie->path = parse_header_param(str, "path"))) {
		unsigned char *w;
		cookie->path = stracpy("/");
		add_to_strn(&cookie->path, document);
		for (w = cookie->path; *w; w++) if (end_of_dir(*w)) {
			*w = 0;
			break;
		}
		for (w = cookie->path + strlen(cookie->path) - 1; w >= cookie->path; w--)
			if (*w == '/') {
				w[1] = 0;
				break;
			}
	} else {
		if (!cookie->path[0] || cookie->path[strlen(cookie->path) - 1] != '/')
			add_to_strn(&cookie->path, "/");
		if (cookie->path[0] != '/') {
			add_to_strn(&cookie->path, "x");
			memmove(cookie->path + 1, cookie->path, strlen(cookie->path) - 1);
			cookie->path[0] = '/';
		}
	}
	if (!(cookie->domain = parse_header_param(str, "domain"))) cookie->domain = stracpy(server);
	if (cookie->domain[0] == '.') memmove(cookie->domain, cookie->domain + 1, strlen(cookie->domain));
	if ((s = parse_header_param(str, "secure"))) {
		cookie->secure = 1;
		mem_free(s);
	} else cookie->secure = 0;
	if (check_domain_security(server, cookie->domain)) {
		mem_free(cookie->domain);
		cookie->domain = stracpy(server);
	}
	cookie->id = cookie_id++;
	foreach (cs, c_servers) if (!_stricmp(cs->server, server)) {
		if (cs->accept) goto ok;
		else {
			free_cookie(cookie);
			mem_free(cookie);
			mem_free(server);
			return 0;
		}
	}
	if (accept_cookies != ACCEPT_ALL) {
		free_cookie(cookie);
		mem_free(cookie);
		mem_free(server);
		return 1;
	}
	ok:
	accept_cookie(cookie);
	mem_free(server);
	return 0;
}

void accept_cookie(struct cookie *c)
{
	struct c_domain *cd;
	struct cookie *d, *e;
	foreach(d, cookies) if (!_stricmp(d->name, c->name) && !_stricmp(d->domain, c->domain)) {
		e = d;
		d = d->prev;
		del_from_list(e);
		free_cookie(e);
		mem_free(e);
	}
	add_to_list(cookies, c);
	foreach(cd, c_domains) if (!_stricmp(cd->domain, c->domain)) return;
	if (!(cd = mem_alloc(sizeof(struct c_domain) + strlen(c->domain) + 1))) return;
	strcpy(cd->domain, c->domain);
	add_to_list(c_domains, cd);
        save_cookies();
}

void delete_cookie(struct cookie *c)
{
	struct c_domain *cd;
	struct cookie *d;
	foreach(d, cookies) if (!_stricmp(d->domain, c->domain)) goto x;
	foreach(cd, c_domains) if (!_stricmp(cd->domain, c->domain)) {
		del_from_list(cd);
		mem_free(cd);
		break;
	}
	x:
	del_from_list(c);
	free_cookie(c);
	mem_free(c);
}

struct cookie *find_cookie_id(void *idp)
{
	int id = (int)idp;
	struct cookie *c;
	foreach(c, cookies) if (c->id == id) return c;
	return NULL;
}

void reject_cookie(void *idp)
{
	struct cookie *c;
	if (!(c = find_cookie_id(idp))) return;
	delete_cookie(c);
}

void cookie_default(void *idp, int a)
{
	struct cookie *c;
	struct c_server *s;
	if (!(c = find_cookie_id(idp))) return;
	foreach(s, c_servers) if (!_stricmp(s->server, c->server)) goto found;
	if ((s = mem_alloc(sizeof(struct c_server) + strlen(c->server) + 1))) {
		strcpy(s->server, c->server);
		add_to_list(c_servers, s);
		found:
		s->accept = a;
	}
}

void accept_cookie_always(void *idp)
{
	cookie_default(idp, 1);
}

void accept_cookie_never(void *idp)
{
	cookie_default(idp, 0);
	reject_cookie(idp);
}

int is_in_domain(unsigned char *d, unsigned char *s)
{
	int dl = strlen(d);
	int sl = strlen(s);
	if (dl > sl) return 0;
	if (dl == sl) return !_stricmp(d, s);
	if (s[sl - dl - 1] != '.') return 0;
	return !casecmp(d, s + sl - dl, dl);
}

int is_path_prefix(unsigned char *d, unsigned char *s)
{
	int dl = strlen(d);
	int sl = strlen(s);
	if (dl > sl) return 0;
	return !memcmp(d, s, dl);
}

int cookie_expired(struct cookie *c)	/* parse_http_date is broken */
{
  	return 0 && (c->expires && c->expires < time(NULL));
}

void send_cookies(unsigned char **s, int *l, unsigned char *url)
{
	int nc = 0;
	struct c_domain *cd;
	struct cookie *c, *d;
	unsigned char *server = get_host_name(url);
	unsigned char *data = get_url_data(url);
	if (data > url) data--;
	foreach (cd, c_domains) if (is_in_domain(cd->domain, server)) goto ok;
	mem_free(server);
	return;
	ok:
	foreach (c, cookies) if (is_in_domain(c->domain, server)) if (is_path_prefix(c->path, data)) {
		if (cookie_expired(c)) {
			d = c;
			c = c->prev;
			del_from_list(d);
			free_cookie(d);
			mem_free(d);
			continue;
		}
		if (c->secure && strncmp(url, "https://", 8)) continue;
		if (!nc) add_to_str(s, l, "Cookie: "), nc = 1;
		else add_to_str(s, l, "; ");
		add_to_str(s, l, c->name);
		add_to_str(s, l, "=");
		add_to_str(s, l, c->value);
	}
	if (nc) add_to_str(s, l, "\r\n");
	mem_free(server);
}

void load_cookies() {
	/* Buffer size is set to be enough to read long lines that
	 * save_cookies may write. 6 is choosen after the fprintf(..) call
	 * in save_cookies(). --Zas */
	unsigned char in_buffer[6 * MAX_STR_LEN];
	unsigned char *cookfile, *p, *q;
	FILE *fp;
	struct cookie *c;

	/* Must be called after init_home */
	/* if (!elinks_home) return; */ /* straconcat() checks that --Zas */

	cookfile = straconcat(links_home, "cookies", NULL);
	if (!cookfile) return;

	/* Do it here, as we will delete whole cookies list if the file was
	 * removed */
	free_list(c_domains);

	foreach(c, cookies)
		free_cookie(c);
	free_list(cookies);

	fp = fopen(cookfile, "r");
	mem_free(cookfile);
	if (!fp) return;

	while (fgets(in_buffer, 6 * MAX_STR_LEN, fp)) {
		struct cookie *cookie = mem_calloc(sizeof(struct cookie));

		if (!cookie) return;

		q = in_buffer;
		p = strchr(in_buffer, '\t');
		if (!p)	goto inv;
		*p++ = '\0';
		cookie->name = stracpy(q);

		q = p;
		p = strchr(p, '\t');
		if (!p) goto inv;
		*p++ = '\0';
		cookie->value = stracpy(q);

		q = p;
		p = strchr(p, '\t');
		if (!p) goto inv;
		*p++ = '\0';
		cookie->server = stracpy(q);

		q = p;
		p = strchr(p, '\t');
		if (!p) goto inv;
		*p++ = '\0';
		cookie->path = stracpy(q);

		q = p;
		p = strchr(p, '\t');
		if (!p) goto inv;
		*p++ = '\0';
		cookie->domain = stracpy(q);

		q = p;
		p = strchr(p, '\t');
		if (!p) goto inv;
		*p++ = '\0';
		cookie->expires = atol(q);

		cookie->secure = atoi(p);

		cookie->id = cookie_id++;

		/* XXX: We don't want to overwrite the cookies file
		 * periodically to our death. */
		cookies_nosave = 1;
		accept_cookie(cookie);
		cookies_nosave = 0;

		continue;

inv:
		free_cookie(cookie);
		mem_free(cookie);
	}

	fclose(fp);
}
void init_cookies()
{
    load_cookies();
}

void save_cookies()
{
    unsigned char *cookfile;
    struct cookie *c;
    FILE *file;

    if(cookies_nosave) return;

	cookfile = straconcat(links_home, "cookies", NULL);
    if (!cookfile) return;

    file=fopen(cookfile,"w");
    mem_free(cookfile);

    if (!file) return;

    foreach(c,cookies)
        if(c->expires && !cookie_expired(c))
            fprintf(file,"%s\t%s\t%s\t%s\t%s\t%ld\t%d\n",
                   c->name, c->value,
                   c->server ? c->server : (unsigned char *) "",
                   c->path ? c->path : (unsigned char *) "",
                   c->domain ? c->domain: (unsigned char*) "",
                   c->expires, c->secure);
    fclose(file);
}

void cleanup_cookies()
{
	struct cookie *c;
	free_list(c_domains);
        save_cookies();
        /* !!! FIXME: save cookies */
	foreach (c, cookies) free_cookie(c);
	free_list(cookies);
}

