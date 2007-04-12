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

/*
 * These all return:
 *   0 for complete success
 *   -1 for a miscellaneous client error
 *   an HTTP error code if the server returned one (300-499 range)
 *   an httpfs error code if the httpfs provider returned one (8082xxxx range,
 *      it seems)
 *   
 * hfs_do_simple: makes a request, collects all the data, gives it back
 * hfs_do_chunked: makes a request, gives it to a callback function in chunks
 *                 the callback does *not* own the data, and must not free it
 *                 or keep a pointer to it; copy it if you want to keep it.
 *                 This difference from the httpclient layer is ugly and may
 *                 change at some point, even though that will cause a memory
 *                 leak in all existing clients
 * hfs_do_post_simple: POSTs a request, using a callback function to get chunks
 *                 of data.  The size must be known beforehand, an apparent
 *                 unfortunate limitation of the ReplayTV httpfs server.
 *                 writefile is the only (current/known) httpfs command that
 *                 needs POST
 *
 * command: the httpfs command, one of: ls fstat volinfo cp mv rm create
 *                                      mkdir readfile writefile
 * arguments are passed as tag/value pairs, NULL terminated.
 * args for cp: src dest
 * arg for create, fstat, ls, mkdir, rm, volinfo: name
 * args for mv: old new
 * args for readfile, writefile: name pos size
 */

#ifndef HTTPFSCLIENT_H
#define HTTPFSCLIENT_H

#include "httpclient.h"
#include "rtv.h"

#define URLSIZE 512

extern int hfs_output_errors;

extern unsigned long hfs_do_simple(char ** presult,
                                   const char * address, const char * command,
                                   ...);
extern unsigned long hfs_do_chunked(void (*)(unsigned char *, size_t, void *),
                                    void *,
                                    const char * address, u16 msec_delay,
                                    const char * command,
                                    ...);
extern unsigned long hfs_do_post_simple(char ** presult, const char * address,
                                        int (*fn)(unsigned char *, size_t, void *),
                                        void *v,
                                        unsigned long size,
                                        const char * command,
                                        ...);
extern struct hc * hfs_do_chunked_open(const char * address, const char * command, ...);
extern size_t hfs_do_chunked_read(struct hc * hc, char * buf, size_t pos, size_t len);
extern void hfs_do_chunked_close(struct hc * hc);


#define RTV_ENOFILE 80820005
#define RTV_EEXIST  80820018
#define RTV_EPERM   80820024
#endif
