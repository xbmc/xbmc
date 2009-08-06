/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * g_atomic_*: atomic operations.
 * Copyright (C) 2003 Sebastian Wilhelmi
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
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#if defined(G_DISABLE_SINGLE_INCLUDES) && !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_ATOMIC_H__
#define __G_ATOMIC_H__

#include <glib/gtypes.h>

G_BEGIN_DECLS

gint     g_atomic_int_exchange_and_add         (volatile gint G_GNUC_MAY_ALIAS *atomic,
						gint      	   val);
void     g_atomic_int_add                      (volatile gint G_GNUC_MAY_ALIAS *atomic,
						gint      	   val);
gboolean g_atomic_int_compare_and_exchange     (volatile gint G_GNUC_MAY_ALIAS *atomic,
						gint      	   oldval,
						gint      	   newval);
gboolean g_atomic_pointer_compare_and_exchange (volatile gpointer G_GNUC_MAY_ALIAS *atomic, 
						gpointer  	   oldval, 
						gpointer  	   newval);

gint     g_atomic_int_get                      (volatile gint G_GNUC_MAY_ALIAS *atomic);
void     g_atomic_int_set                      (volatile gint G_GNUC_MAY_ALIAS *atomic,
						gint               newval);
gpointer g_atomic_pointer_get                  (volatile gpointer G_GNUC_MAY_ALIAS *atomic);
void     g_atomic_pointer_set                  (volatile gpointer G_GNUC_MAY_ALIAS *atomic,
						gpointer           newval);

#ifndef G_ATOMIC_OP_MEMORY_BARRIER_NEEDED
# define g_atomic_int_get(atomic) 		((gint)*(atomic))
# define g_atomic_int_set(atomic, newval) 	((void) (*(atomic) = (newval)))
# define g_atomic_pointer_get(atomic) 		((gpointer)*(atomic))
# define g_atomic_pointer_set(atomic, newval)	((void) (*(atomic) = (newval)))
#else
# define g_atomic_int_get(atomic) \
 ((void) sizeof (gchar [sizeof (*(atomic)) == sizeof (gint) ? 1 : -1]), \
  (g_atomic_int_get) ((volatile gint G_GNUC_MAY_ALIAS *) (void *) (atomic)))
# define g_atomic_int_set(atomic, newval) \
 ((void) sizeof (gchar [sizeof (*(atomic)) == sizeof (gint) ? 1 : -1]), \
  (g_atomic_int_set) ((volatile gint G_GNUC_MAY_ALIAS *) (void *) (atomic), (newval)))
# define g_atomic_pointer_get(atomic) \
 ((void) sizeof (gchar [sizeof (*(atomic)) == sizeof (gpointer) ? 1 : -1]), \
  (g_atomic_pointer_get) ((volatile gpointer G_GNUC_MAY_ALIAS *) (void *) (atomic)))
# define g_atomic_pointer_set(atomic, newval) \
 ((void) sizeof (gchar [sizeof (*(atomic)) == sizeof (gpointer) ? 1 : -1]), \
  (g_atomic_pointer_set) ((volatile gpointer G_GNUC_MAY_ALIAS *) (void *) (atomic), (newval)))
#endif /* G_ATOMIC_OP_MEMORY_BARRIER_NEEDED */

#define g_atomic_int_inc(atomic) (g_atomic_int_add ((atomic), 1))
#define g_atomic_int_dec_and_test(atomic)				\
  (g_atomic_int_exchange_and_add ((atomic), -1) == 1)

G_END_DECLS

#endif /* __G_ATOMIC_H__ */
