/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#if defined(G_DISABLE_SINGLE_INCLUDES) && !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_CACHE_H__
#define __G_CACHE_H__

#include <glib/glist.h>

G_BEGIN_DECLS

typedef struct _GCache          GCache;

typedef gpointer        (*GCacheNewFunc)        (gpointer       key);
typedef gpointer        (*GCacheDupFunc)        (gpointer       value);
typedef void            (*GCacheDestroyFunc)    (gpointer       value);

/* Caches
 */
GCache*  g_cache_new           (GCacheNewFunc      value_new_func,
                                GCacheDestroyFunc  value_destroy_func,
                                GCacheDupFunc      key_dup_func,
                                GCacheDestroyFunc  key_destroy_func,
                                GHashFunc          hash_key_func,
                                GHashFunc          hash_value_func,
                                GEqualFunc         key_equal_func);
void     g_cache_destroy       (GCache            *cache);
gpointer g_cache_insert        (GCache            *cache,
                                gpointer           key);
void     g_cache_remove        (GCache            *cache,
                                gconstpointer      value);
void     g_cache_key_foreach   (GCache            *cache,
                                GHFunc             func,
                                gpointer           user_data);
#ifndef G_DISABLE_DEPRECATED
void     g_cache_value_foreach (GCache            *cache,
                                GHFunc             func,
                                gpointer           user_data);
#endif

G_END_DECLS

#endif /* __G_CACHE_H__ */
