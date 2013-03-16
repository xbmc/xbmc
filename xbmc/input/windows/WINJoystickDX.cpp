/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "WINJoystickDX.h"
#include "system.h"
#include "utils/log.h"

#include <dinput.h>
#include <dinputd.h>

extern HWND g_hWnd;

#define MAX_AXISAMOUNT  32768
#define AXIS_MIN       -32768  /* minimum value for axis coordinate */
#define AXIS_MAX        32767  /* maximum value for axis coordinate */

#define JOY_POV_360  JOY_POVBACKWARD * 2
#define JOY_POV_NE   (JOY_POVFORWARD + JOY_POVRIGHT) / 2
#define JOY_POV_SE   (JOY_POVRIGHT + JOY_POVBACKWARD) / 2
#define JOY_POV_SW   (JOY_POVBACKWARD + JOY_POVLEFT) / 2
#define JOY_POV_NW   (JOY_POVLEFT + JOY_POV_360) / 2


// A context to hold our DirectInput handle and accumulated joystick objects
struct JoystickEnumContext
{
  JoystickArray  joystickItems;
  LPDIRECTINPUT8 pDirectInput;
};

// DirectInput handle, we hold onto it and release it when freeing resources
LPDIRECTINPUT8 CJoystickDX::m_pDirectInput = NULL;

CJoystickDX::CJoystickDX(LPDIRECTINPUTDEVICE8 joystickDevice, const std::string &name, const DIDEVCAPS &devCaps)
  : m_joystickDevice(joystickDevice), m_state()
{
  // m_state.id is set in Initialize() before adding it to the joystick list
  m_state.name        = name;
  m_state.buttonCount = std::min(m_state.buttonCount, (unsigned int)devCaps.dwButtons);
  m_state.hatCount    = std::min(m_state.hatCount, (unsigned int)devCaps.dwPOVs);
  m_state.axisCount   = std::min(m_state.axisCount, (unsigned int)devCaps.dwAxes);
}

/* static */
void CJoystickDX::Initialize(JoystickArray &joysticks)
{
  DeInitialize(joysticks);

  HRESULT hr;
  JoystickEnumContext context;

  if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8,
      (VOID**)&m_pDirectInput, NULL)))
  {
    CLog::Log(LOGERROR, __FUNCTION__" : Failed to create DirectInput");
  }
  else
  {
    context.pDirectInput = m_pDirectInput;
    if (FAILED(hr = context.pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback,
        &context, DIEDFL_ATTACHEDONLY)) || context.joystickItems.size() == 0)
    {
      CLog::Log(LOGERROR, __FUNCTION__" : No Joystick found");
    }
    else
    {
      for (JoystickArray::const_iterator it = context.joystickItems.begin(); it != context.joystickItems.end(); it++)
      {
        boost::shared_ptr<CJoystickDX> jdx = boost::dynamic_pointer_cast<CJoystickDX>(*it);
        if (jdx && jdx->InitAxes())
        {
          // Set the ID based on its position in the list
          jdx->m_state.id = joysticks.size();
          joysticks.push_back(jdx);
        }
      }
    }
  }
}

BOOL CALLBACK CJoystickDX::EnumJoysticksCallback(const DIDEVICEINSTANCE *pdidInstance, VOID *pContext)
{
  HRESULT hr;

  JoystickEnumContext *context = reinterpret_cast<JoystickEnumContext*>(pContext);
  LPDIRECTINPUTDEVICE8 pJoystick = NULL;

  // Obtain an interface to the enumerated joystick.
  hr = context->pDirectInput->CreateDevice(pdidInstance->guidInstance, &pJoystick, NULL);
  if (FAILED(hr))
    CLog::Log(LOGERROR, __FUNCTION__" : Failed to CreateDevice: %s", pdidInstance->tszProductName);
  else
  {
    // Set the data format to "simple joystick" - a predefined data format.
    //
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    if (FAILED(hr = pJoystick->SetDataFormat(&c_dfDIJoystick2)))
      CLog::Log(LOGERROR, __FUNCTION__" : Failed to SetDataFormat on: %s", pdidInstance->tszProductName);
    else
    {
      // Set the cooperative level to let DInput know how this device should
      // interact with the system and with other DInput applications.
      if (FAILED(hr = pJoystick->SetCooperativeLevel(g_hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
        CLog::Log(LOGERROR, __FUNCTION__" : Failed to SetCooperativeLevel on: %s", pdidInstance->tszProductName);
      else
      {
        DIDEVCAPS diDevCaps;
        diDevCaps.dwSize = sizeof(DIDEVCAPS);
        if (FAILED(hr = pJoystick->GetCapabilities(&diDevCaps)))
          CLog::Log(LOGERROR, __FUNCTION__" : Failed to GetCapabilities for: %s", pdidInstance->tszProductName);
        else
        {
          CLog::Log(LOGNOTICE, __FUNCTION__" : Enabled Joystick: \"%s\" (DirectInput)", pdidInstance->tszProductName);
          CLog::Log(LOGNOTICE, __FUNCTION__" : Total Axes: %d Total Hats: %d Total Buttons: %d",
              diDevCaps.dwAxes, diDevCaps.dwPOVs, diDevCaps.dwButtons);

          context->joystickItems.push_back(boost::shared_ptr<IJoystick>(new CJoystickDX(pJoystick,
              pdidInstance->tszProductName, diDevCaps)));
        }
      }
    }
  }

  return DIENUM_CONTINUE;
}

bool CJoystickDX::InitAxes()
{
  HRESULT hr;

  LPDIRECTINPUTDEVICE8 pJoy = m_joystickDevice;

  // Enumerate the joystick objects. The callback function enabled user
  // interface elements for objects that are found, and sets the min/max
  // values properly for discovered axes.
  if (FAILED(hr = pJoy->EnumObjects(EnumObjectsCallback, pJoy, DIDFT_ALL)))
  {
    CLog::Log(LOGERROR, __FUNCTION__" : Failed to enumerate objects");
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
// Name: EnumObjectsCallback()
// Desc: Callback function for enumerating objects (axes, buttons, POVs) on a
//       joystick. This function enables user interface elements for objects
//       that are found to exist, and scales axes min/max values.
//-----------------------------------------------------------------------------
BOOL CALLBACK CJoystickDX::EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext)
{
  LPDIRECTINPUTDEVICE8 pJoy = reinterpret_cast<LPDIRECTINPUTDEVICE8>(pContext);

  // For axes that are returned, set the DIPROP_RANGE property for the
  // enumerated axis in order to scale min/max values.
  if (pdidoi->dwType & DIDFT_AXIS)
  {
    DIPROPRANGE diprg;
    diprg.diph.dwSize = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow = DIPH_BYID;
    diprg.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
    diprg.lMin = AXIS_MIN;
    diprg.lMax = AXIS_MAX;

    // Set the range for the axis
    if (FAILED(pJoy->SetProperty(DIPROP_RANGE, &diprg.diph)))
      CLog::Log(LOGERROR, __FUNCTION__" : Failed to set property on %s", pdidoi->tszName);
  }
  return DIENUM_CONTINUE;
}

void CJoystickDX::Release()
{
  // Unacquire the device one last time just in case the app tried to exit
  // while the device is still acquired.
  if (m_joystickDevice)
    m_joystickDevice->Unacquire();
  SAFE_RELEASE(m_joystickDevice);
}

/* static */
void CJoystickDX::DeInitialize(JoystickArray &joysticks)
{
  for (int i = 0; i < (int)joysticks.size(); i++)
  {
    if (boost::dynamic_pointer_cast<CJoystickDX>(joysticks[i]))
      joysticks.erase(joysticks.begin() + i--);
  }
  // Release any DirectInput objects
  SAFE_RELEASE(m_pDirectInput);
}

void CJoystickDX::Update()
{
  HRESULT hr;

  LPDIRECTINPUTDEVICE8 pJoy = m_joystickDevice;
  DIJOYSTATE2 js; // DInput joystick state

  hr = pJoy->Poll();

  if (FAILED(hr))
  {
    int i = 0;
    // DInput is telling us that the input stream has been interrupted. We
    // aren't tracking any state between polls, so we don't have any special
    // reset that needs to be done. We just re-acquire and try again.
    do
    {
      hr = pJoy->Acquire();
    }
    while ((hr == DIERR_INPUTLOST) && (i++ < 10));

    // hr may be DIERR_OTHERAPPHASPRIO or other errors. This may occur when the
    // app is minimized or in the process of switching, so just try again later.
    return;
  }

  // Get the input's device state
  if (FAILED(hr = pJoy->GetDeviceState(sizeof(DIJOYSTATE2), &js)))
    return; // The device should have been acquired during the Poll()

  // Gamepad buttons
  for (unsigned int b = 0; b < m_state.buttonCount; b++)
    m_state.buttons[b] = ((js.rgbButtons[b] & 0x80) ? 1: 0);

  // Gamepad hats
  for (unsigned int h = 0; h < m_state.hatCount; h++)
  {
    m_state.hats[h].Center();
    bool bCentered = ((js.rgdwPOV[h] & 0xFFFF) == 0xFFFF);
    if (!bCentered)
    {
      if ((JOY_POV_NW <= js.rgdwPOV[h] && js.rgdwPOV[h] <= JOY_POV_360) || js.rgdwPOV[h] <= JOY_POV_NE)
        m_state.hats[h].up = 1;
      else if (JOY_POV_SE <= js.rgdwPOV[h] && js.rgdwPOV[h] <= JOY_POV_SW)
        m_state.hats[h].down = 1;

      if (JOY_POV_NE <= js.rgdwPOV[h] && js.rgdwPOV[h] <= JOY_POV_SE)
        m_state.hats[h].right = 1;
      else if (JOY_POV_SW <= js.rgdwPOV[h] && js.rgdwPOV[h] <= JOY_POV_NW)
        m_state.hats[h].left = 1;
    }
  }

  // Gamepad axes
  long amounts[] = {js.lX, js.lY, js.lZ, js.lRx, js.lRy, js.lRz};
  for (unsigned int a = 0; a < std::min(m_state.axisCount, 6U); a++)
    m_state.axes[a] = NormalizeAxis(amounts[a], MAX_AXISAMOUNT);
}
