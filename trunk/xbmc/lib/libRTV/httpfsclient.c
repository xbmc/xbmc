#ifndef __GNUC__
#pragma code_seg( "RTV_TEXT" )
#pragma data_seg( "RTV_DATA" )
#pragma bss_seg( "RTV_BSS" )
#pragma const_seg( "RTV_RD" )
#endif

/*
 * Copyright (C) 2002 John Todd Larason <jtl@molehill.org>
 *
 * Parts based on ReplayPC 0.3 by Matthew T. Linehan and others
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

#include "httpfsclient.h"
#include "sleep.h"
#include "crypt.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int hfs_output_errors = 1;

#define ARGBUFSIZE 2048
static int make_httpfs_url(char * dst, size_t size,
                           const char * address, const char * command,
                           va_list args) 
{
    char * d, * tag, * value, *argp;
    int argno = 0;
    size_t l, argl;
    char argbuf[ARGBUFSIZE];          /* XXX bad, should fix this */

    (void) size;

    l = strlen(address) + strlen(command) + strlen("http:///httpfs-?");
    if (l >= size) {
        //if (hfs_output_errors)
            //fprintf(stderr, "Error: make_httpfs_url: address + command too long for buffer\n");
        return -1;
    }
    
    d = dst;
    d += sprintf(d, "http://%s/httpfs-%s?", address, command);

    argp = argbuf;
    argl = 0;
    while ((tag = va_arg(args, char *)) != NULL) {
        value = va_arg(args, char *);
        if (value == NULL) {
            continue;
        }
        if (argno)
            argl++;
        argl += strlen(tag)+1+strlen(value);
        if (argl >= sizeof(argbuf)) {
            //if (hfs_output_errors)
                //fprintf(stderr, "Error: make_httpfs_url: with arg %s, argbuf too small\n", tag);
            return -1;
        }
        if (argno)
            *argp++ = '&';
        argp += sprintf(argp, "%s=%s", tag, value);
        argno++;
    }
    
    if (replaytv_version.major >= 5 ||
        (replaytv_version.major == 4 && replaytv_version.minor >= 3)) {
        unsigned char ctext[ARGBUFSIZE+32] ;
        u32 ctextlen;
        unsigned int i;

        if (l + strlen("__Q_=") + 2*argl + 32 >= size) {
            //if (hfs_output_errors)
              //  fprintf(stderr, "Error: make_httpfs_url: encrypted args too large for buffer\n");
            return -1;
        }

        strcpy(d, "__Q_=");
        d += strlen(d);
        rtv_encrypt(argbuf, argl, ctext, sizeof ctext, &ctextlen, 1);
        for (i = 0; i < ctextlen; i++)
            d += sprintf(d, "%02x", ctext[i]);
    } else {
        if (l + argl >= size) {
            //if (hfs_output_errors)
                //fprintf(stderr, "Error: make_httpfs_url: args too large for buffer\n");
            return -1;
        }
        strcpy(d, argbuf);
    }
    
    return 0;
}

static int add_httpfs_headers(struct hc * hc)
{
    hc_add_req_header(hc, "Authorization",   "Basic Uk5TQmFzaWM6QTd4KjgtUXQ=" );
    hc_add_req_header(hc, "User-Agent",      "Replay-HTTPFS/1");
    hc_add_req_header(hc, "Accept-Encoding", "text/plain");

    return 0;
}

static struct hc * make_request(const char * address, const char * command,
                                va_list ap)
{
    struct hc * hc = NULL;
    char url[URLSIZE];
    int http_status;

    if (make_httpfs_url(url, sizeof url, address, command, ap) < 0)
        goto exit;

    hc = hc_start_request(url);
    if (!hc) {
        if (hfs_output_errors)
            perror("Error: make_request(): hc_start_request()"); 
        if (errno == 0) {
//            fprintf(stderr, "Check your clock.\n"
  //                  "The system clock must be within 40 seconds of the Replay's.");
        }
        goto exit;
    } 

    if (add_httpfs_headers(hc)) {
        goto exit;
    }

    hc_send_request(hc);

    http_status = hc_get_status(hc);
    if (http_status/100 != 2) {
        //if (hfs_output_errors) 
            //fprintf(stderr, "Error: http status %d\n", http_status);
        goto exit;
    }

    return hc;
    
exit:
    if (hc)
        hc_free(hc);
    return NULL;
}

    
unsigned long hfs_do_simple(char ** presult, const char * address,
                            const char * command, ...)
{
    va_list       ap;
    struct hc *   hc;
    char *        tmp, * e;
    int           http_status;
    unsigned long rtv_status;
    
    va_start(ap, command);
    hc = make_request(address, command, ap);
    va_end(ap);

    if (!hc)
        return -1;

    http_status = hc_get_status(hc);
    if (http_status/100 != 2) {
        //if (hfs_output_errors)
          //  fprintf(stderr, "Error: http status %d\n", http_status);
        hc_free(hc);
        return http_status;
    }

    tmp = hc_read_all(hc);
    hc_free(hc);

    e = strchr(tmp, '\n');
    if (e) {
        *presult = strdup(e+1);
        rtv_status = strtoul(tmp, NULL, 10);
        free(tmp);
        return rtv_status;
    } else if (http_status == 204) {
        *presult = NULL;
        free(tmp);
        return 0;
    } else {
        //if (hfs_output_errors)
          //  fprintf(stderr, "Error: end of httpfs status line not found\n");
        return -1;
    }
}

struct hfs_data
{
    void (*fn)(unsigned char *, size_t, void *);
    void * v;
    unsigned long status;
    u16 msec_delay;
    u8 firsttime;
	char * buffer;
	size_t bufLen;
};

/* XXX should this free the buf, or make a new one on the first block
 * and let our caller free them if it wants? */
static void hfs_callback(unsigned char * buf, size_t len, void * vd)
{
    struct hfs_data * data = vd;
    unsigned char * buf_data_start;
    
    if (data->firsttime) {
        unsigned char * e;

        data->firsttime = 0;

        /* First line: error code */
        e = strchr(buf, '\n');
        if (e)
            *e = '\0';
        data->status = strtoul(buf, NULL, 16);

        e++;
        len -= (e - buf);
        buf_data_start = e;
    } else {
        buf_data_start = buf;
    }

    data->fn(buf_data_start, len, data->v);
    free(buf);

    if (data->msec_delay)
        rtv_sleep(data->msec_delay);
}

unsigned long hfs_do_chunked(void (*fn)(unsigned char *, size_t, void *),
                             void *v,
                             const char * address,
                             u16 msec_delay,
                             const char * command,
                             ...)
{
    struct hfs_data data;
    struct hc *   hc;
    va_list ap;
    
    va_start(ap, command);
    hc = make_request(address, command, ap);
    va_end(ap);

    if (!hc)
        return -1;

    memset(&data, 0, sizeof data);
    data.fn         = fn;
    data.v          = v;
    data.firsttime  = 1;
    data.msec_delay = msec_delay;
    
    hc_read_pieces(hc, hfs_callback, &data);
    hc_free(hc);

    return data.status;
}

unsigned long hfs_do_post_simple(char ** presult, const char * address,
                                 int (*fn)(unsigned char *, size_t, void *),
                                 void *v,
                                 unsigned long size,
                                 const char * command, ...)
{
    char          buf[URLSIZE];
    va_list       ap;
    struct hc *   hc;
    char *        tmp, * e;
    int           http_status;
    unsigned long rtv_status;
    
    va_start(ap, command);
    if (make_httpfs_url(buf, sizeof buf, address, command, ap) < 0)
        return -1;
    va_end(ap);

    errno = 0;
    hc = hc_start_request(buf);
    if (!hc) {
        if (hfs_output_errors)
            perror("Error: hfs_do_simple(): hc_start_request()");
        if (errno == 0) {
            //fprintf(stderr, "Check your clock.\n"
            //        "The system clock must be within 40 seconds of the Replay's.");
        }
        return -1;
    } 
    sprintf(buf, "%lu", size);
    if (add_httpfs_headers(hc) != 0)
        return -1;
    
    hc_add_req_header(hc, "Content-Length",  buf);
    
    hc_post_request(hc, fn, v);

    http_status = hc_get_status(hc);
    if (http_status/100 != 2) {
        //if (hfs_output_errors)
          //  fprintf(stderr, "Error: http status %d\n", http_status);
        hc_free(hc);
        return http_status;
    }
    
    tmp = hc_read_all(hc);
    hc_free(hc);

    e = strchr(tmp, '\n');
    if (e) {
        *presult = strdup(e+1);
        rtv_status = strtoul(tmp, NULL, 10);
        free(tmp);
        return rtv_status;
    } else if (http_status == 204) {
        *presult = NULL;
        free(tmp);
        return 0;
    } else {
        //if (hfs_output_errors)
            //fprintf(stderr, "Error: end of httpfs status line not found\n");
        return -1;
    }
}


static void hfs_callback2(unsigned char * buf, size_t len, void * vd)
{
    struct hfs_data * data = vd;
    unsigned char * buf_data_start;
    
    if (data->firsttime) {
        unsigned char * e;

        data->firsttime = 0;

        /* First line: error code */
        e = strchr(buf, '\n');
        if (e)
            *e = '\0';
        data->status = strtoul(buf, NULL, 16);

        e++;
        len -= (e - buf);
        buf_data_start = e;
    } else {
        buf_data_start = buf;
    }

    /* append to buffer, this may be called multiple times */
    memcpy(data->buffer+data->bufLen, buf_data_start, len);
    data->bufLen += len;
    free(buf);
}

struct hc * hfs_do_chunked_open(const char * address, const char * command, ...)
{
    struct hc *   hc;
    va_list ap;
    
    va_start(ap, command);
    hc = make_request(address, command, ap);
    va_end(ap);
	
    return hc;
}

size_t hfs_do_chunked_read(struct hc * hc, char * buf, size_t pos, size_t len)
{
    struct hfs_data data;

    memset(&data, 0, sizeof data);
    data.buffer          = buf;
	if (!pos)
		data.firsttime  = 1;
    
    hc_read_pieces_len(hc, hfs_callback2, &data, len);

    return data.bufLen;
}

void hfs_do_chunked_close(struct hc * hc)
{
    hc_free(hc);
}

