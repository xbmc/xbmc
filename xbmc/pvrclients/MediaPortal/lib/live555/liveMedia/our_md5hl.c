#define LENGTH 16
/* md5hl.c
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dkuug.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include "our_md5.h"
#include "NetCommon.h"

#ifndef BUFSIZ //pocket pc
#define BUFSIZ 255
#endif


char *
our_MD5End(MD5_CTX *ctx, char *buf)
{
    int i;
    unsigned char digest[LENGTH];
    static const char hex[]="0123456789abcdef";

    if (!buf)
        buf = (char*)malloc(2*LENGTH + 1);
    if (!buf)
	return 0;
    our_MD5Final(digest, ctx);
    for (i = 0; i < LENGTH; i++) {
	buf[i+i] = hex[digest[i] >> 4];
	buf[i+i+1] = hex[digest[i] & 0x0f];
    }
    buf[i+i] = '\0';
    return buf;
}

char *
our_MD5File(const char *filename, char *buf)
{
    unsigned char buffer[BUFSIZ];
    MD5_CTX ctx;
    int i;
	FILE* f;

    our_MD5Init(&ctx);
    f = fopen(filename, "r");
    if (f == NULL) return 0;
    while ((i = fread(buffer,1,sizeof buffer,f)) > 0) {
	ourMD5Update(&ctx,buffer,i);
    }
    fclose(f);
    if (i < 0) return 0;
    return our_MD5End(&ctx, buf);
}

char *
our_MD5Data (const unsigned char *data, unsigned int len, char *buf)
{
    MD5_CTX ctx;

    our_MD5Init(&ctx);
    ourMD5Update(&ctx,data,len);
    return our_MD5End(&ctx, buf);
}
