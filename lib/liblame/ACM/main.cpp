/**
 *
 * Lame ACM wrapper, encode/decode MP3 based RIFF/AVI files in MS Windows
 *
 *  Copyright (c) 2002 Steve Lhomme <steve.lhomme at free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
/*!
	\author Steve Lhomme
	\version \$Id: main.cpp,v 1.5 2006/12/25 21:37:34 robert Exp $
*/

#if !defined(STRICT)
#define STRICT
#endif // STRICT

#include <windows.h>

/// The ACM is considered as a driver and run in Kernel-Mode
/// So the new/delete operators have to be overriden in order to use memory
/// readable out of the calling process

void * operator new( unsigned int cb )
{
	return LocalAlloc(LPTR, cb); // VirtualAlloc
}

void operator delete(void *block) {
	LocalFree(block);
}

extern "C" {

	void *acm_Calloc( size_t num, size_t size )
	{
		return LocalAlloc(LPTR, num * size); // VirtualAlloc
	}

	void *acm_Malloc( size_t size )
	{
		return LocalAlloc(LPTR, size); // VirtualAlloc
	}

	void acm_Free( void * mem)
	{
		LocalFree(mem);
	}
};

////// End of memory instrumentation

#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>

#include <assert.h>

#include "AEncodeProperties.h"
#include "ACM.h"
#include "resource.h"
#include "adebug.h"


ADbg * debug = NULL;

LONG WINAPI DriverProc(DWORD dwDriverId, HDRVR hdrvr, UINT msg, LONG lParam1, LONG lParam2)
{

	switch (msg)
	{
		case DRV_OPEN: // acmDriverOpen
		{
			if (debug == NULL) {
				debug = new ADbg(DEBUG_LEVEL_CREATION);
				debug->setPrefix("LAMEdrv");
			}

			if (debug != NULL)
			{
				// Sent when the driver is opened.
				if (lParam2 != NULL)
					debug->OutPut(DEBUG_LEVEL_MSG, "DRV_OPEN (ID 0x%08X), pDesc = 0x%08X",dwDriverId,lParam2);
				else
					debug->OutPut(DEBUG_LEVEL_MSG, "DRV_OPEN (ID 0x%08X), pDesc = NULL",dwDriverId);
			}

			if (lParam2 != NULL) {
				LPACMDRVOPENDESC pDesc = (LPACMDRVOPENDESC)lParam2;

				if (pDesc->fccType != ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC) {
					if (debug != NULL)
					{
						debug->OutPut(DEBUG_LEVEL_FUNC_CODE, "wrong pDesc->fccType (0x%08X)",pDesc->fccType);
					}
					return NULL;
				}
			} else {
				if (debug != NULL)
				{
					debug->OutPut(DEBUG_LEVEL_FUNC_CODE, "pDesc == NULL");
				}
			}

			ACM * ThisACM = new ACM(GetDriverModuleHandle(hdrvr));

			if (debug != NULL)
			{
				debug->OutPut(DEBUG_LEVEL_FUNC_CODE, "OPENED instance 0x%08X",ThisACM);
			}

			return (LONG)ThisACM;// returns 0L to fail
								// value subsequently used
								// for dwDriverId.
		}
		break;

		case DRV_CLOSE: // acmDriverClose
		{
			if (debug != NULL)
			{
				// Sent when the driver is closed. Drivers are
				// unloaded when the open count reaches zero.
				debug->OutPut(DEBUG_LEVEL_MSG, "DRV_CLOSE");
			}

			ACM * ThisACM = (ACM *)dwDriverId;
			delete ThisACM;
			if (debug != NULL)
			{
				debug->OutPut(DEBUG_LEVEL_FUNC_CODE, "CLOSED instance 0x%08X",ThisACM);
				delete debug;
				debug = NULL;
			}
			return 1L;  // returns 0L to fail
		}
		break;

		case DRV_LOAD:
		{
			// nothing to do
			if (debug != NULL)
			{
//				debug->OutPut(DEBUG_LEVEL_MSG, "DRV_LOAD, version %s %s %s", ACM_VERSION, __DATE__, __TIME__);
				debug->OutPut(DEBUG_LEVEL_MSG, "DRV_LOAD, %s %s",  __DATE__, __TIME__);
			}
			return 1L;
		}
		break;

		case DRV_ENABLE:
		{
			// nothing to do
			if (debug != NULL)
			{
				debug->OutPut(DEBUG_LEVEL_MSG, "DRV_ENABLE");
			}
			return 1L;
		}
		break;

		case DRV_DISABLE:
		{
			// nothing to do
			if (debug != NULL)
			{
				debug->OutPut(DEBUG_LEVEL_MSG, "DRV_DISABLE");
			}
			return 1L;
		}
		break;

		case DRV_FREE:
		{
			if (debug != NULL)
			{
				debug->OutPut(DEBUG_LEVEL_MSG, "DRV_FREE");
			}
			return 1L;
		}
		break;

		default:
		{
			ACM * ThisACM = (ACM *)dwDriverId;

			if (ThisACM != NULL)
				return ThisACM->DriverProcedure(hdrvr, msg, lParam1, lParam2);
			else
			{
				if (debug != NULL)
				{
					debug->OutPut(DEBUG_LEVEL_MSG, "Driver not opened, unknown message (0x%08X), lParam1 = 0x%08X, lParam2 = 0x%08X", msg, lParam1, lParam2);
				}

				return DefDriverProc (dwDriverId, hdrvr, msg, lParam1, lParam2);
			}
		}
		break;
	}
}
 
