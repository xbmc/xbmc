/* HTTP Authentication support */ 
#include "links.h"

#define IS_QUOTE(x) ((x) == '"' || (x) == '\'')

int need_auth;

struct list_head http_auth_basic_list = { &http_auth_basic_list, &http_auth_basic_list };


void init_auth()
{
    need_auth=0;
}

/* Returns a valid host url for http authentification or NULL. */
/* FIXME: This really belongs to url.c, but it would look alien there. */
static unsigned char *
get_auth_url(unsigned char *url)
{
        unsigned char *protocol = get_protocol_name(url);
        unsigned char *host = get_host_name(url);
        unsigned char *port = get_port_str(url);
        unsigned char *newurl = NULL;

        /* protocol == NULL is tested in straconcat() */;
        newurl = straconcat(protocol, "://", host, NULL);
        if (!newurl) goto end;

        if (port && *port) {
                add_to_strn(&newurl, ":");
                add_to_strn(&newurl, port);
        } else {
                if (!_stricmp(protocol, "http")) {
                        /* RFC2616 section 3.2.2
                         * If the port is empty or not given, port 80 is
                         * assumed. */
                        add_to_strn(&newurl, ":");
                        add_to_strn(&newurl, "80");
                } else if (!_stricmp(protocol, "https")) {
                        add_to_strn(&newurl, ":");
                        add_to_strn(&newurl, "443");
                }
        }

end:
        if (protocol) mem_free(protocol);
        if (host) mem_free(host);
        if (port) mem_free(port);

        return newurl;
}


/* Find if url/realm is in auth list. If a matching url is found, but realm is
 * NULL, it returns the first record found. If realm isn't NULL, it returns
 * the first record that matches exactly (url and realm) if any. */
struct http_auth_basic *
find_auth_entry(unsigned char *url, unsigned char *realm)
{
        struct http_auth_basic *entry = NULL, *tmp_entry;

        if (!url || !*url) return NULL;

        foreach(tmp_entry, http_auth_basic_list) {
                if (!_stricmp(tmp_entry->url, url)) {
                        /* Found a matching url. */
                        entry = tmp_entry;
                        if (realm) {
                                /* From RFC 2617 section 1.2:
                                 * The realm value (case-sensitive), in
                                 * combination with the canonical root
                                 * URL (the absolute URI for the server
                                 * whose abs_path is empty; see section
                                 * 5.1.2 of [2]) of the server being accessed,
                                 * defines the protection space. */
                                if (tmp_entry->realm
                                    && !strcmp(tmp_entry->realm, realm)) {
                                        /* Exact match. */
                                        break; /* Stop here. */
                                }
                        } else {
                                /* Since realm is NULL, stops immediatly. */
                                break;
                        }
                }
        }

        return entry;
}

/* Add a Basic Auth entry if needed. Returns -1 on error, 0 if entry do not
 * exists and user/pass are in url, 1 if exact entry already exists or is
 * in blocked state, 2 if entry was added. */
/* FIXME: use an enum for return codes. */
int
add_auth_entry(unsigned char *url, unsigned char *realm)
{
        struct http_auth_basic *entry;
        unsigned char *user = get_user_name(url);
        unsigned char *pass = get_pass(url);
        unsigned char *newurl = get_auth_url(url);
        int ret = -1;

        if (!newurl || !user || !pass) goto end;

        /* Is host/realm already known ? */
        entry = find_auth_entry(newurl, realm);
        if (entry) {
                /* Found an entry. */
                if (entry->blocked == 1) {
                        /* Waiting for user/pass in dialog. */
                        ret = 1;
                        goto end;
                }

                /* If we have user/pass info then check if identical to
                 * those in entry. */
                if ((*user || *pass) && entry->uid && entry->passwd) {
                        if (((!realm && !entry->realm)
                             || (realm && entry->realm
                                 && !strcmp(realm, entry->realm)))
                            && !strcmp(user, entry->uid)
                            && !strcmp(pass, entry->passwd)) {
                                /* Same host/realm/pass/user. */
                                ret = 1;
                                goto end;
                        }
                }

                /* Delete entry and re-create it... */
                /* FIXME: Could be better... */
                del_auth_entry(entry);
        }

        /* Create a new entry. */
        entry = mem_alloc(sizeof(struct http_auth_basic));
        if (!entry) goto end;
        memset(entry, 0, sizeof(struct http_auth_basic));

        entry->url = newurl;
        entry->url_len = strlen(entry->url); /* FIXME: Not really needed. */

        if (realm) {
                /* Copy realm value. */
                entry->realm = stracpy(realm);
                if (!entry->realm) {
                        mem_free(entry);
                        goto end;
                }
        }

        if (*user || *pass) {
                /* Copy user and pass info if any in passed url. */
                entry->uid = mem_alloc(MAX_UID_LEN);
                if (!entry->uid) {
                        mem_free(entry);
                        goto end;
                }
                safe_strncpy(entry->uid, user, MAX_UID_LEN);

                entry->passwd = mem_alloc(MAX_PASSWD_LEN);
                if (!entry->passwd) {
                        mem_free(entry);
                        goto end;
                }
                safe_strncpy(entry->passwd, pass, MAX_PASSWD_LEN);

                ret = 0; /* Entry added with user/pass from url. */
        }

        add_to_list(http_auth_basic_list, entry);

        if (ret) ret = 2; /* Entry added. */

end:
        if (ret == -1 || ret == 1) {
               if (newurl) mem_free(newurl);
        }

        if (user) mem_free(user);
        if (pass) mem_free(pass);

        return ret;
}

/* Find an entry in auth list by url. If url contains user/pass information
 * and entry does not exist then entry is created.
 * If entry exists but user/pass passed in url is different, then entry is
 * updated (but not if user/pass is set in dialog).
 * It returns NULL on failure, or a base 64 encoded user + pass suitable to
 * use in Authorization header. */
unsigned char *
find_auth(unsigned char *url)
{
        struct http_auth_basic *entry = NULL;
        unsigned char *uid, *ret = NULL;
        unsigned char *newurl = get_auth_url(url);
        unsigned char *user = get_user_name(url);
        unsigned char *pass = get_pass(url);

        if (!newurl) goto end;

again:
        entry = find_auth_entry(newurl, NULL);

        /* Check is user/pass info is in url. */
        if ((user && *user) || (pass && *pass)) {
                /* If we've got an entry, but with different user/pass or no
                 * entry, then we try to create or modify it and retry. */
                if ((entry && !entry->valid && entry->uid && entry->passwd
                     && (strcmp(user, entry->uid) || strcmp(pass, entry->passwd)))
                    || !entry) {
                        if (add_auth_entry(url, NULL) == 0) {
                                /* An entry was re-created, we free user/pass
                                 * before retry to prevent infinite loop. */
                                if (user) {
                                        mem_free(user);
                                        user = NULL;
                                }
                                if (pass) {
                                        mem_free(pass);
                                        pass = NULL;
                                }
                                goto again;
                        }
                }
        }

        /* No entry found. */
        if (!entry) goto end;

        /* Sanity check. */
        if (!entry->passwd || !entry->uid) {
                del_auth_entry(entry);
                goto end;
        }

        /* RFC2617 section 2 [Basic Authentication Scheme]
         * To receive authorization, the client sends the userid and password,
         * separated by a single colon (":") character, within a base64 [7]
         * encoded string in the credentials. */

        /* Create base64 encoded string. */
        uid = straconcat(entry->uid, ":", entry->passwd, NULL);
        if (!uid) goto end;

        ret = base64_encode(uid);
        mem_free(uid);

end:
        if (newurl) mem_free(newurl);
        if (user) mem_free(user);
        if (pass) mem_free(pass);

        return ret;
}

/* Delete an entry from auth list. */
void
del_auth_entry(struct http_auth_basic *entry)
{
        if (entry->url) mem_free(entry->url);
        if (entry->realm) mem_free(entry->realm);
        if (entry->uid) mem_free(entry->uid);
        if (entry->passwd) mem_free(entry->passwd);
        del_from_list(entry);
        mem_free(entry);
}

/* Free all entries in auth list and questions in queue. */
void
free_auth()
{
        while (!list_empty(http_auth_basic_list))
                del_auth_entry(http_auth_basic_list.next);
}

unsigned char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char *base64_encode(unsigned char *in)
{
	unsigned char *out, *outstr;
	int inlen = strlen(in);
	
	outstr = out = mem_alloc(((inlen / 3) + 1) * 4 + 1 );
	if (!outstr) return NULL;

	while (inlen >= 3) {
		*out++ = base64_chars[(int) (*in >> 2) ];
		*out++ = base64_chars[(int) ((*in << 4 | *(in + 1) >> 4) & 63) ];
		*out++ = base64_chars[(int) ((*(in + 1) << 2 | *(in + 2) >> 6) & 63) ];
		*out++ = base64_chars[(int) (*(in + 2) & 63) ];
		inlen -= 3; in += 3;
	}
	if (inlen == 1) {
		*out++ = base64_chars[(int) (*in >> 2) ];
		*out++ = base64_chars[(int) (*in << 4 & 63) ];
		*out++ = '=';
		*out++ = '=';
	}
	if (inlen == 2) {
		*out++ = base64_chars[(int) (*in >> 2) ];
		*out++ = base64_chars[(int) ((*in << 4 | *(in + 1) >> 4) & 63) ];
		*out++ = base64_chars[(int) ((*(in + 1) << 2) & 63) ];
		*out++ = '=';
	}
	*out = 0;
	
	return outstr;
}

/* Parse string param="value", return value as new string or NULL if any
 * error. */
unsigned char *get_http_header_param(unsigned char *e, unsigned char *name)
{
	unsigned char *n, *start;
	int i = 0;

again:
	while (*e && upcase(*e++) != upcase(*name));
	if (!*e) return NULL;
	n = name + 1;
	while (*n && upcase(*e) == upcase(*n)) e++, n++;
	if (*n) goto again;
	while (WHITECHAR(*e)) e++;
	if (*e++ != '=') return NULL;
	while (WHITECHAR(*e)) e++;

	start = e;
	if (!IS_QUOTE(*e)) while (*e && !WHITECHAR(*e)) e++;
	else {
		char uu = *e++;

		start++;
		while (*e != uu) {
			if (!*e) return NULL;
			e++;
		}
	}

	while (start < e && *start == ' ') start++;
	while (start < e && *(e - 1) == ' ') e--;
	if (start == e) return NULL;

	n = mem_alloc(e - start + 1);
	if (!n) return NULL;
	while (start < e) {
		if (*start < ' ') n[i] = '.';
		else n[i] = *start;
		i++; start++;
	}
	n[i] = 0;

	return n;
}

/* Auth dialog */

void
auth_layout(struct dialog_data *dlg)
{
        struct terminal *term = dlg->win->term;
        int max = 0, min = 0;
        int w, rw;
        int y = -1;

	max_text_width(term, TXT(T_USERID), &max, AL_LEFT);
        min_text_width(term, TXT(T_USERID), &min, AL_LEFT);
        max_text_width(term, TXT(T_PASSWORD), &max, AL_LEFT);
        min_text_width(term, TXT(T_PASSWORD), &min, AL_LEFT);
        max_buttons_width(term, dlg->items + 2, 2,  &max);
        min_buttons_width(term, dlg->items + 2, 2,  &min);

        w = dlg->win->term->x * 9 / 10 - 2 * DIALOG_LB;
        if (w < min) w = min;
        if (w > dlg->win->term->x - 2 * DIALOG_LB) w = dlg->win->term->x - 2 * DIALOG_LB;
        if (w < 1) w = 1;
        rw = 0;
        if (dlg->dlg->udata) {
                dlg_format_text(dlg, NULL, dlg->dlg->udata, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
                y += gf_val(1, G_BFU_FONT_SIZE);
        }

        dlg_format_text(dlg, NULL, TXT(T_USERID), 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE*2);
        dlg_format_text(dlg, NULL, TXT(T_PASSWORD), 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE*2);
        dlg_format_buttons(dlg, NULL, dlg->items + 2, 2, 0, &y, w, &rw, AL_CENTER);
        w = rw;
        dlg->xw = w + 2 * DIALOG_LB;
        dlg->yw = y + 2 * DIALOG_TB;
        center_dlg(dlg);
        draw_dlg(dlg);
        y = dlg->y + DIALOG_TB;
        if (dlg->dlg->udata) {
                dlg_format_text(dlg, term, dlg->dlg->udata, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
                y += gf_val(1, G_BFU_FONT_SIZE);
        }
        dlg_format_text(dlg, term, TXT(T_USERID), dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
        dlg_format_field(dlg, term, &dlg->items[0], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
        dlg_format_text(dlg, term, TXT(T_PASSWORD), dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
        dlg_format_field(dlg, term, &dlg->items[1], dlg->x + DIALOG_LB, &y, w, NULL, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
        dlg_format_buttons(dlg, term, &dlg->items[2], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

int
auth_ok(struct dialog_data *dlg, struct dialog_item_data *di)
{
        ((struct http_auth_basic *)dlg->dlg->udata2)->blocked = 0;
        reload(dlg->dlg->refresh_data, -1);
        return ok_dialog(dlg, di);
}

int
auth_cancel(struct dialog_data *dlg, struct dialog_item_data *di)
{
        ((struct http_auth_basic *)dlg->dlg->udata2)->blocked = 0;
        del_auth_entry(dlg->dlg->udata2);
        return cancel_dialog(dlg, di);
}

/* FIXME: This should be exported properly. --pasky */
extern struct list_head http_auth_basic_list;

void
do_auth_dialog(struct session *ses)
{
        /* TODO: complete rewrite */
        struct dialog *d;
        struct terminal *term = ses->term;
        struct http_auth_basic *a = NULL;

        if (!list_empty(http_auth_basic_list)
            && !((struct http_auth_basic *) http_auth_basic_list.next)->valid)
                a = (struct http_auth_basic *) http_auth_basic_list.next;
        if (!a || a->blocked) return;
        a->valid = 1;
        a->blocked = 1;
        if (!a->uid) {
                if (!(a->uid = mem_alloc(MAX_UID_LEN))) {
                        del_auth_entry(a);
                        return;
                }
                *a->uid = 0;
        }
        if (!a->passwd) {
                if (!(a->passwd = mem_alloc(MAX_PASSWD_LEN))) {
                        del_auth_entry(a);
                        return;
                }
                *a->passwd = 0;
        }
        d = mem_alloc(sizeof(struct dialog) + 5 * sizeof(struct dialog_item)
                      + strlen(_(TXT(T_ENTER_USERNAME), term))
                      + (a->realm ? strlen(a->realm) : 0)
                      + strlen(_(TXT(T_AT), term)) + strlen(a->url) + 1);
        if (!d) return;
        memset(d, 0, sizeof(struct dialog) + 5 * sizeof(struct dialog_item));
        d->title = TXT(T_USERID);
        d->fn = auth_layout;

        d->udata = (char *)d + sizeof(struct dialog) + 5 * sizeof(struct dialog_item);
        strcpy(d->udata, _(TXT(T_ENTER_USERNAME), term));
        if (a->realm) strcat(d->udata, a->realm);
        strcat(d->udata, _(TXT(T_AT), term));
        strcat(d->udata, a->url);

        d->udata2 = a;
        d->refresh_data = ses;

        d->items[0].type = D_FIELD;
        d->items[0].dlen = MAX_UID_LEN;
        d->items[0].data = a->uid;

        d->items[1].type = D_FIELD_PASS;
        d->items[1].dlen = MAX_PASSWD_LEN;
        d->items[1].data = a->passwd;

        d->items[2].type = D_BUTTON;
        d->items[2].gid = B_ENTER;
        d->items[2].fn = auth_ok;
        d->items[2].text = TXT(T_OK);

        d->items[3].type = D_BUTTON;
        d->items[3].gid = B_ESC;
        d->items[3].fn = auth_cancel;
        d->items[3].text = TXT(T_CANCEL);

        d->items[4].type = D_END;
        do_dialog(term, d, getml(d, NULL));
        a->blocked = 0;

}

/* Concatenate all strings parameters. Parameters list must _always_ be
 * terminated by a NULL pointer.  If first parameter is NULL or allocation
 * failure, return NULL.  On success, returns a pointer to a dynamically
 * allocated string.
 *
 * Example:
 * ...
 * unsigned char *s = straconcat("A", "B", "C", NULL);
 * if (!s) return;
 * printf("%s", s); -> print "ABC"
 * mem_free(s); -> free memory used by s
 */
unsigned char *straconcat(unsigned char *str, ...)
{
	va_list ap;
	unsigned char *a;
	unsigned char *s;
	unsigned int len;

	if (!str) return NULL;

	s = stracpy(str);
	if (!s) return NULL;

	len = strlen(s) + 1;

	va_start(ap, str);
	while ((a = va_arg(ap, unsigned char *))) {
		unsigned char *p;

		len += strlen(a);
		p = mem_realloc(s, len);
		if (!p) {
			mem_free(s);
			va_end(ap);
			return NULL;
		}
		s = p;
		strcat(s, a);
	}

	va_end(ap);

	return s;
}
