/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** log.h
**
** Error logging header file
** $Id: log.h,v 1.4 2000/06/09 15:12:25 matt Exp $
*/

#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

extern int log_init(void);
extern void log_shutdown(void);
extern void log_print(const char *string);
extern void log_printf(const char *format, ...);

#endif /* _LOG_H_ */

/*
** $Log: log.h,v $
** Revision 1.4  2000/06/09 15:12:25  matt
** initial revision
**
*/

