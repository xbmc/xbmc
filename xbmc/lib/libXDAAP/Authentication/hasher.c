// Place the code and data below here into the LIBXDAAP section.
#ifndef __GNUC__
#pragma code_seg( "LIBXDAAP_TEXT" )
#pragma data_seg( "LIBXDAAP_DATA" )
#pragma bss_seg( "LIBXDAAP_BSS" )
#pragma const_seg( "LIBXDAAP_RD" )
#endif

/* Copyright (c) 2004 David Hammerton
 * david@crazney.net
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <string.h>
#include "md5.h"

static int staticHashDone = 0;
static char staticHash_42[256*65] = {0};
static char staticHash_45[256*65] = {0};

static const char hexchars[] = "0123456789ABCDEF";
static const char appleCopyright[] = "Copyright 2003 Apple Computer, Inc.";

static void DigestToString(const unsigned char *digest, char *string)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        unsigned char tmp = digest[i];
        string[i*2+1] = hexchars[tmp & 0x0f];
        string[i*2] = hexchars[(tmp >> 4) & 0x0f];
    }
}

static void GenerateStatic_42()
{
    MD5_CTX ctx;
    unsigned char *p = (unsigned char *) staticHash_42;
    int i;
    unsigned char buf[16];

    for (i = 0; i < 256; i++)
    {
        OpenDaap_MD5Init(&ctx, 0);

#define MD5_STRUPDATE(str) OpenDaap_MD5Update(&ctx, (const unsigned char *)str, strlen(str))

        if ((i & 0x80) != 0)
            MD5_STRUPDATE("Accept-Language");
        else
            MD5_STRUPDATE("user-agent");

        if ((i & 0x40) != 0)
            MD5_STRUPDATE("max-age");
        else
            MD5_STRUPDATE("Authorization");

        if ((i & 0x20) != 0)
            MD5_STRUPDATE("Client-DAAP-Version");
        else
            MD5_STRUPDATE("Accept-Encoding");

        if ((i & 0x10) != 0)
            MD5_STRUPDATE("daap.protocolversion");
        else
            MD5_STRUPDATE("daap.songartist");

        if ((i & 0x08) != 0)
            MD5_STRUPDATE("daap.songcomposer");
        else
            MD5_STRUPDATE("daap.songdatemodified");

        if ((i & 0x04) != 0)
            MD5_STRUPDATE("daap.songdiscnumber");
        else
            MD5_STRUPDATE("daap.songdisabled");

        if ((i & 0x02) != 0)
            MD5_STRUPDATE("playlist-item-spec");
        else
            MD5_STRUPDATE("revision-number");

        if ((i & 0x01) != 0)
            MD5_STRUPDATE("session-id");
        else
            MD5_STRUPDATE("content-codes");
#undef MD5_STRUPDATE

        OpenDaap_MD5Final(&ctx, buf);
        DigestToString((unsigned char *) buf, (char *) p);
        p += 65;
    }
}

static void GenerateStatic_45()
{
    MD5_CTX ctx;
    unsigned char *p = (unsigned char *) staticHash_45;
    int i;
    unsigned char buf[16];

    for (i = 0; i < 256; i++)
    {
        OpenDaap_MD5Init(&ctx, 1);

#define MD5_STRUPDATE(str) OpenDaap_MD5Update(&ctx, (const unsigned char *)str, strlen(str))

        if ((i & 0x40) != 0)
            MD5_STRUPDATE("eqwsdxcqwesdc");
        else
            MD5_STRUPDATE("op[;lm,piojkmn");

        if ((i & 0x20) != 0)
            MD5_STRUPDATE("876trfvb 34rtgbvc");
        else
            MD5_STRUPDATE("=-0ol.,m3ewrdfv");

        if ((i & 0x10) != 0)
            MD5_STRUPDATE("87654323e4rgbv ");
        else
            MD5_STRUPDATE("1535753690868867974342659792");

        if ((i & 0x08) != 0)
            MD5_STRUPDATE("Song Name");
        else
            MD5_STRUPDATE("DAAP-CLIENT-ID:");

        if ((i & 0x04) != 0)
            MD5_STRUPDATE("111222333444555");
        else
            MD5_STRUPDATE("4089961010");

        if ((i & 0x02) != 0)
            MD5_STRUPDATE("playlist-item-spec");
        else
            MD5_STRUPDATE("revision-number");

        if ((i & 0x01) != 0)
            MD5_STRUPDATE("session-id");
        else
            MD5_STRUPDATE("content-codes");

        if ((i & 0x80) != 0)
            MD5_STRUPDATE("IUYHGFDCXWEDFGHN");
        else
            MD5_STRUPDATE("iuytgfdxwerfghjm");

#undef MD5_STRUPDATE

        OpenDaap_MD5Final(&ctx, buf);
        DigestToString((unsigned char *) buf, (char *) p);
        p += 65;
    }
}

void GenerateHash(short version_major,
                  const char *url, char hashSelect,
                  char *outhash,
                  int request_id)
{
    unsigned char buf[16];
    MD5_CTX ctx;

    char *hashTable = (version_major == 3) ?
                      staticHash_45 : staticHash_42;

    if (!staticHashDone)
    {
        GenerateStatic_42();
        GenerateStatic_45();
        staticHashDone = 1;
    }

    OpenDaap_MD5Init(&ctx, (version_major == 3) ? 1 : 0);

    OpenDaap_MD5Update(&ctx, (const unsigned char *) url, strlen((char *) url));
    OpenDaap_MD5Update(&ctx, (const unsigned char *) appleCopyright, strlen(appleCopyright));

    OpenDaap_MD5Update(&ctx, (const unsigned char *) &hashTable[hashSelect * 65], 32);

    if (request_id && version_major == 3)
    {
        char scribble[20];
        sprintf(scribble, "%u", request_id);
        OpenDaap_MD5Update(&ctx, (const unsigned char *) scribble, strlen(scribble));
    }

    OpenDaap_MD5Final(&ctx, buf);
    DigestToString((const unsigned char *) buf, (char *) outhash);
}

