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

#include "httpclient.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAX_CHUNK 32768

struct hc_headers 
{
    char * tag;
    char * value;
    struct hc_headers * next;
};

struct hc 
{
    struct nc * nc;
    char * hostname;
    short port;
    char * localpart;
    char * status;
    struct hc_headers * req_headers;
    struct hc_headers * rsp_headers;
	size_t curr_chunk_len;
};

static void hc_h_free(struct hc_headers * hh)
{
    free(hh->tag);
    free(hh->value);
    free(hh);
}

static struct hc_headers * hc_h_make(const char * tag, const char * value)
{
    struct hc_headers * h;
    
    h = malloc(sizeof *h);
    if (!h)
        goto error;
    memset(h, 0, sizeof *h);
    
    h->tag = strdup(tag);
    if (!h->tag)
        goto error;
    
    h->value = strdup(value);
    if (!h->value)
        goto error;
    
    return h;
error:
    if (h)
        hc_h_free(h);
    return NULL;
}

void hc_free(struct hc * hc)
{
    struct hc_headers * n;

    if (hc) {
        free(hc->hostname);
        free(hc->localpart);
        free(hc->status);
        while (hc->req_headers) {
            n = hc->req_headers->next;
            hc_h_free(hc->req_headers);
            hc->req_headers = n;
        }
        while (hc->rsp_headers) {
            n = hc->rsp_headers->next;
            hc_h_free(hc->rsp_headers);
            hc->rsp_headers = n;
        }
        if (hc->nc)
            nc_close(hc->nc);
        free(hc);
    }
}

int hc_add_req_header(struct hc * hc, const char * tag, const char * value)
{
    struct hc_headers * h;

    h = hc_h_make(tag, value);
    if (!h)
        goto error;
    h->next = hc->req_headers;
    hc->req_headers = h;
    return 0;
error:
    return -1;
}

struct hc * hc_start_request(char * url)
{
    struct hc * hc;
    char * e;

    hc = malloc(sizeof *hc);
    memset(hc, 0, sizeof *hc);
    
    if (strncmp(url, "http://", 7) != 0) {
        goto error;
    }
    url += 7;
    e = strchr(url, '/');
    if (!e) {
        hc->hostname = strdup(url);
        hc->localpart = strdup("/");
    } else {
        hc->hostname        = malloc(e - url + 1);
        if (!hc->hostname) goto error;
        memcpy(hc->hostname, url, e - url);
        hc->hostname[e-url] = '\0';
        hc->localpart       = strdup(e);
        if (!hc->localpart) goto error;
    }

    if (hc_add_req_header(hc, "Host", hc->hostname) < 0)
        goto error;

    e = strchr(hc->hostname, ':');
    if (e) {
        *e = '\0';
        hc->port = (short)strtoul(e+1, NULL, 10);
    } else {
        hc->port = 80;
    }

    hc->nc = nc_open(hc->hostname, hc->port);
    if (!hc->nc)
        goto error;
    
    return hc;
error:
    if (hc)
        hc_free(hc);
    return NULL;
}

int hc_send_request(struct hc * hc)
{
    char buffer[1024];
    size_t l;
    struct hc_headers * h;

    l = strlen(hc->localpart) + strlen("GET  HTTP/1.1\r\n");
    for (h = hc->req_headers; h; h = h->next)
        l += strlen(h->tag) + strlen(h->value) + 4;
    l += 3;
    if (l > sizeof buffer) {
        //fprintf(stderr, "ERROR: hc_send_request: buffer too small; need %lu\n",(unsigned long)l);
        return -1;
    }

    l = sprintf(buffer, "GET %s HTTP/1.1\r\n", hc->localpart);
    for (h = hc->req_headers; h; h = h->next)
        l += sprintf(buffer+l, "%s: %s\r\n", h->tag, h->value);
    l += sprintf(buffer+l, "\r\n");
    nc_write(hc->nc, (unsigned char*)buffer, l);

    if (nc_read_line(hc->nc, (unsigned char*)buffer, sizeof buffer) <= 0) {
        perror("ERROR: hc_send_request nc_read_line");
        return -1;
    }
    
    hc->status = strdup(buffer);
    while (nc_read_line(hc->nc, (unsigned char*)buffer, sizeof buffer) > 0) {
        char * e;
        e = strchr(buffer, ':');
        if (e) {
            *e = '\0';
            e++;
            while (isspace(*e))
                e++;
            h = hc_h_make(buffer, e);
            h->next = hc->rsp_headers;
            hc->rsp_headers = h;
        } else {
            //fprintf(stderr, "ERROR: hc_send_request got invalid header line :%s:\n", buffer);
        }
    }
    
    return 0;
}

int hc_post_request(struct hc * hc,
                    int (*callback)(unsigned char *, size_t, void *),
                    void * v)
{
    char buffer[1024];
    size_t l;
    struct hc_headers * h;

    l = strlen(hc->localpart) + strlen("POST  HTTP/1.0\r\n");
    for (h = hc->req_headers; h; h = h->next)
        l += strlen(h->tag) + strlen(h->value) + 4;
    l += 3;
    if (l > sizeof buffer) {
        //fprintf(stderr, "ERROR: hc_post_request: buffer too small; need %lu\n",(unsigned long)l);
        return -1;
    }

    l = sprintf(buffer, "POST %s HTTP/1.0\r\n", hc->localpart);
    for (h = hc->req_headers; h; h = h->next)
        l += sprintf(buffer+l, "%s: %s\r\n", h->tag, h->value);
    l += sprintf(buffer+l, "\r\n");
    nc_write(hc->nc, (unsigned char*)buffer, l);

    while ((l = callback((unsigned char*)buffer, sizeof buffer, v)) != 0) {
        int r;
        r = nc_write(hc->nc, (unsigned char*)buffer, l);
//      fprintf(stderr, "%d\n", r);
        
    }
  
    if (nc_read_line(hc->nc, (unsigned char*)buffer, sizeof buffer) <= 0) {
        perror("ERROR: hc_post_request nc_read_line");
        return -1;
    }
    hc->status = strdup(buffer);
    while (nc_read_line(hc->nc, (unsigned char*)buffer, sizeof buffer) > 0) {
        char * e;
        e = strchr(buffer, ':');
        if (e) {
            *e = '\0';
            e++;
            while (isspace(*e))
                e++;
            h = hc_h_make(buffer, e);
            h->next = hc->rsp_headers;
            hc->rsp_headers = h;
        } else {
            //fprintf(stderr, "ERROR: hc_post_request got invalid header line :%s:\n", buffer);
        }
    }
    
    return 0;
}

int hc_get_status(struct hc * hc)
{
    return strtoul(hc->status + 9, NULL, 10);
}

char * hc_lookup_rsp_header(struct hc * hc, const char * tag)
{
    struct hc_headers * h;
    
    for (h = hc->rsp_headers; h; h = h->next)
        if (strcasecmp(tag, h->tag) == 0)
            return h->value;
    return NULL;
}

extern int hc_read_pieces(struct hc * hc,
                          void (*callback)(unsigned char *, size_t, void *),
                          void * v)
{
    char * te;
    int chunked = 0;
    int done = 0;
    char * buf;
    size_t len, len_read, octets_to_go;
    char * cl;
    
    cl = hc_lookup_rsp_header(hc, "Content-Length");
    if (cl) {
        octets_to_go = strtoul(cl, NULL, 10);
    } else {
        octets_to_go = 0;
    }
    
    te = hc_lookup_rsp_header(hc, "Transfer-Encoding");
    if (te && strcmp(te, "chunked") == 0) {
        chunked = 1;
    }
    while (!done) {
        if (chunked) {
            char lenstr[32];

            nc_read_line(hc->nc, (unsigned char*)lenstr, sizeof lenstr);
            len = strtoul(lenstr, NULL, 16);
        } else {
            if (cl) {
                len = octets_to_go;
                if (len > MAX_CHUNK)
                    len = MAX_CHUNK;
            } else {
                len = MAX_CHUNK;
            }
        }
        if (len) {
            buf = malloc(len+1);
            len_read = nc_read(hc->nc, (unsigned char*)buf, len);
            if (len_read < len)
                done = 1;
            buf[len_read] = '\0';
            callback((unsigned char*)buf, len_read, v);
            if (cl) {
                octets_to_go -= len_read;
                if (octets_to_go == 0 && !chunked) {
                    done = 1;
                }
            }
        } else {
            done = 1;
        }
        if (chunked) {
            char linebuf[80];
            /* if done, then any non-blank lines read here are
               supposed to be treated as HTTP header lines, but since
               we didn't say we could accept trailers, the server's
               only allowed to send them if it's happy with us
               discarding them. (2616 3.7b).  The Replay doesn't use
               trailers anyway */
            while (nc_read_line(hc->nc, (unsigned char*)linebuf, sizeof linebuf) > 0)
                ;
        }
    }
    return 0;
}

struct chunk
{
    char * buf;
    size_t len;
    struct chunk * next;
};

struct read_all_data 
{
    struct chunk * start;
    struct chunk * end;
    size_t total;
};

static void read_all_callback(unsigned char * buf, size_t len, void * vd)
{
    struct read_all_data * data = vd;
    struct chunk * chunk;

    chunk = malloc(sizeof *chunk);
    chunk->buf = (char*)buf;
    chunk->len = len;
    chunk->next = NULL;
    
    if (data->end) {
        data->end->next = chunk;
        data->end = chunk;
    } else {
        data->start = data->end = chunk;
    }

    data->total += len;
}

unsigned char * hc_read_all_len(struct hc * hc, size_t * plen)
{
    struct read_all_data data;
    struct chunk * chunk, * next;
    size_t cur;
    unsigned char * r;
    
    data.start = data.end = NULL;
    data.total = 0;
    
    hc_read_pieces(hc, read_all_callback, &data);
    
    r = malloc(data.total + 1);
    cur = 0;
    for (chunk = data.start; chunk; chunk = next) {
        memcpy(r+cur, chunk->buf, chunk->len);
        cur += chunk->len;
        free(chunk->buf);
        next = chunk->next;
        free(chunk);
    }

    r[data.total] = '\0';
    if (plen)
        *plen = data.total;
    
    return r;
}

unsigned char * hc_read_all(struct hc * hc)
{
    return hc_read_all_len(hc, NULL);
}


extern int hc_read_pieces_len(struct hc * hc,
                          void (*callback)(unsigned char *, size_t, void *),
                          void * v, size_t len_to_read)
{
    char * te;
    int chunked = 0;
    int done = 0;
    char * buf;
    size_t len, len_read, octets_to_go;
    char * cl;
    
	// NOTE: len_to_read is not currently implemented for non-chunked content.
	// The ReplayTV only sends chunked content so this doesn't matter here.
    cl = hc_lookup_rsp_header(hc, "Content-Length");
    if (cl) {
        octets_to_go = strtoul(cl, NULL, 10);
    } else {
        octets_to_go = 0;
    }
    
    te = hc_lookup_rsp_header(hc, "Transfer-Encoding");
    if (te && strcmp(te, "chunked") == 0) {
        chunked = 1;
    }
    while (!done) {
        if (chunked) {
            char lenstr[32];
			
			if (!hc->curr_chunk_len) {
				nc_read_line(hc->nc, (unsigned char*)lenstr, sizeof lenstr);
				hc->curr_chunk_len = strtoul(lenstr, NULL, 16);
			}
			if (len_to_read > hc->curr_chunk_len) {
				len = hc->curr_chunk_len;
			} else {
				len = len_to_read;
			}
        } else {
            if (cl) {
                len = octets_to_go;
                if (len > MAX_CHUNK)
                    len = MAX_CHUNK;
            } else {
                len = MAX_CHUNK;
            }
        }
        if (len) {
            buf = malloc(len+1);
            len_read = nc_read(hc->nc, (unsigned char*)buf, len);
            if (len_read <= len)
                done = 1;
            buf[len_read] = '\0';
            callback((unsigned char*)buf, len_read, v);
            if (cl) {
                octets_to_go -= len_read;
                if (octets_to_go == 0 && !chunked) {
                    done = 1;
                }
            }
			hc->curr_chunk_len -= len_read;
        } else {
            done = 1;
        }
        if (chunked && !hc->curr_chunk_len) {
            char linebuf[80];
            /* if done, then any non-blank lines read here are
               supposed to be treated as HTTP header lines, but since
               we didn't say we could accept trailers, the server's
               only allowed to send them if it's happy with us
               discarding them. (2616 3.7b).  The Replay doesn't use
               trailers anyway */
            while (nc_read_line(hc->nc, (unsigned char*)linebuf, sizeof linebuf) > 0)
                ;
        }
    }
    return 0;
}

