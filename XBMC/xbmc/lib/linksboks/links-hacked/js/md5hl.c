/* md5hl.c
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dkuug.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $Id: md5hl.c,v 1.3 2003/05/11 22:28:14 karpov Exp $
 *
 */

/* This file is a part of the Links project, released under GPL.
 */

/*
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
*/

#include "../links.h"
#include "md5.h"

char *
MD5End(JS_MD5_CTX *ctx, char *buf)
{
    int i;
    unsigned char digest[MD5_HASHBYTES];
    static const char hex[]="0123456789abcdef";

    if (!buf)
        buf = mem_alloc(33);
    if (!buf)
	return 0;
    MD5Final(digest,ctx);
    for (i=0;i<MD5_HASHBYTES;i++) {
	buf[i+i] = hex[digest[i] >> 4];
	buf[i+i+1] = hex[digest[i] & 0x0f];
    }
    buf[i+i] = '\0';
    return buf;
}

char *
MD5File (const char *filename, char *buf)
{
    unsigned char buffer[BUFSIZ];
    JS_MD5_CTX ctx;
    int f,i,j;

    MD5Init(&ctx);
    f = open(filename,O_RDONLY);
    if (f < 0) return 0;
    while ((i = read(f,buffer,sizeof buffer)) > 0) {
	MD5Update(&ctx,buffer,i);
    }
    j = errno;
    close(f);
    errno = j;
    if (i < 0) return 0;
    return MD5End(&ctx, buf);
}

char *
MD5Data (const unsigned char *data, unsigned int len, char *buf)
{
    JS_MD5_CTX ctx;

    MD5Init(&ctx);
    MD5Update(&ctx,data,len);
    return MD5End(&ctx, buf);
}

