/*
 *      Copyright (C) 2007-2013 Team XBMC
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

#include "system.h"
#include "SDLJoystick.h"
#include "peripherals/devices/PeripheralImon.h"
#include "settings/AdvancedSettings.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/RegExp.h"

#include <math.h>

#ifdef HAS_SDL_JOYSTICK
#include <SDL2/SDL.h>

using namespace std;

CJoystick::CJoystick()
{
  m_joystickEnabled = false;
  SetDeadzone(0);
  Reset();
}

void CJoystick::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == "input.enablejoystick")
    SetEnabled(((CSettingBool*)setting)->GetValue() && PERIPHERALS::CPeripheralImon::GetCountOfImonsConflictWithDInput() == 0);
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

void CJoystick::Initialize()
{
  if (!IsEnabled())
    return;

  if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0)
  {
    CLog::Log(LOGERROR, "(Re)start joystick subsystem failed : %s",SDL_GetError());
    return;
  }

  Reset();

  // Set deadzone range
  SetDeadzone(g_advancedSettings.m_controllerDeadzone);

  // joysticks will be loaded once SDL sees them
}

void CJoystick::AddJoystick(int joyIndex)
{
  SDL_Joystick *joy = SDL_JoystickOpen(joyIndex);
  if (!joy)
    return;

  std::string joyName = std::string(SDL_JoystickName(joy));


  // Some (Microsoft) Keyboards are recognized as Joysticks by modern kernels
  // Don't enumerate them
  // https://bugs.launchpad.net/ubuntu/+source/linux/+bug/390959
  // NOTICE: Enabled Joystick: Microsoft Wired Keyboard 600
  // Details: Total Axis: 37 Total Hats: 0 Total Buttons: 57
  // NOTICE: Enabled Joystick: Microsoft Microsoft® 2.4GHz Transceiver v6.0
  // Details: Total Axis: 37 Total Hats: 0 Total Buttons: 57
  // also checks if we have at least 1 button, fixes
  // NOTICE: Enabled Joystick: ST LIS3LV02DL Accelerometer
  // Details: Total Axis: 3 Total Hats: 0 Total Buttons: 0
  int num_axis = SDL_JoystickNumAxes(joy);
  int num_buttons = SDL_JoystickNumButtons(joy);
  int num_hats = SDL_JoystickNumHats(joy);
  if ((num_axis > 20 && num_buttons > 50) || num_buttons == 0)
  {
    CLog::Log(LOGNOTICE, "Ignoring Joystick %s Axis: %d Buttons: %d: invalid device properties",
      joyName.c_str(), num_axis, num_buttons);
    return;
  }

  // get details
  CLog::Log(LOGNOTICE, "Enabled Joystick: %s", joyName.c_str());
  CLog::Log(LOGNOTICE, "Details: Total Axis: %d Total Hats: %d Total Buttons: %d",
    num_axis, num_hats, num_buttons);

  // store joystick and its instance id for lookups
  int id = SDL_JoystickInstanceID(joy);
  m_Joysticks[id] = joy;
  m_Axes.resize(m_Axes.size() + num_axis);

  // all indices may have changed in m_Joysticks, so we have to invalidate m_Axes
  ApplyAxesConfigs();
  Reset();
}

void CJoystick::RemoveJoystick(int id)
{
  CLog::Log(LOGDEBUG, "Joystick with id %d removed", id);

  // invalidate our current values
  SDL_Joystick *joy = m_Joysticks[id];
  m_Axes.resize(m_Axes.size() - SDL_JoystickNumAxes(joy));
  SDL_JoystickClose(joy);
  m_Joysticks.erase(id);
  // all indices may have changed in m_Joysticks, so we have to invalidate m_Axes
  ApplyAxesConfigs();
  Reset();
}

void CJoystick::Update()
{
  if (!IsEnabled())
    return;

  // update the state of all opened joysticks
  SDL_JoystickUpdate();
  int axisNum;
  int buttonIdx = -1, buttonNum;
  int hatIdx = -1, hatNum;

  // go through all joysticks
  std::map<int, SDL_Joystick*>::const_iterator it;
  for (it = m_Joysticks.begin(); it != m_Joysticks.end(); ++it)
  {
    SDL_Joystick *joy = it->second;
    int numb = SDL_JoystickNumButtons(joy);
    int numhat = SDL_JoystickNumHats(joy);
    int numax = SDL_JoystickNumAxes(joy);
    uint8_t hatval;

    for (int b = 0 ; b<numb ; b++)
    {
      if (SDL_JoystickGetButton(joy, b))
      {
        buttonIdx = MapButton(joy, b);
        break;
      }
    }

    for (int h = 0; h < numhat; h++)
    {
      hatval = SDL_JoystickGetHat(joy, h);
      if (hatval != SDL_HAT_CENTERED)
      {
        hatIdx = MapHat(joy, h);
        m_HatState = hatval;
        break; 
      }
    }

    // get axis states
    for (int a = 0 ; a<numax ; a++)
    {
      int axisIdx = MapAxis(joy, a);
      m_Axes[axisIdx].val = SDL_JoystickGetAxis(joy, a);
    }
  }

  SDL_Joystick *joy;
  m_AxisIdx = GetAxisWithMaxAmount();
  if (m_AxisIdx >= 0)
  {
    MapAxis(m_AxisIdx, joy, axisNum);
    // CLog::Log(LOGDEBUG, "Joystick %d Axis %d Amount %d", JoystickIndex(joy), axisNum + 1, m_Axes[m_AxisIdx].val);
  }

  if (hatIdx == -1 && m_HatIdx != -1)
  {
    // now centered
    MapHat(m_HatIdx, joy, hatNum);
    CLog::Log(LOGDEBUG, "Joystick %d hat %u hat centered", JoystickIndex(joy), hatNum + 1);
    m_pressTicksHat = 0;
  }
  else if (hatIdx != m_HatIdx)
  {
    MapHat(hatIdx, joy, hatNum);
    CLog::Log(LOGDEBUG, "Joystick %d hat %u value %d", JoystickIndex(joy), hatNum + 1, m_HatState);
    m_pressTicksHat = SDL_GetTicks();
  }
  m_HatIdx = hatIdx;

  if (buttonIdx == -1 && m_ButtonIdx != -1)
  {
    MapButton(m_ButtonIdx, joy, buttonNum);
    CLog::Log(LOGDEBUG, "Joystick %d button %d Up", JoystickIndex(joy), buttonNum + 1);
    m_pressTicksButton = 0;
    m_ButtonIdx = -1;
  }
  else if (buttonIdx != m_ButtonIdx)
  {
    MapButton(buttonIdx, joy, buttonNum);
    CLog::Log(LOGDEBUG, "Joystick %d button %d Down", JoystickIndex(joy), buttonNum + 1);
    m_pressTicksButton = SDL_GetTicks();
  }
  m_ButtonIdx = buttonIdx;
}

void CJoystick::Update(SDL_Event& joyEvent)
{
  if (!IsEnabled())
    return;

  switch(joyEvent.type)
  {
  // we do not handle the button/axis/hat events here because
  // only Update() is equipped to handle keyrepeats
  case SDL_JOYDEVICEADDED:
    AddJoystick(joyEvent.jdevice.which);
    break;

  case SDL_JOYDEVICEREMOVED:
    RemoveJoystick(joyEvent.jdevice.which);
    break;
  }
}

bool CJoystick::GetHat(std::string &joyName, int &id, int &position, bool consider_repeat)
{
  if (!IsEnabled() || !IsHatActive())
    return false;

  SDL_Joystick *joy;
  MapHat(m_HatIdx, joy, id);
  joyName = std::string(SDL_JoystickName(joy));
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
  uint32_t nowTicks = SDL_GetTicks();
  if (nowTicks - m_pressTicksHat < 500) // 500ms delay before we repeat
    return false;
  if (nowTicks - lastPressTicksHat < 100) // 100ms delay before successive repeats
    return false;
  lastPressTicksHat = nowTicks;

  return true;
}

bool CJoystick::GetButton(std::string &joyName, int& id, bool consider_repeat)
{
  if (!IsEnabled() || !IsButtonActive())
    return false;

  SDL_Joystick *joy;
  MapButton(m_ButtonIdx, joy, id);
  joyName = SDL_JoystickName(joy);

  if (!consider_repeat)
    return true;

  static uint32_t lastPressTicksButton = 0;
  // allow it if it is the first press
  if (lastPressTicksButton < m_pressTicksButton)
  {
    lastPressTicksButton = m_pressTicksButton;
    return true;
  }
  uint32_t nowTicks = SDL_GetTicks();
  if (nowTicks - m_pressTicksButton < 500) // 500ms delay before we repeat
    return false;
  if (nowTicks - lastPressTicksButton < 100) // 100ms delay before successive repeats
    return false;

  lastPressTicksButton = nowTicks;
  return true;
}

bool CJoystick::GetAxis(std::string &joyName, int& id) const
{ 
  if (!IsEnabled() || !IsAxisActive()) 
    return false;

  SDL_Joystick *joy;
  MapAxis(m_AxisIdx, joy, id);
  joyName = std::string(SDL_JoystickName(joy));

  return true;
}

int CJoystick::GetAxisWithMaxAmount() const
{
  int maxAmount= 0, axis = -1;
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

float CJoystick::GetAmount(std::string &joyName, int axisNum) const
{
  // find matching SDL_Joystick*
  std::map<int, SDL_Joystick*>::const_iterator it;
  for (it = m_Joysticks.begin(); it != m_Joysticks.end(); ++it)
    if (std::string(SDL_JoystickName(it->second)) == joyName)
      break;

  if (it == m_Joysticks.end())
    return 0; // invalid joy name

  int axisIdx = MapAxis(it->second, axisNum);
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

bool CJoystick::ReleaseJoysticks()
{
  // close open joysticks
  for (std::map<int, SDL_Joystick*>::const_iterator it = m_Joysticks.begin(); it != m_Joysticks.end(); ++it)
    SDL_JoystickClose(it->second);
  m_Joysticks.clear();
  m_Axes.clear();
  Reset();

  // Restart SDL joystick subsystem
  SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
  if (SDL_WasInit(SDL_INIT_JOYSTICK) !=  0)
  {
    CLog::Log(LOGERROR, "Stop joystick subsystem failed");
    return false;
  }
  return true;
}

bool CJoystick::Reinitialize()
{
  if( !ReleaseJoysticks() ) return false;
  Initialize();

  return true;
}

void CJoystick::LoadAxesConfigs(const std::map<std::shared_ptr<CRegExp>, AxesConfig> &axesConfigs)
{
  m_AxesConfigs.clear();
  m_AxesConfigs.insert(axesConfigs.begin(), axesConfigs.end());
}

void CJoystick::ApplyAxesConfigs()
{
  // load axes configuration from keymap
  int axesCount = 0;
  for (std::map<int, SDL_Joystick*>::const_iterator it = m_Joysticks.begin(); it != m_Joysticks.end(); it++)
  {
    std::string joyName(SDL_JoystickName(it->second));
    std::map<std::shared_ptr<CRegExp>, AxesConfig>::const_iterator axesCfg;
    for (axesCfg = m_AxesConfigs.begin(); axesCfg != m_AxesConfigs.end(); axesCfg++)
    {
      if (axesCfg->first->RegFind(joyName) >= 0)
        break;
    }
    if (axesCfg != m_AxesConfigs.end())
    {
      for (AxesConfig::const_iterator it = axesCfg->second.begin(); it != axesCfg->second.end(); ++it)
      {
        int axis = axesCount + it->axis - 1;
        m_Axes[axis].trigger = it->isTrigger;
        m_Axes[axis].rest = it->rest;
      }
    }
    axesCount += SDL_JoystickNumAxes(it->second);
  }
}
  
int CJoystick::JoystickIndex(const std::string &joyName) const
{
  int i = 0;
  for (std::map<int, SDL_Joystick*>::const_iterator it = m_Joysticks.begin(); it != m_Joysticks.end(); ++it)
  {
    if (joyName == std::string(SDL_JoystickName(it->second)))
      return i;
    i++;
  }
  return -1;
}

int CJoystick::JoystickIndex(SDL_Joystick *joy) const
{
  int i = 0;
  for (std::map<int, SDL_Joystick*>::const_iterator it = m_Joysticks.begin(); it != m_Joysticks.end(); ++it)
  {
    if (it->second == joy)
      return i;
    i++;
  }
  return -1;
}

int CJoystick::MapAxis(SDL_Joystick *joy, int axisNum) const
{
  int idx = 0;
  std::map<int, SDL_Joystick*>::const_iterator it;
  for (it = m_Joysticks.begin(); it != m_Joysticks.end() && it->second != joy; ++it)
    idx += SDL_JoystickNumAxes(it->second);
  if (it == m_Joysticks.end())
    return -1;
  else
    return idx + axisNum;
}

void CJoystick::MapAxis(int axisIdx, SDL_Joystick *&joy, int &axisNum) const
{
  int axisCount = 0;
  std::map<int, SDL_Joystick*>::const_iterator it;
  for (it = m_Joysticks.begin(); it != m_Joysticks.end() && axisCount + SDL_JoystickNumAxes(it->second) <= axisIdx; ++it)
    axisCount += SDL_JoystickNumAxes(it->second);
  if (it != m_Joysticks.end())
  {
    joy = it->second;
    axisNum = axisIdx - axisCount;
  }
  else
  {
    joy = NULL;
    axisNum = -1;
  }
}

int CJoystick::MapButton(SDL_Joystick *joy, int buttonNum) const
{
  int idx = 0;
  std::map<int, SDL_Joystick*>::const_iterator it;
  for (it = m_Joysticks.begin(); it != m_Joysticks.end() && it->second != joy; ++it)
    idx += SDL_JoystickNumButtons(it->second);
  if (it == m_Joysticks.end())
    return -1;
  else
    return idx + buttonNum;
}

void CJoystick::MapButton(int buttonIdx, SDL_Joystick *&joy, int &buttonNum) const
{
  int buttonCount = 0;
  std::map<int, SDL_Joystick*>::const_iterator it;
  for (it = m_Joysticks.begin(); it != m_Joysticks.end() && buttonCount + SDL_JoystickNumButtons(it->second) <= buttonIdx; ++it)
    buttonCount += SDL_JoystickNumButtons(it->second);
  if (it != m_Joysticks.end())
  {
    joy = it->second;
    buttonNum = buttonIdx - buttonCount;
  }
  else
  {
    joy = NULL;
    buttonNum = -1;
  }
}

int CJoystick::MapHat(SDL_Joystick *joy, int hatNum) const
{
  int idx = 0;
  std::map<int, SDL_Joystick*>::const_iterator it;
  for (it = m_Joysticks.begin(); it != m_Joysticks.end() && it->second != joy; ++it)
    idx += SDL_JoystickNumHats(it->second);
  if (it == m_Joysticks.end())
    return -1;
  else
    return idx + hatNum;
}

void CJoystick::MapHat(int hatIdx, SDL_Joystick *&joy, int& hatNum) const
{
  int hatCount = 0;
  std::map<int, SDL_Joystick*>::const_iterator it;
  for (it = m_Joysticks.begin(); it != m_Joysticks.end() && hatCount + SDL_JoystickNumHats(it->second) <= hatIdx; ++it)
    hatCount += SDL_JoystickNumHats(it->second);
  if (it != m_Joysticks.end())
  {
    joy = it->second;
    hatNum = hatIdx - hatCount;
  }
  else
  {
    joy = NULL;
    hatNum = -1;
  }
}

#endif
