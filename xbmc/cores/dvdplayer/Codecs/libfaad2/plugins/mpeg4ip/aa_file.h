/*
** MPEG4IP plugin for FAAD2
** Copyright (C) 2003 Bill May wmay@cisco.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: aa_file.h,v 1.1 2003/08/07 17:21:21 menno Exp $
**/
/*
 * aa_file.h - prototypes for aac files
 */
int create_media_for_aac_file (CPlayerSession *pspstr,
                   const char *name,
                   const char **errmsg);
