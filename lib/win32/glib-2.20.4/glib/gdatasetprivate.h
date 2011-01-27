/* GLIB - Library of useful routines for C programming
 * gdataset-private.h: Internal macros for accessing dataset values
 * Copyright (C) 2005  Red Hat
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

#ifndef __G_DATASETPRIVATE_H__
#define __G_DATASETPRIVATE_H__

#include <glib.h>

G_BEGIN_DECLS

/* GET_FLAGS is implemented via atomic pointer access, to allow memory
 * barriers to take effect without acquiring the global dataset mutex.
 */
#define G_DATALIST_GET_FLAGS(datalist)				\
  ((gsize) g_atomic_pointer_get (datalist) & G_DATALIST_FLAGS_MASK)


G_END_DECLS

#endif /* __G_DATASETPRIVATE_H__ */
