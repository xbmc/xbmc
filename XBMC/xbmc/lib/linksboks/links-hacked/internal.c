/*
 * Handler for internal: pseudo-protocol.
 *
 * Here will be 'about', 'keys' (after keys rebinding implementation),
 * and so on.
 *
 * TODO: Internationalize
 *
 */

#include "links.h"

/* We must declare it before... */
struct included_files {
        unsigned char *filename;
        unsigned char *XBESectionName;
};

//#include "docs/included.inc"
// ysbox: should change that!
struct included_files included[] = {
		{"logo.gif", "LOGO" },
		{"license", "LICENSE" },
		{"about", "ABOUT" },
		{ NULL, NULL }};


/* returns 1 if one of included files requested; this file will be shown
 to the user */
static int check_included(unsigned char *name, struct connection *c, struct cache_entry *e)
{
#if XBOX_USE_SECTIONS
        int i;

        for(i=0;included[i].filename;i++)
                if(included[i].XBESectionName &&
                   !strcmp(name, included[i].filename) &&
				   XGetSectionHandle(included[i].XBESectionName) != INVALID_HANDLE_VALUE)
				{
                        /* Send included file to the user */
                        add_fragment(e, 0, XLoadSection(included[i].XBESectionName), XGetSectionSize(XGetSectionHandle(included[i].XBESectionName)));
                        truncate_entry(e, XGetSectionSize(XGetSectionHandle(included[i].XBESectionName)), 1);
                        c->cache->incomplete = 0;
                        setcstate(c, S_OKAY);

                        return 1;
                }
#endif
        return 0;
}

/* Get name of requested file */
static unsigned char *get_requested_name(unsigned char *url)
{
        if(strlen(url)<9)
                return NULL;

        return stracpy(url+9);
}




/* Create page with list of compiled-in features */
static unsigned char *get_features()
{
        unsigned char *text = init_str();
        int length = 0;
        int i;

		add_to_str(&text, &length,
                   "<h2>LinksBoks</h2>\n"
                   "<h3>Enabled optional features:</h3>\n");

#ifdef G
        {
                unsigned char *drivers = (unsigned char*)list_graphics_drivers();

                add_to_str(&text, &length, "<br>Graphics with drivers: ");
                add_to_str(&text, &length, drivers);
                add_to_str(&text, &length,"\n");
                mem_free(drivers);
        }
#endif
#ifdef JS
        add_to_str(&text, &length, "<br>Javascript engine\n");
#endif
#ifdef HAVE_SSL
        add_to_str(&text, &length, "<br>SSL support\n");
#endif
#ifdef HAVE_LUA
        add_to_str(&text, &length, "<br>LUA scripting\n");
#endif
#ifdef HAVE_FREETYPE
        add_to_str(&text, &length, "<br>FreeType font backend\n");
#endif
#ifdef XBOX_USE_XFONT
		add_to_str(&text, &length, "<br>Xbox XFONT font backend\n");
#endif
#ifdef XBOX_USE_FREETYPE
		add_to_str(&text, &length, "<br>Xbox FreeType font backend\n");
#endif
#ifdef HAVE_GLOBHIST
        add_to_str(&text, &length, "<br>Global history\n");
#endif
#ifdef FORM_SAVE
        add_to_str(&text, &length, "<br>Form saving and loading\n");
#endif
#ifdef HAVE_ZLIB_H
        add_to_str(&text, &length, "<br>Content-Encoding: gzip\n");
#endif
#ifdef HAVE_BZLIB_H
        add_to_str(&text, &length, "<br>Content-Encoding: bzip2\n");
#endif
#ifdef BACKTRACE
        add_to_str(&text, &length, "<br>Backtrace on segfault\n");
#endif

        return text;
}

#ifdef GLOBHIST
/* Create page with list of recently-visited pages */
static unsigned char *get_globhist()
{
        unsigned char *text = init_str();
        int length = 0;
        struct global_history_item *item;

        add_to_str(&text, &length,
                   "<h3>Your Global History:</h3>\n"
                   "<table border=1>"
                   "<tr><th>Title</th><th>Last visit</th></tr>\n");
        foreach(item, global_history.items){
                add_to_str(&text, &length,"<tr><td><a href=\"");
                add_to_str(&text, &length,item->url);
                add_to_str(&text, &length,"\">");
                add_to_str(&text, &length,
                           strlen(item->title)
                           ? item->title
                           : (unsigned char*)"Untitled");
                add_to_str(&text, &length,"</a></td><td>");
//                add_to_str(&text, &length,ctime(&item->last_visit));
                add_to_str(&text, &length,"</td></tr>\n");
        }
        add_to_str(&text, &length,
                   "</table>");

        return text;
}
#endif

/* Create page with list of current downloads */
static unsigned char *get_downloads()
{
        unsigned char *text = init_str();
        int length = 0;
        struct download *down;

        add_to_str(&text, &length, "<head><title>Downloads Manager</title><meta http-equiv=\"Refresh\" content=\"1\"></head>\n");
        add_to_str(&text, &length, "<h3>Your Downloads:</h3>");
        if(list_empty(downloads))
                add_to_str(&text, &length, "<h4>None yet...</h4>");
        else {
                add_to_str(&text, &length, "<table border=1><tr><th>Url</th><th>%</th><th>ETA</th><th>Transferred</th><th>Speed</th></tr>");
                foreach(down, downloads) {
                        struct status *stat = &down->stat;
                        add_to_str(&text, &length, "<tr><td align=left>");
                        add_to_str(&text, &length, down->url);
                        add_to_str(&text, &length, "</td>");
                        if (stat->state == S_TRANS && stat->prg->elapsed / 100) {
                                int pos  = stat->prg->pos;
                                int size = stat->prg->size;

                                /* Percents */
                                add_to_str(&text, &length, "<td align=right>");
                                if(size>0){
                                        add_num_to_str(&text, &length, (int)((longlong)100 * (longlong)pos / (longlong)size));
                                } else
                                        add_to_str(&text, &length, "??");
                                add_to_str(&text, &length, "</td>");

                                /* Estimated time */
                                add_to_str(&text, &length, "<td align=right>");
                                if (size >= 0 && stat->prg->loaded > 0)
                                        add_time_to_str(&text, &length, (stat->prg->size - stat->prg->pos) / ((longlong)stat->prg->loaded * 10 / (stat->prg->elapsed / 100)) * 1000);
                                else
                                        add_to_str(&text, &length, "??");
                                add_to_str(&text, &length, "</td>");

                                /* Transferred */
                                add_to_str(&text, &length, "<td align=right>");
                                add_xnum_to_str(&text, &length, pos);
                                if (stat->prg->size >= 0){
                                        add_to_str(&text, &length, "/");
                                        add_xnum_to_str(&text, &length, size);
                                }
                                add_to_str(&text, &length, "</td>");

                                /* Speed */
                                add_to_str(&text, &length, "<td>");
                                add_xnum_to_str(&text, &length, (longlong)stat->prg->loaded * 10 / (stat->prg->elapsed / 100));
                                add_to_str(&text, &length, "/s");
                                if (stat->prg->elapsed >= CURRENT_SPD_AFTER * SPD_DISP_TIME){
                                        add_to_str(&text, &length, " (");
                                        add_xnum_to_str(&text, &length, stat->prg->cur_loaded / (CURRENT_SPD_SEC * SPD_DISP_TIME / 1000));
                                        add_to_str(&text, &length, "/s now)");
                                }
                                add_to_str(&text, &length, "</td></tr>\n");

                        } else {
                                add_to_str(&text, &length, "<td colspan=4>");
                                add_to_str(&text, &length, get_err_msg(stat->state));
                                add_to_str(&text, &length, "</td></tr>\n");
                        }
                }
                add_to_str(&text, &length, "</table>\n");
        }

        return text;
}

/* Create page with list of currently cached urls */
static unsigned char *get_cache()
{
        unsigned char *text = init_str();
        int length = 0;
	struct cache_entry *ce, *cache;

        cache = (struct cache_entry *)cache_info(CI_LIST);

        add_to_str(&text, &length, "<head><title>Cache Manager</title><meta http-equiv=\"Refresh\" content=\"1\"></head>\n");
        add_to_str(&text, &length, "<h3>Your Cache:</h3>\n");
	foreach(ce, *cache) {
		add_to_str(&text, &length, "<a href=\"");
                add_to_str(&text, &length, ce->url);
		add_to_str(&text, &length, "\">");
                add_to_str(&text, &length, ce->url);
                add_to_str(&text, &length, "</a><br>\n");
        }

        return text;
}

/* Create page with list of current connections */
static unsigned char *get_connections()
{
        unsigned char *text = init_str();
        int length = 0;
	struct connection *conn, *conn_queue;

        conn_queue = (struct connection *)connect_info(CI_LIST);

        add_to_str(&text, &length, "<head><title>Connections Manager</title><meta http-equiv=\"Refresh\" content=\"1\"></head>\n");
        add_to_str(&text, &length, "<h3>Current connections:</h3>\n");
        foreach(conn, *conn_queue) {
                switch(conn->state){
                case(S_WAIT):
                        add_to_str(&text, &length, "WAIT");
                        break;
                case(S_DNS):
                        add_to_str(&text, &length, "DNS");
                        break;
                case(S_CONN):
                        add_to_str(&text, &length, "CONN");
                        break;
                case(S_SSL_NEG):
                        add_to_str(&text, &length, "SSL_NEG");
                        break;
                case(S_SENT):
                        add_to_str(&text, &length, "SENT");
                        break;
                case(S_LOGIN):
                        add_to_str(&text, &length, "LOGIN");
                        break;
                case(S_GETH):
                        add_to_str(&text, &length, "GETH");
                        break;
                case(S_PROC):
                        add_to_str(&text, &length, "PROC");
                        break;
                case(S_TRANS):
                        add_to_str(&text, &length, "TRANS");
                        break;
                default:
                        add_num_to_str(&text, &length, conn->state);
                }
		add_to_str(&text, &length, " ");
                add_to_str(&text, &length, conn->url);
                add_to_str(&text, &length, "<br>\n");
        }

        return text;
}


void internal_func(struct connection *c)
{
        unsigned char *name = get_requested_name(c->url);
        struct cache_entry *e;
        unsigned char *header;

        /* Very primitive heuristics to determine file type */
        if(strstr(name, ".jpeg"))
                header = stracpy("\r\nContent-Type: image/jpeg;\r\n");
        else if(strstr(name, ".gif"))
                header = stracpy("\r\nContent-Type: image/gif;\r\n");
        else
                header = stracpy("\r\nContent-Type: text/html; charset=utf-8\r\n");

        if (get_cache_entry(c->url, &e)) {
                setcstate(c, S_OUT_OF_MEM);
                goto finish;
        }

        if (e->head) mem_free(e->head);
	e->head = header;
	c->cache = e;

        if (!check_included(name, c, e)){
                unsigned char *text;

                /* Some autogenerated stuff... */
                if(!strcmp(name,"features")) {
                        text=get_features();
#ifdef GLOBHIST
                } else if(!strcmp(name,"history")) {
                        text=get_globhist();
#endif
                } else if(!strcmp(name,"downloads")) {
                        text=get_downloads();
                } else if(!strcmp(name,"cache")) {
                        text=get_cache();
                } else if(!strcmp(name,"connections")) {
                        text=get_connections();
                } else {
                        setcstate(c, S_BAD_URL);
                        goto finish;
                }

                add_fragment(e, 0, text, strlen(text));
                truncate_entry(e, strlen(text), 1);

                c->cache->incomplete = 0;
                setcstate(c, S_OKAY);

                if(text) mem_free(text);
        }

finish:
        mem_free(name);
        abort_connection(c);
}
