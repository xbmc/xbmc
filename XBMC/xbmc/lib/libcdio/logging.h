/*
    $Id$

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  CDIO_LOG_DEBUG = 1,
  CDIO_LOG_INFO,
  CDIO_LOG_WARN,
  CDIO_LOG_ERROR,
  CDIO_LOG_ASSERT
} cdio_log_level_t;

void
cdio_log (cdio_log_level_t level, const char format[], ...) GNUC_PRINTF(2, 3);
    
typedef void (*cdio_log_handler_t) (cdio_log_level_t level, 
                                    const char message[]);

cdio_log_handler_t
cdio_log_set_handler (cdio_log_handler_t new_handler);

void
cdio_debug (const char format[], ...) GNUC_PRINTF(1,2);

void
cdio_info (const char format[], ...) GNUC_PRINTF(1,2);

void
cdio_warn (const char format[], ...) GNUC_PRINTF(1,2);

void
cdio_error (const char format[], ...) GNUC_PRINTF(1,2);

#ifdef __cplusplus
}
#endif

#endif /* __LOGGING_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
