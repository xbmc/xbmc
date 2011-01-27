/*
 * Copyright (C) 2008 the xine project
 * 
 * This file is part of LibMMS, an MMS protocol handling library.
 * 
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the ree Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: mms.c,v 1.31 2007/12/11 20:35:01 jwrdegoede Exp $
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <glib.h>

G_BEGIN_DECLS

gdouble     util_guint64_to_gdouble (guint64 value);

#ifdef WIN32
#define     guint64_to_gdouble(value)   util_guint64_to_gdouble(value)
#else
#define     guint64_to_gdouble(value)   ((gdouble) (value))
#endif

G_END_DECLS

#endif /* __UTILS_H__ */
