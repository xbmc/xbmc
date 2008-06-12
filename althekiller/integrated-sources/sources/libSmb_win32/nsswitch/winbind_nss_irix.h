/* 
   Unix SMB/CIFS implementation.

   Winbind daemon for ntdom nss module

   Copyright (C) Tim Potter 2000
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA  02111-1307, USA.   
*/

#ifndef _WINBIND_NSS_IRIX_H
#define _WINBIND_NSS_IRIX_H

/* following required to prevent warnings of double definition
 * of datum from ns_api.h
*/
#ifdef DATUM
#define _DATUM_DEFINED
#endif

#include <ns_api.h>

typedef enum
{
  NSS_STATUS_SUCCESS=NS_SUCCESS,
  NSS_STATUS_NOTFOUND=NS_NOTFOUND,
  NSS_STATUS_UNAVAIL=NS_UNAVAIL,
  NSS_STATUS_TRYAGAIN=NS_TRYAGAIN
} NSS_STATUS;

#endif /* _WINBIND_NSS_IRIX_H */
