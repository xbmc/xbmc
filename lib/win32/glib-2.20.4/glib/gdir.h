/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gdir.c: Simplified wrapper around the DIRENT functions.
 *
 * Copyright 2001 Hans Breuer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#if defined(G_DISABLE_SINGLE_INCLUDES) && !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_DIR_H__
#define __G_DIR_H__

#include <glib/gerror.h>

G_BEGIN_DECLS

typedef struct _GDir GDir;

#ifdef G_OS_WIN32
/* For DLL ABI stability, keep old names for old (non-UTF-8) functionality. */
#define g_dir_open g_dir_open_utf8
#define g_dir_read_name g_dir_read_name_utf8
#endif

GDir    *                g_dir_open           (const gchar  *path,
					       guint         flags,
					       GError      **error);
G_CONST_RETURN gchar    *g_dir_read_name      (GDir         *dir);
void                     g_dir_rewind         (GDir         *dir);
void                     g_dir_close          (GDir         *dir);

G_END_DECLS

#endif /* __G_DIR_H__ */
