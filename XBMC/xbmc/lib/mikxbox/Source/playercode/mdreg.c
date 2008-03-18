/*	MikMod sound library
	(c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id$

  Routine for registering all drivers in libmikmod for the current platform.

==============================================================================*/

#include "xbsection_start.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mikmod.h"
#include "mikmod_internals.h"

void _mm_registeralldrivers(void)
{
	/* Register network drivers */
#ifdef DRV_AF
	_mm_registerdriver(&drv_AF);
#endif
#ifdef DRV_ESD
	_mm_registerdriver(&drv_esd);
#endif

	/* Register hardware drivers - hardware mixing */
#ifdef DRV_ULTRA
	_mm_registerdriver(&drv_ultra);
#endif

	/* Register hardware drivers - software mixing */
#ifdef DRV_AIX
	_mm_registerdriver(&drv_aix);
#endif
#ifdef DRV_ALSA
	_mm_registerdriver(&drv_alsa);
#endif
#ifdef DRV_HP
	_mm_registerdriver(&drv_hp);
#endif
#ifdef DRV_OSS
	_mm_registerdriver(&drv_oss);
#endif
#ifdef DRV_SGI
	_mm_registerdriver(&drv_sgi);
#endif
#ifdef DRV_SUN
	_mm_registerdriver(&drv_sun);
#endif
#ifdef DRV_DART
	_mm_registerdriver(&drv_dart);
#endif
#ifdef DRV_OS2
	_mm_registerdriver(&drv_os2);
#endif
#ifdef DRV_DS
	_mm_registerdriver(&drv_ds);
#endif
#ifdef DRV_WIN
	_mm_registerdriver(&drv_win);
#endif
#ifdef DRV_MAC
	_mm_registerdriver(&drv_mac);
#endif

#ifdef WIN32
	_mm_registerdriver(&drv_ds_raw);
#endif
	
#ifndef WIN32
	/* Register disk writers */
	_mm_registerdriver(&drv_raw);
	_mm_registerdriver(&drv_wav);
#endif

	/* Register other drivers */
#ifdef DRV_PIPE
	_mm_registerdriver(&drv_pipe);
#endif
#ifndef macintosh 
#ifndef WIN32
	_mm_registerdriver(&drv_stdout);
#endif
#endif

	_mm_registerdriver(&drv_nos);
}

void MikMod_RegisterAllDrivers(void)
{
	MUTEX_LOCK(lists);
	_mm_registeralldrivers();
	MUTEX_UNLOCK(lists);
}

/* ex:set ts=4: */
