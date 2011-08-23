#ifndef __NEWRES_H__
#define __NEWRES_H__

#if !defined(UNDER_CE)
#define UNDER_CE _WIN32_WCE
#endif

#if defined(_WIN32_WCE)
	#if !defined(WCEOLE_ENABLE_DIALOGEX)
		#define DIALOGEX DIALOG DISCARDABLE
	#endif
	#include <commctrl.h>
	#define  SHMENUBAR RCDATA
	#if defined(WIN32_PLATFORM_PSPC) && (_WIN32_WCE >= 300)
		#include <aygshell.h>
		#define AFXCE_IDR_SCRATCH_SHMENU  28700
	#else
		#define I_IMAGENONE		(-2)
		#define NOMENU			0xFFFF
		#define IDS_SHNEW		1

		#define IDM_SHAREDNEW        10
		#define IDM_SHAREDNEWDEFAULT 11
	#endif // _WIN32_WCE_PSPC
	#define AFXCE_IDD_SAVEMODIFIEDDLG 28701
#endif // _WIN32_WCE

#ifdef RC_INVOKED
#ifndef _INC_WINDOWS
#define _INC_WINDOWS
	#include "winuser.h"           // extract from windows header
//	#include "winver.h"   
#endif
#endif

#ifdef IDC_STATIC
#undef IDC_STATIC
#endif
#define IDC_STATIC      (-1)

#endif //__NEWRES_H__
