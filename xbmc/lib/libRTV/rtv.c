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

#include "rtv.h"

#include <string.h>
#include <stdlib.h>

struct replaytv_version replaytv_version = {4, 3, 0, 280};

u8 rtv_to_u8(unsigned char ** pp) 
{
    unsigned char * p;
    u8 r;

    p = *pp;
    r = *p++;
    *pp = p;

    return r;
}

u16 rtv_to_u16(unsigned char ** pp)
{
    u16 r;
    r  = rtv_to_u8(pp) << 8;
    r |= rtv_to_u8(pp);
    return r;
}

u32 rtv_to_u32(unsigned char ** pp)
{
    u32 r;
    r  = rtv_to_u16(pp) << 16;
    r |= rtv_to_u16(pp);
    return r;
}

u64 rtv_to_u64(unsigned char ** pp)
{
    u64 r;

    r  = (u64)(rtv_to_u32(pp)) << 32;
    r |= (u64)(rtv_to_u32(pp));
    return r;
}

void rtv_to_buf_len(unsigned char ** pp, unsigned char * b, size_t l)
{
    unsigned char * p;
    p = *pp;
    memcpy(b, p, l);
    p += l;
    *pp = p;
}

void rtv_from_u8(unsigned char ** pp, u8 v)
{
    unsigned char * p;

    p = *pp;
    *p++ = v;
    *pp = p;
}
        
void rtv_from_u16(unsigned char ** pp, u16 v)
{
    rtv_from_u8(pp, (u8)((v & 0xff00) >> 8));
    rtv_from_u8(pp, (u8)((v & 0x00ff)     ));
}

void rtv_from_u32(unsigned char ** pp, u32 v)
{
    rtv_from_u16(pp, (u16)((v & 0xffff0000) >> 16));
    rtv_from_u16(pp, (u16)((v & 0x0000ffff)      ));
}

void rtv_from_u64(unsigned char ** pp, u64 v)
{
    rtv_from_u32(pp, (u32)((v & 0xffffffff00000000LL) >> 32));
    rtv_from_u32(pp, (u32)((v & 0x00000000ffffffffLL)      ));
}

void rtv_from_buf_len(unsigned char ** pp, unsigned char * b, size_t l)
{
    unsigned char * p;
    p = *pp;
    memcpy(p, b, l);
    p += l;
    *pp = p;
}

/* supports formats:
 * 520411140
 * 4.1
 * 4.1(140)
 * 4.1 (140)
 * 4.1.1
 * 4.1.1.140
 * 4.1.1(140)
 * 4.1.1 (140)
 */
extern int rtv_set_replaytv_version(char * version)
{
    if (strncmp(version, "520", 3) == 0 &&
        strlen(version) == 9) {
        replaytv_version.major = version[3] - '0';
        replaytv_version.minor = version[4] - '0';
        replaytv_version.patch = version[5] - '0';
        replaytv_version.build = atoi(version+6);
        return 0;
    }
    replaytv_version.major = strtoul(version, &version, 10);
    if (version == NULL || *version != '.')
        return -1;              /* need at least major&minor */
    version++;
    replaytv_version.minor = strtoul(version, &version, 10);
    switch (*version) {
        case '\0':              /* '4.1' style */
            replaytv_version.patch = 0;
            replaytv_version.build = 0;
            return 0;
        case '.':               /* '4.1.1'... style */
            version++;
            replaytv_version.patch = strtoul(version, &version, 10);
            break;
        case ' ':               /* '4.1 (140)' */
        case '(':               /* '4.1(140)' */
            replaytv_version.patch = 0;
            break;
        default:
            return -1;
    }
    if (*version == '\0') {     /* '4.1.1' */
        replaytv_version.build = 0;
        return 0;
    }
    switch (*version) {
        case '.':               /* '4.1.1.140' */
            version++;
            break;
        case ' ':               /* '4.1 (140)' or '4.1.1 (140)' */
            version++;
            if (*version != '(')
                return -1;
            version++;
            break;
        case '(':               /* '4.1(140)' or '4.1.1(140)' */
            version++;
            break;
        default:
            return -1;
    }
    replaytv_version.build = strtoul(version, &version, 10);
    if (*version == '\0')
        return 0;
    if (*version == ')')
        return 0;
    return -1;
}

int rtv_split_lines(char * src, char *** pdst)
{
    int num_lines, i;
    char * p;
    char ** dst;

    num_lines = 0;
    p = src;
    while (p) {
        p = strchr(p, '\n');
        if (p) {
            p++;
            num_lines++;
        }
    }

    dst = calloc(num_lines, sizeof(char *));
    dst[0] = src;
    p = src;
    i = 1;
    while (p) {
        p = strchr(p, '\n');
        if (p) {
            *p = '\0';
            p++;
            if (*p) {
                dst[i] = p;
                i++;
            }
        }
    }

    *pdst = dst;

    return num_lines;
}

void rtv_free_lines(int num_lines, char ** lines) 
{
    (void)num_lines;            /* not used with the above implementation */
    
    free(lines);
}
