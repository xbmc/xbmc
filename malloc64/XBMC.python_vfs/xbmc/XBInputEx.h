// XBInputEx.h: interface for the XBInputEx class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XBINPUTEX_H__A3816A1D_6A04_4295_95C0_AF9708BA0D07__INCLUDED_)
#define AFX_XBINPUTEX_H__A3816A1D_6A04_4295_95C0_AF9708BA0D07__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifdef __cplusplus
extern "C"
{
#endif

#include "XBIRRemote.h"

#ifdef __cplusplus
}
#endif



typedef struct _XINPUT_STATEEX
{
  DWORD dwPacketNumber;
  XINPUT_IR_REMOTE IR_Remote;
}
XINPUT_STATEEX, *PXINPUT_STATEEX;


//-----------------------------------------------------------------------------
// Name: struct XBGAMEPAD
// Desc: structure for holding Game pad data
//-----------------------------------------------------------------------------
struct XBIR_REMOTE : public XINPUT_IR_REMOTE
{
  // Device properties
  XINPUT_CAPABILITIES caps;
  HANDLE hDevice;

  // Rumble properties
  XINPUT_RUMBLE Remote_Feedback;
  XINPUT_FEEDBACK Feedback;

  // Flags for whether game pad was just inserted or removed
  BOOL bInserted;
  BOOL bRemoved;

  // Flag for held down push
  BOOL bHeldDown;
};

//-----------------------------------------------------------------------------
// Global access to ir remote devices
//-----------------------------------------------------------------------------
extern XBIR_REMOTE g_IR_Remote[4];

//-----------------------------------------------------------------------------
// Name: XBInput_CreateIR_Remotes()
// Desc: Creates the ir remote devices
//-----------------------------------------------------------------------------
HRESULT XBInput_CreateIR_Remotes( );

//-----------------------------------------------------------------------------
// Name: XBInput_GetInput()
// Desc: Processes input from the ir remote
//-----------------------------------------------------------------------------
VOID XBInput_GetInput( XBIR_REMOTE* pIR_Remote = NULL);



#endif // !defined(AFX_XBINPUTEX_H__A3816A1D_6A04_4295_95C0_AF9708BA0D07__INCLUDED_)

