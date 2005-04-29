// XBInputEx.cpp: implementation of the XBInputEx class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdio.h>
#include "XBInputEx.h"
#include "Settings.h"

//#define REMOTE_DEBUG 1

//-----------------------------------------------------------------------------
// Globals for the Remote
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif

  extern XPP_DEVICE_TYPE XDEVICE_TYPE_IR_REMOTE_TABLE;
#define     XDEVICE_TYPE_IR_REMOTE           (&XDEVICE_TYPE_IR_REMOTE_TABLE)

#ifdef __cplusplus
}
#endif

DWORD g_prevPacketNumber[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
DWORD g_eventsSinceFirstEvent[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

bool bIsRepeating;

// Global instance of input states
XINPUT_STATEEX g_InputStatesEx[4];

// Global instance of custom ir remote devices
XBIR_REMOTE g_IR_Remote[4];




//-----------------------------------------------------------------------------
// Name: XBInput_CreateIR_Remotes()
// Desc: Creates the infra-red remote devices
//-----------------------------------------------------------------------------
HRESULT XBInput_CreateIR_Remotes( )
{

  // Get a mask of all currently available devices
  DWORD dwDeviceMask = XGetDevices( XDEVICE_TYPE_IR_REMOTE );

  // Open the devices
  for ( DWORD i = 0; i < XGetPortCount(); i++ )
  {
    ZeroMemory( &g_InputStatesEx[i], sizeof(XINPUT_STATEEX) );
    ZeroMemory( &g_IR_Remote[i], sizeof(XBIR_REMOTE) );
    if ( dwDeviceMask & (1 << i) )
    {
      XINPUT_POLLING_PARAMETERS pollValues;
      pollValues.fAutoPoll = TRUE;
      pollValues.fInterruptOut = TRUE;
      pollValues.bInputInterval = 8;
      pollValues.bOutputInterval = 8;
      pollValues.ReservedMBZ1 = 0;
      pollValues.ReservedMBZ2 = 0;

      // Get a handle to the device
      g_IR_Remote[i].hDevice = XInputOpen( XDEVICE_TYPE_IR_REMOTE, i,
                                           XDEVICE_NO_SLOT, &pollValues );

      g_prevPacketNumber[i] = -1;
      g_eventsSinceFirstEvent[i] = 0;
    }
  }

  // Created devices are kept global, but for those who prefer member
  // variables, they can get a pointer to the remote returned.

  return S_OK;
}


//-----------------------------------------------------------------------------
// Name: XBInput_GetInput()
// Desc: Processes input from the IR Remote
//-----------------------------------------------------------------------------
VOID XBInput_GetInput( XBIR_REMOTE* pIR_Remote)
{
  if ( NULL == pIR_Remote ) return ;
  if (pIR_Remote)
  {
    for (int i = 0; i < 4; ++i)
    {
      ZeroMemory( &pIR_Remote[i], sizeof(XBIR_REMOTE) );
    }
  }
  XINPUT_POLLING_PARAMETERS pollValues;
  pollValues.fAutoPoll = TRUE;
  pollValues.fInterruptOut = TRUE;
  pollValues.bInputInterval = 8;
  pollValues.bOutputInterval = 8;
  pollValues.ReservedMBZ1 = 0;
  pollValues.ReservedMBZ2 = 0;

  // TCR 3-21 Controller Discovery
  // Get status about Remote insertions and removals. Note that, in order to
  // not miss devices, we will check for removed device BEFORE checking for
  // insertions.
  // Looks like the Remote doesn't send a signal when it's removed...
  DWORD dwInsertions, dwRemovals;
  if ( XGetDeviceChanges( XDEVICE_TYPE_IR_REMOTE, &dwInsertions, &dwRemovals ))
  {
    // Loop through all gamepads
    for ( DWORD i = 0; i < XGetPortCount(); i++ )
    {
      // Handle removed devices.
      g_IR_Remote[i].bRemoved = ( dwRemovals & (1 << i) ) ? TRUE : FALSE;
      if ( g_IR_Remote[i].bRemoved )
      {
        // if the controller was removed after XGetDeviceChanges but before
        // XInputOpen, the device handle will be NULL
        if ( g_IR_Remote[i].hDevice )
          XInputClose( g_IR_Remote[i].hDevice );
        g_IR_Remote[i].hDevice = NULL;
      }

      // Handle inserted devices
      g_IR_Remote[i].bInserted = ( dwInsertions & (1 << i) ) ? TRUE : FALSE;

      if ( g_IR_Remote[i].bInserted )
      {
        // TCR 1-14 Device Types
        g_IR_Remote[i].hDevice = XInputOpen( XDEVICE_TYPE_IR_REMOTE, i, XDEVICE_NO_SLOT, &pollValues);
      }
    }
  }

  // Loop through all gamepads
  for ( DWORD i = 0; i < XGetPortCount(); i++ )
  {
    // If we have a valid device, poll it's state and track button changes
    if ( g_IR_Remote[i].hDevice )
    {
      // Read the input state
      ZeroMemory( &g_InputStatesEx[i], sizeof(XINPUT_STATEEX) );
      if (ERROR_SUCCESS == XInputGetState( g_IR_Remote[i].hDevice, (XINPUT_STATE*) &g_InputStatesEx[i] ))
      {
        if (g_prevPacketNumber[i] != g_InputStatesEx[i].dwPacketNumber)
        {
          // Got a fresh packet
          g_prevPacketNumber[i] = g_InputStatesEx[i].dwPacketNumber;

          // Count the number of events since firstEvent was set (when repeat or button release)
          // Seems that firstEvent is often the first button push, but can also be
          // any event with counter not equal to 62 through 68.  (Not sure why exactly :P)
          if (g_InputStatesEx[i].IR_Remote.firstEvent > 0 || g_InputStatesEx[i].IR_Remote.counter < 62 || g_InputStatesEx[i].IR_Remote.counter > 68)
          {
            g_eventsSinceFirstEvent[i] = 0;
            bIsRepeating = false;
          }
          else
          {
            g_eventsSinceFirstEvent[i]++;
          }
          
#ifdef REMOTE_DEBUG
              char szTmp[256];
               sprintf(szTmp, "pkt:%i cnt:%i region:%i wbuttons:%i firstEvent:%i sinceFirst:%i...",
                   g_prevPacketNumber[i],
                   g_InputStatesEx[i].IR_Remote.counter,
                   g_InputStatesEx[i].IR_Remote.region,
                   g_InputStatesEx[i].IR_Remote.wButtons,
                   g_InputStatesEx[i].IR_Remote.firstEvent,
                   g_eventsSinceFirstEvent[i], timeGetTime());
#endif

          bool bSendMessage = true;
          // If this is the first event or if at least 2 non first events have passed (assume repeat)
          if (g_eventsSinceFirstEvent[i] > 0 && !bIsRepeating)
          { // check for repeats (g_stSettings.m_iRepeatDelayIR is in milliseconds, so to translate
            // into packets it's about delay/60, as each packet comes approximately every 60ms.
            if ((int)g_eventsSinceFirstEvent[i] < g_stSettings.m_iRepeatDelayIR/60)
              bSendMessage = false;
            else
              bIsRepeating = true;
          }
          
          if (bSendMessage)
          {
            // Copy remote to local structure
            memcpy( &pIR_Remote[i], &g_InputStatesEx[i].IR_Remote, sizeof(XINPUT_IR_REMOTE) );
            pIR_Remote[i].hDevice = (HANDLE)1;
            
#ifdef REMOTE_DEBUG
                  strcat(szTmp, "accepted\n");
             
                 }
                 else
                 {
                  strcat(szTmp, "ignored\n");
#endif
            
          }
#ifdef REMOTE_DEBUG 
                 CLog::Log(LOGERROR, "REMOTE: %s", szTmp);
#endif
        }
      }
      else
      {
        // Needs to reset it... don't know a better way to do it
        XInputClose( g_IR_Remote[i].hDevice);
        g_IR_Remote[i].hDevice = XInputOpen( XDEVICE_TYPE_IR_REMOTE, i,
                                             XDEVICE_NO_SLOT, &pollValues);
        g_prevPacketNumber[i] = -1;
        g_eventsSinceFirstEvent[i] = 0;
      }
    }
  }
}


