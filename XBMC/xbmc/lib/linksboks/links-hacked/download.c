/* Some downloading stuff */

#include "links.h"

struct session *get_download_ses(struct download *down)
{
	struct session *ses;
	foreach(ses, sessions) if (ses == down->ses) return ses;
	if (!list_empty(sessions)) return sessions.next;
	return NULL;
}

void abort_download(struct download *down)
{
	if (down->win) delete_window(down->win);
	if (down->ask) delete_window(down->ask);
	if (down->stat.state >= 0) change_connection(&down->stat, NULL, PRI_CANCEL);
	mem_free(down->url);
	if (down->handle != -1) close(down->handle);
	if (down->prog) {
		unlink(down->file);
		mem_free(down->prog);
	}
	mem_free(down->file);
	del_from_list(down);
	mem_free(down);
}

void abort_all_downloads()
{
	while (!list_empty(downloads)) abort_download(downloads.next);
}

void kill_downloads_to_file(unsigned char *file)
{
	struct download *down;
	foreach(down, downloads) if (!strcmp(down->file, file)) down = down->prev, abort_download(down->next);
}

void undisplay_download(struct download *down)
{
	if (down->win) delete_window(down->win);
}

int dlg_abort_download(struct dialog_data *dlg, struct dialog_item_data *di)
{
	register_bottom_half((void (*)(void *))abort_download, dlg->dlg->udata);
	return 0;
}

int dlg_undisplay_download(struct dialog_data *dlg, struct dialog_item_data *di)
{
	register_bottom_half((void (*)(void *))undisplay_download, dlg->dlg->udata);
	return 0;
}

void download_abort_function(struct dialog_data *dlg)
{
	struct download *down = dlg->dlg->udata;
	down->win = NULL;
}

void download_window_function(struct dialog_data *dlg)
{
	struct download *down = dlg->dlg->udata;
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, x, y;
	int t = 0;
	unsigned char *m, *u;
	struct status *stat = &down->stat;
	if (!F) redraw_below_window(dlg->win);
	down->win = dlg->win;
	if (stat->state == S_TRANS && stat->prg->elapsed / 100) {
		int l = 0;
		m = init_str();
		t = 1;
		add_to_str(&m, &l, _(TXT(T_RECEIVED), term));
		add_to_str(&m, &l, " ");
		add_xnum_to_str(&m, &l, stat->prg->pos);
		if (stat->prg->size >= 0)
			add_to_str(&m, &l, " "), add_to_str(&m, &l, _(TXT(T_OF),term)), add_to_str(&m, &l, " "), add_xnum_to_str(&m, &l, stat->prg->size), add_to_str(&m, &l, " ");
		add_to_str(&m, &l, "\n");
		if (stat->prg->elapsed >= CURRENT_SPD_AFTER * SPD_DISP_TIME)
			add_to_str(&m, &l, _(TXT(T_AVERAGE_SPEED), term));
		else add_to_str(&m, &l, _(TXT(T_SPEED), term));
		add_to_str(&m, &l, " ");
		add_xnum_to_str(&m, &l, (longlong)stat->prg->loaded * 10 / (stat->prg->elapsed / 100));
		add_to_str(&m, &l, "/s");
		if (stat->prg->elapsed >= CURRENT_SPD_AFTER * SPD_DISP_TIME) 
			add_to_str(&m, &l, ", "), add_to_str(&m, &l, _(TXT(T_CURRENT_SPEED), term)), add_to_str(&m, &l, " "),
			add_xnum_to_str(&m, &l, stat->prg->cur_loaded / (CURRENT_SPD_SEC * SPD_DISP_TIME / 1000)),
			add_to_str(&m, &l, "/s");
		add_to_str(&m, &l, "\n");
		add_to_str(&m, &l, _(TXT(T_ELAPSED_TIME), term));
		add_to_str(&m, &l, " ");
		add_time_to_str(&m, &l, stat->prg->elapsed);
		if (stat->prg->size >= 0 && stat->prg->loaded > 0) {
			add_to_str(&m, &l, ", ");
			add_to_str(&m, &l, _(TXT(T_ESTIMATED_TIME), term));
			add_to_str(&m, &l, " ");
			/*add_time_to_str(&m, &l, stat->prg->elapsed / 1000 * stat->prg->size / stat->prg->loaded * 1000 - stat->prg->elapsed);*/
			add_time_to_str(&m, &l, (stat->prg->size - stat->prg->pos) / ((longlong)stat->prg->loaded * 10 / (stat->prg->elapsed / 100)) * 1000);
		}
	} else m = stracpy(_(get_err_msg(stat->state), term));
	if (!m) return;
	u = stracpy(down->url);
	if (strchr(u, POST_CHAR)) *strchr(u, POST_CHAR) = 0;
	max_text_width(term, u, &max, AL_LEFT);
	min_text_width(term, u, &min, AL_LEFT);
	max_text_width(term, m, &max, AL_LEFT);
	min_text_width(term, m, &min, AL_LEFT);
	max_buttons_width(term, dlg->items, dlg->n, &max);
	min_buttons_width(term, dlg->items, dlg->n, &min);
	w = dlg->win->term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w < min) w = min;
	if (w > dlg->win->term->x - 2 * DIALOG_LB) w = dlg->win->term->x - 2 * DIALOG_LB;
	if (t && stat->prg->size >= 0) {
		if (w < DOWN_DLG_MIN) w = DOWN_DLG_MIN;
	} else {
		if (w > max) w = max;
	}
	if (w < 1) w = 1;
	y = 0;
	dlg_format_text(dlg, NULL, u, 0, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	if (t && stat->prg->size >= 0) y += gf_val(2, 2 * G_BFU_FONT_SIZE);
	dlg_format_text(dlg, NULL, m, 0, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items, dlg->n, 0, &y, w, NULL, AL_CENTER);
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + gf_val(1, G_BFU_FONT_SIZE);
	x = dlg->x + DIALOG_LB;
	dlg_format_text(dlg, term, u, x, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	if (t && stat->prg->size >= 0) {
		if (!F) {
			unsigned char q[64];
			int p = w - 6;
			y++;
			set_only_char(term, x, y, '[');
			set_only_char(term, x + w - 5, y, ']');
			fill_area(term, x + 1, y, (int)((longlong)p * (longlong)stat->prg->pos / (longlong)stat->prg->size), 1, COLOR_DIALOG_METER);
			sprintf(q, "%3d%%", (int)((longlong)100 * (longlong)stat->prg->pos / (longlong)stat->prg->size));
			print_text(term, x + w - 4, y, strlen(q), q, COLOR_DIALOG_TEXT);
			y++;
#ifdef G
		} else {
			unsigned char q[64];
			int p, s, ss, m;
			y += G_BFU_FONT_SIZE;
			sprintf(q, "] %3d%%", (int)((longlong)100 * (longlong)stat->prg->pos / (longlong)stat->prg->size));
			s = g_text_width(bfu_style_bw_mono, "[");
			ss = g_text_width(bfu_style_bw_mono, q);
			p = w - s - ss;
			if (p < 0) p = 0;
			m = (int)((longlong)p * (longlong)stat->prg->pos / (longlong)stat->prg->size);
			g_print_text(drv, term->dev, x, y, bfu_style_bw_mono, "[", NULL);
			drv->fill_area(term->dev, x + s, y, x + s + m, y + G_BFU_FONT_SIZE, bfu_fg_color);
			drv->fill_area(term->dev, x + s + m, y, x + s + p, y + G_BFU_FONT_SIZE, bfu_bg_color);
			g_print_text(drv, term->dev, x + w - ss, y, bfu_style_bw_mono, q, NULL);
			if (dlg->s) exclude_from_set(&dlg->s, x, y, x + w, y + G_BFU_FONT_SIZE);
			y += G_BFU_FONT_SIZE;
#endif
		}
	}
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text(dlg, term, m, x, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items, dlg->n, x, &y, w, NULL, AL_CENTER);
	mem_free(u);
	mem_free(m);
}

void display_download(struct terminal *term, struct download *down, struct session *ses)
{
	struct dialog *dlg;
	struct download *dd;
	foreach(dd, downloads) if (dd == down) goto found;
	return;
	found:
	if (!(dlg = mem_alloc(sizeof(struct dialog) + 3 * sizeof(struct dialog_item)))) return;
	memset(dlg, 0, sizeof(struct dialog) + 3 * sizeof(struct dialog_item));
	undisplay_download(down);
	down->ses = ses;
	dlg->title = TXT(T_DOWNLOAD);
	dlg->fn = download_window_function;
	dlg->abort = download_abort_function;
	dlg->udata = down;
	dlg->align = AL_CENTER;
	dlg->items[0].type = D_BUTTON;
	dlg->items[0].gid = B_ENTER | B_ESC;
	dlg->items[0].fn = dlg_undisplay_download;
	dlg->items[0].text = TXT(T_BACKGROUND);
	dlg->items[1].type = D_BUTTON;
	dlg->items[1].gid = 0;
	dlg->items[1].fn = dlg_abort_download;
	dlg->items[1].text = TXT(T_ABORT);
	dlg->items[2].type = D_END;
	do_dialog(term, dlg, getml(dlg, NULL));
}

void download_data(struct status *stat, struct download *down)
{

	struct cache_entry *ce;
	struct fragment *frag;
	if (stat->state >= S_WAIT && stat->state < S_TRANS) goto end_store;
	if (!(ce = stat->ce)) goto end_store;
	if (ce->last_modified)
	down->remotetime = parse_http_date(ce->last_modified);
/*	  fprintf(stderr,"\nFEFE date %s\n",ce->last_modified); */
	if (ce->redirect && down->redirect_cnt++ < MAX_REDIRECTS) {
		unsigned char *u, *p;
		if (stat->state >= 0) change_connection(&down->stat, NULL, PRI_CANCEL);
		u = join_urls(down->url, ce->redirect);
		if (!u) goto x;
                if (!options_get_bool("http_bugs_302_redirect")) if (!ce->redirect_get && (p = strchr(down->url, POST_CHAR))) add_to_strn(&u, p);
		mem_free(down->url);
		down->url = u;
		down->stat.state = S_WAIT_REDIR;
		if (down->win) {
			struct event ev = { EV_REDRAW, 0, 0, 0 };
			ev.x = down->win->term->x;
			ev.y = down->win->term->y;
			down->win->handler(down->win, &ev, 0);
		}
		/*if (!strchr(down->url, POST_CHAR)) {*/
			load_url(down->url, NULL, &down->stat, PRI_DOWNLOAD, NC_CACHE);
			return;
		/*} else {
			unsigned char *msg = init_str();
			int l = 0;
			add_bytes_to_str(&msg, &l, down->url, (unsigned char *)strchr(down->url, POST_CHAR) - down->url);
			msg_box(get_download_ses(down)->term, getml(msg, NULL), TXT(T_WARNING), AL_CENTER | AL_EXTD_TEXT, TXT(T_DO_YOU_WANT_TO_FOLLOW_REDIRECT_AND_POST_FORM_DATA_TO_URL), "", msg, "?", NULL, down, 3, TXT(T_YES), down_post_yes, B_ENTER, TXT(T_NO), down_post_no, 0, TXT(T_CANCEL), down_post_cancel, B_ESC);
		}*/
	}
x:
        foreach(frag, ce->frag) if (frag->offset <= down->last_pos && frag->offset + frag->length > down->last_pos) {
		int w = write(down->handle, frag->data + down->last_pos - frag->offset, frag->length - (down->last_pos - frag->offset));
		if (w == -1) {
			detach_connection(stat, down->last_pos);
			if (!list_empty(sessions)) {
				unsigned char *msg = stracpy(down->file);
				unsigned char *emsg = stracpy(strerror(errno));
				msg_box(get_download_ses(down)->term, getml(msg, emsg, NULL), TXT(T_DOWNLOAD_ERROR), AL_CENTER | AL_EXTD_TEXT, TXT(T_COULD_NOT_WRITE_TO_FILE), " ", msg, ": ", emsg, NULL, NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
			}
			abort_download(down);
			return;
		}
		down->last_pos += w;
	}
	detach_connection(stat, down->last_pos);
	end_store:;
	if (stat->state < 0) {
		if (stat->state != S_OKAY) {
			unsigned char *t = get_err_msg(stat->state);
			if (t) {
				unsigned char *tt = stracpy(down->url);
				if (strchr(tt, POST_CHAR)) *strchr(tt, POST_CHAR) = 0;
				msg_box(get_download_ses(down)->term, getml(tt, NULL), TXT(T_DOWNLOAD_ERROR), AL_CENTER | AL_EXTD_TEXT, TXT(T_ERROR_DOWNLOADING), " ", tt, ":\n\n", t, NULL, get_download_ses(down), 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC/*, TXT(T_RETRY), NULL, 0 !!! FIXME: retry */);
			}
		} else {
			if (down->prog) {
				close(down->handle), down->handle = -1;
				exec_on_terminal(get_download_ses(down)->term, down->prog, down->file, !!down->prog_flags);
				mem_free(down->prog), down->prog = NULL;
			} else if (down->remotetime && options_get_bool("network_download_utime")) {
//				struct utimbuf foo;
//				foo.actime = foo.modtime = down->remotetime;
//				utime(down->file, &foo);
			}
                }
		abort_download(down);
		return;
        }
	if (down->win) {
		struct event ev = { EV_REDRAW, 0, 0, 0 };
		ev.x = down->win->term->x;
		ev.y = down->win->term->y;
		down->win->handler(down->win, &ev, 0);
        }

}

struct lun_hop {
	struct terminal *term;
	unsigned char *ofile, *file;

	void (*callback)(struct terminal *, unsigned char *, void *, int);
	void *data;
};

static void
lun_alternate(struct lun_hop *lun_hop)
{
	lun_hop->callback(lun_hop->term, lun_hop->file, lun_hop->data, 0);
	if (lun_hop->ofile) mem_free(lun_hop->ofile);
	mem_free(lun_hop);
}

static void
lun_overwrite(struct lun_hop *lun_hop)
{
	lun_hop->callback(lun_hop->term, lun_hop->ofile, lun_hop->data, 0);
	if (lun_hop->file) mem_free(lun_hop->file);
	mem_free(lun_hop);
}

static void
lun_resume(struct lun_hop *lun_hop)
{
	lun_hop->callback(lun_hop->term, lun_hop->ofile, lun_hop->data, 1);
	if (lun_hop->file) mem_free(lun_hop->file);
	mem_free(lun_hop);
}

static void
lun_cancel(struct lun_hop *lun_hop)
{
	lun_hop->callback(lun_hop->term, NULL, lun_hop->data, 0);
	if (lun_hop->ofile) mem_free(lun_hop->ofile);
	if (lun_hop->file) mem_free(lun_hop->file);
	mem_free(lun_hop);
}

/* Only returns true/false. */
inline int
file_exists(const unsigned char *filename)
{
#ifdef HAVE_ACCESS
	return access(filename, F_OK) >= 0;
#else
	struct stat buf;
	return stat (filename, &buf) >= 0;
#endif
}

unsigned char *
expand_tilde(unsigned char *fi)
{
	unsigned char *file = init_str();
	int fl = 0;

	if (fi[0] == '~' && dir_sep(fi[1])) {
		unsigned char *home = getenv("HOME");

		if (home) {
			add_to_str(&file, &fl, home);
			fi++;
		}
	}

	add_to_str(&file, &fl, fi);
	return file;
}

/* Return unique file name based on a prefix by adding suffix counter. */
unsigned char *
get_unique_name(unsigned char *fileprefix)
{
	unsigned char *prefix;
	unsigned char *file;
	int memtrigger = 1;
	int suffix = 1;
	int digits = 0;
	int prefixlen;

	/* This 'copy_string' is not really needed, but it's replacing a call
	 * to 'expand_tilde', so the rest of the code doesn't need to be touched.
	 * This function should be cleaned, anyway, to get rid of the 'mem_free'
	 * calls for 'prefix', etc... NOTE THAT NOW THIS FUNCTION WANTS 'fileprefix'
	 * already 'tildexpanded'. This is fixed in current uses of this function. */
	copy_string(&prefix, fileprefix);
	if (!prefix) return NULL;
	prefixlen = strlen(prefix);
	file = prefix;

	while (file_exists(file)) {
		if (!(suffix < memtrigger)) {
			if (suffix >= 10000)
				internal("Too big suffix in get_unique_name().");
			memtrigger *= 10;
			digits++;

			if (file != prefix) mem_free(file);
			file = mem_alloc(prefixlen + 2 + digits);
			if (!file) return prefix;
			safe_strncpy(file, prefix, prefixlen + 1);
		}

		sprintf(&file[prefixlen], ".%d", suffix);
		suffix++;
	}

	if (prefix != file) mem_free(prefix);
	return file;
}

static void
lookup_unique_name(struct terminal *term, unsigned char *ofile, int resume,
		void (*callback)(struct terminal *, unsigned char *, void *, int),
		void *data)
{
        struct lun_hop *lun_hop;

        int overwrite = options_get_bool("network_download_prevent_overwriting");
        unsigned char *ofilex=expand_tilde(ofile);
	unsigned char *file;

	file = get_unique_name(ofile);

	if (!file || !strcmp(ofilex, file)) {
                if(file!=ofilex) mem_free(ofilex);
		/* Still nothing special to do... */
		callback(term, file, data, 0);
		return;
	}

	/* overwrite == 0 (ask) and file != ofilex (=> original file already
	 * exists) */

	lun_hop = mem_calloc(sizeof(struct lun_hop));
	if (!lun_hop) {
		if(file!=ofilex) mem_free(file);
                mem_free(ofilex);
		callback(term, NULL, data, 0);
		return;
	}
	lun_hop->term = term;
	lun_hop->ofile = ofilex;
	lun_hop->file = (file!=ofilex) ? file : stracpy(ofilex);
	lun_hop->callback = callback;
	lun_hop->data = data;

        msg_box(term, NULL,
                TXT(T_FILE_OVERWRITE), AL_CENTER | AL_EXTD_TEXT,
                TXT(T_FILE_ALREADY_EXISTS),
		lun_hop->ofile ? lun_hop->ofile : (unsigned char *) "", "\n\n",
		TXT(T_ALTERNATIVE_FILENAME), file ? file : (unsigned char *) "", NULL,
		lun_hop, 3,
		TXT(T_SAVE_UNDER_ALTERNATIVE_NAME), lun_alternate, B_ENTER,
		TXT(T_OVERWRITE_FILE), lun_overwrite, 0,
                /*
                TXT(T_RESUME_DOWNLOAD), lun_resume, 0,
                */
                TXT(T_CANCEL), lun_cancel, B_ESC);
}


static void create_download_file_do(struct terminal *, unsigned char *, void *, int);

struct cdf_hop {
	unsigned char **real_file;
	int safe;

	void (*callback)(struct terminal *, int, void *, int);
	void *data;
};

void
create_download_file(struct terminal *term, unsigned char *fi,
		     unsigned char **real_file, int safe, int resume,
		     void (*callback)(struct terminal *, int, void *, int),
		     void *data)
{
	struct cdf_hop *cdf_hop = mem_calloc(sizeof(struct cdf_hop));
	unsigned char *wd;


        if (!cdf_hop) {
		callback(term, -1, data, 0);
		return;
	}

	cdf_hop->real_file = real_file;
	cdf_hop->safe = safe;
	cdf_hop->callback = callback;
	cdf_hop->data = data;

	/* FIXME: The wd bussiness is probably useless here? --pasky */
	wd = get_cwd();
	set_cwd(term->cwd);

	/* Also the tilde will be expanded here. */
	lookup_unique_name(term, fi, resume, create_download_file_do, cdf_hop);

	if (wd) {
		set_cwd(wd);
		mem_free(wd);
	}
}

#define NO_FILE_SECURITY
static void
create_download_file_do(struct terminal *term, unsigned char *file, void *data,
			int resume)
{
	struct cdf_hop *cdf_hop = data;
	unsigned char *download_dir = options_get("network_download_directory");
	unsigned char *wd;
	int h = -1;
	int i;
	int saved_errno;
#ifdef NO_FILE_SECURITY
	int sf = 0;
#else
	int sf = cdf_hop->safe;
#endif


        if (!file) goto finish;


	wd = (unsigned char *)mem_alloc( 1024 );
	if( !download_dir )
		sprintf( wd, "D:\\%s", file );
	else
		strcpy( wd, file );

	/*wd = get_cwd();
	set_cwd(term->cwd);*/

//        h = open(file, O_CREAT | O_WRONLY | (resume ? 0 : O_TRUNC) );
        h = open(wd, O_CREAT | O_WRONLY | O_BINARY | (resume ? 0 : O_TRUNC) | (sf && !resume ? O_EXCL : 0),
		 sf ? 0600 : 0666);
	saved_errno = errno; /* Saved in case of ... --Zas */


	if (wd) {
		set_cwd(wd);
		mem_free(wd);
	}

	if (h == -1) {
		unsigned char *msg = stracpy(file);
		unsigned char *msge = stracpy(strerror(saved_errno));

		if (msg && msge) {
                        msg_box(term, getml(msg, msge, NULL),
                                TXT(T_DOWNLOAD_ERROR), AL_CENTER | AL_EXTD_TEXT,
                                TXT(T_COULD_NOT_WRITE_TO_FILE), " ", msg, ": ",
                                msge, NULL,
				NULL, 1,
				TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		} else {
			if (msg) mem_free(msg);
			if (msge) mem_free(msge);
		}

	} else {
		//set_bin(h);

#if 0
                /* Uncomment if you want download dialog to remember last download dir --karpov*/
                if (!cdf_hop->safe) {
			strncpy(download_dir, file, MAX_STR_LEN);

			/* Find the used directory so it's available in history */
			for (i = strlen(download_dir); i >= 0; i--)
				if (dir_sep(download_dir[i]))
					break;
			download_dir[i + 1] = 0;
                }
#endif
        }

	if (cdf_hop->real_file)
		*cdf_hop->real_file = file;
	else
		mem_free(file);

finish:
	cdf_hop->callback(term, h, cdf_hop->data, resume);
	mem_free(cdf_hop);
	return;
}


unsigned char *
get_temp_name(unsigned char *url)
{
	int l, nl;
	unsigned char *name;
	unsigned char *fn, *fnn, *fnnn, *s;
	unsigned char *nm;
	unsigned char *basename[15];

	sprintf(basename, "links-%d", (rand() % 9000000) + 1000000);

	nm = tempnam(NULL, basename);

	if (!nm) return NULL;

	name = init_str();
	if (!name) {
		mem_free(nm);
		return NULL;
	}
	nl = 0;
	add_to_str(&name, &nl, nm);
	free(nm);

	get_filename_from_url(url, &fn, &l);

	fnnn = NULL;
	for (fnn = fn; fnn < fn + l; fnn++)
		if (*fnn == '.')
			fnnn = fnn;

	if (fnnn) {
		s = memacpy(fnnn, l - (fnnn - fn));
		if (s) {
			check_shell_security(&s);
			add_to_str(&name, &nl, s);
			mem_free(s);
		}
	}

	return name;
}


unsigned char *
subst_file(unsigned char *prog, unsigned char *file)
{
	unsigned char *n = init_str();
	int l = 0;

	if (!n) return NULL;

	while (*prog) {
		int p;

		for (p = 0; prog[p] && prog[p] != '%'; p++);

		add_bytes_to_str(&n, &l, prog, p);
		prog += p;

		if (*prog == '%') {
#if defined(HAVE_CYGWIN_CONV_TO_FULL_WIN32_PATH)
#ifdef MAX_PATH
			unsigned char new_path[MAX_PATH];
#else
			unsigned char new_path[1024];
#endif

			cygwin_conv_to_full_win32_path(file, new_path);
			add_to_str(&n, &l, new_path);
#else
			add_to_str(&n, &l, file);
#endif
			prog++;
		}
	}

	return n;
}


static void common_download_do(struct terminal *, int, void *, int);

struct cmdw_hop {
	struct session *ses;
	unsigned char *real_file;
};

static void
common_download(struct session *ses, unsigned char *file, int resume)
{
	struct cmdw_hop *cmdw_hop;

        if (!ses->dn_url) return;

	cmdw_hop = mem_calloc(sizeof(struct cmdw_hop));
	if (!cmdw_hop) return;
	cmdw_hop->ses = ses;

	kill_downloads_to_file(file);

	create_download_file(ses->term, file, &cmdw_hop->real_file, 0, resume,
			common_download_do, cmdw_hop);
}

static void
common_download_do(struct terminal *term, int fd, void *data, int resume)
{
	struct cmdw_hop *cmdw_hop = data;
	struct download *down = NULL;
	unsigned char *url = cmdw_hop->ses->dn_url;
	struct stat buf;

	if (!cmdw_hop->real_file) goto error;

	down = mem_calloc(sizeof(struct download));
	if (!down) goto error;

	down->url = stracpy(url);
	if (!down->url) goto error;

	down->file = cmdw_hop->real_file;

#ifdef __XBOX__
	if (fstat(xbox_get_file(fd), &buf)) goto error;
#else
	if (fstat(fd, &buf)) goto error;
#endif
	down->last_pos = resume ? (int) buf.st_size : 0;

	down->stat.end = (void (*)(struct status *, void *)) download_data;
	down->stat.data = down;
	down->handle = fd;
	down->ses = cmdw_hop->ses;
	down->remotetime = 0;

	add_to_list(downloads, down);
	load_url(url, NULL, &down->stat, PRI_DOWNLOAD, NC_CACHE);

        display_download(cmdw_hop->ses->term, down, cmdw_hop->ses);

	mem_free(cmdw_hop);
	return;

error:
	if (down) {
		if (down->url) mem_free(down->url);
		mem_free(down);
	}
	mem_free(cmdw_hop);
}

void
start_download(struct session *ses, unsigned char *file)
{
	common_download(ses, file, 0);
}

void
resume_download(struct session *ses, unsigned char *file)
{
	common_download(ses, file, 1);
}

