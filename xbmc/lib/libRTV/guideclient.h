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

#ifndef GUIDECLIENT_H
#define GUIDECLIENT_H

#include "httpclient.h"

unsigned long guide_client_get_snapshot(unsigned char ** presult,
                                        unsigned char ** ptimestamp,
                                        unsigned long * psize,
                                        const char * address,
                                        const char * cur_timestamp);

/* XXX should also have is_show_in_use and delete_show */
#endif
