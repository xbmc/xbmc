/* https.c
 * HTTPS protocol client implementation
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

#ifdef HAVE_SSL

#ifdef __XBOX__
#define PATH_MAX	255
#endif

SSL_CTX *context = 0;

SSL *getSSL(void)
{
	char f_randfile[PATH_MAX];

	if (!context) {
#ifndef __XBOX__
		if (RAND_egd(RAND_file_name(f_randfile, sizeof(f_randfile)))<0){
			/* Not an EGD, so read and write to it */
			if (RAND_load_file(f_randfile, -1))
			       RAND_write_file(f_randfile);
		}
#endif
		SSLeay_add_ssl_algorithms();
		context = SSL_CTX_new(SSLv23_client_method());
		SSL_CTX_set_options(context, SSL_OP_ALL);
//		SSL_CTX_set_default_verify_paths(context);
	}
	return (SSL_new(context));
}
void ssl_finish(void)
{
	if (context) SSL_CTX_free(context);
}

void https_func(struct connection *c)
{
	c->ssl = (void *)-1;
	http_func(c);
}

#else

void https_func(struct connection *c)
{
	setcstate(c, S_NO_SSL);
	abort_connection(c);
}

#endif
