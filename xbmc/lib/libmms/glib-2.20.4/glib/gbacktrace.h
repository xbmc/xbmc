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

#ifndef __G_BACKTRACE_H__
#define __G_BACKTRACE_H__

#include <glib/gtypes.h>
#include <signal.h>

G_BEGIN_DECLS

/* Fatal error handlers.
 * g_on_error_query() will prompt the user to either
 * [E]xit, [H]alt, [P]roceed or show [S]tack trace.
 * g_on_error_stack_trace() invokes gdb, which attaches to the current
 * process and shows a stack trace.
 * These function may cause different actions on non-unix platforms.
 * The prg_name arg is required by gdb to find the executable, if it is
 * passed as NULL, g_on_error_query() will try g_get_prgname().
 */
void g_on_error_query (const gchar *prg_name);
void g_on_error_stack_trace (const gchar *prg_name);

/* Hacker macro to place breakpoints for selected machines.
 * Actual use is strongly discouraged of course ;)
 */
#if (defined (__i386__) || defined (__x86_64__)) && defined (__GNUC__) && __GNUC__ >= 2
#  define G_BREAKPOINT()	G_STMT_START{ __asm__ __volatile__ ("int $03"); }G_STMT_END
#elif (defined (_MSC_VER) || defined (__DMC__)) && defined (_M_IX86)
#  define G_BREAKPOINT()	G_STMT_START{ __asm int 3h }G_STMT_END
#elif defined (_MSC_VER)
#  define G_BREAKPOINT()	G_STMT_START{ __debugbreak(); }G_STMT_END
#elif defined (__alpha__) && !defined(__osf__) && defined (__GNUC__) && __GNUC__ >= 2
#  define G_BREAKPOINT()	G_STMT_START{ __asm__ __volatile__ ("bpt"); }G_STMT_END
#else	/* !__i386__ && !__alpha__ */
#  define G_BREAKPOINT()	G_STMT_START{ raise (SIGTRAP); }G_STMT_END
#endif	/* __i386__ */

G_END_DECLS

#endif /* __G_BACKTRACE_H__ */
