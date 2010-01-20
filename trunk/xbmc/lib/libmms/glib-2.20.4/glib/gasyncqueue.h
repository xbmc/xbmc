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

#ifndef __G_ASYNCQUEUE_H__
#define __G_ASYNCQUEUE_H__

#include <glib/gthread.h>

G_BEGIN_DECLS

typedef struct _GAsyncQueue GAsyncQueue;

/* Asyncronous Queues, can be used to communicate between threads */

/* Get a new GAsyncQueue with the ref_count 1 */
GAsyncQueue*  g_async_queue_new                 (void);

GAsyncQueue*  g_async_queue_new_full            (GDestroyNotify item_free_func);

/* Lock and unlock a GAsyncQueue. All functions lock the queue for
 * themselves, but in certain cirumstances you want to hold the lock longer,
 * thus you lock the queue, call the *_unlocked functions and unlock it again.
 */
void         g_async_queue_lock                 (GAsyncQueue      *queue);
void         g_async_queue_unlock               (GAsyncQueue      *queue);

/* Ref and unref the GAsyncQueue. */
GAsyncQueue* g_async_queue_ref                  (GAsyncQueue      *queue);
void         g_async_queue_unref                (GAsyncQueue      *queue);

#ifndef G_DISABLE_DEPRECATED
/* You don't have to hold the lock for calling *_ref and *_unref anymore. */
void         g_async_queue_ref_unlocked         (GAsyncQueue      *queue);
void         g_async_queue_unref_and_unlock     (GAsyncQueue      *queue);
#endif /* !G_DISABLE_DEPRECATED */

/* Push data into the async queue. Must not be NULL. */
void         g_async_queue_push                 (GAsyncQueue      *queue,
						 gpointer          data);
void         g_async_queue_push_unlocked        (GAsyncQueue      *queue,
						 gpointer          data);

void         g_async_queue_push_sorted          (GAsyncQueue      *queue,
						 gpointer          data,
						 GCompareDataFunc  func,
						 gpointer          user_data);
void         g_async_queue_push_sorted_unlocked (GAsyncQueue      *queue,
						 gpointer          data,
						 GCompareDataFunc  func,
						 gpointer          user_data);

/* Pop data from the async queue. When no data is there, the thread is blocked
 * until data arrives.
 */
gpointer     g_async_queue_pop                  (GAsyncQueue      *queue);
gpointer     g_async_queue_pop_unlocked         (GAsyncQueue      *queue);

/* Try to pop data. NULL is returned in case of empty queue. */
gpointer     g_async_queue_try_pop              (GAsyncQueue      *queue);
gpointer     g_async_queue_try_pop_unlocked     (GAsyncQueue      *queue);



/* Wait for data until at maximum until end_time is reached. NULL is returned
 * in case of empty queue. 
 */
gpointer     g_async_queue_timed_pop            (GAsyncQueue      *queue,
						 GTimeVal         *end_time);
gpointer     g_async_queue_timed_pop_unlocked   (GAsyncQueue      *queue,
						 GTimeVal         *end_time);

/* Return the length of the queue. Negative values mean that threads
 * are waiting, positve values mean that there are entries in the
 * queue. Actually this function returns the length of the queue minus
 * the number of waiting threads, g_async_queue_length == 0 could also
 * mean 'n' entries in the queue and 'n' thread waiting. Such can
 * happen due to locking of the queue or due to scheduling. 
 */
gint         g_async_queue_length               (GAsyncQueue      *queue);
gint         g_async_queue_length_unlocked      (GAsyncQueue      *queue);
void         g_async_queue_sort                 (GAsyncQueue      *queue,
						 GCompareDataFunc  func,
						 gpointer          user_data);
void         g_async_queue_sort_unlocked        (GAsyncQueue      *queue,
						 GCompareDataFunc  func,
						 gpointer          user_data);

/* Private API */
GMutex*      _g_async_queue_get_mutex           (GAsyncQueue      *queue);

G_END_DECLS

#endif /* __G_ASYNCQUEUE_H__ */
