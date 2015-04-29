/*
*      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include <memory>
#include <dinput.h>

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
  m_joystickEnabled = false;
  SetDeadzone(0);
  Reset();
}

CJoystick::~CJoystick()
{
  ReleaseJoysticks();
}

void CJoystick::Reset()
{
  m_AxisIdx = -1;
  m_ButtonIdx = -1;
  m_HatIdx = -1;
  m_HatState = SDL_HAT_CENTERED;
  for (std::vector<AxisState>::iterator it = m_Axes.begin(); it != m_Axes.end(); ++it)
    it->reset();
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
  m_Axes.clear();
  m_devCaps.clear();
  Reset();
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
          std::string joyName(pdidInstance->tszProductName);
          p_this->m_JoystickNames.push_back(joyName);;
          p_this->m_Axes.resize(p_this->m_Axes.size() + 6); // dinput hardcodes to 6 axes
          p_this->m_devCaps.push_back(diDevCaps);

          // load axes configuration from keymap
          const AxesConfig* axesCfg = CButtonTranslator::GetInstance().GetAxesConfigFor(joyName);
          if (axesCfg != NULL)
          {
            for (AxesConfig::const_iterator it = axesCfg->begin(); it != axesCfg->end(); ++it)
            {
              int axis = p_this->MapAxis(pJoystick, it->axis - 1);
              p_this->m_Axes[axis].trigger = it->isTrigger;
              p_this->m_Axes[axis].rest = it->rest;
            }
          }
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

  // Set deadzone range
  SetDeadzone(g_advancedSettings.m_controllerDeadzone);
}

void CJoystick::Update()
{
  if (!IsEnabled())
    return;

  int axisNum;
  int buttonIdx = -1, buttonNum;
  int hatIdx = -1, hatNum;

  // go through all joysticks
  for (size_t j = 0; j < m_pJoysticks.size(); j++)
  {
    LPDIRECTINPUTDEVICE8 pjoy = m_pJoysticks[j];
    DIDEVCAPS caps = m_devCaps[j];
    HRESULT hr;
    DIJOYSTATE2 js;           // DInput joystick state
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
      Reset();
      return;
    }

    // Get the input's device state
    if( FAILED( hr = pjoy->GetDeviceState( sizeof( DIJOYSTATE2 ), &js ) ) )
    {
      Reset();
      return; // The device should have been acquired during the Poll()
    }

    // get button states first, they take priority over axis
    for (int b = 0; b < caps.dwButtons; b++)
    {
      if (js.rgbButtons[b] & 0x80)
      {
        buttonIdx = MapButton(pjoy, b);
        j = m_pJoysticks.size();
        break;
      }
    }

    // get hat position
    for (int h = 0; h < caps.dwPOVs; h++)
    {
      if ((LOWORD(js.rgdwPOV[h]) == 0xFFFF) != true)
      {
        uint8_t hatval = SDL_HAT_CENTERED;

        if ((js.rgdwPOV[0] > JOY_POVLEFT) || (js.rgdwPOV[0] < JOY_POVRIGHT))
          hatval |= SDL_HAT_UP;

        if ((js.rgdwPOV[0] > JOY_POVFORWARD) && (js.rgdwPOV[0] < JOY_POVBACKWARD))
          hatval |= SDL_HAT_RIGHT;

        if ((js.rgdwPOV[0] > JOY_POVRIGHT) && (js.rgdwPOV[0] < JOY_POVLEFT))
          hatval |= SDL_HAT_DOWN;

        if (js.rgdwPOV[0] > JOY_POVBACKWARD)
          hatval |= SDL_HAT_LEFT;

        if (hatval != SDL_HAT_CENTERED)
        {
          hatIdx = MapHat(pjoy, h);
          j = m_pJoysticks.size();
          m_HatState = hatval;
        }
      }
    }

    // get axis states
    int axisIdx = MapAxis(pjoy, 0);
    m_Axes[axisIdx + 0].val = js.lX;
    m_Axes[axisIdx + 1].val = js.lY;
    m_Axes[axisIdx + 2].val = js.lZ;
    m_Axes[axisIdx + 3].val = js.lRx;
    m_Axes[axisIdx + 4].val = js.lRy;
    m_Axes[axisIdx + 5].val = js.lRz;
  }

  LPDIRECTINPUTDEVICE8 pjoy;
  m_AxisIdx = GetAxisWithMaxAmount();
  if (m_AxisIdx >= 0)
  {
    MapAxis(m_AxisIdx, pjoy, axisNum);
    // CLog::Log(LOGDEBUG, "Joystick %d Axis %d Amount %d", JoystickIndex(pjoy), axisNum + 1, m_Axes[m_AxisIdx].val);
  }

  if (hatIdx == -1 && m_HatIdx != -1)
  {
    // now centered
    MapHat(m_HatIdx, pjoy, hatNum);
    CLog::Log(LOGDEBUG, "Joystick %d hat %u hat centered", JoystickIndex(pjoy), hatNum + 1);
    m_pressTicksHat = 0;
  }
  else if (hatIdx != m_HatIdx)
  {
    MapHat(hatIdx, pjoy, hatNum);
    CLog::Log(LOGDEBUG, "Joystick %d hat %u value %d", JoystickIndex(pjoy), hatNum + 1, m_HatState);
    m_pressTicksHat = XbmcThreads::SystemClockMillis();
  }
  m_HatIdx = hatIdx;

  if (buttonIdx == -1 && m_ButtonIdx != -1)
  {
    MapButton(m_ButtonIdx, pjoy, buttonNum);
    CLog::Log(LOGDEBUG, "Joystick %d button %d Up", JoystickIndex(pjoy), buttonNum + 1);
    m_pressTicksButton = 0;
    m_ButtonIdx = -1;
  }
  else if (buttonIdx != m_ButtonIdx)
  {
    MapButton(buttonIdx, pjoy, buttonNum);
    CLog::Log(LOGDEBUG, "Joystick %d button %d Down", JoystickIndex(pjoy), buttonNum + 1);
    m_pressTicksButton = XbmcThreads::SystemClockMillis();
  }
  m_ButtonIdx = buttonIdx;

}

bool CJoystick::GetHat(std::string &joyName, int &id, int &position, bool consider_repeat)
{
  if (!IsEnabled() || !IsHatActive())
    return false;
  LPDIRECTINPUTDEVICE8 joy;
  MapHat(m_HatIdx, joy, id);
  joyName = m_JoystickNames[JoystickIndex(joy)];
  position = m_HatState;

  if (!consider_repeat)
    return true;

  static uint32_t lastPressTicksHat = 0;
  // allow it if it is the first press
  if (lastPressTicksHat < m_pressTicksHat)
  {
    lastPressTicksHat = m_pressTicksHat;
    return true;
  }
  uint32_t nowTicks = XbmcThreads::SystemClockMillis();
  if (nowTicks - m_pressTicksHat < 500) // 500ms delay before we repeat
    return false;
  if (nowTicks - lastPressTicksHat < 100) // 100ms delay before successive repeats
    return false;
  lastPressTicksHat = nowTicks;

  return true;
}

bool CJoystick::GetButton(std::string &joyName, int &id, bool consider_repeat)
{
  if (!IsEnabled() || !IsButtonActive())
    return false;

  LPDIRECTINPUTDEVICE8 joy;
  MapButton(m_ButtonIdx, joy, id);
  joyName = m_JoystickNames[JoystickIndex(joy)];

  if (!consider_repeat)
    return true;

  static uint32_t lastPressTicksButton = 0;
  // allow it if it is the first press
  if (lastPressTicksButton < m_pressTicksButton)
  {
    lastPressTicksButton = m_pressTicksButton;
    return true;
  }
  uint32_t nowTicks = XbmcThreads::SystemClockMillis();
  if (nowTicks - m_pressTicksButton < 500) // 500ms delay before we repeat
    return false;
  if (nowTicks - lastPressTicksButton < 100) // 100ms delay before successive repeats
    return false;

  lastPressTicksButton = nowTicks;
  return true;
}

bool CJoystick::GetAxis(std::string &joyName, int &id)
{
  if (!IsEnabled() || !IsAxisActive())
    return false;

  LPDIRECTINPUTDEVICE8 joy;
  MapAxis(m_AxisIdx, joy, id);
  joyName = m_JoystickNames[JoystickIndex(joy)];

  return true;
}

bool CJoystick::GetAxes(std::list<std::pair<std::string, int> >& axes, bool consider_still)
{
  std::list<std::pair<std::string, int> > ret;
  if (!IsEnabled() || !IsAxisActive())
    return false;

  LPDIRECTINPUTDEVICE8 joy;
  int axisId;

  for (size_t i = 0; i < m_Axes.size(); ++i)
  {
    int deadzone = m_Axes[i].trigger ? 0 : m_DeadzoneRange;
    int amount = m_Axes[i].val - m_Axes[i].rest;
    if (consider_still || abs(amount) > deadzone)
    {
      MapAxis(i, joy, axisId);
      ret.push_back(std::pair<std::string, int>(m_JoystickNames[JoystickIndex(joy)], axisId));
    }
  }
  axes = ret;

  return true;
}

int CJoystick::GetAxisWithMaxAmount() const
{
  int maxAmount = 0, axis = -1;
  for (size_t i = 0; i < m_Axes.size(); i++)
  {
    int deadzone = m_Axes[i].trigger ? 0 : m_DeadzoneRange,
      amount = abs(m_Axes[i].val - m_Axes[i].rest);
    if (amount > deadzone && amount > maxAmount)
    {
      maxAmount = amount;
      axis = i;
    }
  }
  return axis;
}

float CJoystick::GetAmount(const std::string &joyName, int axisNum) const
{
  int joyIdx = JoystickIndex(joyName);
  if (joyIdx == -1)
    return 0;

  int axisIdx = MapAxis(m_pJoysticks[joyIdx], axisNum);
  int amount = m_Axes[axisIdx].val - m_Axes[axisIdx].rest;
  int range = MAX_AXISAMOUNT - m_Axes[axisIdx].rest;
  // deadzones on axes are undesirable
  int deadzone = m_Axes[axisIdx].trigger ? 0 : m_DeadzoneRange;
  if (amount > deadzone)
    return (float)(amount - deadzone) / (float)(range - deadzone);
  else if (amount < -deadzone)
    return (float)(amount + deadzone) / (float)(range - deadzone);

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

int CJoystick::JoystickIndex(const std::string &joyName) const
{
  for (size_t i = 0; i < m_JoystickNames.size(); ++i)
  {
    if (joyName == m_JoystickNames[i])
      return i;
  }
  return -1;
}

int CJoystick::JoystickIndex(LPDIRECTINPUTDEVICE8 joy) const
{
  for (size_t i = 0; i < m_pJoysticks.size(); ++i)
  {
    if (joy == m_pJoysticks[i])
      return i;
  }
  return -1;
}

int CJoystick::MapAxis(LPDIRECTINPUTDEVICE8 joy, int axisNum) const
{
  int idx = 0;
  size_t i;
  for (i = 0; i != m_pJoysticks.size() && m_pJoysticks[i] != joy; ++i)
    idx += m_devCaps[i].dwAxes;
  if (i == m_pJoysticks.size())
    return -1;
  else
    return idx + axisNum;
}

void CJoystick::MapAxis(int axisIdx, LPDIRECTINPUTDEVICE8 &joy, int &axisNum) const
{
  int axisCount = 0;
  size_t i;
  for (i = 0; i != m_pJoysticks.size() && m_pJoysticks[i] != joy && axisCount + m_devCaps[i].dwAxes <= axisIdx; ++i)
    axisCount += m_devCaps[i].dwAxes;
  if (i != m_pJoysticks.size())
  {
    joy = m_pJoysticks[i];
    axisNum = axisIdx - axisCount;
  }
  else
  {
    joy = NULL;
    axisNum = -1;
  }
}

int CJoystick::MapButton(LPDIRECTINPUTDEVICE8 joy, int buttonNum) const
{
  int idx = 0;
  size_t i;
  for (i = 0; i != m_pJoysticks.size() && m_pJoysticks[i] != joy; ++i)
    idx += m_devCaps[i].dwButtons;
  if (i == m_pJoysticks.size())
    return -1;
  else
    return idx + buttonNum;
}

void CJoystick::MapButton(int buttonIdx, LPDIRECTINPUTDEVICE8 &joy, int &buttonNum) const
{
  int buttonCount = 0;
  size_t i;
  for (i = 0; i != m_pJoysticks.size() && m_pJoysticks[i] != joy && buttonCount + m_devCaps[i].dwButtons <= buttonIdx; ++i)
    buttonCount += m_devCaps[i].dwButtons;
  if (i != m_pJoysticks.size())
  {
    joy = m_pJoysticks[i];
    buttonNum = buttonIdx - buttonCount;
  }
  else
  {
    joy = NULL;
    buttonNum = -1;
  }
}

int CJoystick::MapHat(LPDIRECTINPUTDEVICE8 joy, int hatNum) const
{
  int idx = 0;
  size_t i;
  for (i = 0; i != m_pJoysticks.size() && m_pJoysticks[i] != joy; ++i)
    idx += m_devCaps[i].dwPOVs;
  if (i == m_pJoysticks.size())
    return -1;
  else
    return idx + hatNum;
}

void CJoystick::MapHat(int hatIdx, LPDIRECTINPUTDEVICE8 &joy, int &hatNum) const
{
  int hatCount = 0;
  size_t i;
  for (i = 0; i != m_pJoysticks.size() && m_pJoysticks[i] != joy && hatCount + m_devCaps[i].dwPOVs <= hatIdx; ++i)
    hatCount += m_devCaps[i].dwPOVs;
  if (i != m_pJoysticks.size())
  {
    joy = m_pJoysticks[i];
    hatNum = hatIdx - hatCount;
  }
  else
  {
    joy = NULL;
    hatNum = -1;
  }
}
