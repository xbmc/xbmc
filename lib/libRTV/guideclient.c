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
#include "guideclient.h"
#include "httpclient.h"

#include <string.h>
#include <stdio.h>

#define MIN(x,y) ((x)<(y)?(x):(y))

struct snapshot_data 
{
    int firsttime;
    int filesize;
    int bytes_read;
    char * timestamp;
    char * buf;
    u32 status;
};

static void get_snapshot_callback(unsigned char * buf, size_t len, void * vd)
{
    struct snapshot_data * data = vd;
    unsigned char * buf_data_start;
    unsigned long bytes_to_read;
    
    if (data->firsttime) {
        unsigned char * end, * equal, * cur;

        data->firsttime = 0;

        /* First line: error code */
        cur = buf;
        end = (unsigned char*)strchr((char*)cur, '\n');
        if (end) *end = '\0';
        data->status = strtoul((char*)cur, NULL, 16);
        if (!end) return;
        do {
            cur = end + 1;
            if (*cur == '#') {
                end = (unsigned char*)strchr((char*)cur, '\0');
                len -= (end - buf);
                buf_data_start = end;
                break;
            }
            end = (unsigned char*)strchr((char*)cur, '\n');
            if (!end) return;
            *end = '\0';
            equal = (unsigned char*)strchr((char*)cur, '=');
            if (!equal) return;
            if (strncmp((char*)cur, "guide_file_name=", (int)(equal-cur+1)) == 0) {
                data->timestamp = malloc(strlen((const char*)(equal+1))+1);
                strcpy((char*)data->timestamp, (const char*)(equal+1));
            } else if (strncmp((char*)cur, "FileLength=", (int)(equal-cur+1)) == 0) {
                data->filesize = strtoul((char*)equal+1, NULL, 0);
                data->buf = malloc(data->filesize);
            } /* also "RemoteFileName", but we don't expose it */
        } while (1);
    } else {
        buf_data_start = buf;
    }

    bytes_to_read = MIN(len, (unsigned)(data->filesize - data->bytes_read));
    memcpy(data->buf + data->bytes_read, buf_data_start, bytes_to_read);
    data->bytes_read += bytes_to_read;

    free(buf);
}

unsigned long guide_client_get_snapshot(unsigned char ** presult,
                                        unsigned char ** ptimestamp,
                                        unsigned long * psize,
                                        const char * address,
                                        const char * cur_timestamp)
{
    struct snapshot_data data;
    char url[512];
    struct hc * hc;
    
    memset(&data, 0, sizeof data);
    data.firsttime = 1;
    data.status    = -1;

    sprintf(url, "http://%s/http_replay_guide-get_snapshot?"
            "guide_file_name=%s&serial_no=RTV4080K0000000000",
            address,
            cur_timestamp);

    hc = hc_start_request(url);
    if (!hc) {
        perror("Error: guide_client_get_snapshot(): hc_start_request()");
        goto exit;
    }


    hc_send_request(hc);
    hc_read_pieces(hc, get_snapshot_callback, &data);

    hc_free(hc);

    *ptimestamp = (unsigned char*)data.timestamp;
    *presult    = (unsigned char*)data.buf;
    *psize      = data.filesize;

exit:
    return data.status;
}

