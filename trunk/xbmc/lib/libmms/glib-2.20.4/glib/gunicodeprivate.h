/* gunicodeprivate.h
 *
 * Copyright (C) 2003 Noah Levitt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __G_UNICODE_PRIVATE_H__
#define __G_UNICODE_PRIVATE_H__

#include "glib.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL gunichar *_g_utf8_normalize_wc
				(const gchar    *str,
				gssize          max_len,
				GNormalizeMode  mode);

G_END_DECLS

#endif /* __G_UNICODE_PRIVATE_H__ */
