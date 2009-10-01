/*
   $Id: buffering_write.h,v 1.2 2004/12/19 01:43:38 rocky Exp $
 
   Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
   Copyright (C) 1998 Monty <xiphmont@mit.edu>
 
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.
 
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
 */

extern long blocking_write(int outf, char *buffer, long i_num);

/** buffering_write() - buffers data to a specified size before writing.
 *
 * Restrictions:
 * - MUST CALL BUFFERING_CLOSE() WHEN FINISHED!!!
 *
 */
extern long buffering_write(int outf, char *buffer, long num);

/** buffering_close() - writes out remaining buffered data before
 * closing file.
 *
 */
extern int buffering_close(int fd);


