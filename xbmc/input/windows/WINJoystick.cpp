/*
*      Copyright (C) 2012 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "WINJoystick.h"
#include "input/ButtonTranslator.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#include <math.h>

#include <dinput.h>
#include <dinputd.h>

using namespace std;

extern HWND g_hWnd;

#define MAX_AXISAMOUNT  32768
#define AXIS_MIN       -32768  /* minimum value for axis coordinate */
#define AXIS_MAX        32767  /* maximum value for axis coordinate */

#if !defined(HAS_SDL)
#define SDL_HAT_CENTERED    0x00
#define SDL_HAT_UP          0x01
#define SDL_HAT_RIGHT       0x02
#define SDL_HAT_DOWN        0x04
#define SDL_HAT_LEFT        0x08
#define SDL_HAT_RIGHTUP     (SDL_HAT_RIGHT|SDL_HAT_UP)
#define SDL_HAT_RIGHTDOWN   (SDL_HAT_RIGHT|SDL_HAT_DOWN)
#define SDL_HAT_LEFTUP      (SDL_HAT_LEFT|SDL_HAT_UP)
#define SDL_HAT_LEFTDOWN    (SDL_HAT_LEFT|SDL_HAT_DOWN)
#endif

CJoystick::CJoystick()
{
  CSingleLock lock(m_critSection);
  Reset(true);
  m_joystickEnabled = false;
  m_NumAxes = 0;
  m_AxisId = 0;
  m_JoyId = 0;
  m_ButtonId = 0;
  m_HatId = 0;
  m_HatState = SDL_HAT_CENTERED;
  m_ActiveFlags = JACTIVE_NONE;
  SetDeadzone(0);

  m_pDI = NULL;
  m_lastPressTicks = 0;
  m_lastTicks = 0;
}

CJoystick::~CJoystick()
{
  ReleaseJoysticks();
}

void CJoystick::ReleaseJoysticks()
{
  CSingleLock lock(m_critSection);
  // Unacquire the device one last time just in case
  // the app tried to exit while the device is still acquired.
  for(std::vector<LPDIRECTINPUTDEVICE8>::iterator it = m_pJoysticks.begin(); it != m_pJoysticks.end(); ++it)
  {
    if( (*it) )
      (*it)->Unacquire();
    SAFE_RELEASE( (*it) );
  }
  m_pJoysticks.clear();
  m_JoystickNames.clear();
  m_devCaps.clear();
  m_HatId = 0;
  m_ButtonId = 0;
  m_HatState = SDL_HAT_CENTERED;
  m_ActiveFlags = JACTIVE_NONE;
  Reset(true);
  m_lastPressTicks = 0;
  m_lastTicks = 0;
  // Release any DirectInput objects.
  SAFE_RELEASE( m_pDI );
}

BOOL CALLBACK CJoystick::EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext )
{
  HRESULT hr;
  CJoystick* p_this = (CJoystick*) pContext;
  LPDIRECTINPUTDEVICE8    pJoystick = NULL;

  // Obtain an interface to the enumerated joystick.
  hr = p_this->m_pDI->CreateDevice( pdidInstance->guidInstance, &pJoystick, NULL );
  if( SUCCEEDED( hr ) )
  {
    // Set the data format to "simple joystick" - a predefined data format
    //
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    if( SUCCEEDED( hr = pJoystick->SetDataFormat( &c_dfDIJoystick2 ) ) )
    {
      // Set the cooperative level to let DInput know how this device should
      // interact with the system and with other DInput applications.
      if( SUCCEEDED( hr = pJoystick->SetCooperativeLevel( g_hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND ) ) )
      {
        DIDEVCAPS diDevCaps;
        diDevCaps.dwSize = sizeof(DIDEVCAPS);
        if (SUCCEEDED(hr = pJoystick->GetCapabilities(&diDevCaps)))
        {
          CLog::Log(LOGNOTICE, __FUNCTION__" : Enabled Joystick: %s", pdidInstance->tszProductName);
          CLog::Log(LOGNOTICE, __FUNCTION__" : Total Axis: %d Total Hats: %d Total Buttons: %d", diDevCaps.dwAxes, diDevCaps.dwPOVs, diDevCaps.dwButtons);
          p_this->m_pJoysticks.push_back(pJoystick);
          p_this->m_JoystickNames.push_back(pdidInstance->tszProductName);
          p_this->m_devCaps.push_back(diDevCaps);
        }
        else
          CLog::Log(LOGDEBUG, __FUNCTION__" : Failed to GetCapabilities for: %s", pdidInstance->tszProductName);
      }
      else
        CLog::Log(LOGDEBUG, __FUNCTION__" : Failed to SetCooperativeLevel on: %s", pdidInstance->tszProductName);

    }
    else
      CLog::Log(LOGDEBUG, __FUNCTION__" : Failed to SetDataFormat on: %s", pdidInstance->tszProductName);
  }
  else
    CLog::Log(LOGDEBUG, __FUNCTION__" : Failed to CreateDevice: %s", pdidInstance->tszProductName);

  return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// Name: EnumObjectsCallback()
// Desc: Callback function for enumerating objects (axes, buttons, POVs) on a
//       joystick. This function enables user interface elements for objects
//       that are found to exist, and scales axes min/max values.
//-----------------------------------------------------------------------------
BOOL CALLBACK CJoystick::EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext )
{

  LPDIRECTINPUTDEVICE8 pJoy = (LPDIRECTINPUTDEVICE8) pContext;

  // For axes that are returned, set the DIPROP_RANGE property for the
  // enumerated axis in order to scale min/max values.
  if( pdidoi->dwType & DIDFT_AXIS )
  {
      DIPROPRANGE diprg;
      diprg.diph.dwSize = sizeof( DIPROPRANGE );
      diprg.diph.dwHeaderSize = sizeof( DIPROPHEADER );
      diprg.diph.dwHow = DIPH_BYID;
      diprg.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
      diprg.lMin = AXIS_MIN;
      diprg.lMax = AXIS_MAX;

      // Set the range for the axis
      if( FAILED( pJoy->SetProperty( DIPROP_RANGE, &diprg.diph ) ) )
        CLog::Log(LOGDEBUG, __FUNCTION__" : Failed to set property on %s", pdidoi->tszName);
  }

  return DIENUM_CONTINUE;
}

void CJoystick::Initialize()
{
  if (!IsEnabled())
    return;

  HRESULT hr;

  // clear old joystick names
  ReleaseJoysticks();
  CSingleLock lock(m_critSection);

  if( FAILED( hr = DirectInput8Create( GetModuleHandle( NULL ), DIRECTINPUT_VERSION, IID_IDirectInput8, ( VOID** )&m_pDI, NULL ) ) )
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" : Failed to create DirectInput");
    return;
  }

  if( FAILED( hr = m_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, this, DIEDFL_ATTACHEDONLY ) ) )
    return;

  if(m_pJoysticks.size() == 0)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" : No Joystick found");
    return;
  }

  for(std::vector<LPDIRECTINPUTDEVICE8>::iterator it = m_pJoysticks.begin(); it != m_pJoysticks.end(); ++it)
  {
    LPDIRECTINPUTDEVICE8 pJoy = (*it);
    // Enumerate the joystick objects. The callback function enabled user
    // interface elements for objects that are found, and sets the min/max
    // values property for discovered axes.
    if( FAILED( hr = pJoy->EnumObjects( EnumObjectsCallback, pJoy, DIDFT_ALL ) ) )
      CLog::Log(LOGDEBUG, __FUNCTION__" : Failed to enumerate objects");
  }

  m_JoyId = -1;

  // Set deadzone range
  SetDeadzone(g_advancedSettings.m_controllerDeadzone);
}

void CJoystick::Reset(bool axis /*=true*/)
{
  if (axis)
  {
    SetAxisActive(false);
    for (int i = 0 ; i<MAX_AXES ; i++)
    {
      ResetAxis(i);
    }
  }
}

void CJoystick::Update()
{
  if (!IsEnabled())
    return;

  int buttonId    = -1;
  int axisId      = -1;
  int hatId       = -1;
  int numhat      = -1;
  int numj        = m_pJoysticks.size();
  if (numj <= 0)
    return;

  // go through all joysticks
  for (int j = 0; j<numj; j++)
  {
    LPDIRECTINPUTDEVICE8 pjoy = m_pJoysticks[j];
    HRESULT hr;
    DIJOYSTATE2 js;           // DInput joystick state

    m_NumAxes = (m_devCaps[j].dwAxes > MAX_AXES) ? MAX_AXES : m_devCaps[j].dwAxes;
    numhat    = (m_devCaps[j].dwPOVs > 4) ? 4 : m_devCaps[j].dwPOVs;

    hr = pjoy->Poll();
    if( FAILED( hr ) )
    {
      int i=0;
      // DInput is telling us that the input stream has been
      // interrupted. We aren't tracking any state between polls, so
      // we don't have any special reset that needs to be done. We
      // just re-acquire and try again.
      hr = pjoy->Acquire();
      while( (hr == DIERR_INPUTLOST) && (i++ < 10)  )
          hr = pjoy->Acquire();

      // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
      // may occur when the app is minimized or in the process of
      // switching, so just try again later
      return;
    }

    // Get the input's device state
    if( FAILED( hr = pjoy->GetDeviceState( sizeof( DIJOYSTATE2 ), &js ) ) )
      return; // The device should have been acquired during the Poll()

    // get button states first, they take priority over axis
    for( int b = 0; b < 128; b++ )
    {
      if( js.rgbButtons[b] & 0x80 )
      {
        m_JoyId = j;
        buttonId = b+1;
        j = numj-1;
        break;
      }
    }

    // get hat position
    m_HatState = SDL_HAT_CENTERED;
    for (int h = 0; h < numhat; h++)
    {
      if((LOWORD(js.rgdwPOV[h]) == 0xFFFF) != true)
      {
        m_JoyId = j;
        hatId = h + 1;
        j = numj-1;
        if ( (js.rgdwPOV[0] > JOY_POVLEFT) || (js.rgdwPOV[0] < JOY_POVRIGHT) )
          m_HatState |= SDL_HAT_UP;

        if ( (js.rgdwPOV[0] > JOY_POVFORWARD) && (js.rgdwPOV[0] < JOY_POVBACKWARD) )
          m_HatState |= SDL_HAT_RIGHT;

        if ( (js.rgdwPOV[0] > JOY_POVRIGHT) && (js.rgdwPOV[0] < JOY_POVLEFT) )
          m_HatState |= SDL_HAT_DOWN;

        if ( js.rgdwPOV[0] > JOY_POVBACKWARD )
          m_HatState |= SDL_HAT_LEFT;
        break;
      }
    }

    // get axis states
    m_Amount[0] = 0;
    m_Amount[1] = js.lX;
    m_Amount[2] = js.lY;
    m_Amount[3] = js.lZ;
    m_Amount[4] = js.lRx;
    m_Amount[5] = js.lRy;
    m_Amount[6] = js.lRz;

    m_AxisId = GetAxisWithMaxAmount();
    if (m_AxisId)
    {
      m_JoyId = j;
      j = numj-1;
      break;
    }
  }

  if(hatId==-1)
  {
    if(m_HatId!=0)
      CLog::Log(LOGDEBUG, "Joystick %d hat %d Centered", m_JoyId, abs(hatId));
    m_pressTicksHat = 0;
    SetHatActive(false);
    m_HatId = 0;
  }
  else
  {
    if(hatId!=m_HatId)
    {
      CLog::Log(LOGDEBUG, "Joystick %d hat %u Down", m_JoyId, hatId);
      m_HatId = hatId;
      m_pressTicksHat = XbmcThreads::SystemClockMillis();
    }
    SetHatActive();
  }

  if (buttonId==-1)
  {
    if (m_ButtonId!=0)
    {
      CLog::Log(LOGDEBUG, "Joystick %d button %d Up", m_JoyId, m_ButtonId);
    }
    m_pressTicksButton = 0;
    SetButtonActive(false);
    m_ButtonId = 0;
  }
  else
  {
    if (buttonId!=m_ButtonId)
    {
      CLog::Log(LOGDEBUG, "Joystick %d button %d Down", m_JoyId, buttonId);
      m_ButtonId = buttonId;
      m_pressTicksButton = XbmcThreads::SystemClockMillis();
    }
    SetButtonActive();
  }

}

bool CJoystick::GetHat(int &id, int &position,bool consider_repeat)
{
  if (!IsEnabled() || !IsHatActive())
  {
    id = position = 0;
    return false;
  }
  position = m_HatState;
  id = m_HatId;
  if (!consider_repeat)
    return true;

  uint32_t nowTicks = 0;

  if ((m_HatId>=0) && m_pressTicksHat)
  {
    // return the id if it's the first press
    if (m_lastPressTicks!=m_pressTicksHat)
    {
      m_lastPressTicks = m_pressTicksHat;
      return true;
    }
    nowTicks = XbmcThreads::SystemClockMillis();
    if ((nowTicks-m_pressTicksHat)<500) // 500ms delay before we repeat
      return false;
    if ((nowTicks-m_lastTicks)<100) // 100ms delay before successive repeats
      return false;

    m_lastTicks = nowTicks;
  }

  return true;
}

bool CJoystick::GetButton(int &id, bool consider_repeat)
{
  if (!IsEnabled() || !IsButtonActive())
  {
    id = 0;
    return false;
  }
  if (!consider_repeat)
  {
    id = m_ButtonId;
    return true;
  }

  uint32_t nowTicks = 0;

  if ((m_ButtonId>=0) && m_pressTicksButton)
  {
    // return the id if it's the first press
    if (m_lastPressTicks!=m_pressTicksButton)
    {
      m_lastPressTicks = m_pressTicksButton;
      id = m_ButtonId;
      return true;
    }
    nowTicks = XbmcThreads::SystemClockMillis();
    if ((nowTicks-m_pressTicksButton)<500) // 500ms delay before we repeat
    {
      return false;
    }
    if ((nowTicks-m_lastTicks)<100) // 100ms delay before successive repeats
    {
      return false;
    }
    m_lastTicks = nowTicks;
  }
  id = m_ButtonId;
  return true;
}

bool CJoystick::GetAxis (int &id)
{ 
  if (!IsEnabled() || !IsAxisActive()) 
  {
    id = 0;
    return false; 
  }
  id = m_AxisId; 
  return true; 
}

int CJoystick::GetAxisWithMaxAmount()
{
  int maxAmount = 0;
  int axis = 0;
  int tempf;
  for (int i = 1 ; i<=m_NumAxes ; i++)
  {
    tempf = abs(m_Amount[i]);
    if (tempf>m_DeadzoneRange && tempf>maxAmount)
    {
      maxAmount = tempf;
      axis = i;
    }
  }
  SetAxisActive(0 != maxAmount);
  return axis;
}

float CJoystick::GetAmount(int axis)
{
  if (m_Amount[axis] > m_DeadzoneRange)
    return (float)(m_Amount[axis]-m_DeadzoneRange)/(float)(MAX_AXISAMOUNT-m_DeadzoneRange);
  if (m_Amount[axis] < -m_DeadzoneRange)
    return (float)(m_Amount[axis]+m_DeadzoneRange)/(float)(MAX_AXISAMOUNT-m_DeadzoneRange);
  return 0;
}

void CJoystick::SetEnabled(bool enabled /*=true*/)
{
  if( enabled && !m_joystickEnabled )
  {
    m_joystickEnabled = true;
    Initialize();
  }
  else if( !enabled && m_joystickEnabled )
  {
    ReleaseJoysticks();
    m_joystickEnabled = false;
  }
}

float CJoystick::SetDeadzone(float val)
{
  if (val<0) val=0;
  if (val>1) val=1;
  m_DeadzoneRange = (int)(val*MAX_AXISAMOUNT);
  return val;
}

bool CJoystick::Reinitialize()
{
  Initialize();
  return true;
}

void CJoystick::Acquire()
{
  if (!IsEnabled())
    return;
  if(!m_pJoysticks.empty())
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Focus back, acquire Joysticks");
    for(std::vector<LPDIRECTINPUTDEVICE8>::iterator it = m_pJoysticks.begin(); it != m_pJoysticks.end(); ++it)
    {
      if( (*it) )
        (*it)->Acquire();
    }
  }
}
