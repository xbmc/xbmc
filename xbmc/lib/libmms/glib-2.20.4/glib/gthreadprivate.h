/* GLIB - Library of useful routines for C programming
 *
 * gthreadprivate.h - GLib internal thread system related declarations.
 *
 *  Copyright (C) 2003 Sebastian Wilhelmi
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

#ifndef __G_THREADPRIVATE_H__
#define __G_THREADPRIVATE_H__

G_BEGIN_DECLS

/* System thread identifier comparision and assignment */
#if GLIB_SIZEOF_SYSTEM_THREAD == SIZEOF_VOID_P
# define g_system_thread_equal_simple(thread1, thread2)			\
   ((thread1).dummy_pointer == (thread2).dummy_pointer)
# define g_system_thread_assign(dest, src)				\
   ((dest).dummy_pointer = (src).dummy_pointer)
#else /* GLIB_SIZEOF_SYSTEM_THREAD != SIZEOF_VOID_P */
# define g_system_thread_equal_simple(thread1, thread2)			\
   (memcmp (&(thread1), &(thread2), GLIB_SIZEOF_SYSTEM_THREAD) == 0)
# define g_system_thread_assign(dest, src)				\
   (memcpy (&(dest), &(src), GLIB_SIZEOF_SYSTEM_THREAD))
#endif /* GLIB_SIZEOF_SYSTEM_THREAD == SIZEOF_VOID_P */

#define g_system_thread_equal(thread1, thread2)				\
  (g_thread_functions_for_glib_use.thread_equal ? 			\
   g_thread_functions_for_glib_use.thread_equal (&(thread1), &(thread2)) :\
   g_system_thread_equal_simple((thread1), (thread2)))

/* Is called from gthread/gthread-impl.c */
void g_thread_init_glib (void);

/* base initializers, may only use g_mutex_new(), g_cond_new() */
G_GNUC_INTERNAL void _g_mem_thread_init_noprivate_nomessage (void);
/* initializers that may also use g_private_new() */
G_GNUC_INTERNAL void _g_slice_thread_init_nomessage	    (void);
G_GNUC_INTERNAL void _g_messages_thread_init_nomessage      (void);

/* full fledged initializers */
G_GNUC_INTERNAL void _g_convert_thread_init (void);
G_GNUC_INTERNAL void _g_rand_thread_init (void);
G_GNUC_INTERNAL void _g_main_thread_init (void);
G_GNUC_INTERNAL void _g_atomic_thread_init (void);
G_GNUC_INTERNAL void _g_utils_thread_init (void);

#ifdef G_OS_WIN32
G_GNUC_INTERNAL void _g_win32_thread_init (void);
#endif /* G_OS_WIN32 */

G_END_DECLS

#endif /* __G_THREADPRIVATE_H__ */
