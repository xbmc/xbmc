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

#ifndef __G_DATASET_H__
#define __G_DATASET_H__

#include <glib/gquark.h>

G_BEGIN_DECLS

typedef struct _GData           GData;

typedef void            (*GDataForeachFunc)     (GQuark         key_id,
                                                 gpointer       data,
                                                 gpointer       user_data);

/* Keyed Data List
 */
void     g_datalist_init                (GData            **datalist);
void     g_datalist_clear               (GData            **datalist);
gpointer g_datalist_id_get_data         (GData            **datalist,
					 GQuark             key_id);
void     g_datalist_id_set_data_full    (GData            **datalist,
					 GQuark             key_id,
					 gpointer           data,
					 GDestroyNotify     destroy_func);
gpointer g_datalist_id_remove_no_notify (GData            **datalist,
					 GQuark             key_id);
void     g_datalist_foreach             (GData            **datalist,
					 GDataForeachFunc   func,
					 gpointer           user_data);

/**
 * G_DATALIST_FLAGS_MASK:
 *
 * A bitmask that restricts the possible flags passed to
 * g_datalist_set_flags(). Passing a flags value where
 * flags & ~G_DATALIST_FLAGS_MASK != 0 is an error.
 */
#define G_DATALIST_FLAGS_MASK 0x3

void     g_datalist_set_flags           (GData            **datalist,
					 guint              flags);
void     g_datalist_unset_flags         (GData            **datalist,
					 guint              flags);
guint    g_datalist_get_flags           (GData            **datalist);

#define   g_datalist_id_set_data(dl, q, d)      \
     g_datalist_id_set_data_full ((dl), (q), (d), NULL)
#define   g_datalist_id_remove_data(dl, q)      \
     g_datalist_id_set_data ((dl), (q), NULL)
#define   g_datalist_get_data(dl, k)            \
     (g_datalist_id_get_data ((dl), g_quark_try_string (k)))
#define   g_datalist_set_data_full(dl, k, d, f) \
     g_datalist_id_set_data_full ((dl), g_quark_from_string (k), (d), (f))
#define   g_datalist_remove_no_notify(dl, k)    \
     g_datalist_id_remove_no_notify ((dl), g_quark_try_string (k))
#define   g_datalist_set_data(dl, k, d)         \
     g_datalist_set_data_full ((dl), (k), (d), NULL)
#define   g_datalist_remove_data(dl, k)         \
     g_datalist_id_set_data ((dl), g_quark_try_string (k), NULL)


/* Location Associated Keyed Data
 */
void      g_dataset_destroy             (gconstpointer    dataset_location);
gpointer  g_dataset_id_get_data         (gconstpointer    dataset_location,
                                         GQuark           key_id);
void      g_dataset_id_set_data_full    (gconstpointer    dataset_location,
                                         GQuark           key_id,
                                         gpointer         data,
                                         GDestroyNotify   destroy_func);
gpointer  g_dataset_id_remove_no_notify (gconstpointer    dataset_location,
                                         GQuark           key_id);
void      g_dataset_foreach             (gconstpointer    dataset_location,
                                         GDataForeachFunc func,
                                         gpointer         user_data);
#define   g_dataset_id_set_data(l, k, d)        \
     g_dataset_id_set_data_full ((l), (k), (d), NULL)
#define   g_dataset_id_remove_data(l, k)        \
     g_dataset_id_set_data ((l), (k), NULL)
#define   g_dataset_get_data(l, k)              \
     (g_dataset_id_get_data ((l), g_quark_try_string (k)))
#define   g_dataset_set_data_full(l, k, d, f)   \
     g_dataset_id_set_data_full ((l), g_quark_from_string (k), (d), (f))
#define   g_dataset_remove_no_notify(l, k)      \
     g_dataset_id_remove_no_notify ((l), g_quark_try_string (k))
#define   g_dataset_set_data(l, k, d)           \
     g_dataset_set_data_full ((l), (k), (d), NULL)
#define   g_dataset_remove_data(l, k)           \
     g_dataset_id_set_data ((l), g_quark_try_string (k), NULL)

G_END_DECLS

#endif /* __G_DATASET_H__ */
