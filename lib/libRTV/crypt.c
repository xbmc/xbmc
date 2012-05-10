#ifndef __GNUC__
#pragma code_seg( "RTV_TEXT" )
#pragma data_seg( "RTV_DATA" )
#pragma bss_seg( "RTV_BSS" )
#pragma const_seg( "RTV_RD" )
#endif

/*
 * Copyright (C) 2002 John Todd Larason <jtl@molehill.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 */

#include "crypt.h"
#include "md5.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static void checksum(unsigned char * dest, unsigned const char * src, u32 len,
                     int checksum_num)
{
    
    static unsigned char extradata[][64] = {{
        0x41,0x47,0xc8,0x09, 0xba,0x3c,0x99,0x6a,
        0xda,0x09,0x9a,0x0f, 0xc0,0xd3,0x47,0xca,
        0xd1,0x95,0x81,0x19, 0xab,0x17,0xc6,0x5f,
        0xad,0xea,0xe5,0x75, 0x9c,0x49,0x18,0xa5,
        0xdf,0x35,0x46,0x5b, 0x78,0x0e,0xcb,0xc7,
        0x8c,0x3e,0xf4,0x90, 0xa2,0xb7,0x8e,0x00,
        0x53,0x8d,0x4c,0xab, 0x13,0xa5,0x16,0x00,
        0xff,0xb8,0x4b,0x20, 0x29,0x22,0x9d,0xee,
    }, {
        0xda,0x76,0x5c,0xd4, 0x34,0xc3,0xd7,0x2c,
        0xac,0x40,0xb8,0xd8, 0x59,0xbc,0x59,0x34, 
        0xaa,0xbf,0x89,0xbd, 0x85,0xe8,0x40,0x27,
        0x78,0x2b,0x18,0x6e, 0xa6,0x6e,0x5a,0xc6, 
        0xda,0xe3,0x86,0x84, 0x40,0x14,0x2a,0x23,
        0x4f,0x5d,0x38,0x5e, 0x7f,0xd9,0x73,0x7d, 
        0xe4,0x80,0x3d,0x21, 0x28,0x41,0xf1,0xb2,
        0x96,0x43,0x2b,0xcc, 0x0c,0x9d,0x26,0xb9,
    }};

	rtv_md5_context c;
	rtv_md5_starts(&c);
    rtv_md5_update(&c, src, len);
    rtv_md5_update(&c, extradata[checksum_num], sizeof extradata[checksum_num]);
    rtv_md5_finish(&c, dest);

}

static u32 cryptblock(u32 k, char * buf, u32 size)
{
    u32 i;

    for (i = 0; i < size; i++) {
        k = k * 0xb8f7 + 0x15bb9;
        buf[i] ^= k;
    }
    return k;
}

/* cyphertext bytes:
 *
 * 2, 4, 1, 7 -- key material block 1
 * 0, 3, 5, 6 -- unknown/unused
 * 8..23      -- checksum(24..end)
 * 24..27     -- crypt(0x42ffdfa9)
 * 28..31     -- crypt(time_t)
 * 32..end    -- crypt(text)
 *
 * it looks like you should be able to decrypt 24..end in one step, after
 * getting the key material, if you don't want to do the sanity check first
 * and don't mind it all in one buffer
 */
 
int rtv_decrypt(const char * cyphertext, u32 cyphertext_len,
                char * plainbuf, u32 plainbuf_len,
                u32 * p_time, u32 * p_plain_len,
                int checksum_num)
{
    unsigned char key_buf[4];
    unsigned char sanity_buf[4];
    unsigned char time_buf[4];
    unsigned char csum_buf[16];
    unsigned char * p;
    u32 key;
    u32 sanity;
#if VERBOSE_OBFUSC
    unsigned char obfusc_buf[4];
    u32 obfusc;
#endif

    if (plainbuf_len < cyphertext_len - 32)
        return -1;

    /* unshuffle the key and unxor it */
    key_buf[0] = cyphertext[2];
    key_buf[1] = cyphertext[4];
    key_buf[2] = cyphertext[1];
    key_buf[3] = cyphertext[7];
    p = key_buf;
    key = rtv_to_u32(&p) ^ 0xcb0baf47;

#if VERBOSE_OBFUSC
    obfusc_buf[0] = cyphertext[0];
    obfusc_buf[1] = cyphertext[3];
    obfusc_buf[2] = cyphertext[5];
    obfusc_buf[3] = cyphertext[6];
    p = key_buf;
    obfusc = rtv_to_u32(&p);
    //fprintf(stderr, "Key: %ld (0x%lx)\n", (unsigned long)key, (unsigned long)key);
    //fprintf(stderr, "Obfusc: %ld (0x%lx)\n", (unsigned long)obfusc, (unsigned long)obfusc);
#endif

    /* check the sanity field */
    memcpy(sanity_buf, cyphertext + 24, 4);
    key = cryptblock(key, (char*)sanity_buf, 4);
    p = sanity_buf;
    sanity = rtv_to_u32(&p);
    if (sanity != 0x42ffdfa9)
        return -1;

    /* decrypt the time field */
    memcpy(time_buf, cyphertext + 28, 4);
    key = cryptblock(key, (char*)time_buf, 4);

    /* decrypt the actual text */
    memcpy(plainbuf, cyphertext + 32, cyphertext_len - 32);
    cryptblock(key, (char*)plainbuf, cyphertext_len - 32);

    /* check the checksum */
    checksum(csum_buf, (unsigned const char*)(cyphertext + 24), cyphertext_len - 24, checksum_num);
    if (memcmp(csum_buf, cyphertext + 8, 16) != 0)
        return -2;

    if (p_plain_len) {
        *p_plain_len = cyphertext_len - 32;
    }
    if (p_time) {
        p = time_buf;
        *p_time = rtv_to_u32(&p);
#if VERBOSE_OBFUSC
//	fprintf(stderr, "Time: %ld (0x%lx)\n", (unsigned long)*p_time, (unsigned long)*p_time);
#endif
    }
    return 0;
}

int rtv_encrypt(const char * plaintext, u32 plaintext_len,
                char * cyphertext, u32 buffer_len, u32 * cyphertext_len,
                int checksum_num)
{
    u32 key;
    u32 t;
    u32 obfusc;
    unsigned char key_buf[4];
    unsigned char obfusc_buf[4];
    unsigned char * p;
    
    if (buffer_len < plaintext_len + 32)
        return -1;

    /* make up a key and obfuscatory material; get the time */
    key    = rand();
    obfusc = rand();
    t      = (u32)time(NULL);

    p = NULL;//getenv("TIMEOFF");
    if (p)
        t += atoi((char*)p);

    /* encrypt the key */
    p = key_buf;
    rtv_from_u32(&p, key ^ 0xcb0baf47);

    p = obfusc_buf;
    rtv_from_u32(&p, obfusc);

    /* scramble the key and obfusc material */
    cyphertext[0]  = obfusc_buf[3];
    cyphertext[1]  = key_buf[2];
    cyphertext[2]  = key_buf[0];
    cyphertext[3]  = obfusc_buf[2];
    cyphertext[4]  = key_buf[1];
    cyphertext[5]  = obfusc_buf[1];
    cyphertext[6]  = obfusc_buf[0];
    cyphertext[7]  = key_buf[3];

    /* store the sanity check & time */
    p = (unsigned char*)(cyphertext + 24);
    rtv_from_u32(&p, 0x42ffdfa9);
    rtv_from_u32(&p, t);

    /* copy the plaintext */
    memcpy(p, plaintext, plaintext_len);

    /* encrypt the whole thing */
    cryptblock(key, cyphertext+24, plaintext_len+8);

    /* fill in the checksum */
    checksum((unsigned char*)(cyphertext + 8), (unsigned const char*)(cyphertext + 24), plaintext_len + 8, checksum_num);

    /* and we're done */
    *cyphertext_len = plaintext_len + 32;
    return 0;
}
