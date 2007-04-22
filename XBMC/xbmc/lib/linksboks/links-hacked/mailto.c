/* mailto.c
 * mailto:// processing
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

void prog_func(struct terminal *term, unsigned char *prog, unsigned char *param, unsigned char *name)
{
	unsigned char *cmd;
	if (!prog || !*prog) {
		msg_box(term, NULL, TXT(T_NO_PROGRAM), AL_CENTER | AL_EXTD_TEXT, TXT(T_NO_PROGRAM_SPECIFIED_FOR), " ", name, ".", NULL, NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
		return;
	}
	if ((cmd = subst_file(prog, param))) {
		exec_on_terminal(term, cmd, "", 1);
		mem_free(cmd);
	}
}

void mailto_func(struct session *ses, unsigned char *url)
{
	unsigned char *user, *host, *m;
	int f = 1;
	if (!(user = get_user_name(url))) goto fail;
	if (!(host = get_host_name(url))) goto fail1;
	if (!(m = mem_alloc(strlen(user) + strlen(host) + 2))) goto fail2;
	f = 0;
	strcpy(m, user);
	strcat(m, "@");
	strcat(m, host);
	check_shell_security(&m);
        prog_func(ses->term, options_get("network_program_mailto"), m, TXT(T_MAIL));
        mem_free(m);
	fail2:
	mem_free(host);
	fail1:
	mem_free(user);
	fail:
	if (f) msg_box(ses->term, NULL, TXT(T_BAD_URL_SYNTAX), AL_CENTER, TXT(T_BAD_MAILTO_URL), NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
}

void tn_func(struct session *ses, unsigned char *url, unsigned char *prog, unsigned char *t1, unsigned char *t2)
{
	unsigned char *m;
	unsigned char *h, *p;
	int hl, pl;
	unsigned char *hh, *pp;
	int f = 1;
	if (parse_url(url, NULL, NULL, NULL, NULL, NULL, &h, &hl, &p, &pl, NULL, NULL, NULL) || !hl) goto fail;
	if (!(hh = memacpy(h, hl))) goto fail;
	if (pl && !(pp = memacpy(p, pl))) goto fail1;
	check_shell_security(&hh);
	if (pl) check_shell_security(&pp);
	if (!(m = mem_alloc(strlen(hh) + (pl ? strlen(pp) : 0) + 2))) goto fail2;
	f = 0;
	strcpy(m, hh);
	if (pl) {
		strcat(m, " ");
		strcat(m, pp);
		m[hl + 1 + pl] = 0;
	}
	prog_func(ses->term, prog, m, t1);
	mem_free(m);
	fail2:
	if (pl) mem_free(pp);
	fail1:
	mem_free(hh);
	fail:
	if (f) msg_box(ses->term, NULL, TXT(T_BAD_URL_SYNTAX), AL_CENTER, t2, NULL, 1, TXT(T_CANCEL), NULL, B_ENTER | B_ESC);
}

void telnet_func(struct session *ses, unsigned char *url)
{
	tn_func(ses, url, options_get("network_program_telnet"), TXT(T_TELNET), TXT(T_BAD_TELNET_URL));
}

void tn3270_func(struct session *ses, unsigned char *url)
{
	tn_func(ses, url, options_get("network_program_tn3270"), TXT(T_TN3270), TXT(T_BAD_TN3270_URL));
}
