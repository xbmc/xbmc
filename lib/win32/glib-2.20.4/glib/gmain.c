/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gmain.c: Main loop abstraction, timeouts, and idle functions
 * Copyright 1998 Owen Taylor
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

/* 
 * MT safe
 */

#include "config.h"

/* Uncomment the next line (and the corresponding line in gpoll.c) to
 * enable debugging printouts if the environment variable
 * G_MAIN_POLL_DEBUG is set to some value.
 */
/* #define G_MAIN_POLL_DEBUG */

#ifdef _WIN32
/* Always enable debugging printout on Windows, as it is more often
 * needed there...
 */
#define G_MAIN_POLL_DEBUG
#endif

#define _GNU_SOURCE  /* for pipe2 */

#include "glib.h"
#include "gthreadprivate.h"
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <errno.h>

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#endif /* G_OS_WIN32 */

#ifdef G_OS_BEOS
#include <sys/socket.h>
#include <sys/wait.h>
#endif /* G_OS_BEOS */

#ifdef G_OS_UNIX
#include <fcntl.h>
#include <sys/wait.h>
#endif

#include "galias.h"

/* Types */

typedef struct _GTimeoutSource GTimeoutSource;
typedef struct _GChildWatchSource GChildWatchSource;
typedef struct _GPollRec GPollRec;
typedef struct _GSourceCallback GSourceCallback;

typedef enum
{
  G_SOURCE_READY = 1 << G_HOOK_FLAG_USER_SHIFT,
  G_SOURCE_CAN_RECURSE = 1 << (G_HOOK_FLAG_USER_SHIFT + 1)
} GSourceFlags;

#ifdef G_THREADS_ENABLED
typedef struct _GMainWaiter GMainWaiter;

struct _GMainWaiter
{
  GCond *cond;
  GMutex *mutex;
};
#endif  

typedef struct _GMainDispatch GMainDispatch;

struct _GMainDispatch
{
  gint depth;
  GSList *dispatching_sources; /* stack of current sources */
};

#ifdef G_MAIN_POLL_DEBUG
gboolean _g_main_poll_debug = FALSE;
#endif

struct _GMainContext
{
#ifdef G_THREADS_ENABLED
  /* The following lock is used for both the list of sources
   * and the list of poll records
   */
  GStaticMutex mutex;
  GCond *cond;
  GThread *owner;
  guint owner_count;
  GSList *waiters;
#endif  

  gint ref_count;

  GPtrArray *pending_dispatches;
  gint timeout;			/* Timeout for current iteration */

  guint next_id;
  GSource *source_list;
  gint in_check_or_prepare;

  GPollRec *poll_records;
  guint n_poll_records;
  GPollFD *cached_poll_array;
  guint cached_poll_array_size;

#ifdef G_THREADS_ENABLED  
#ifndef G_OS_WIN32
/* this pipe is used to wake up the main loop when a source is added.
 */
  gint wake_up_pipe[2];
#else /* G_OS_WIN32 */
  HANDLE wake_up_semaphore;
#endif /* G_OS_WIN32 */

  GPollFD wake_up_rec;
  gboolean poll_waiting;

/* Flag indicating whether the set of fd's changed during a poll */
  gboolean poll_changed;
#endif /* G_THREADS_ENABLED */

  GPollFunc poll_func;

  GTimeVal current_time;
  gboolean time_is_current;
};

struct _GSourceCallback
{
  guint ref_count;
  GSourceFunc func;
  gpointer    data;
  GDestroyNotify notify;
};

struct _GMainLoop
{
  GMainContext *context;
  gboolean is_running;
  gint ref_count;
};

struct _GTimeoutSource
{
  GSource     source;
  GTimeVal    expiration;
  guint       interval;
  guint	      granularity;
};

struct _GChildWatchSource
{
  GSource     source;
  GPid        pid;
  gint        child_status;
#ifdef G_OS_WIN32
  GPollFD     poll;
#else /* G_OS_WIN32 */
  gint        count;
  gboolean    child_exited;
#endif /* G_OS_WIN32 */
};

struct _GPollRec
{
  GPollFD *fd;
  GPollRec *next;
  gint priority;
};

#ifdef G_THREADS_ENABLED
#define LOCK_CONTEXT(context) g_static_mutex_lock (&context->mutex)
#define UNLOCK_CONTEXT(context) g_static_mutex_unlock (&context->mutex)
#define G_THREAD_SELF g_thread_self ()
#else
#define LOCK_CONTEXT(context) (void)0
#define UNLOCK_CONTEXT(context) (void)0
#define G_THREAD_SELF NULL
#endif

#define SOURCE_DESTROYED(source) (((source)->flags & G_HOOK_FLAG_ACTIVE) == 0)
#define SOURCE_BLOCKED(source) (((source)->flags & G_HOOK_FLAG_IN_CALL) != 0 && \
		                ((source)->flags & G_SOURCE_CAN_RECURSE) == 0)

#define SOURCE_UNREF(source, context)                       \
   G_STMT_START {                                           \
    if ((source)->ref_count > 1)                            \
      (source)->ref_count--;                                \
    else                                                    \
      g_source_unref_internal ((source), (context), TRUE);  \
   } G_STMT_END


/* Forward declarations */

static void g_source_unref_internal             (GSource      *source,
						 GMainContext *context,
						 gboolean      have_lock);
static void g_source_destroy_internal           (GSource      *source,
						 GMainContext *context,
						 gboolean      have_lock);
static void g_main_context_poll                 (GMainContext *context,
						 gint          timeout,
						 gint          priority,
						 GPollFD      *fds,
						 gint          n_fds);
static void g_main_context_add_poll_unlocked    (GMainContext *context,
						 gint          priority,
						 GPollFD      *fd);
static void g_main_context_remove_poll_unlocked (GMainContext *context,
						 GPollFD      *fd);
static void g_main_context_wakeup_unlocked      (GMainContext *context);

static gboolean g_timeout_prepare  (GSource     *source,
				    gint        *timeout);
static gboolean g_timeout_check    (GSource     *source);
static gboolean g_timeout_dispatch (GSource     *source,
				    GSourceFunc  callback,
				    gpointer     user_data);
static gboolean g_child_watch_prepare  (GSource     *source,
				        gint        *timeout);
static gboolean g_child_watch_check    (GSource     *source);
static gboolean g_child_watch_dispatch (GSource     *source,
					GSourceFunc  callback,
					gpointer     user_data);
static gboolean g_idle_prepare     (GSource     *source,
				    gint        *timeout);
static gboolean g_idle_check       (GSource     *source);
static gboolean g_idle_dispatch    (GSource     *source,
				    GSourceFunc  callback,
				    gpointer     user_data);

G_LOCK_DEFINE_STATIC (main_loop);
static GMainContext *default_main_context;
static GSList *main_contexts_without_pipe = NULL;

#ifndef G_OS_WIN32
/* Child status monitoring code */
enum {
  CHILD_WATCH_UNINITIALIZED,
  CHILD_WATCH_INITIALIZED_SINGLE,
  CHILD_WATCH_INITIALIZED_THREADED
};
static gint child_watch_init_state = CHILD_WATCH_UNINITIALIZED;
static gint child_watch_count = 1;
static gint child_watch_wake_up_pipe[2] = {0, 0};
#endif /* !G_OS_WIN32 */
G_LOCK_DEFINE_STATIC (main_context_list);
static GSList *main_context_list = NULL;

static gint timer_perturb = -1;

GSourceFuncs g_timeout_funcs =
{
  g_timeout_prepare,
  g_timeout_check,
  g_timeout_dispatch,
  NULL
};

GSourceFuncs g_child_watch_funcs =
{
  g_child_watch_prepare,
  g_child_watch_check,
  g_child_watch_dispatch,
  NULL
};

GSourceFuncs g_idle_funcs =
{
  g_idle_prepare,
  g_idle_check,
  g_idle_dispatch,
  NULL
};

/**
 * g_main_context_ref:
 * @context: a #GMainContext
 * 
 * Increases the reference count on a #GMainContext object by one.
 *
 * Returns: the @context that was passed in (since 2.6)
 **/
GMainContext *
g_main_context_ref (GMainContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (g_atomic_int_get (&context->ref_count) > 0, NULL); 

  g_atomic_int_inc (&context->ref_count);

  return context;
}

static inline void
poll_rec_list_free (GMainContext *context,
		    GPollRec     *list)
{
  g_slice_free_chain (GPollRec, list, next);
}

/**
 * g_main_context_unref:
 * @context: a #GMainContext
 * 
 * Decreases the reference count on a #GMainContext object by one. If
 * the result is zero, free the context and free all associated memory.
 **/
void
g_main_context_unref (GMainContext *context)
{
  GSource *source;
  g_return_if_fail (context != NULL);
  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0); 

  if (!g_atomic_int_dec_and_test (&context->ref_count))
    return;

  G_LOCK (main_context_list);
  main_context_list = g_slist_remove (main_context_list, context);
  G_UNLOCK (main_context_list);

  source = context->source_list;
  while (source)
    {
      GSource *next = source->next;
      g_source_destroy_internal (source, context, FALSE);
      source = next;
    }

#ifdef G_THREADS_ENABLED  
  g_static_mutex_free (&context->mutex);
#endif

  g_ptr_array_free (context->pending_dispatches, TRUE);
  g_free (context->cached_poll_array);

  poll_rec_list_free (context, context->poll_records);
  
#ifdef G_THREADS_ENABLED
  if (g_thread_supported())
    {
#ifndef G_OS_WIN32
      close (context->wake_up_pipe[0]);
      close (context->wake_up_pipe[1]);
#else
      CloseHandle (context->wake_up_semaphore);
#endif
    } 
  else
    main_contexts_without_pipe = g_slist_remove (main_contexts_without_pipe, 
						 context);

  if (context->cond != NULL)
    g_cond_free (context->cond);
#endif
  
  g_free (context);
}

#ifdef G_THREADS_ENABLED
static void 
g_main_context_init_pipe (GMainContext *context)
{
# ifndef G_OS_WIN32
  if (context->wake_up_pipe[0] != -1)
    return;

#ifdef HAVE_PIPE2
  /* if this fails, we fall through and try pipe */
  pipe2 (context->wake_up_pipe, O_CLOEXEC);
#endif
  if (context->wake_up_pipe[0] == -1)
    {
      if (pipe (context->wake_up_pipe) < 0)
        g_error ("Cannot create pipe main loop wake-up: %s\n",
  	         g_strerror (errno));
 
      fcntl (context->wake_up_pipe[0], F_SETFD, FD_CLOEXEC);
      fcntl (context->wake_up_pipe[1], F_SETFD, FD_CLOEXEC);
    }

  context->wake_up_rec.fd = context->wake_up_pipe[0];
  context->wake_up_rec.events = G_IO_IN;
# else
  if (context->wake_up_semaphore != NULL)
    return;
  context->wake_up_semaphore = CreateSemaphore (NULL, 0, 100, NULL);
  if (context->wake_up_semaphore == NULL)
    g_error ("Cannot create wake-up semaphore: %s",
	     g_win32_error_message (GetLastError ()));
  context->wake_up_rec.fd = (gintptr) context->wake_up_semaphore;
  context->wake_up_rec.events = G_IO_IN;

  if (_g_main_poll_debug)
    g_print ("wake-up semaphore: %p\n", context->wake_up_semaphore);
# endif
  g_main_context_add_poll_unlocked (context, 0, &context->wake_up_rec);
}

void
_g_main_thread_init (void)
{
  GSList *curr = main_contexts_without_pipe;
  while (curr)
    {
      g_main_context_init_pipe ((GMainContext *)curr->data);
      curr = curr->next;
    }
  g_slist_free (main_contexts_without_pipe);
  main_contexts_without_pipe = NULL;  
}
#endif /* G_THREADS_ENABLED */

/**
 * g_main_context_new:
 * 
 * Creates a new #GMainContext structure.
 * 
 * Return value: the new #GMainContext
 **/
GMainContext *
g_main_context_new (void)
{
  GMainContext *context = g_new0 (GMainContext, 1);

#ifdef G_MAIN_POLL_DEBUG
  {
    static gboolean beenhere = FALSE;

    if (!beenhere)
      {
	if (getenv ("G_MAIN_POLL_DEBUG") != NULL)
	  _g_main_poll_debug = TRUE;
	beenhere = TRUE;
      }
  }
#endif

#ifdef G_THREADS_ENABLED
  g_static_mutex_init (&context->mutex);

  context->owner = NULL;
  context->waiters = NULL;

# ifndef G_OS_WIN32
  context->wake_up_pipe[0] = -1;
  context->wake_up_pipe[1] = -1;
# else
  context->wake_up_semaphore = NULL;
# endif
#endif

  context->ref_count = 1;

  context->next_id = 1;
  
  context->source_list = NULL;
  
  context->poll_func = g_poll;
  
  context->cached_poll_array = NULL;
  context->cached_poll_array_size = 0;
  
  context->pending_dispatches = g_ptr_array_new ();
  
  context->time_is_current = FALSE;
  
#ifdef G_THREADS_ENABLED
  if (g_thread_supported ())
    g_main_context_init_pipe (context);
  else
    main_contexts_without_pipe = g_slist_prepend (main_contexts_without_pipe, 
						  context);
#endif

  G_LOCK (main_context_list);
  main_context_list = g_slist_append (main_context_list, context);

#ifdef G_MAIN_POLL_DEBUG
  if (_g_main_poll_debug)
    g_print ("created context=%p\n", context);
#endif

  G_UNLOCK (main_context_list);

  return context;
}

/**
 * g_main_context_default:
 * 
 * Returns the default main context. This is the main context used
 * for main loop functions when a main loop is not explicitly
 * specified.
 * 
 * Return value: the default main context.
 **/
GMainContext *
g_main_context_default (void)
{
  /* Slow, but safe */
  
  G_LOCK (main_loop);

  if (!default_main_context)
    {
      default_main_context = g_main_context_new ();
#ifdef G_MAIN_POLL_DEBUG
      if (_g_main_poll_debug)
	g_print ("default context=%p\n", default_main_context);
#endif
    }

  G_UNLOCK (main_loop);

  return default_main_context;
}

/* Hooks for adding to the main loop */

/**
 * g_source_new:
 * @source_funcs: structure containing functions that implement
 *                the sources behavior.
 * @struct_size: size of the #GSource structure to create.
 * 
 * Creates a new #GSource structure. The size is specified to
 * allow creating structures derived from #GSource that contain
 * additional data. The size passed in must be at least
 * <literal>sizeof (GSource)</literal>.
 * 
 * The source will not initially be associated with any #GMainContext
 * and must be added to one with g_source_attach() before it will be
 * executed.
 * 
 * Return value: the newly-created #GSource.
 **/
GSource *
g_source_new (GSourceFuncs *source_funcs,
	      guint         struct_size)
{
  GSource *source;

  g_return_val_if_fail (source_funcs != NULL, NULL);
  g_return_val_if_fail (struct_size >= sizeof (GSource), NULL);
  
  source = (GSource*) g_malloc0 (struct_size);

  source->source_funcs = source_funcs;
  source->ref_count = 1;
  
  source->priority = G_PRIORITY_DEFAULT;

  source->flags = G_HOOK_FLAG_ACTIVE;

  /* NULL/0 initialization for all other fields */
  
  return source;
}

/* Holds context's lock
 */
static void
g_source_list_add (GSource      *source,
		   GMainContext *context)
{
  GSource *tmp_source, *last_source;
  
  last_source = NULL;
  tmp_source = context->source_list;
  while (tmp_source && tmp_source->priority <= source->priority)
    {
      last_source = tmp_source;
      tmp_source = tmp_source->next;
    }

  source->next = tmp_source;
  if (tmp_source)
    tmp_source->prev = source;
  
  source->prev = last_source;
  if (last_source)
    last_source->next = source;
  else
    context->source_list = source;
}

/* Holds context's lock
 */
static void
g_source_list_remove (GSource      *source,
		      GMainContext *context)
{
  if (source->prev)
    source->prev->next = source->next;
  else
    context->source_list = source->next;

  if (source->next)
    source->next->prev = source->prev;

  source->prev = NULL;
  source->next = NULL;
}

/**
 * g_source_attach:
 * @source: a #GSource
 * @context: a #GMainContext (if %NULL, the default context will be used)
 * 
 * Adds a #GSource to a @context so that it will be executed within
 * that context. Remove it by calling g_source_destroy().
 *
 * Return value: the ID (greater than 0) for the source within the 
 *   #GMainContext. 
 **/
guint
g_source_attach (GSource      *source,
		 GMainContext *context)
{
  guint result = 0;
  GSList *tmp_list;

  g_return_val_if_fail (source->context == NULL, 0);
  g_return_val_if_fail (!SOURCE_DESTROYED (source), 0);
  
  if (!context)
    context = g_main_context_default ();

  LOCK_CONTEXT (context);

  source->context = context;
  result = source->source_id = context->next_id++;

  source->ref_count++;
  g_source_list_add (source, context);

  tmp_list = source->poll_fds;
  while (tmp_list)
    {
      g_main_context_add_poll_unlocked (context, source->priority, tmp_list->data);
      tmp_list = tmp_list->next;
    }

#ifdef G_THREADS_ENABLED
  /* Now wake up the main loop if it is waiting in the poll() */
  g_main_context_wakeup_unlocked (context);
#endif

  UNLOCK_CONTEXT (context);

  return result;
}

static void
g_source_destroy_internal (GSource      *source,
			   GMainContext *context,
			   gboolean      have_lock)
{
  if (!have_lock)
    LOCK_CONTEXT (context);
  
  if (!SOURCE_DESTROYED (source))
    {
      GSList *tmp_list;
      gpointer old_cb_data;
      GSourceCallbackFuncs *old_cb_funcs;
      
      source->flags &= ~G_HOOK_FLAG_ACTIVE;

      old_cb_data = source->callback_data;
      old_cb_funcs = source->callback_funcs;

      source->callback_data = NULL;
      source->callback_funcs = NULL;

      if (old_cb_funcs)
	{
	  UNLOCK_CONTEXT (context);
	  old_cb_funcs->unref (old_cb_data);
	  LOCK_CONTEXT (context);
	}

      if (!SOURCE_BLOCKED (source))
	{
	  tmp_list = source->poll_fds;
	  while (tmp_list)
	    {
	      g_main_context_remove_poll_unlocked (context, tmp_list->data);
	      tmp_list = tmp_list->next;
	    }
	}
	  
      g_source_unref_internal (source, context, TRUE);
    }

  if (!have_lock)
    UNLOCK_CONTEXT (context);
}

/**
 * g_source_destroy:
 * @source: a #GSource
 * 
 * Removes a source from its #GMainContext, if any, and mark it as
 * destroyed.  The source cannot be subsequently added to another
 * context.
 **/
void
g_source_destroy (GSource *source)
{
  GMainContext *context;
  
  g_return_if_fail (source != NULL);
  
  context = source->context;
  
  if (context)
    g_source_destroy_internal (source, context, FALSE);
  else
    source->flags &= ~G_HOOK_FLAG_ACTIVE;
}

/**
 * g_source_get_id:
 * @source: a #GSource
 * 
 * Returns the numeric ID for a particular source. The ID of a source
 * is a positive integer which is unique within a particular main loop 
 * context. The reverse
 * mapping from ID to source is done by g_main_context_find_source_by_id().
 *
 * Return value: the ID (greater than 0) for the source
 **/
guint
g_source_get_id (GSource *source)
{
  guint result;
  
  g_return_val_if_fail (source != NULL, 0);
  g_return_val_if_fail (source->context != NULL, 0);

  LOCK_CONTEXT (source->context);
  result = source->source_id;
  UNLOCK_CONTEXT (source->context);
  
  return result;
}

/**
 * g_source_get_context:
 * @source: a #GSource
 * 
 * Gets the #GMainContext with which the source is associated.
 * Calling this function on a destroyed source is an error.
 * 
 * Return value: the #GMainContext with which the source is associated,
 *               or %NULL if the context has not yet been added
 *               to a source.
 **/
GMainContext *
g_source_get_context (GSource *source)
{
  g_return_val_if_fail (!SOURCE_DESTROYED (source), NULL);

  return source->context;
}

/**
 * g_source_add_poll:
 * @source:a #GSource 
 * @fd: a #GPollFD structure holding information about a file
 *      descriptor to watch.
 * 
 * Adds a file descriptor to the set of file descriptors polled for
 * this source. This is usually combined with g_source_new() to add an
 * event source. The event source's check function will typically test
 * the @revents field in the #GPollFD struct and return %TRUE if events need
 * to be processed.
 **/
void
g_source_add_poll (GSource *source,
		   GPollFD *fd)
{
  GMainContext *context;
  
  g_return_if_fail (source != NULL);
  g_return_if_fail (fd != NULL);
  g_return_if_fail (!SOURCE_DESTROYED (source));
  
  context = source->context;

  if (context)
    LOCK_CONTEXT (context);
  
  source->poll_fds = g_slist_prepend (source->poll_fds, fd);

  if (context)
    {
      if (!SOURCE_BLOCKED (source))
	g_main_context_add_poll_unlocked (context, source->priority, fd);
      UNLOCK_CONTEXT (context);
    }
}

/**
 * g_source_remove_poll:
 * @source:a #GSource 
 * @fd: a #GPollFD structure previously passed to g_source_add_poll().
 * 
 * Removes a file descriptor from the set of file descriptors polled for
 * this source. 
 **/
void
g_source_remove_poll (GSource *source,
		      GPollFD *fd)
{
  GMainContext *context;
  
  g_return_if_fail (source != NULL);
  g_return_if_fail (fd != NULL);
  g_return_if_fail (!SOURCE_DESTROYED (source));
  
  context = source->context;

  if (context)
    LOCK_CONTEXT (context);
  
  source->poll_fds = g_slist_remove (source->poll_fds, fd);

  if (context)
    {
      if (!SOURCE_BLOCKED (source))
	g_main_context_remove_poll_unlocked (context, fd);
      UNLOCK_CONTEXT (context);
    }
}

/**
 * g_source_set_callback_indirect:
 * @source: the source
 * @callback_data: pointer to callback data "object"
 * @callback_funcs: functions for reference counting @callback_data
 *                  and getting the callback and data
 * 
 * Sets the callback function storing the data as a refcounted callback
 * "object". This is used internally. Note that calling 
 * g_source_set_callback_indirect() assumes
 * an initial reference count on @callback_data, and thus
 * @callback_funcs->unref will eventually be called once more
 * than @callback_funcs->ref.
 **/
void
g_source_set_callback_indirect (GSource              *source,
				gpointer              callback_data,
				GSourceCallbackFuncs *callback_funcs)
{
  GMainContext *context;
  gpointer old_cb_data;
  GSourceCallbackFuncs *old_cb_funcs;
  
  g_return_if_fail (source != NULL);
  g_return_if_fail (callback_funcs != NULL || callback_data == NULL);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  old_cb_data = source->callback_data;
  old_cb_funcs = source->callback_funcs;

  source->callback_data = callback_data;
  source->callback_funcs = callback_funcs;
  
  if (context)
    UNLOCK_CONTEXT (context);
  
  if (old_cb_funcs)
    old_cb_funcs->unref (old_cb_data);
}

static void
g_source_callback_ref (gpointer cb_data)
{
  GSourceCallback *callback = cb_data;

  callback->ref_count++;
}


static void
g_source_callback_unref (gpointer cb_data)
{
  GSourceCallback *callback = cb_data;

  callback->ref_count--;
  if (callback->ref_count == 0)
    {
      if (callback->notify)
	callback->notify (callback->data);
      g_free (callback);
    }
}

static void
g_source_callback_get (gpointer     cb_data,
		       GSource     *source, 
		       GSourceFunc *func,
		       gpointer    *data)
{
  GSourceCallback *callback = cb_data;

  *func = callback->func;
  *data = callback->data;
}

static GSourceCallbackFuncs g_source_callback_funcs = {
  g_source_callback_ref,
  g_source_callback_unref,
  g_source_callback_get,
};

/**
 * g_source_set_callback:
 * @source: the source
 * @func: a callback function
 * @data: the data to pass to callback function
 * @notify: a function to call when @data is no longer in use, or %NULL.
 * 
 * Sets the callback function for a source. The callback for a source is
 * called from the source's dispatch function.
 *
 * The exact type of @func depends on the type of source; ie. you
 * should not count on @func being called with @data as its first
 * parameter.
 * 
 * Typically, you won't use this function. Instead use functions specific
 * to the type of source you are using.
 **/
void
g_source_set_callback (GSource        *source,
		       GSourceFunc     func,
		       gpointer        data,
		       GDestroyNotify  notify)
{
  GSourceCallback *new_callback;

  g_return_if_fail (source != NULL);

  new_callback = g_new (GSourceCallback, 1);

  new_callback->ref_count = 1;
  new_callback->func = func;
  new_callback->data = data;
  new_callback->notify = notify;

  g_source_set_callback_indirect (source, new_callback, &g_source_callback_funcs);
}


/**
 * g_source_set_funcs:
 * @source: a #GSource
 * @funcs: the new #GSourceFuncs
 * 
 * Sets the source functions (can be used to override 
 * default implementations) of an unattached source.
 * 
 * Since: 2.12
 */
void
g_source_set_funcs (GSource     *source,
	           GSourceFuncs *funcs)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (source->context == NULL);
  g_return_if_fail (source->ref_count > 0);
  g_return_if_fail (funcs != NULL);

  source->source_funcs = funcs;
}

/**
 * g_source_set_priority:
 * @source: a #GSource
 * @priority: the new priority.
 * 
 * Sets the priority of a source. While the main loop is being
 * run, a source will be dispatched if it is ready to be dispatched and no sources 
 * at a higher (numerically smaller) priority are ready to be dispatched.
 **/
void
g_source_set_priority (GSource  *source,
		       gint      priority)
{
  GSList *tmp_list;
  GMainContext *context;
  
  g_return_if_fail (source != NULL);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);
  
  source->priority = priority;

  if (context)
    {
      /* Remove the source from the context's source and then
       * add it back so it is sorted in the correct plcae
       */
      g_source_list_remove (source, source->context);
      g_source_list_add (source, source->context);

      if (!SOURCE_BLOCKED (source))
	{
	  tmp_list = source->poll_fds;
	  while (tmp_list)
	    {
	      g_main_context_remove_poll_unlocked (context, tmp_list->data);
	      g_main_context_add_poll_unlocked (context, priority, tmp_list->data);
	      
	      tmp_list = tmp_list->next;
	    }
	}
      
      UNLOCK_CONTEXT (source->context);
    }
}

/**
 * g_source_get_priority:
 * @source: a #GSource
 * 
 * Gets the priority of a source.
 * 
 * Return value: the priority of the source
 **/
gint
g_source_get_priority (GSource *source)
{
  g_return_val_if_fail (source != NULL, 0);

  return source->priority;
}

/**
 * g_source_set_can_recurse:
 * @source: a #GSource
 * @can_recurse: whether recursion is allowed for this source
 * 
 * Sets whether a source can be called recursively. If @can_recurse is
 * %TRUE, then while the source is being dispatched then this source
 * will be processed normally. Otherwise, all processing of this
 * source is blocked until the dispatch function returns.
 **/
void
g_source_set_can_recurse (GSource  *source,
			  gboolean  can_recurse)
{
  GMainContext *context;
  
  g_return_if_fail (source != NULL);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);
  
  if (can_recurse)
    source->flags |= G_SOURCE_CAN_RECURSE;
  else
    source->flags &= ~G_SOURCE_CAN_RECURSE;

  if (context)
    UNLOCK_CONTEXT (context);
}

/**
 * g_source_get_can_recurse:
 * @source: a #GSource
 * 
 * Checks whether a source is allowed to be called recursively.
 * see g_source_set_can_recurse().
 * 
 * Return value: whether recursion is allowed.
 **/
gboolean
g_source_get_can_recurse (GSource  *source)
{
  g_return_val_if_fail (source != NULL, FALSE);
  
  return (source->flags & G_SOURCE_CAN_RECURSE) != 0;
}

/**
 * g_source_ref:
 * @source: a #GSource
 * 
 * Increases the reference count on a source by one.
 * 
 * Return value: @source
 **/
GSource *
g_source_ref (GSource *source)
{
  GMainContext *context;
  
  g_return_val_if_fail (source != NULL, NULL);

  context = source->context;

  if (context)
    LOCK_CONTEXT (context);

  source->ref_count++;

  if (context)
    UNLOCK_CONTEXT (context);

  return source;
}

/* g_source_unref() but possible to call within context lock
 */
static void
g_source_unref_internal (GSource      *source,
			 GMainContext *context,
			 gboolean      have_lock)
{
  gpointer old_cb_data = NULL;
  GSourceCallbackFuncs *old_cb_funcs = NULL;

  g_return_if_fail (source != NULL);
  
  if (!have_lock && context)
    LOCK_CONTEXT (context);

  source->ref_count--;
  if (source->ref_count == 0)
    {
      old_cb_data = source->callback_data;
      old_cb_funcs = source->callback_funcs;

      source->callback_data = NULL;
      source->callback_funcs = NULL;

      if (context && !SOURCE_DESTROYED (source))
	{
	  g_warning (G_STRLOC ": ref_count == 0, but source is still attached to a context!");
	  source->ref_count++;
	}
      else if (context)
	g_source_list_remove (source, context);

      if (source->source_funcs->finalize)
	source->source_funcs->finalize (source);
      
      g_slist_free (source->poll_fds);
      source->poll_fds = NULL;
      g_free (source);
    }
  
  if (!have_lock && context)
    UNLOCK_CONTEXT (context);

  if (old_cb_funcs)
    {
      if (have_lock)
	UNLOCK_CONTEXT (context);
      
      old_cb_funcs->unref (old_cb_data);

      if (have_lock)
	LOCK_CONTEXT (context);
    }
}

/**
 * g_source_unref:
 * @source: a #GSource
 * 
 * Decreases the reference count of a source by one. If the
 * resulting reference count is zero the source and associated
 * memory will be destroyed. 
 **/
void
g_source_unref (GSource *source)
{
  g_return_if_fail (source != NULL);

  g_source_unref_internal (source, source->context, FALSE);
}

/**
 * g_main_context_find_source_by_id:
 * @context: a #GMainContext (if %NULL, the default context will be used)
 * @source_id: the source ID, as returned by g_source_get_id(). 
 * 
 * Finds a #GSource given a pair of context and ID.
 * 
 * Return value: the #GSource if found, otherwise, %NULL
 **/
GSource *
g_main_context_find_source_by_id (GMainContext *context,
				  guint         source_id)
{
  GSource *source;
  
  g_return_val_if_fail (source_id > 0, NULL);

  if (context == NULL)
    context = g_main_context_default ();
  
  LOCK_CONTEXT (context);
  
  source = context->source_list;
  while (source)
    {
      if (!SOURCE_DESTROYED (source) &&
	  source->source_id == source_id)
	break;
      source = source->next;
    }

  UNLOCK_CONTEXT (context);

  return source;
}

/**
 * g_main_context_find_source_by_funcs_user_data:
 * @context: a #GMainContext (if %NULL, the default context will be used).
 * @funcs: the @source_funcs passed to g_source_new().
 * @user_data: the user data from the callback.
 * 
 * Finds a source with the given source functions and user data.  If
 * multiple sources exist with the same source function and user data,
 * the first one found will be returned.
 * 
 * Return value: the source, if one was found, otherwise %NULL
 **/
GSource *
g_main_context_find_source_by_funcs_user_data (GMainContext *context,
					       GSourceFuncs *funcs,
					       gpointer      user_data)
{
  GSource *source;
  
  g_return_val_if_fail (funcs != NULL, NULL);

  if (context == NULL)
    context = g_main_context_default ();
  
  LOCK_CONTEXT (context);

  source = context->source_list;
  while (source)
    {
      if (!SOURCE_DESTROYED (source) &&
	  source->source_funcs == funcs &&
	  source->callback_funcs)
	{
	  GSourceFunc callback;
	  gpointer callback_data;

	  source->callback_funcs->get (source->callback_data, source, &callback, &callback_data);
	  
	  if (callback_data == user_data)
	    break;
	}
      source = source->next;
    }

  UNLOCK_CONTEXT (context);

  return source;
}

/**
 * g_main_context_find_source_by_user_data:
 * @context: a #GMainContext
 * @user_data: the user_data for the callback.
 * 
 * Finds a source with the given user data for the callback.  If
 * multiple sources exist with the same user data, the first
 * one found will be returned.
 * 
 * Return value: the source, if one was found, otherwise %NULL
 **/
GSource *
g_main_context_find_source_by_user_data (GMainContext *context,
					 gpointer      user_data)
{
  GSource *source;
  
  if (context == NULL)
    context = g_main_context_default ();
  
  LOCK_CONTEXT (context);

  source = context->source_list;
  while (source)
    {
      if (!SOURCE_DESTROYED (source) &&
	  source->callback_funcs)
	{
	  GSourceFunc callback;
	  gpointer callback_data = NULL;

	  source->callback_funcs->get (source->callback_data, source, &callback, &callback_data);

	  if (callback_data == user_data)
	    break;
	}
      source = source->next;
    }

  UNLOCK_CONTEXT (context);

  return source;
}

/**
 * g_source_remove:
 * @tag: the ID of the source to remove.
 * 
 * Removes the source with the given id from the default main context. 
 * The id of
 * a #GSource is given by g_source_get_id(), or will be returned by the
 * functions g_source_attach(), g_idle_add(), g_idle_add_full(),
 * g_timeout_add(), g_timeout_add_full(), g_child_watch_add(),
 * g_child_watch_add_full(), g_io_add_watch(), and g_io_add_watch_full().
 *
 * See also g_source_destroy(). You must use g_source_destroy() for sources
 * added to a non-default main context.
 *
 * Return value: %TRUE if the source was found and removed.
 **/
gboolean
g_source_remove (guint tag)
{
  GSource *source;
  
  g_return_val_if_fail (tag > 0, FALSE);

  source = g_main_context_find_source_by_id (NULL, tag);
  if (source)
    g_source_destroy (source);

  return source != NULL;
}

/**
 * g_source_remove_by_user_data:
 * @user_data: the user_data for the callback.
 * 
 * Removes a source from the default main loop context given the user
 * data for the callback. If multiple sources exist with the same user
 * data, only one will be destroyed.
 * 
 * Return value: %TRUE if a source was found and removed. 
 **/
gboolean
g_source_remove_by_user_data (gpointer user_data)
{
  GSource *source;
  
  source = g_main_context_find_source_by_user_data (NULL, user_data);
  if (source)
    {
      g_source_destroy (source);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * g_source_remove_by_funcs_user_data:
 * @funcs: The @source_funcs passed to g_source_new()
 * @user_data: the user data for the callback
 * 
 * Removes a source from the default main loop context given the
 * source functions and user data. If multiple sources exist with the
 * same source functions and user data, only one will be destroyed.
 * 
 * Return value: %TRUE if a source was found and removed. 
 **/
gboolean
g_source_remove_by_funcs_user_data (GSourceFuncs *funcs,
				    gpointer      user_data)
{
  GSource *source;

  g_return_val_if_fail (funcs != NULL, FALSE);

  source = g_main_context_find_source_by_funcs_user_data (NULL, funcs, user_data);
  if (source)
    {
      g_source_destroy (source);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * g_get_current_time:
 * @result: #GTimeVal structure in which to store current time.
 * 
 * Equivalent to the UNIX gettimeofday() function, but portable.
 **/
void
g_get_current_time (GTimeVal *result)
{
#ifndef G_OS_WIN32
  struct timeval r;

  g_return_if_fail (result != NULL);

  /*this is required on alpha, there the timeval structs are int's
    not longs and a cast only would fail horribly*/
  gettimeofday (&r, NULL);
  result->tv_sec = r.tv_sec;
  result->tv_usec = r.tv_usec;
#else
  FILETIME ft;
  guint64 time64;

  g_return_if_fail (result != NULL);

  GetSystemTimeAsFileTime (&ft);
  memmove (&time64, &ft, sizeof (FILETIME));

  /* Convert from 100s of nanoseconds since 1601-01-01
   * to Unix epoch. Yes, this is Y2038 unsafe.
   */
  time64 -= G_GINT64_CONSTANT (116444736000000000);
  time64 /= 10;

  result->tv_sec = time64 / 1000000;
  result->tv_usec = time64 % 1000000;
#endif
}

static void
g_main_dispatch_free (gpointer dispatch)
{
  g_slice_free (GMainDispatch, dispatch);
}

/* Running the main loop */

static GMainDispatch *
get_dispatch (void)
{
  static GStaticPrivate depth_private = G_STATIC_PRIVATE_INIT;
  GMainDispatch *dispatch = g_static_private_get (&depth_private);
  if (!dispatch)
    {
      dispatch = g_slice_new0 (GMainDispatch);
      g_static_private_set (&depth_private, dispatch, g_main_dispatch_free);
    }

  return dispatch;
}

/**
 * g_main_depth:
 *
 * Returns the depth of the stack of calls to
 * g_main_context_dispatch() on any #GMainContext in the current thread.
 *  That is, when called from the toplevel, it gives 0. When
 * called from within a callback from g_main_context_iteration()
 * (or g_main_loop_run(), etc.) it returns 1. When called from within 
 * a callback to a recursive call to g_main_context_iterate(),
 * it returns 2. And so forth.
 *
 * This function is useful in a situation like the following:
 * Imagine an extremely simple "garbage collected" system.
 *
 * |[
 * static GList *free_list;
 * 
 * gpointer
 * allocate_memory (gsize size)
 * { 
 *   gpointer result = g_malloc (size);
 *   free_list = g_list_prepend (free_list, result);
 *   return result;
 * }
 * 
 * void
 * free_allocated_memory (void)
 * {
 *   GList *l;
 *   for (l = free_list; l; l = l->next);
 *     g_free (l->data);
 *   g_list_free (free_list);
 *   free_list = NULL;
 *  }
 * 
 * [...]
 * 
 * while (TRUE); 
 *  {
 *    g_main_context_iteration (NULL, TRUE);
 *    free_allocated_memory();
 *   }
 * ]|
 *
 * This works from an application, however, if you want to do the same
 * thing from a library, it gets more difficult, since you no longer
 * control the main loop. You might think you can simply use an idle
 * function to make the call to free_allocated_memory(), but that
 * doesn't work, since the idle function could be called from a
 * recursive callback. This can be fixed by using g_main_depth()
 *
 * |[
 * gpointer
 * allocate_memory (gsize size)
 * { 
 *   FreeListBlock *block = g_new (FreeListBlock, 1);
 *   block->mem = g_malloc (size);
 *   block->depth = g_main_depth ();   
 *   free_list = g_list_prepend (free_list, block);
 *   return block->mem;
 * }
 * 
 * void
 * free_allocated_memory (void)
 * {
 *   GList *l;
 *   
 *   int depth = g_main_depth ();
 *   for (l = free_list; l; );
 *     {
 *       GList *next = l->next;
 *       FreeListBlock *block = l->data;
 *       if (block->depth > depth)
 *         {
 *           g_free (block->mem);
 *           g_free (block);
 *           free_list = g_list_delete_link (free_list, l);
 *         }
 *               
 *       l = next;
 *     }
 *   }
 * ]|
 *
 * There is a temptation to use g_main_depth() to solve
 * problems with reentrancy. For instance, while waiting for data
 * to be received from the network in response to a menu item,
 * the menu item might be selected again. It might seem that
 * one could make the menu item's callback return immediately
 * and do nothing if g_main_depth() returns a value greater than 1.
 * However, this should be avoided since the user then sees selecting
 * the menu item do nothing. Furthermore, you'll find yourself adding
 * these checks all over your code, since there are doubtless many,
 * many things that the user could do. Instead, you can use the
 * following techniques:
 *
 * <orderedlist>
 *  <listitem>
 *   <para>
 *     Use gtk_widget_set_sensitive() or modal dialogs to prevent
 *     the user from interacting with elements while the main
 *     loop is recursing.
 *   </para>
 *  </listitem>
 *  <listitem>
 *   <para>
 *     Avoid main loop recursion in situations where you can't handle
 *     arbitrary  callbacks. Instead, structure your code so that you
 *     simply return to the main loop and then get called again when
 *     there is more work to do.
 *   </para>
 *  </listitem>
 * </orderedlist>
 * 
 * Return value: The main loop recursion level in the current thread
 **/
int
g_main_depth (void)
{
  GMainDispatch *dispatch = get_dispatch ();
  return dispatch->depth;
}

/**
 * g_main_current_source:
 *
 * Returns the currently firing source for this thread.
 * 
 * Return value: The currently firing source or %NULL.
 *
 * Since: 2.12
 */
GSource *
g_main_current_source (void)
{
  GMainDispatch *dispatch = get_dispatch ();
  return dispatch->dispatching_sources ? dispatch->dispatching_sources->data : NULL;
}

/**
 * g_source_is_destroyed:
 * @source: a #GSource
 *
 * Returns whether @source has been destroyed.
 *
 * This is important when you operate upon your objects 
 * from within idle handlers, but may have freed the object 
 * before the dispatch of your idle handler.
 *
 * |[
 * static gboolean 
 * idle_callback (gpointer data)
 * {
 *   SomeWidget *self = data;
 *    
 *   GDK_THREADS_ENTER (<!-- -->);
 *   /<!-- -->* do stuff with self *<!-- -->/
 *   self->idle_id = 0;
 *   GDK_THREADS_LEAVE (<!-- -->);
 *    
 *   return FALSE;
 * }
 *  
 * static void 
 * some_widget_do_stuff_later (SomeWidget *self)
 * {
 *   self->idle_id = g_idle_add (idle_callback, self);
 * }
 *  
 * static void 
 * some_widget_finalize (GObject *object)
 * {
 *   SomeWidget *self = SOME_WIDGET (object);
 *    
 *   if (self->idle_id)
 *     g_source_remove (self->idle_id);
 *    
 *   G_OBJECT_CLASS (parent_class)->finalize (object);
 * }
 * ]|
 *
 * This will fail in a multi-threaded application if the 
 * widget is destroyed before the idle handler fires due 
 * to the use after free in the callback. A solution, to 
 * this particular problem, is to check to if the source
 * has already been destroy within the callback.
 *
 * |[
 * static gboolean 
 * idle_callback (gpointer data)
 * {
 *   SomeWidget *self = data;
 *   
 *   GDK_THREADS_ENTER ();
 *   if (!g_source_is_destroyed (g_main_current_source ()))
 *     {
 *       /<!-- -->* do stuff with self *<!-- -->/
 *     }
 *   GDK_THREADS_LEAVE ();
 *   
 *   return FALSE;
 * }
 * ]|
 *
 * Return value: %TRUE if the source has been destroyed
 *
 * Since: 2.12
 */
gboolean
g_source_is_destroyed (GSource *source)
{
  return SOURCE_DESTROYED (source);
}


/* Temporarily remove all this source's file descriptors from the
 * poll(), so that if data comes available for one of the file descriptors
 * we don't continually spin in the poll()
 */
/* HOLDS: source->context's lock */
static void
block_source (GSource *source)
{
  GSList *tmp_list;

  g_return_if_fail (!SOURCE_BLOCKED (source));

  tmp_list = source->poll_fds;
  while (tmp_list)
    {
      g_main_context_remove_poll_unlocked (source->context, tmp_list->data);
      tmp_list = tmp_list->next;
    }
}

/* HOLDS: source->context's lock */
static void
unblock_source (GSource *source)
{
  GSList *tmp_list;
  
  g_return_if_fail (!SOURCE_BLOCKED (source)); /* Source already unblocked */
  g_return_if_fail (!SOURCE_DESTROYED (source));
  
  tmp_list = source->poll_fds;
  while (tmp_list)
    {
      g_main_context_add_poll_unlocked (source->context, source->priority, tmp_list->data);
      tmp_list = tmp_list->next;
    }
}

/* HOLDS: context's lock */
static void
g_main_dispatch (GMainContext *context)
{
  GMainDispatch *current = get_dispatch ();
  guint i;

  for (i = 0; i < context->pending_dispatches->len; i++)
    {
      GSource *source = context->pending_dispatches->pdata[i];

      context->pending_dispatches->pdata[i] = NULL;
      g_assert (source);

      source->flags &= ~G_SOURCE_READY;

      if (!SOURCE_DESTROYED (source))
	{
	  gboolean was_in_call;
	  gpointer user_data = NULL;
	  GSourceFunc callback = NULL;
	  GSourceCallbackFuncs *cb_funcs;
	  gpointer cb_data;
	  gboolean need_destroy;

	  gboolean (*dispatch) (GSource *,
				GSourceFunc,
				gpointer);
	  GSList current_source_link;

	  dispatch = source->source_funcs->dispatch;
	  cb_funcs = source->callback_funcs;
	  cb_data = source->callback_data;

	  if (cb_funcs)
	    cb_funcs->ref (cb_data);
	  
	  if ((source->flags & G_SOURCE_CAN_RECURSE) == 0)
	    block_source (source);
	  
	  was_in_call = source->flags & G_HOOK_FLAG_IN_CALL;
	  source->flags |= G_HOOK_FLAG_IN_CALL;

	  if (cb_funcs)
	    cb_funcs->get (cb_data, source, &callback, &user_data);

	  UNLOCK_CONTEXT (context);

	  current->depth++;
	  /* The on-stack allocation of the GSList is unconventional, but
	   * we know that the lifetime of the link is bounded to this
	   * function as the link is kept in a thread specific list and
	   * not manipulated outside of this function and its descendants.
	   * Avoiding the overhead of a g_slist_alloc() is useful as many
	   * applications do little more than dispatch events.
	   *
	   * This is a performance hack - do not revert to g_slist_prepend()!
	   */
	  current_source_link.data = source;
	  current_source_link.next = current->dispatching_sources;
	  current->dispatching_sources = &current_source_link;
	  need_destroy = ! dispatch (source,
				     callback,
				     user_data);
	  g_assert (current->dispatching_sources == &current_source_link);
	  current->dispatching_sources = current_source_link.next;
	  current->depth--;
	  
	  if (cb_funcs)
	    cb_funcs->unref (cb_data);

 	  LOCK_CONTEXT (context);
	  
	  if (!was_in_call)
	    source->flags &= ~G_HOOK_FLAG_IN_CALL;

	  if ((source->flags & G_SOURCE_CAN_RECURSE) == 0 &&
	      !SOURCE_DESTROYED (source))
	    unblock_source (source);
	  
	  /* Note: this depends on the fact that we can't switch
	   * sources from one main context to another
	   */
	  if (need_destroy && !SOURCE_DESTROYED (source))
	    {
	      g_assert (source->context == context);
	      g_source_destroy_internal (source, context, TRUE);
	    }
	}
      
      SOURCE_UNREF (source, context);
    }

  g_ptr_array_set_size (context->pending_dispatches, 0);
}

/* Holds context's lock */
static inline GSource *
next_valid_source (GMainContext *context,
		   GSource      *source)
{
  GSource *new_source = source ? source->next : context->source_list;

  while (new_source)
    {
      if (!SOURCE_DESTROYED (new_source))
	{
	  new_source->ref_count++;
	  break;
	}
      
      new_source = new_source->next;
    }

  if (source)
    SOURCE_UNREF (source, context);
	  
  return new_source;
}

/**
 * g_main_context_acquire:
 * @context: a #GMainContext
 * 
 * Tries to become the owner of the specified context.
 * If some other thread is the owner of the context,
 * returns %FALSE immediately. Ownership is properly
 * recursive: the owner can require ownership again
 * and will release ownership when g_main_context_release()
 * is called as many times as g_main_context_acquire().
 *
 * You must be the owner of a context before you
 * can call g_main_context_prepare(), g_main_context_query(),
 * g_main_context_check(), g_main_context_dispatch().
 * 
 * Return value: %TRUE if the operation succeeded, and
 *   this thread is now the owner of @context.
 **/
gboolean 
g_main_context_acquire (GMainContext *context)
{
#ifdef G_THREADS_ENABLED
  gboolean result = FALSE;
  GThread *self = G_THREAD_SELF;

  if (context == NULL)
    context = g_main_context_default ();
  
  LOCK_CONTEXT (context);

  if (!context->owner)
    {
      context->owner = self;
      g_assert (context->owner_count == 0);
    }

  if (context->owner == self)
    {
      context->owner_count++;
      result = TRUE;
    }

  UNLOCK_CONTEXT (context); 
  
  return result;
#else /* !G_THREADS_ENABLED */
  return TRUE;
#endif /* G_THREADS_ENABLED */
}

/**
 * g_main_context_release:
 * @context: a #GMainContext
 * 
 * Releases ownership of a context previously acquired by this thread
 * with g_main_context_acquire(). If the context was acquired multiple
 * times, the ownership will be released only when g_main_context_release()
 * is called as many times as it was acquired.
 **/
void
g_main_context_release (GMainContext *context)
{
#ifdef G_THREADS_ENABLED
  if (context == NULL)
    context = g_main_context_default ();
  
  LOCK_CONTEXT (context);

  context->owner_count--;
  if (context->owner_count == 0)
    {
      context->owner = NULL;

      if (context->waiters)
	{
	  GMainWaiter *waiter = context->waiters->data;
	  gboolean loop_internal_waiter =
	    (waiter->mutex == g_static_mutex_get_mutex (&context->mutex));
	  context->waiters = g_slist_delete_link (context->waiters,
						  context->waiters);
	  if (!loop_internal_waiter)
	    g_mutex_lock (waiter->mutex);
	  
	  g_cond_signal (waiter->cond);
	  
	  if (!loop_internal_waiter)
	    g_mutex_unlock (waiter->mutex);
	}
    }

  UNLOCK_CONTEXT (context); 
#endif /* G_THREADS_ENABLED */
}

/**
 * g_main_context_wait:
 * @context: a #GMainContext
 * @cond: a condition variable
 * @mutex: a mutex, currently held
 * 
 * Tries to become the owner of the specified context,
 * as with g_main_context_acquire(). But if another thread
 * is the owner, atomically drop @mutex and wait on @cond until 
 * that owner releases ownership or until @cond is signaled, then
 * try again (once) to become the owner.
 * 
 * Return value: %TRUE if the operation succeeded, and
 *   this thread is now the owner of @context.
 **/
gboolean
g_main_context_wait (GMainContext *context,
		     GCond        *cond,
		     GMutex       *mutex)
{
#ifdef G_THREADS_ENABLED
  gboolean result = FALSE;
  GThread *self = G_THREAD_SELF;
  gboolean loop_internal_waiter;
  
  if (context == NULL)
    context = g_main_context_default ();

  loop_internal_waiter = (mutex == g_static_mutex_get_mutex (&context->mutex));
  
  if (!loop_internal_waiter)
    LOCK_CONTEXT (context);

  if (context->owner && context->owner != self)
    {
      GMainWaiter waiter;

      waiter.cond = cond;
      waiter.mutex = mutex;

      context->waiters = g_slist_append (context->waiters, &waiter);
      
      if (!loop_internal_waiter)
	UNLOCK_CONTEXT (context);
      g_cond_wait (cond, mutex);
      if (!loop_internal_waiter)      
	LOCK_CONTEXT (context);

      context->waiters = g_slist_remove (context->waiters, &waiter);
    }

  if (!context->owner)
    {
      context->owner = self;
      g_assert (context->owner_count == 0);
    }

  if (context->owner == self)
    {
      context->owner_count++;
      result = TRUE;
    }

  if (!loop_internal_waiter)
    UNLOCK_CONTEXT (context); 
  
  return result;
#else /* !G_THREADS_ENABLED */
  return TRUE;
#endif /* G_THREADS_ENABLED */
}

/**
 * g_main_context_prepare:
 * @context: a #GMainContext
 * @priority: location to store priority of highest priority
 *            source already ready.
 * 
 * Prepares to poll sources within a main loop. The resulting information
 * for polling is determined by calling g_main_context_query ().
 * 
 * Return value: %TRUE if some source is ready to be dispatched
 *               prior to polling.
 **/
gboolean
g_main_context_prepare (GMainContext *context,
			gint         *priority)
{
  gint i;
  gint n_ready = 0;
  gint current_priority = G_MAXINT;
  GSource *source;

  if (context == NULL)
    context = g_main_context_default ();
  
  LOCK_CONTEXT (context);

  context->time_is_current = FALSE;

  if (context->in_check_or_prepare)
    {
      g_warning ("g_main_context_prepare() called recursively from within a source's check() or "
		 "prepare() member.");
      UNLOCK_CONTEXT (context);
      return FALSE;
    }

#ifdef G_THREADS_ENABLED
  if (context->poll_waiting)
    {
      g_warning("g_main_context_prepare(): main loop already active in another thread");
      UNLOCK_CONTEXT (context);
      return FALSE;
    }
  
  context->poll_waiting = TRUE;
#endif /* G_THREADS_ENABLED */

#if 0
  /* If recursing, finish up current dispatch, before starting over */
  if (context->pending_dispatches)
    {
      if (dispatch)
	g_main_dispatch (context, &current_time);
      
      UNLOCK_CONTEXT (context);
      return TRUE;
    }
#endif

  /* If recursing, clear list of pending dispatches */

  for (i = 0; i < context->pending_dispatches->len; i++)
    {
      if (context->pending_dispatches->pdata[i])
	SOURCE_UNREF ((GSource *)context->pending_dispatches->pdata[i], context);
    }
  g_ptr_array_set_size (context->pending_dispatches, 0);
  
  /* Prepare all sources */

  context->timeout = -1;
  
  source = next_valid_source (context, NULL);
  while (source)
    {
      gint source_timeout = -1;

      if ((n_ready > 0) && (source->priority > current_priority))
	{
	  SOURCE_UNREF (source, context);
	  break;
	}
      if (SOURCE_BLOCKED (source))
	goto next;

      if (!(source->flags & G_SOURCE_READY))
	{
	  gboolean result;
	  gboolean (*prepare)  (GSource  *source, 
				gint     *timeout);

	  prepare = source->source_funcs->prepare;
	  context->in_check_or_prepare++;
	  UNLOCK_CONTEXT (context);

	  result = (*prepare) (source, &source_timeout);

	  LOCK_CONTEXT (context);
	  context->in_check_or_prepare--;

	  if (result)
	    source->flags |= G_SOURCE_READY;
	}

      if (source->flags & G_SOURCE_READY)
	{
	  n_ready++;
	  current_priority = source->priority;
	  context->timeout = 0;
	}
      
      if (source_timeout >= 0)
	{
	  if (context->timeout < 0)
	    context->timeout = source_timeout;
	  else
	    context->timeout = MIN (context->timeout, source_timeout);
	}

    next:
      source = next_valid_source (context, source);
    }

  UNLOCK_CONTEXT (context);
  
  if (priority)
    *priority = current_priority;
  
  return (n_ready > 0);
}

/**
 * g_main_context_query:
 * @context: a #GMainContext
 * @max_priority: maximum priority source to check
 * @timeout_: location to store timeout to be used in polling
 * @fds: location to store #GPollFD records that need to be polled.
 * @n_fds: length of @fds.
 * 
 * Determines information necessary to poll this main loop.
 * 
 * Return value: the number of records actually stored in @fds,
 *   or, if more than @n_fds records need to be stored, the number
 *   of records that need to be stored.
 **/
gint
g_main_context_query (GMainContext *context,
		      gint          max_priority,
		      gint         *timeout,
		      GPollFD      *fds,
		      gint          n_fds)
{
  gint n_poll;
  GPollRec *pollrec;
  
  LOCK_CONTEXT (context);

  pollrec = context->poll_records;
  n_poll = 0;
  while (pollrec && max_priority >= pollrec->priority)
    {
      /* We need to include entries with fd->events == 0 in the array because
       * otherwise if the application changes fd->events behind our back and 
       * makes it non-zero, we'll be out of sync when we check the fds[] array.
       * (Changing fd->events after adding an FD wasn't an anticipated use of 
       * this API, but it occurs in practice.) */
      if (n_poll < n_fds)
	{
	  fds[n_poll].fd = pollrec->fd->fd;
	  /* In direct contradiction to the Unix98 spec, IRIX runs into
	   * difficulty if you pass in POLLERR, POLLHUP or POLLNVAL
	   * flags in the events field of the pollfd while it should
	   * just ignoring them. So we mask them out here.
	   */
	  fds[n_poll].events = pollrec->fd->events & ~(G_IO_ERR|G_IO_HUP|G_IO_NVAL);
	  fds[n_poll].revents = 0;
	}

      pollrec = pollrec->next;
      n_poll++;
    }

#ifdef G_THREADS_ENABLED
  context->poll_changed = FALSE;
#endif
  
  if (timeout)
    {
      *timeout = context->timeout;
      if (*timeout != 0)
	context->time_is_current = FALSE;
    }
  
  UNLOCK_CONTEXT (context);

  return n_poll;
}

/**
 * g_main_context_check:
 * @context: a #GMainContext
 * @max_priority: the maximum numerical priority of sources to check
 * @fds: array of #GPollFD's that was passed to the last call to
 *       g_main_context_query()
 * @n_fds: return value of g_main_context_query()
 * 
 * Passes the results of polling back to the main loop.
 * 
 * Return value: %TRUE if some sources are ready to be dispatched.
 **/
gboolean
g_main_context_check (GMainContext *context,
		      gint          max_priority,
		      GPollFD      *fds,
		      gint          n_fds)
{
  GSource *source;
  GPollRec *pollrec;
  gint n_ready = 0;
  gint i;
   
  LOCK_CONTEXT (context);

  if (context->in_check_or_prepare)
    {
      g_warning ("g_main_context_check() called recursively from within a source's check() or "
		 "prepare() member.");
      UNLOCK_CONTEXT (context);
      return FALSE;
    }
  
#ifdef G_THREADS_ENABLED
  if (!context->poll_waiting)
    {
#ifndef G_OS_WIN32
      gchar a;
      read (context->wake_up_pipe[0], &a, 1);
#endif
    }
  else
    context->poll_waiting = FALSE;

  /* If the set of poll file descriptors changed, bail out
   * and let the main loop rerun
   */
  if (context->poll_changed)
    {
      UNLOCK_CONTEXT (context);
      return FALSE;
    }
#endif /* G_THREADS_ENABLED */
  
  pollrec = context->poll_records;
  i = 0;
  while (i < n_fds)
    {
      if (pollrec->fd->events)
	pollrec->fd->revents = fds[i].revents;

      pollrec = pollrec->next;
      i++;
    }

  source = next_valid_source (context, NULL);
  while (source)
    {
      if ((n_ready > 0) && (source->priority > max_priority))
	{
	  SOURCE_UNREF (source, context);
	  break;
	}
      if (SOURCE_BLOCKED (source))
	goto next;

      if (!(source->flags & G_SOURCE_READY))
	{
	  gboolean result;
	  gboolean (*check) (GSource  *source);

	  check = source->source_funcs->check;
	  
	  context->in_check_or_prepare++;
	  UNLOCK_CONTEXT (context);
	  
	  result = (*check) (source);
	  
	  LOCK_CONTEXT (context);
	  context->in_check_or_prepare--;
	  
	  if (result)
	    source->flags |= G_SOURCE_READY;
	}

      if (source->flags & G_SOURCE_READY)
	{
	  source->ref_count++;
	  g_ptr_array_add (context->pending_dispatches, source);

	  n_ready++;

          /* never dispatch sources with less priority than the first
           * one we choose to dispatch
           */
          max_priority = source->priority;
	}

    next:
      source = next_valid_source (context, source);
    }

  UNLOCK_CONTEXT (context);

  return n_ready > 0;
}

/**
 * g_main_context_dispatch:
 * @context: a #GMainContext
 * 
 * Dispatches all pending sources.
 **/
void
g_main_context_dispatch (GMainContext *context)
{
  LOCK_CONTEXT (context);

  if (context->pending_dispatches->len > 0)
    {
      g_main_dispatch (context);
    }

  UNLOCK_CONTEXT (context);
}

/* HOLDS context lock */
static gboolean
g_main_context_iterate (GMainContext *context,
			gboolean      block,
			gboolean      dispatch,
			GThread      *self)
{
  gint max_priority;
  gint timeout;
  gboolean some_ready;
  gint nfds, allocated_nfds;
  GPollFD *fds = NULL;

  UNLOCK_CONTEXT (context);

#ifdef G_THREADS_ENABLED
  if (!g_main_context_acquire (context))
    {
      gboolean got_ownership;

      LOCK_CONTEXT (context);

      g_return_val_if_fail (g_thread_supported (), FALSE);

      if (!block)
	return FALSE;

      if (!context->cond)
	context->cond = g_cond_new ();

      got_ownership = g_main_context_wait (context,
					   context->cond,
					   g_static_mutex_get_mutex (&context->mutex));

      if (!got_ownership)
	return FALSE;
    }
  else
    LOCK_CONTEXT (context);
#endif /* G_THREADS_ENABLED */
  
  if (!context->cached_poll_array)
    {
      context->cached_poll_array_size = context->n_poll_records;
      context->cached_poll_array = g_new (GPollFD, context->n_poll_records);
    }

  allocated_nfds = context->cached_poll_array_size;
  fds = context->cached_poll_array;
  
  UNLOCK_CONTEXT (context);

  g_main_context_prepare (context, &max_priority); 
  
  while ((nfds = g_main_context_query (context, max_priority, &timeout, fds, 
				       allocated_nfds)) > allocated_nfds)
    {
      LOCK_CONTEXT (context);
      g_free (fds);
      context->cached_poll_array_size = allocated_nfds = nfds;
      context->cached_poll_array = fds = g_new (GPollFD, nfds);
      UNLOCK_CONTEXT (context);
    }

  if (!block)
    timeout = 0;
  
  g_main_context_poll (context, timeout, max_priority, fds, nfds);
  
  some_ready = g_main_context_check (context, max_priority, fds, nfds);
  
  if (dispatch)
    g_main_context_dispatch (context);
  
#ifdef G_THREADS_ENABLED
  g_main_context_release (context);
#endif /* G_THREADS_ENABLED */    

  LOCK_CONTEXT (context);

  return some_ready;
}

/**
 * g_main_context_pending:
 * @context: a #GMainContext (if %NULL, the default context will be used)
 *
 * Checks if any sources have pending events for the given context.
 * 
 * Return value: %TRUE if events are pending.
 **/
gboolean 
g_main_context_pending (GMainContext *context)
{
  gboolean retval;

  if (!context)
    context = g_main_context_default();

  LOCK_CONTEXT (context);
  retval = g_main_context_iterate (context, FALSE, FALSE, G_THREAD_SELF);
  UNLOCK_CONTEXT (context);
  
  return retval;
}

/**
 * g_main_context_iteration:
 * @context: a #GMainContext (if %NULL, the default context will be used) 
 * @may_block: whether the call may block.
 * 
 * Runs a single iteration for the given main loop. This involves
 * checking to see if any event sources are ready to be processed,
 * then if no events sources are ready and @may_block is %TRUE, waiting
 * for a source to become ready, then dispatching the highest priority
 * events sources that are ready. Otherwise, if @may_block is %FALSE 
 * sources are not waited to become ready, only those highest priority 
 * events sources will be dispatched (if any), that are ready at this 
 * given moment without further waiting.
 *
 * Note that even when @may_block is %TRUE, it is still possible for 
 * g_main_context_iteration() to return %FALSE, since the the wait may 
 * be interrupted for other reasons than an event source becoming ready.
 * 
 * Return value: %TRUE if events were dispatched.
 **/
gboolean
g_main_context_iteration (GMainContext *context, gboolean may_block)
{
  gboolean retval;

  if (!context)
    context = g_main_context_default();
  
  LOCK_CONTEXT (context);
  retval = g_main_context_iterate (context, may_block, TRUE, G_THREAD_SELF);
  UNLOCK_CONTEXT (context);
  
  return retval;
}

/**
 * g_main_loop_new:
 * @context: a #GMainContext  (if %NULL, the default context will be used).
 * @is_running: set to %TRUE to indicate that the loop is running. This
 * is not very important since calling g_main_loop_run() will set this to
 * %TRUE anyway.
 * 
 * Creates a new #GMainLoop structure.
 * 
 * Return value: a new #GMainLoop.
 **/
GMainLoop *
g_main_loop_new (GMainContext *context,
		 gboolean      is_running)
{
  GMainLoop *loop;

  if (!context)
    context = g_main_context_default();
  
  g_main_context_ref (context);

  loop = g_new0 (GMainLoop, 1);
  loop->context = context;
  loop->is_running = is_running != FALSE;
  loop->ref_count = 1;
  
  return loop;
}

/**
 * g_main_loop_ref:
 * @loop: a #GMainLoop
 * 
 * Increases the reference count on a #GMainLoop object by one.
 * 
 * Return value: @loop
 **/
GMainLoop *
g_main_loop_ref (GMainLoop *loop)
{
  g_return_val_if_fail (loop != NULL, NULL);
  g_return_val_if_fail (g_atomic_int_get (&loop->ref_count) > 0, NULL);

  g_atomic_int_inc (&loop->ref_count);

  return loop;
}

/**
 * g_main_loop_unref:
 * @loop: a #GMainLoop
 * 
 * Decreases the reference count on a #GMainLoop object by one. If
 * the result is zero, free the loop and free all associated memory.
 **/
void
g_main_loop_unref (GMainLoop *loop)
{
  g_return_if_fail (loop != NULL);
  g_return_if_fail (g_atomic_int_get (&loop->ref_count) > 0);

  if (!g_atomic_int_dec_and_test (&loop->ref_count))
    return;

  g_main_context_unref (loop->context);
  g_free (loop);
}

/**
 * g_main_loop_run:
 * @loop: a #GMainLoop
 * 
 * Runs a main loop until g_main_loop_quit() is called on the loop.
 * If this is called for the thread of the loop's #GMainContext,
 * it will process events from the loop, otherwise it will
 * simply wait.
 **/
void 
g_main_loop_run (GMainLoop *loop)
{
  GThread *self = G_THREAD_SELF;

  g_return_if_fail (loop != NULL);
  g_return_if_fail (g_atomic_int_get (&loop->ref_count) > 0);

#ifdef G_THREADS_ENABLED
  if (!g_main_context_acquire (loop->context))
    {
      gboolean got_ownership = FALSE;
      
      /* Another thread owns this context */
      if (!g_thread_supported ())
	{
	  g_warning ("g_main_loop_run() was called from second thread but "
		     "g_thread_init() was never called.");
	  return;
	}
      
      LOCK_CONTEXT (loop->context);

      g_atomic_int_inc (&loop->ref_count);

      if (!loop->is_running)
	loop->is_running = TRUE;

      if (!loop->context->cond)
	loop->context->cond = g_cond_new ();
          
      while (loop->is_running && !got_ownership)
	got_ownership = g_main_context_wait (loop->context,
					     loop->context->cond,
					     g_static_mutex_get_mutex (&loop->context->mutex));
      
      if (!loop->is_running)
	{
	  UNLOCK_CONTEXT (loop->context);
	  if (got_ownership)
	    g_main_context_release (loop->context);
	  g_main_loop_unref (loop);
	  return;
	}

      g_assert (got_ownership);
    }
  else
    LOCK_CONTEXT (loop->context);
#endif /* G_THREADS_ENABLED */ 

  if (loop->context->in_check_or_prepare)
    {
      g_warning ("g_main_loop_run(): called recursively from within a source's "
		 "check() or prepare() member, iteration not possible.");
      return;
    }

  g_atomic_int_inc (&loop->ref_count);
  loop->is_running = TRUE;
  while (loop->is_running)
    g_main_context_iterate (loop->context, TRUE, TRUE, self);

  UNLOCK_CONTEXT (loop->context);
  
#ifdef G_THREADS_ENABLED
  g_main_context_release (loop->context);
#endif /* G_THREADS_ENABLED */    
  
  g_main_loop_unref (loop);
}

/**
 * g_main_loop_quit:
 * @loop: a #GMainLoop
 * 
 * Stops a #GMainLoop from running. Any calls to g_main_loop_run()
 * for the loop will return. 
 *
 * Note that sources that have already been dispatched when 
 * g_main_loop_quit() is called will still be executed.
 **/
void 
g_main_loop_quit (GMainLoop *loop)
{
  g_return_if_fail (loop != NULL);
  g_return_if_fail (g_atomic_int_get (&loop->ref_count) > 0);

  LOCK_CONTEXT (loop->context);
  loop->is_running = FALSE;
  g_main_context_wakeup_unlocked (loop->context);

#ifdef G_THREADS_ENABLED
  if (loop->context->cond)
    g_cond_broadcast (loop->context->cond);
#endif /* G_THREADS_ENABLED */

  UNLOCK_CONTEXT (loop->context);
}

/**
 * g_main_loop_is_running:
 * @loop: a #GMainLoop.
 * 
 * Checks to see if the main loop is currently being run via g_main_loop_run().
 * 
 * Return value: %TRUE if the mainloop is currently being run.
 **/
gboolean
g_main_loop_is_running (GMainLoop *loop)
{
  g_return_val_if_fail (loop != NULL, FALSE);
  g_return_val_if_fail (g_atomic_int_get (&loop->ref_count) > 0, FALSE);

  return loop->is_running;
}

/**
 * g_main_loop_get_context:
 * @loop: a #GMainLoop.
 * 
 * Returns the #GMainContext of @loop.
 * 
 * Return value: the #GMainContext of @loop
 **/
GMainContext *
g_main_loop_get_context (GMainLoop *loop)
{
  g_return_val_if_fail (loop != NULL, NULL);
  g_return_val_if_fail (g_atomic_int_get (&loop->ref_count) > 0, NULL);
 
  return loop->context;
}

/* HOLDS: context's lock */
static void
g_main_context_poll (GMainContext *context,
		     gint          timeout,
		     gint          priority,
		     GPollFD      *fds,
		     gint          n_fds)
{
#ifdef  G_MAIN_POLL_DEBUG
  GTimer *poll_timer;
  GPollRec *pollrec;
  gint i;
#endif

  GPollFunc poll_func;

  if (n_fds || timeout != 0)
    {
#ifdef	G_MAIN_POLL_DEBUG
      if (_g_main_poll_debug)
	{
	  g_print ("polling context=%p n=%d timeout=%d\n",
		   context, n_fds, timeout);
	  poll_timer = g_timer_new ();
	}
#endif

      LOCK_CONTEXT (context);

      poll_func = context->poll_func;
      
      UNLOCK_CONTEXT (context);
      if ((*poll_func) (fds, n_fds, timeout) < 0 && errno != EINTR)
	{
#ifndef G_OS_WIN32
	  g_warning ("poll(2) failed due to: %s.",
		     g_strerror (errno));
#else
	  /* If g_poll () returns -1, it has already called g_warning() */
#endif
	}
      
#ifdef	G_MAIN_POLL_DEBUG
      if (_g_main_poll_debug)
	{
	  LOCK_CONTEXT (context);

	  g_print ("g_main_poll(%d) timeout: %d - elapsed %12.10f seconds",
		   n_fds,
		   timeout,
		   g_timer_elapsed (poll_timer, NULL));
	  g_timer_destroy (poll_timer);
	  pollrec = context->poll_records;

	  while (pollrec != NULL)
	    {
	      i = 0;
	      while (i < n_fds)
		{
		  if (fds[i].fd == pollrec->fd->fd &&
		      pollrec->fd->events &&
		      fds[i].revents)
		    {
		      g_print (" [" G_POLLFD_FORMAT " :", fds[i].fd);
		      if (fds[i].revents & G_IO_IN)
			g_print ("i");
		      if (fds[i].revents & G_IO_OUT)
			g_print ("o");
		      if (fds[i].revents & G_IO_PRI)
			g_print ("p");
		      if (fds[i].revents & G_IO_ERR)
			g_print ("e");
		      if (fds[i].revents & G_IO_HUP)
			g_print ("h");
		      if (fds[i].revents & G_IO_NVAL)
			g_print ("n");
		      g_print ("]");
		    }
		  i++;
		}
	      pollrec = pollrec->next;
	    }
	  g_print ("\n");

	  UNLOCK_CONTEXT (context);
	}
#endif
    } /* if (n_fds || timeout != 0) */
}

/**
 * g_main_context_add_poll:
 * @context: a #GMainContext (or %NULL for the default context)
 * @fd: a #GPollFD structure holding information about a file
 *      descriptor to watch.
 * @priority: the priority for this file descriptor which should be
 *      the same as the priority used for g_source_attach() to ensure that the
 *      file descriptor is polled whenever the results may be needed.
 * 
 * Adds a file descriptor to the set of file descriptors polled for
 * this context. This will very seldomly be used directly. Instead
 * a typical event source will use g_source_add_poll() instead.
 **/
void
g_main_context_add_poll (GMainContext *context,
			 GPollFD      *fd,
			 gint          priority)
{
  if (!context)
    context = g_main_context_default ();
  
  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0);
  g_return_if_fail (fd);

  LOCK_CONTEXT (context);
  g_main_context_add_poll_unlocked (context, priority, fd);
  UNLOCK_CONTEXT (context);
}

/* HOLDS: main_loop_lock */
static void 
g_main_context_add_poll_unlocked (GMainContext *context,
				  gint          priority,
				  GPollFD      *fd)
{
  GPollRec *lastrec, *pollrec;
  GPollRec *newrec = g_slice_new (GPollRec);

  /* This file descriptor may be checked before we ever poll */
  fd->revents = 0;
  newrec->fd = fd;
  newrec->priority = priority;

  lastrec = NULL;
  pollrec = context->poll_records;
  while (pollrec && priority >= pollrec->priority)
    {
      lastrec = pollrec;
      pollrec = pollrec->next;
    }
  
  if (lastrec)
    lastrec->next = newrec;
  else
    context->poll_records = newrec;

  newrec->next = pollrec;

  context->n_poll_records++;

#ifdef G_THREADS_ENABLED
  context->poll_changed = TRUE;

  /* Now wake up the main loop if it is waiting in the poll() */
  g_main_context_wakeup_unlocked (context);
#endif
}

/**
 * g_main_context_remove_poll:
 * @context:a #GMainContext 
 * @fd: a #GPollFD descriptor previously added with g_main_context_add_poll()
 * 
 * Removes file descriptor from the set of file descriptors to be
 * polled for a particular context.
 **/
void
g_main_context_remove_poll (GMainContext *context,
			    GPollFD      *fd)
{
  if (!context)
    context = g_main_context_default ();
  
  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0);
  g_return_if_fail (fd);

  LOCK_CONTEXT (context);
  g_main_context_remove_poll_unlocked (context, fd);
  UNLOCK_CONTEXT (context);
}

static void
g_main_context_remove_poll_unlocked (GMainContext *context,
				     GPollFD      *fd)
{
  GPollRec *pollrec, *lastrec;

  lastrec = NULL;
  pollrec = context->poll_records;

  while (pollrec)
    {
      if (pollrec->fd == fd)
	{
	  if (lastrec != NULL)
	    lastrec->next = pollrec->next;
	  else
	    context->poll_records = pollrec->next;

	  g_slice_free (GPollRec, pollrec);

	  context->n_poll_records--;
	  break;
	}
      lastrec = pollrec;
      pollrec = pollrec->next;
    }

#ifdef G_THREADS_ENABLED
  context->poll_changed = TRUE;
  
  /* Now wake up the main loop if it is waiting in the poll() */
  g_main_context_wakeup_unlocked (context);
#endif
}

/**
 * g_source_get_current_time:
 * @source:  a #GSource
 * @timeval: #GTimeVal structure in which to store current time.
 * 
 * Gets the "current time" to be used when checking 
 * this source. The advantage of calling this function over
 * calling g_get_current_time() directly is that when 
 * checking multiple sources, GLib can cache a single value
 * instead of having to repeatedly get the system time.
 **/
void
g_source_get_current_time (GSource  *source,
			   GTimeVal *timeval)
{
  GMainContext *context;
  
  g_return_if_fail (source->context != NULL);
 
  context = source->context;

  LOCK_CONTEXT (context);

  if (!context->time_is_current)
    {
      g_get_current_time (&context->current_time);
      context->time_is_current = TRUE;
    }
  
  *timeval = context->current_time;
  
  UNLOCK_CONTEXT (context);
}

/**
 * g_main_context_set_poll_func:
 * @context: a #GMainContext
 * @func: the function to call to poll all file descriptors
 * 
 * Sets the function to use to handle polling of file descriptors. It
 * will be used instead of the poll() system call 
 * (or GLib's replacement function, which is used where 
 * poll() isn't available).
 *
 * This function could possibly be used to integrate the GLib event
 * loop with an external event loop.
 **/
void
g_main_context_set_poll_func (GMainContext *context,
			      GPollFunc     func)
{
  if (!context)
    context = g_main_context_default ();
  
  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0);

  LOCK_CONTEXT (context);
  
  if (func)
    context->poll_func = func;
  else
    context->poll_func = g_poll;

  UNLOCK_CONTEXT (context);
}

/**
 * g_main_context_get_poll_func:
 * @context: a #GMainContext
 * 
 * Gets the poll function set by g_main_context_set_poll_func().
 * 
 * Return value: the poll function
 **/
GPollFunc
g_main_context_get_poll_func (GMainContext *context)
{
  GPollFunc result;
  
  if (!context)
    context = g_main_context_default ();
  
  g_return_val_if_fail (g_atomic_int_get (&context->ref_count) > 0, NULL);

  LOCK_CONTEXT (context);
  result = context->poll_func;
  UNLOCK_CONTEXT (context);

  return result;
}

/* HOLDS: context's lock */
/* Wake the main loop up from a poll() */
static void
g_main_context_wakeup_unlocked (GMainContext *context)
{
#ifdef G_THREADS_ENABLED
  if (g_thread_supported() && context->poll_waiting)
    {
      context->poll_waiting = FALSE;
#ifndef G_OS_WIN32
      write (context->wake_up_pipe[1], "A", 1);
#else
      ReleaseSemaphore (context->wake_up_semaphore, 1, NULL);
#endif
    }
#endif
}

/**
 * g_main_context_wakeup:
 * @context: a #GMainContext
 * 
 * If @context is currently waiting in a poll(), interrupt
 * the poll(), and continue the iteration process.
 **/
void
g_main_context_wakeup (GMainContext *context)
{
  if (!context)
    context = g_main_context_default ();
  
  g_return_if_fail (g_atomic_int_get (&context->ref_count) > 0);

  LOCK_CONTEXT (context);
  g_main_context_wakeup_unlocked (context);
  UNLOCK_CONTEXT (context);
}

/**
 * g_main_context_is_owner:
 * @context: a #GMainContext
 * 
 * Determines whether this thread holds the (recursive)
 * ownership of this #GMaincontext. This is useful to
 * know before waiting on another thread that may be
 * blocking to get ownership of @context.
 *
 * Returns: %TRUE if current thread is owner of @context.
 *
 * Since: 2.10
 **/
gboolean
g_main_context_is_owner (GMainContext *context)
{
  gboolean is_owner;

  if (!context)
    context = g_main_context_default ();

#ifdef G_THREADS_ENABLED
  LOCK_CONTEXT (context);
  is_owner = context->owner == G_THREAD_SELF;
  UNLOCK_CONTEXT (context);
#else
  is_owner = TRUE;
#endif

  return is_owner;
}

/* Timeouts */

static void
g_timeout_set_expiration (GTimeoutSource *timeout_source,
			  GTimeVal       *current_time)
{
  guint seconds = timeout_source->interval / 1000;
  guint msecs = timeout_source->interval - seconds * 1000;

  timeout_source->expiration.tv_sec = current_time->tv_sec + seconds;
  timeout_source->expiration.tv_usec = current_time->tv_usec + msecs * 1000;
  if (timeout_source->expiration.tv_usec >= 1000000)
    {
      timeout_source->expiration.tv_usec -= 1000000;
      timeout_source->expiration.tv_sec++;
    }
  if (timer_perturb==-1)
    {
      /*
       * we want a per machine/session unique 'random' value; try the dbus
       * address first, that has a UUID in it. If there is no dbus, use the
       * hostname for hashing.
       */
      const char *session_bus_address = g_getenv ("DBUS_SESSION_BUS_ADDRESS");
      if (!session_bus_address)
        session_bus_address = g_getenv ("HOSTNAME");
      if (session_bus_address)
        timer_perturb = ABS ((gint) g_str_hash (session_bus_address));
      else
        timer_perturb = 0;
    }
  if (timeout_source->granularity)
    {
      gint remainder;
      gint gran; /* in usecs */
      gint perturb;

      gran = timeout_source->granularity * 1000;
      perturb = timer_perturb % gran;
      /*
       * We want to give each machine a per machine pertubation;
       * shift time back first, and forward later after the rounding
       */

      timeout_source->expiration.tv_usec -= perturb;
      if (timeout_source->expiration.tv_usec < 0)
        {
          timeout_source->expiration.tv_usec += 1000000;
          timeout_source->expiration.tv_sec--;
        }

      remainder = timeout_source->expiration.tv_usec % gran;
      if (remainder >= gran/4) /* round up */
        timeout_source->expiration.tv_usec += gran;
      timeout_source->expiration.tv_usec -= remainder;
      /* shift back */
      timeout_source->expiration.tv_usec += perturb;

      /* the rounding may have overflown tv_usec */
      while (timeout_source->expiration.tv_usec > 1000000)
        {
          timeout_source->expiration.tv_usec -= 1000000;
          timeout_source->expiration.tv_sec++;
        }
    }
}

static gboolean
g_timeout_prepare (GSource *source,
		   gint    *timeout)
{
  glong sec;
  glong msec;
  GTimeVal current_time;
  
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;

  g_source_get_current_time (source, &current_time);

  sec = timeout_source->expiration.tv_sec - current_time.tv_sec;
  msec = (timeout_source->expiration.tv_usec - current_time.tv_usec) / 1000;

  /* We do the following in a rather convoluted fashion to deal with
   * the fact that we don't have an integral type big enough to hold
   * the difference of two timevals in millseconds.
   */
  if (sec < 0 || (sec == 0 && msec < 0))
    msec = 0;
  else
    {
      glong interval_sec = timeout_source->interval / 1000;
      glong interval_msec = timeout_source->interval % 1000;

      if (msec < 0)
	{
	  msec += 1000;
	  sec -= 1;
	}
      
      if (sec > interval_sec ||
	  (sec == interval_sec && msec > interval_msec))
	{
	  /* The system time has been set backwards, so we
	   * reset the expiration time to now + timeout_source->interval;
	   * this at least avoids hanging for long periods of time.
	   */
	  g_timeout_set_expiration (timeout_source, &current_time);
	  msec = MIN (G_MAXINT, timeout_source->interval);
	}
      else
	{
	  msec = MIN (G_MAXINT, (guint)msec + 1000 * (guint)sec);
	}
    }

  *timeout = (gint)msec;
  
  return msec == 0;
}

static gboolean 
g_timeout_check (GSource *source)
{
  GTimeVal current_time;
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;

  g_source_get_current_time (source, &current_time);
  
  return ((timeout_source->expiration.tv_sec < current_time.tv_sec) ||
	  ((timeout_source->expiration.tv_sec == current_time.tv_sec) &&
	   (timeout_source->expiration.tv_usec <= current_time.tv_usec)));
}

static gboolean
g_timeout_dispatch (GSource     *source,
		    GSourceFunc  callback,
		    gpointer     user_data)
{
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;

  if (!callback)
    {
      g_warning ("Timeout source dispatched without callback\n"
		 "You must call g_source_set_callback().");
      return FALSE;
    }
 
  if (callback (user_data))
    {
      GTimeVal current_time;

      g_source_get_current_time (source, &current_time);
      g_timeout_set_expiration (timeout_source, &current_time);

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * g_timeout_source_new:
 * @interval: the timeout interval in milliseconds.
 * 
 * Creates a new timeout source.
 *
 * The source will not initially be associated with any #GMainContext
 * and must be added to one with g_source_attach() before it will be
 * executed.
 * 
 * Return value: the newly-created timeout source
 **/
GSource *
g_timeout_source_new (guint interval)
{
  GSource *source = g_source_new (&g_timeout_funcs, sizeof (GTimeoutSource));
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;
  GTimeVal current_time;

  timeout_source->interval = interval;

  g_get_current_time (&current_time);
  g_timeout_set_expiration (timeout_source, &current_time);
  
  return source;
}

/**
 * g_timeout_source_new_seconds:
 * @interval: the timeout interval in seconds
 *
 * Creates a new timeout source.
 *
 * The source will not initially be associated with any #GMainContext
 * and must be added to one with g_source_attach() before it will be
 * executed.
 *
 * The scheduling granularity/accuracy of this timeout source will be
 * in seconds.
 *
 * Return value: the newly-created timeout source
 *
 * Since: 2.14	
 **/
GSource *
g_timeout_source_new_seconds (guint interval)
{
  GSource *source = g_source_new (&g_timeout_funcs, sizeof (GTimeoutSource));
  GTimeoutSource *timeout_source = (GTimeoutSource *)source;
  GTimeVal current_time;

  timeout_source->interval = 1000*interval;
  timeout_source->granularity = 1000;

  g_get_current_time (&current_time);
  g_timeout_set_expiration (timeout_source, &current_time);

  return source;
}


/**
 * g_timeout_add_full:
 * @priority: the priority of the timeout source. Typically this will be in
 *            the range between #G_PRIORITY_DEFAULT and #G_PRIORITY_HIGH.
 * @interval: the time between calls to the function, in milliseconds
 *             (1/1000ths of a second)
 * @function: function to call
 * @data:     data to pass to @function
 * @notify:   function to call when the timeout is removed, or %NULL
 * 
 * Sets a function to be called at regular intervals, with the given
 * priority.  The function is called repeatedly until it returns
 * %FALSE, at which point the timeout is automatically destroyed and
 * the function will not be called again.  The @notify function is
 * called when the timeout is destroyed.  The first call to the
 * function will be at the end of the first @interval.
 *
 * Note that timeout functions may be delayed, due to the processing of other
 * event sources. Thus they should not be relied on for precise timing.
 * After each call to the timeout function, the time of the next
 * timeout is recalculated based on the current time and the given interval
 * (it does not try to 'catch up' time lost in delays).
 *
 * This internally creates a main loop source using g_timeout_source_new()
 * and attaches it to the main loop context using g_source_attach(). You can
 * do these steps manually if you need greater control.
 * 
 * Return value: the ID (greater than 0) of the event source.
 **/
guint
g_timeout_add_full (gint           priority,
		    guint          interval,
		    GSourceFunc    function,
		    gpointer       data,
		    GDestroyNotify notify)
{
  GSource *source;
  guint id;
  
  g_return_val_if_fail (function != NULL, 0);

  source = g_timeout_source_new (interval);

  if (priority != G_PRIORITY_DEFAULT)
    g_source_set_priority (source, priority);

  g_source_set_callback (source, function, data, notify);
  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}

/**
 * g_timeout_add:
 * @interval: the time between calls to the function, in milliseconds
 *             (1/1000ths of a second)
 * @function: function to call
 * @data:     data to pass to @function
 * 
 * Sets a function to be called at regular intervals, with the default
 * priority, #G_PRIORITY_DEFAULT.  The function is called repeatedly
 * until it returns %FALSE, at which point the timeout is automatically
 * destroyed and the function will not be called again.  The first call
 * to the function will be at the end of the first @interval.
 *
 * Note that timeout functions may be delayed, due to the processing of other
 * event sources. Thus they should not be relied on for precise timing.
 * After each call to the timeout function, the time of the next
 * timeout is recalculated based on the current time and the given interval
 * (it does not try to 'catch up' time lost in delays).
 *
 * If you want to have a timer in the "seconds" range and do not care
 * about the exact time of the first call of the timer, use the
 * g_timeout_add_seconds() function; this function allows for more
 * optimizations and more efficient system power usage.
 *
 * This internally creates a main loop source using g_timeout_source_new()
 * and attaches it to the main loop context using g_source_attach(). You can
 * do these steps manually if you need greater control.
 * 
 * Return value: the ID (greater than 0) of the event source.
 **/
guint
g_timeout_add (guint32        interval,
	       GSourceFunc    function,
	       gpointer       data)
{
  return g_timeout_add_full (G_PRIORITY_DEFAULT, 
			     interval, function, data, NULL);
}

/**
 * g_timeout_add_seconds_full:
 * @priority: the priority of the timeout source. Typically this will be in
 *            the range between #G_PRIORITY_DEFAULT and #G_PRIORITY_HIGH.
 * @interval: the time between calls to the function, in seconds
 * @function: function to call
 * @data:     data to pass to @function
 * @notify:   function to call when the timeout is removed, or %NULL
 *
 * Sets a function to be called at regular intervals, with @priority.
 * The function is called repeatedly until it returns %FALSE, at which
 * point the timeout is automatically destroyed and the function will
 * not be called again.
 *
 * Unlike g_timeout_add(), this function operates at whole second granularity.
 * The initial starting point of the timer is determined by the implementation
 * and the implementation is expected to group multiple timers together so that
 * they fire all at the same time.
 * To allow this grouping, the @interval to the first timer is rounded
 * and can deviate up to one second from the specified interval.
 * Subsequent timer iterations will generally run at the specified interval.
 *
 * Note that timeout functions may be delayed, due to the processing of other
 * event sources. Thus they should not be relied on for precise timing.
 * After each call to the timeout function, the time of the next
 * timeout is recalculated based on the current time and the given @interval
 *
 * If you want timing more precise than whole seconds, use g_timeout_add()
 * instead.
 *
 * The grouping of timers to fire at the same time results in a more power
 * and CPU efficient behavior so if your timer is in multiples of seconds
 * and you don't require the first timer exactly one second from now, the
 * use of g_timeout_add_seconds() is preferred over g_timeout_add().
 *
 * This internally creates a main loop source using 
 * g_timeout_source_new_seconds() and attaches it to the main loop context 
 * using g_source_attach(). You can do these steps manually if you need 
 * greater control.
 * 
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 2.14
 **/
guint
g_timeout_add_seconds_full (gint           priority,
                            guint32        interval,
                            GSourceFunc    function,
                            gpointer       data,
                            GDestroyNotify notify)
{
  GSource *source;
  guint id;

  g_return_val_if_fail (function != NULL, 0);

  source = g_timeout_source_new_seconds (interval);

  if (priority != G_PRIORITY_DEFAULT)
    g_source_set_priority (source, priority);

  g_source_set_callback (source, function, data, notify);
  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}

/**
 * g_timeout_add_seconds:
 * @interval: the time between calls to the function, in seconds
 * @function: function to call
 * @data: data to pass to @function
 *
 * Sets a function to be called at regular intervals with the default
 * priority, #G_PRIORITY_DEFAULT. The function is called repeatedly until
 * it returns %FALSE, at which point the timeout is automatically destroyed
 * and the function will not be called again.
 *
 * This internally creates a main loop source using 
 * g_timeout_source_new_seconds() and attaches it to the main loop context 
 * using g_source_attach(). You can do these steps manually if you need 
 * greater control. Also see g_timout_add_seconds_full().
 * 
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 2.14
 **/
guint
g_timeout_add_seconds (guint       interval,
                       GSourceFunc function,
                       gpointer    data)
{
  g_return_val_if_fail (function != NULL, 0);

  return g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, interval, function, data, NULL);
}

/* Child watch functions */

#ifdef G_OS_WIN32

static gboolean
g_child_watch_prepare (GSource *source,
		       gint    *timeout)
{
  *timeout = -1;
  return FALSE;
}


static gboolean 
g_child_watch_check (GSource  *source)
{
  GChildWatchSource *child_watch_source;
  gboolean child_exited;

  child_watch_source = (GChildWatchSource *) source;

  child_exited = child_watch_source->poll.revents & G_IO_IN;

  if (child_exited)
    {
      DWORD child_status;

      /*
       * Note: We do _not_ check for the special value of STILL_ACTIVE
       * since we know that the process has exited and doing so runs into
       * problems if the child process "happens to return STILL_ACTIVE(259)"
       * as Microsoft's Platform SDK puts it.
       */
      if (!GetExitCodeProcess (child_watch_source->pid, &child_status))
        {
	  gchar *emsg = g_win32_error_message (GetLastError ());
	  g_warning (G_STRLOC ": GetExitCodeProcess() failed: %s", emsg);
	  g_free (emsg);

	  child_watch_source->child_status = -1;
	}
      else
	child_watch_source->child_status = child_status;
    }

  return child_exited;
}

#else /* G_OS_WIN32 */

static gboolean
check_for_child_exited (GSource *source)
{
  GChildWatchSource *child_watch_source;
  gint count;

  /* protect against another SIGCHLD in the middle of this call */
  count = child_watch_count;

  child_watch_source = (GChildWatchSource *) source;

  if (child_watch_source->child_exited)
    return TRUE;

  if (child_watch_source->count < count)
    {
      gint child_status;

      if (waitpid (child_watch_source->pid, &child_status, WNOHANG) > 0)
	{
	  child_watch_source->child_status = child_status;
	  child_watch_source->child_exited = TRUE;
	}
      child_watch_source->count = count;
    }

  return child_watch_source->child_exited;
}

static gboolean
g_child_watch_prepare (GSource *source,
		       gint    *timeout)
{
  *timeout = -1;

  return check_for_child_exited (source);
}


static gboolean 
g_child_watch_check (GSource  *source)
{
  return check_for_child_exited (source);
}

#endif /* G_OS_WIN32 */

static gboolean
g_child_watch_dispatch (GSource    *source, 
			GSourceFunc callback,
			gpointer    user_data)
{
  GChildWatchSource *child_watch_source;
  GChildWatchFunc child_watch_callback = (GChildWatchFunc) callback;

  child_watch_source = (GChildWatchSource *) source;

  if (!callback)
    {
      g_warning ("Child watch source dispatched without callback\n"
		 "You must call g_source_set_callback().");
      return FALSE;
    }

  (child_watch_callback) (child_watch_source->pid, child_watch_source->child_status, user_data);

  /* We never keep a child watch source around as the child is gone */
  return FALSE;
}

#ifndef G_OS_WIN32

static void
g_child_watch_signal_handler (int signum)
{
  child_watch_count ++;

  if (child_watch_init_state == CHILD_WATCH_INITIALIZED_THREADED)
    {
      write (child_watch_wake_up_pipe[1], "B", 1);
    }
  else
    {
      /* We count on the signal interrupting the poll in the same thread.
       */
    }
}
 
static void
g_child_watch_source_init_single (void)
{
  struct sigaction action;

  g_assert (! g_thread_supported());
  g_assert (child_watch_init_state == CHILD_WATCH_UNINITIALIZED);

  child_watch_init_state = CHILD_WATCH_INITIALIZED_SINGLE;

  action.sa_handler = g_child_watch_signal_handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = SA_NOCLDSTOP;
  sigaction (SIGCHLD, &action, NULL);
}

G_GNUC_NORETURN static gpointer
child_watch_helper_thread (gpointer data) 
{
  while (1)
    {
      gchar b[20];
      GSList *list;

      read (child_watch_wake_up_pipe[0], b, 20);

      /* We were woken up.  Wake up all other contexts in all other threads */
      G_LOCK (main_context_list);
      for (list = main_context_list; list; list = list->next)
	{
	  GMainContext *context;

	  context = list->data;
	  if (g_atomic_int_get (&context->ref_count) > 0)
	    /* Due to racing conditions we can find ref_count == 0, in
	     * that case, however, the context is still not destroyed
	     * and no poll can be active, otherwise the ref_count
	     * wouldn't be 0 */
	    g_main_context_wakeup (context);
	}
      G_UNLOCK (main_context_list);
    }
}

static void
g_child_watch_source_init_multi_threaded (void)
{
  GError *error = NULL;
  struct sigaction action;

  g_assert (g_thread_supported());

  if (pipe (child_watch_wake_up_pipe) < 0)
    g_error ("Cannot create wake up pipe: %s\n", g_strerror (errno));
  fcntl (child_watch_wake_up_pipe[1], F_SETFL, O_NONBLOCK | fcntl (child_watch_wake_up_pipe[1], F_GETFL));

  /* We create a helper thread that polls on the wakeup pipe indefinitely */
  /* FIXME: Think this through for races */
  if (g_thread_create (child_watch_helper_thread, NULL, FALSE, &error) == NULL)
    g_error ("Cannot create a thread to monitor child exit status: %s\n", error->message);
  child_watch_init_state = CHILD_WATCH_INITIALIZED_THREADED;
 
  action.sa_handler = g_child_watch_signal_handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction (SIGCHLD, &action, NULL);
}

static void
g_child_watch_source_init_promote_single_to_threaded (void)
{
  g_child_watch_source_init_multi_threaded ();
}

static void
g_child_watch_source_init (void)
{
  if (g_thread_supported())
    {
      if (child_watch_init_state == CHILD_WATCH_UNINITIALIZED)
	g_child_watch_source_init_multi_threaded ();
      else if (child_watch_init_state == CHILD_WATCH_INITIALIZED_SINGLE)
	g_child_watch_source_init_promote_single_to_threaded ();
    }
  else
    {
      if (child_watch_init_state == CHILD_WATCH_UNINITIALIZED)
	g_child_watch_source_init_single ();
    }
}

#endif /* !G_OS_WIN32 */

/**
 * g_child_watch_source_new:
 * @pid: process to watch. On POSIX the pid of a child process. On
 * Windows a handle for a process (which doesn't have to be a child).
 * 
 * Creates a new child_watch source.
 *
 * The source will not initially be associated with any #GMainContext
 * and must be added to one with g_source_attach() before it will be
 * executed.
 * 
 * Note that child watch sources can only be used in conjunction with
 * <literal>g_spawn...</literal> when the %G_SPAWN_DO_NOT_REAP_CHILD
 * flag is used.
 *
 * Note that on platforms where #GPid must be explicitly closed
 * (see g_spawn_close_pid()) @pid must not be closed while the
 * source is still active. Typically, you will want to call
 * g_spawn_close_pid() in the callback function for the source.
 *
 * Note further that using g_child_watch_source_new() is not 
 * compatible with calling <literal>waitpid(-1)</literal> in 
 * the application. Calling waitpid() for individual pids will
 * still work fine. 
 * 
 * Return value: the newly-created child watch source
 *
 * Since: 2.4
 **/
GSource *
g_child_watch_source_new (GPid pid)
{
  GSource *source = g_source_new (&g_child_watch_funcs, sizeof (GChildWatchSource));
  GChildWatchSource *child_watch_source = (GChildWatchSource *)source;

#ifdef G_OS_WIN32
  child_watch_source->poll.fd = (gintptr) pid;
  child_watch_source->poll.events = G_IO_IN;

  g_source_add_poll (source, &child_watch_source->poll);
#else /* G_OS_WIN32 */
  g_child_watch_source_init ();
#endif /* G_OS_WIN32 */

  child_watch_source->pid = pid;

  return source;
}

/**
 * g_child_watch_add_full:
 * @priority: the priority of the idle source. Typically this will be in the
 *            range between #G_PRIORITY_DEFAULT_IDLE and #G_PRIORITY_HIGH_IDLE.
 * @pid:      process to watch. On POSIX the pid of a child process. On
 * Windows a handle for a process (which doesn't have to be a child).
 * @function: function to call
 * @data:     data to pass to @function
 * @notify:   function to call when the idle is removed, or %NULL
 * 
 * Sets a function to be called when the child indicated by @pid 
 * exits, at the priority @priority.
 *
 * If you obtain @pid from g_spawn_async() or g_spawn_async_with_pipes() 
 * you will need to pass #G_SPAWN_DO_NOT_REAP_CHILD as flag to 
 * the spawn function for the child watching to work.
 * 
 * Note that on platforms where #GPid must be explicitly closed
 * (see g_spawn_close_pid()) @pid must not be closed while the
 * source is still active. Typically, you will want to call
 * g_spawn_close_pid() in the callback function for the source.
 * 
 * GLib supports only a single callback per process id.
 *
 * This internally creates a main loop source using 
 * g_child_watch_source_new() and attaches it to the main loop context 
 * using g_source_attach(). You can do these steps manually if you 
 * need greater control.
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 2.4
 **/
guint
g_child_watch_add_full (gint            priority,
			GPid            pid,
			GChildWatchFunc function,
			gpointer        data,
			GDestroyNotify  notify)
{
  GSource *source;
  guint id;
  
  g_return_val_if_fail (function != NULL, 0);

  source = g_child_watch_source_new (pid);

  if (priority != G_PRIORITY_DEFAULT)
    g_source_set_priority (source, priority);

  g_source_set_callback (source, (GSourceFunc) function, data, notify);
  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}

/**
 * g_child_watch_add:
 * @pid:      process id to watch. On POSIX the pid of a child process. On
 * Windows a handle for a process (which doesn't have to be a child).
 * @function: function to call
 * @data:     data to pass to @function
 * 
 * Sets a function to be called when the child indicated by @pid 
 * exits, at a default priority, #G_PRIORITY_DEFAULT.
 * 
 * If you obtain @pid from g_spawn_async() or g_spawn_async_with_pipes() 
 * you will need to pass #G_SPAWN_DO_NOT_REAP_CHILD as flag to 
 * the spawn function for the child watching to work.
 * 
 * Note that on platforms where #GPid must be explicitly closed
 * (see g_spawn_close_pid()) @pid must not be closed while the
 * source is still active. Typically, you will want to call
 * g_spawn_close_pid() in the callback function for the source.
 *
 * GLib supports only a single callback per process id.
 *
 * This internally creates a main loop source using 
 * g_child_watch_source_new() and attaches it to the main loop context 
 * using g_source_attach(). You can do these steps manually if you 
 * need greater control.
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 2.4
 **/
guint 
g_child_watch_add (GPid            pid,
		   GChildWatchFunc function,
		   gpointer        data)
{
  return g_child_watch_add_full (G_PRIORITY_DEFAULT, pid, function, data, NULL);
}


/* Idle functions */

static gboolean 
g_idle_prepare  (GSource  *source,
		 gint     *timeout)
{
  *timeout = 0;

  return TRUE;
}

static gboolean 
g_idle_check    (GSource  *source)
{
  return TRUE;
}

static gboolean
g_idle_dispatch (GSource    *source, 
		 GSourceFunc callback,
		 gpointer    user_data)
{
  if (!callback)
    {
      g_warning ("Idle source dispatched without callback\n"
		 "You must call g_source_set_callback().");
      return FALSE;
    }
  
  return callback (user_data);
}

/**
 * g_idle_source_new:
 * 
 * Creates a new idle source.
 *
 * The source will not initially be associated with any #GMainContext
 * and must be added to one with g_source_attach() before it will be
 * executed. Note that the default priority for idle sources is
 * %G_PRIORITY_DEFAULT_IDLE, as compared to other sources which
 * have a default priority of %G_PRIORITY_DEFAULT.
 * 
 * Return value: the newly-created idle source
 **/
GSource *
g_idle_source_new (void)
{
  GSource *source;

  source = g_source_new (&g_idle_funcs, sizeof (GSource));
  g_source_set_priority (source, G_PRIORITY_DEFAULT_IDLE);

  return source;
}

/**
 * g_idle_add_full:
 * @priority: the priority of the idle source. Typically this will be in the
 *            range between #G_PRIORITY_DEFAULT_IDLE and #G_PRIORITY_HIGH_IDLE.
 * @function: function to call
 * @data:     data to pass to @function
 * @notify:   function to call when the idle is removed, or %NULL
 * 
 * Adds a function to be called whenever there are no higher priority
 * events pending.  If the function returns %FALSE it is automatically
 * removed from the list of event sources and will not be called again.
 * 
 * This internally creates a main loop source using g_idle_source_new()
 * and attaches it to the main loop context using g_source_attach(). 
 * You can do these steps manually if you need greater control.
 * 
 * Return value: the ID (greater than 0) of the event source.
 **/
guint 
g_idle_add_full (gint           priority,
		 GSourceFunc    function,
		 gpointer       data,
		 GDestroyNotify notify)
{
  GSource *source;
  guint id;
  
  g_return_val_if_fail (function != NULL, 0);

  source = g_idle_source_new ();

  if (priority != G_PRIORITY_DEFAULT_IDLE)
    g_source_set_priority (source, priority);

  g_source_set_callback (source, function, data, notify);
  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}

/**
 * g_idle_add:
 * @function: function to call 
 * @data: data to pass to @function.
 * 
 * Adds a function to be called whenever there are no higher priority
 * events pending to the default main loop. The function is given the
 * default idle priority, #G_PRIORITY_DEFAULT_IDLE.  If the function
 * returns %FALSE it is automatically removed from the list of event
 * sources and will not be called again.
 * 
 * This internally creates a main loop source using g_idle_source_new()
 * and attaches it to the main loop context using g_source_attach(). 
 * You can do these steps manually if you need greater control.
 * 
 * Return value: the ID (greater than 0) of the event source.
 **/
guint 
g_idle_add (GSourceFunc    function,
	    gpointer       data)
{
  return g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, function, data, NULL);
}

/**
 * g_idle_remove_by_data:
 * @data: the data for the idle source's callback.
 * 
 * Removes the idle function with the given data.
 * 
 * Return value: %TRUE if an idle source was found and removed.
 **/
gboolean
g_idle_remove_by_data (gpointer data)
{
  return g_source_remove_by_funcs_user_data (&g_idle_funcs, data);
}

#define __G_MAIN_C__
#include "galiasdef.c"
