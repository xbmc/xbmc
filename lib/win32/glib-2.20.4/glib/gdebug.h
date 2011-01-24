/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __G_DEBUG_H__
#define __G_DEBUG_H__

G_BEGIN_DECLS 

typedef enum {
  G_DEBUG_FATAL_WARNINGS  = 1 << 0,
  G_DEBUG_FATAL_CRITICALS = 1 << 1
} GDebugFlag;


#ifdef G_ENABLE_DEBUG

#define G_NOTE(type, action)            G_STMT_START { \
    if (!_g_debug_initialized)                         \
       { _g_debug_init (); }                           \
    if (_g_debug_flags & G_DEBUG_##type)               \
       { action; };                         } G_STMT_END

#else /* !G_ENABLE_DEBUG */

#define G_NOTE(type, action)
      
#endif /* G_ENABLE_DEBUG */

GLIB_VAR gboolean _g_debug_initialized;
GLIB_VAR guint _g_debug_flags;

G_GNUC_INTERNAL void _g_debug_init (void);

G_END_DECLS

#endif /* __G_DEBUG_H__ */
