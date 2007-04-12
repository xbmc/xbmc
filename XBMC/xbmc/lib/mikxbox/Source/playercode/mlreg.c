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

  Routine for registering all loaders in libmikmod for the current platform.

  Added by ozzy@orkysquad on 29/05/2002  compilation defines for all kind of 
    supported tracker formats.
	
==============================================================================*/
#include "xbsection_start.h"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mikmod.h"
#include "mikmod_internals.h"

void MikMod_RegisterAllLoaders_internal(void)
{
	#ifdef USE_669_FORMAT 
	_mm_registerloader(&load_669);
	#endif
	#ifdef USE_AMF_FORMAT 
	_mm_registerloader(&load_amf);
	#endif
	#ifdef USE_DSM_FORMAT
	_mm_registerloader(&load_dsm);
	#endif
	#ifdef USE_FAR_FORMAT
	_mm_registerloader(&load_far);
	#endif
	#ifdef USE_GDM_FORMAT
	_mm_registerloader(&load_gdm);
	#endif
	#ifdef USE_IT_FORMAT
	_mm_registerloader(&load_it);
	#endif
	#ifdef USE_IMF_FORMAT
	_mm_registerloader(&load_imf);
	#endif
	#ifdef USE_MOD_FORMAT
	_mm_registerloader(&load_mod);
	#endif
	#ifdef USE_MED_FORMAT
	_mm_registerloader(&load_med);
	#endif
	#ifdef USE_MTM_FORMAT
	_mm_registerloader(&load_mtm);
	#endif
	#ifdef USE_OKT_FORMAT
	_mm_registerloader(&load_okt);
	#endif
	#ifdef USE_S3M_FORMAT
	_mm_registerloader(&load_s3m);
	#endif
	#ifdef USE_STM_FORMAT
	_mm_registerloader(&load_stm);
	#endif
	#ifdef USE_STX_FORMAT
	_mm_registerloader(&load_stx);
	#endif
	#ifdef USE_ULT_FORMAT
	_mm_registerloader(&load_ult);
	#endif
	#ifdef USE_UNI_FORMAT
	_mm_registerloader(&load_uni);
	#endif
	#ifdef USE_XM_FORMAT
	_mm_registerloader(&load_xm);
	#endif
	#ifdef USE_M15_FORMAT
	_mm_registerloader(&load_m15);
	#endif
}

void MikMod_RegisterAllLoaders(void)
{
	MUTEX_LOCK(lists);
	MikMod_RegisterAllLoaders_internal();
	MUTEX_UNLOCK(lists);
}
/* ex:set ts=4: */
