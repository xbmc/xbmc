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

#ifndef NETCLIENT_H
#define NETCLIENT_H

#include <stdlib.h>

struct nc;

extern void        nc_error(char * where);
extern struct nc * nc_open(char * address, short port);
extern int         nc_close(struct nc * nc);
extern int         nc_read(struct nc * nc, unsigned char * buf, size_t len);
extern int         nc_read_line(struct nc * nc, unsigned char * buf, size_t maxlen);
extern int         nc_write(struct nc * nc, unsigned char * buf, size_t len);

#endif
