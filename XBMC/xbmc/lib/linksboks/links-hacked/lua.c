#include "links.h"

#ifdef HAVE_LUA

#include <setjmp.h>

lua_State *lua_state;

static struct session *ses;
static struct terminal *errterm;
static jmp_buf errjmp;

#define L	lua_state
#define LS	lua_State *S

static int l_alert(LS)
{
	alert_lua_error((unsigned char *) lua_tostring(S, -1));
	return 0;
}

static int l_current_url(LS)
{
	char *url = ses ? cur_loc(ses)->url : NULL;
	if (!url) lua_pushnil(S);
	else lua_pushstring(S, url);
	return 1;
}

static int l_current_link(LS)
{
	struct f_data_c *fd = current_frame(ses);
	if (fd && fd->vs->current_link != -1) {
		struct link *l = &fd->f_data->links[fd->vs->current_link];
		if (l->type == L_LINK) {
			lua_pushstring(S, l->where);
			return 1;
		}
	}
	lua_pushnil(S);
	return 1;
}

static int l_current_document(LS)
{
	unsigned char *url;
	struct cache_entry *ce;
	struct fragment *f;
	if (!ses || !(url = cur_loc(ses)->url) || find_in_cache(url, &ce) || !(f = ce->frag.next))
		lua_pushnil(S);
	else
		lua_pushlstring(S, f->data, f->length);
	return 1;
}

/* This function is mostly copied from `dump_to_file'.  */
static int l_current_document_formatted(LS)
{
	extern unsigned char frame_dumb[];
	int width, old_width;
	struct f_data_c *f;
	struct f_data *fd;
	int x, y;
	unsigned char *buf;
	int l = 0;

	if (lua_gettop(S) == 0) width = -1;
	else if (!lua_isnumber(S, -1)) goto err;
	else if ((width = lua_tonumber(S, -1)) <= 0) goto err;

	if (!ses || !(f = current_frame(ses))) goto err;
	if (width > 0) {
		old_width = ses->term->x, ses->term->x = width;
		html_interpret(ses->screen);
	}
	fd = f->f_data;
	buf = init_str();
	for (y = 0; y < fd->y; y++) for (x = 0; x <= fd->data[y].l; x++) {
		int c;
		if (x == fd->data[y].l) c = '\n';
		else {
			if (((c = fd->data[y].d[x]) & 0xff) == 1) c += ' ' - 1;
			if ((c >> 15) && (c & 0xff) >= 176 && (c & 0xff) < 224) c = frame_dumb[(c & 0xff) - 176];
		}
		add_chr_to_str(&buf, &l, c);
	}
	lua_pushlstring(S, buf, l);
	mem_free(buf);
	if (width > 0) {
		ses->term->x = old_width;
		html_interpret(ses->screen);
	}
	return 1;

	err:
	lua_pushnil(S);
	return 1;
}

static int l_pipe_read(LS)
{
	FILE *fp;
	unsigned char *s = NULL;
	int len = 0;
	if (!lua_isstring(S, -1) || !(fp = popen(lua_tostring(S, -1), "r"))) {
		lua_pushnil(S); return 1;
	}
	while (!feof(fp)) {
		unsigned char buf[1024];
		int l = fread(buf, 1, sizeof buf, fp);
		s = (!s) ? s = mem_alloc(l) : mem_realloc(s, len+l);
		memcpy(s+len, buf, l);
		len += l;
	}
	pclose(fp);
	lua_pushlstring(S, s, len);
	mem_free(s);
	return 1;
}

static int l_enable_systems_functions(LS)
{
	lua_iolibopen(S);
	lua_register(S, "pipe_read", l_pipe_read);
	return 0;
}

static void do_hooks_file(LS)
{
	int oldtop = lua_gettop(S);
	unsigned char *file = stracpy(links_home);
	add_to_strn(&file, "hooks.lua");
	lua_dofile(S, file);
	mem_free(file);
	lua_settop(S, oldtop);
}

void init_lua()
{
	L = lua_open(0);
	lua_baselibopen(L);
	lua_strlibopen(L);
	lua_register(L, LUA_ALERT, l_alert);
	lua_register(L, "current_url", l_current_url);
	lua_register(L, "current_link", l_current_link);
	lua_register(L, "current_document", l_current_document);
	lua_register(L, "current_document_formatted", l_current_document_formatted);
	lua_register(L, "enable_systems_functions", l_enable_systems_functions);
	do_hooks_file(L);
}

void alert_lua_error(unsigned char *msg)
{
	if (errterm) msg_box(errterm, NULL, TXT(T_LUA_ERROR), AL_LEFT, msg, NULL, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
	else {
		fprintf(stderr, "Lua Error: %s\n", msg);
		sleep(3);
	}
}

static void handle_sigint(void *data)
{
	finish_lua();
	siglongjmp(errjmp, -1);
}

int prepare_lua(struct session *_ses)
{
	ses = _ses;
	errterm = ses ? ses->term : NULL;
	/* XXX this uses the wrong term, I think */
	install_signal_handler(SIGINT, (void (*)(void *))handle_sigint, NULL, 1);
	return sigsetjmp(errjmp, 1);
}

void sig_ctrl_c(struct terminal *t);

void finish_lua()
{
	/* XXX should save previous handler instead of assuming this one */
	install_signal_handler(SIGINT, (void (*)(void *))sig_ctrl_c, errterm, 0);
}

static void lua_console_eval(struct session *ses)
{
	const unsigned char *expr;
	int oldtop;
	if (!(expr = lua_tostring(L, -1))) {
		alert_lua_error("bad argument for eval"); return;
	}
	oldtop = lua_gettop(L);
	if (!prepare_lua(ses)) {
		lua_dostring(L, expr);
		lua_settop(L, oldtop);
		finish_lua();
	}
}

static void lua_console_run(struct session *ses)
{
	unsigned char *cmd;
	if (!(cmd = (unsigned char *)lua_tostring(L, -1)))
		alert_lua_error("bad argument for run");
	else
		exec_on_terminal(ses->term, cmd, "", 1);
}

static void lua_console_goto_url(struct session *ses)
{
	unsigned char *url;
	if (!(url = (unsigned char *)lua_tostring(L, -1)))
		alert_lua_error("bad argument for goto_url");
	else
		goto_url(ses, url);
}

void lua_console(struct session *ses, unsigned char *expr)
{
	int err;
	const unsigned char *act;

	lua_getglobal(L, "lua_console_hook");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_console_eval(ses);
		return;
	}
	lua_pushstring(L, expr);
	if (prepare_lua(ses)) return;
	err = lua_call(L, 1, 2);
	finish_lua();
	if (err) return;
	
	if ((act = lua_tostring(L, -2))) {
		if (!strcmp(act, "eval")) lua_console_eval(ses);
		else if (!strcmp(act, "run")) lua_console_run(ses);
		else if (!strcmp(act, "goto_url")) lua_console_goto_url(ses);
		else alert_lua_error("unrecognised return value from lua_console_hook");
	}
	else if (!lua_isnil(L, -2)) {
		alert_lua_error("bad return type from lua_console_hook");
	}

	lua_pop(L, 2);
}

#endif
