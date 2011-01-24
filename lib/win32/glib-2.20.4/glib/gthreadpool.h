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

#ifndef __G_THREADPOOL_H__
#define __G_THREADPOOL_H__

#include <glib/gthread.h>

G_BEGIN_DECLS

typedef struct _GThreadPool     GThreadPool;

/* Thread Pools
 */

/* The real GThreadPool is bigger, so you may only create a thread
 * pool with the constructor function */
struct _GThreadPool
{
  GFunc func;
  gpointer user_data;
  gboolean exclusive;
};

/* Get a thread pool with the function func, at most max_threads may
 * run at a time (max_threads == -1 means no limit), exclusive == TRUE
 * means, that the threads shouldn't be shared and that they will be
 * prestarted (otherwise they are started as needed) user_data is the
 * 2nd argument to the func */
GThreadPool*    g_thread_pool_new             (GFunc            func,
                                               gpointer         user_data,
                                               gint             max_threads,
                                               gboolean         exclusive,
                                               GError         **error);

/* Push new data into the thread pool. This task is assigned to a thread later
 * (when the maximal number of threads is reached for that pool) or now
 * (otherwise). If necessary a new thread will be started. The function
 * returns immediatly */
void            g_thread_pool_push            (GThreadPool     *pool,
                                               gpointer         data,
                                               GError         **error);

/* Set the number of threads, which can run concurrently for that pool, -1
 * means no limit. 0 means has the effect, that the pool won't process
 * requests until the limit is set higher again */
void            g_thread_pool_set_max_threads (GThreadPool     *pool,
                                               gint             max_threads,
                                               GError         **error);
gint            g_thread_pool_get_max_threads (GThreadPool     *pool);

/* Get the number of threads assigned to that pool. This number doesn't
 * necessarily represent the number of working threads in that pool */
guint           g_thread_pool_get_num_threads (GThreadPool     *pool);

/* Get the number of unprocessed items in the pool */
guint           g_thread_pool_unprocessed     (GThreadPool     *pool);

/* Free the pool, immediate means, that all unprocessed items in the queue
 * wont be processed, wait means, that the function doesn't return immediatly,
 * but after all threads in the pool are ready processing items. immediate
 * does however not mean, that threads are killed. */
void            g_thread_pool_free            (GThreadPool     *pool,
                                               gboolean         immediate,
                                               gboolean         wait_);

/* Set the maximal number of unused threads before threads will be stopped by
 * GLib, -1 means no limit */
void            g_thread_pool_set_max_unused_threads (gint      max_threads);
gint            g_thread_pool_get_max_unused_threads (void);
guint           g_thread_pool_get_num_unused_threads (void);

/* Stop all currently unused threads, but leave the limit untouched */
void            g_thread_pool_stop_unused_threads    (void);

/* Set sort function for priority threading */
void            g_thread_pool_set_sort_function      (GThreadPool      *pool,
		                                      GCompareDataFunc  func,
						      gpointer          user_data);

/* Set maximum time a thread can be idle in the pool before it is stopped */
void            g_thread_pool_set_max_idle_time      (guint             interval);
guint           g_thread_pool_get_max_idle_time      (void);

G_END_DECLS

#endif /* __G_THREADPOOL_H__ */
