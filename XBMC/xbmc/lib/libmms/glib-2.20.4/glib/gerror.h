/* gerror.h - Error reporting system
 *
 *  Copyright 2000 Red Hat, Inc.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *   Boston, MA 02111-1307, USA.
 */

#if defined(G_DISABLE_SINGLE_INCLUDES) && !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_ERROR_H__
#define __G_ERROR_H__

#include <glib/gquark.h>

G_BEGIN_DECLS

typedef struct _GError GError;

struct _GError
{
  GQuark       domain;
  gint         code;
  gchar       *message;
};

GError*  g_error_new           (GQuark         domain,
                                gint           code,
                                const gchar   *format,
                                ...) G_GNUC_PRINTF (3, 4);

GError*  g_error_new_literal   (GQuark         domain,
                                gint           code,
                                const gchar   *message);

void     g_error_free          (GError        *error);
GError*  g_error_copy          (const GError  *error);

gboolean g_error_matches       (const GError  *error,
                                GQuark         domain,
                                gint           code);

/* if (err) *err = g_error_new(domain, code, format, ...), also has
 * some sanity checks.
 */
void     g_set_error           (GError       **err,
                                GQuark         domain,
                                gint           code,
                                const gchar   *format,
                                ...) G_GNUC_PRINTF (4, 5);

void     g_set_error_literal   (GError       **err,
                                GQuark         domain,
                                gint           code,
                                const gchar   *message);

/* if (dest) *dest = src; also has some sanity checks.
 */
void     g_propagate_error     (GError       **dest,
				GError        *src);

/* if (err && *err) { g_error_free(*err); *err = NULL; } */
void     g_clear_error         (GError       **err);

/* if (err) prefix the formatted string to the ->message */
void     g_prefix_error               (GError       **err,
                                       const gchar   *format,
                                       ...) G_GNUC_PRINTF (2, 3);

/* g_propagate_error then g_error_prefix on dest */
void     g_propagate_prefixed_error   (GError       **dest,
                                       GError        *src,
                                       const gchar   *format,
                                       ...) G_GNUC_PRINTF (3, 4);

G_END_DECLS

#endif /* __G_ERROR_H__ */
