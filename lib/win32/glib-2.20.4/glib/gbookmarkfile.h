/* gbookmarkfile.h: parsing and building desktop bookmarks
 *
 * Copyright (C) 2005-2006 Emmanuele Bassi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#if defined(G_DISABLE_SINGLE_INCLUDES) && !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_BOOKMARK_FILE_H__
#define __G_BOOKMARK_FILE_H__

#include <glib/gerror.h>
#include <time.h>

G_BEGIN_DECLS

/* GError enumeration
 */
#define G_BOOKMARK_FILE_ERROR	(g_bookmark_file_error_quark ())

typedef enum
{
  G_BOOKMARK_FILE_ERROR_INVALID_URI,
  G_BOOKMARK_FILE_ERROR_INVALID_VALUE,
  G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED,
  G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
  G_BOOKMARK_FILE_ERROR_READ,
  G_BOOKMARK_FILE_ERROR_UNKNOWN_ENCODING,
  G_BOOKMARK_FILE_ERROR_WRITE,
  G_BOOKMARK_FILE_ERROR_FILE_NOT_FOUND
} GBookmarkFileError;

GQuark g_bookmark_file_error_quark (void);

/*
 * GBookmarkFile
 */
typedef struct _GBookmarkFile GBookmarkFile;

GBookmarkFile *g_bookmark_file_new                 (void);
void           g_bookmark_file_free                (GBookmarkFile  *bookmark);

gboolean       g_bookmark_file_load_from_file      (GBookmarkFile  *bookmark,
						    const gchar    *filename,
						    GError        **error);
gboolean       g_bookmark_file_load_from_data      (GBookmarkFile  *bookmark,
						    const gchar    *data,
						    gsize           length,
						    GError        **error);
gboolean       g_bookmark_file_load_from_data_dirs (GBookmarkFile  *bookmark,
						    const gchar    *file,
						    gchar         **full_path,
						    GError        **error);
gchar *        g_bookmark_file_to_data             (GBookmarkFile  *bookmark,
						    gsize          *length,
						    GError        **error) G_GNUC_MALLOC;
gboolean       g_bookmark_file_to_file             (GBookmarkFile  *bookmark,
						    const gchar    *filename,
						    GError        **error);

void           g_bookmark_file_set_title           (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *title);
gchar *        g_bookmark_file_get_title           (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    GError        **error) G_GNUC_MALLOC;
void           g_bookmark_file_set_description     (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *description);
gchar *        g_bookmark_file_get_description     (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    GError        **error) G_GNUC_MALLOC;
void           g_bookmark_file_set_mime_type       (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *mime_type);
gchar *        g_bookmark_file_get_mime_type       (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    GError        **error) G_GNUC_MALLOC;
void           g_bookmark_file_set_groups          (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar   **groups,
						    gsize           length);
void           g_bookmark_file_add_group           (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *group);
gboolean       g_bookmark_file_has_group           (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *group,
						    GError        **error);
gchar **       g_bookmark_file_get_groups          (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    gsize          *length,
						    GError        **error) G_GNUC_MALLOC;
void           g_bookmark_file_add_application     (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *name,
						    const gchar    *exec);
gboolean       g_bookmark_file_has_application     (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *name,
						    GError        **error);
gchar **       g_bookmark_file_get_applications    (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    gsize          *length,
						    GError        **error) G_GNUC_MALLOC;
gboolean       g_bookmark_file_set_app_info        (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *name,
						    const gchar    *exec,
						    gint            count,
						    time_t          stamp,
						    GError        **error);
gboolean       g_bookmark_file_get_app_info        (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *name,
						    gchar         **exec,
						    guint          *count,
						    time_t         *stamp,
						    GError        **error);
void           g_bookmark_file_set_is_private      (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    gboolean        is_private);
gboolean       g_bookmark_file_get_is_private      (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    GError        **error);
void           g_bookmark_file_set_icon            (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *href,
						    const gchar    *mime_type);
gboolean       g_bookmark_file_get_icon            (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    gchar         **href,
						    gchar         **mime_type,
						    GError        **error);
void           g_bookmark_file_set_added           (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    time_t          added);
time_t         g_bookmark_file_get_added           (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    GError        **error);
void           g_bookmark_file_set_modified        (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    time_t          modified);
time_t         g_bookmark_file_get_modified        (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    GError        **error);
void           g_bookmark_file_set_visited         (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    time_t          visited);
time_t         g_bookmark_file_get_visited         (GBookmarkFile  *bookmark,
						    const gchar    *uri, 
						    GError        **error);
gboolean       g_bookmark_file_has_item            (GBookmarkFile  *bookmark,
						    const gchar    *uri);
gint           g_bookmark_file_get_size            (GBookmarkFile  *bookmark);
gchar **       g_bookmark_file_get_uris            (GBookmarkFile  *bookmark,
						    gsize          *length) G_GNUC_MALLOC;
gboolean       g_bookmark_file_remove_group        (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *group,
						    GError        **error);
gboolean       g_bookmark_file_remove_application  (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    const gchar    *name,
						    GError        **error);
gboolean       g_bookmark_file_remove_item         (GBookmarkFile  *bookmark,
						    const gchar    *uri,
						    GError        **error);
gboolean       g_bookmark_file_move_item           (GBookmarkFile  *bookmark,
						    const gchar    *old_uri,
						    const gchar    *new_uri,
						    GError        **error);

G_END_DECLS

#endif /* __G_BOOKMARK_FILE_H__ */
