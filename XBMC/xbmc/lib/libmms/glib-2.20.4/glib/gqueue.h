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

#ifndef __G_QUEUE_H__
#define __G_QUEUE_H__

#include <glib/glist.h>

G_BEGIN_DECLS

typedef struct _GQueue		GQueue;

struct _GQueue
{
  GList *head;
  GList *tail;
  guint  length;
};

#define G_QUEUE_INIT { NULL, NULL, 0 }

/* Queues
 */
GQueue*  g_queue_new            (void);
void     g_queue_free           (GQueue           *queue);
void     g_queue_init           (GQueue           *queue);
void     g_queue_clear          (GQueue           *queue);
gboolean g_queue_is_empty       (GQueue           *queue);
guint    g_queue_get_length     (GQueue           *queue);
void     g_queue_reverse        (GQueue           *queue);
GQueue * g_queue_copy           (GQueue           *queue);
void     g_queue_foreach        (GQueue           *queue,
				 GFunc             func,
				 gpointer          user_data);
GList *  g_queue_find           (GQueue           *queue,
				 gconstpointer     data);
GList *  g_queue_find_custom    (GQueue           *queue,
				 gconstpointer     data,
				 GCompareFunc      func);
void     g_queue_sort           (GQueue           *queue,
				 GCompareDataFunc  compare_func,
				 gpointer          user_data);

void     g_queue_push_head      (GQueue           *queue,
				 gpointer          data);
void     g_queue_push_tail      (GQueue           *queue,
				 gpointer          data);
void     g_queue_push_nth       (GQueue           *queue,
				 gpointer          data,
				 gint              n);
gpointer g_queue_pop_head       (GQueue           *queue);
gpointer g_queue_pop_tail       (GQueue           *queue);
gpointer g_queue_pop_nth        (GQueue           *queue,
				 guint             n);
gpointer g_queue_peek_head      (GQueue           *queue);
gpointer g_queue_peek_tail      (GQueue           *queue);
gpointer g_queue_peek_nth       (GQueue           *queue,
				 guint             n);
gint     g_queue_index          (GQueue           *queue,
				 gconstpointer     data);
void     g_queue_remove         (GQueue           *queue,
				 gconstpointer     data);
void     g_queue_remove_all     (GQueue           *queue,
				 gconstpointer     data);
void     g_queue_insert_before  (GQueue           *queue,
				 GList            *sibling,
				 gpointer          data);
void     g_queue_insert_after   (GQueue           *queue,
				 GList            *sibling,
				 gpointer          data);
void     g_queue_insert_sorted  (GQueue           *queue,
				 gpointer          data,
				 GCompareDataFunc  func,
				 gpointer          user_data);

void     g_queue_push_head_link (GQueue           *queue,
				 GList            *link_);
void     g_queue_push_tail_link (GQueue           *queue,
				 GList            *link_);
void     g_queue_push_nth_link  (GQueue           *queue,
				 gint              n,
				 GList            *link_);
GList*   g_queue_pop_head_link  (GQueue           *queue);
GList*   g_queue_pop_tail_link  (GQueue           *queue);
GList*   g_queue_pop_nth_link   (GQueue           *queue,
				 guint             n);
GList*   g_queue_peek_head_link (GQueue           *queue);
GList*   g_queue_peek_tail_link (GQueue           *queue);
GList*   g_queue_peek_nth_link  (GQueue           *queue,
				 guint             n);
gint     g_queue_link_index     (GQueue           *queue,
				 GList            *link_);
void     g_queue_unlink         (GQueue           *queue,
				 GList            *link_);
void     g_queue_delete_link    (GQueue           *queue,
				 GList            *link_);

G_END_DECLS

#endif /* __G_QUEUE_H__ */
