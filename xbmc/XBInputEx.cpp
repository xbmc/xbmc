// XBInputEx.cpp: implementation of the XBInputEx class.
//
//////////////////////////////////////////////////////////////////////

#include <xtl.h>
#include "XBInputEx.h"

//-----------------------------------------------------------------------------
// Globals for the Remote
//-----------------------------------------------------------------------------

FLOAT lasttime;

#define INTERVAL 0.1f

DWORD g_prevPacketNumber[10]={0,0,0,0,0,0,0,0,0,0};
// Global instance of input states
XINPUT_STATEEX g_InputStatesEx[4];

// Global instance of custom ir remote devices
XBIR_REMOTE g_IR_Remote[4];




//-----------------------------------------------------------------------------
// Name: XBInput_CreateIR_Remotes()
// Desc: Creates the infra-red remote devices
//-----------------------------------------------------------------------------
HRESULT XBInput_CreateIR_Remotes( XBIR_REMOTE** ppIR_Remote )
{

    // Get a mask of all currently available devices
    DWORD dwDeviceMask = XGetDevices( XDEVICE_TYPE_IR_REMOTE );

    // Open the devices
    for( DWORD i=0; i < XGetPortCount(); i++ )
    {

        ZeroMemory( &g_InputStatesEx[i], sizeof(XINPUT_STATEEX) );
        ZeroMemory( &g_IR_Remote[i], sizeof(XBIR_REMOTE) );
        if( dwDeviceMask & (1<<i) ) 
        {

            XINPUT_POLLING_PARAMETERS pollValues;
            pollValues.fAutoPoll       = TRUE;
            pollValues.fInterruptOut   = TRUE;
            pollValues.bInputInterval  = 255;  
            pollValues.bOutputInterval = 8;
            pollValues.ReservedMBZ1    = 0;
            pollValues.ReservedMBZ2    = 0;


            // Get a handle to the device
            g_IR_Remote[i].hDevice = XInputOpen( XDEVICE_TYPE_IR_REMOTE, i, 
                                                XDEVICE_NO_SLOT, &pollValues );

        }
    }

    // Created devices are kept global, but for those who prefer member
    // variables, they can get a pointer to the remote returned.
    if( ppIR_Remote )
        (*ppIR_Remote) = g_IR_Remote;

    return S_OK;
}






//-----------------------------------------------------------------------------
// Name: XBInput_GetInput()
// Desc: Processes input from the IR Remote
//-----------------------------------------------------------------------------
VOID XBInput_GetInput( XBIR_REMOTE* pIR_Remote, FLOAT m_fTime)
{
	if (m_fTime < lasttime + INTERVAL)
	{
	    for( DWORD i=0; i < XGetPortCount(); i++ )
	    {
			if (pIR_Remote[i].hDevice)
				pIR_Remote[i].wPressedButtons = 0;
		}
		return;
	}
	
	lasttime = m_fTime;
    XINPUT_POLLING_PARAMETERS pollValues;
    pollValues.fAutoPoll       = TRUE;
    pollValues.fInterruptOut   = TRUE;
    pollValues.bInputInterval  = 255;  
    pollValues.bOutputInterval = 8;
    pollValues.ReservedMBZ1    = 0;
    pollValues.ReservedMBZ2    = 0;

    if( NULL == pIR_Remote )
        pIR_Remote = g_IR_Remote;

    // TCR 3-21 Controller Discovery
    // Get status about Remote insertions and removals. Note that, in order to
    // not miss devices, we will check for removed device BEFORE checking for
    // insertions.  
	// Looks like the Remote doesn't send a signal when it's removed...
    DWORD dwInsertions, dwRemovals;
    XGetDeviceChanges( XDEVICE_TYPE_IR_REMOTE, &dwInsertions, &dwRemovals );


    // Loop through all gamepads
    for( DWORD i=0; i < XGetPortCount(); i++ )
    {
        // Handle removed devices.
        pIR_Remote[i].bRemoved = ( dwRemovals & (1<<i) ) ? TRUE : FALSE;
        if( pIR_Remote[i].bRemoved )
        {
            // if the controller was removed after XGetDeviceChanges but before
            // XInputOpen, the device handle will be NULL
            if( pIR_Remote[i].hDevice )
                XInputClose( pIR_Remote[i].hDevice );
            pIR_Remote[i].hDevice = NULL;

        }

        // Handle inserted devices
        pIR_Remote[i].bInserted = ( dwInsertions & (1<<i) ) ? TRUE : FALSE;

        if( pIR_Remote[i].bInserted ) 
        {

            // TCR 1-14 Device Types

            pIR_Remote[i].hDevice = XInputOpen( XDEVICE_TYPE_IR_REMOTE, i, 
                                               XDEVICE_NO_SLOT, &pollValues);
	
        }


        // If we have a valid device, poll it's state and track button changes
        if( pIR_Remote[i].hDevice )
        {
            // Read the input state
            XInputGetState( pIR_Remote[i].hDevice, (XINPUT_STATE*) &g_InputStatesEx[i] );

						if (g_prevPacketNumber[i] != g_InputStatesEx[i].dwPacketNumber)
						{
							g_prevPacketNumber[i]=g_InputStatesEx[i].dwPacketNumber;
							// Copy remote to local structure
							memcpy( &pIR_Remote[i], &g_InputStatesEx[i].IR_Remote, sizeof(XINPUT_IR_REMOTE) );

							// Get the currently pressed button	
							if (pIR_Remote[i].wLastButtons!=pIR_Remote[i].wButtons)
								pIR_Remote[i].wPressedButtons = pIR_Remote[i].wButtons;
							else
							{
								pIR_Remote[i].wPressedButtons = 0;
								pIR_Remote[i].wButtons = 0;
							}

						pIR_Remote[i].wLastButtons = pIR_Remote[i].wButtons;				
						}
						// Needs to reset it... don't know a better way to do it
						XInputClose( pIR_Remote[i].hDevice);
            pIR_Remote[i].hDevice = XInputOpen( XDEVICE_TYPE_IR_REMOTE, i, 
                                               XDEVICE_NO_SLOT, &pollValues);
        }
    }	
}


