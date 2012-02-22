/*
 *  Functions converting HTSMSGs to/from JSON
 *  Copyright (C) 2008 Andreas Öman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HTSMSG_JSON_H_
#define HTSMSG_JSON_H_

#include "htsmsg.h"
#include "htsbuf.h"

/**
 * htsmsg_binary_deserialize
 */
htsmsg_t *htsmsg_json_deserialize(const char *src);

int htsmsg_json_serialize(htsmsg_t *msg, htsbuf_queue_t *hq, int pretty);

#endif /* HTSMSG_JSON_H_ */
