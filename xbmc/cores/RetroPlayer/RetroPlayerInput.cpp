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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "RetroPlayerInput.h"
#include "games/libretro/libretro_wrapped.h"
#include "utils/log.h"

#include <string.h>

#define RETRO_ARRAY_LENGTH(x) (sizeof((x)) / sizeof((x)[0]))
#define RETRO_ABS(X) ((X) >= 0 ? (X) : (-(X)))

#define RETRO_ANALOG_MIN -0x8000
#define RETRO_ANALOG_MAX  0x7fff

// Add DeviceItem support to std::map
bool operator < (const CRetroPlayerInput::DeviceItem &lhs, const CRetroPlayerInput::DeviceItem &rhs)
{
  return lhs.controllerID != rhs.controllerID ? lhs.controllerID < rhs.controllerID :
         lhs.key          != rhs.key          ? lhs.key          < rhs.key :
         lhs.buttonID     != rhs.buttonID     ? lhs.buttonID     < rhs.buttonID :
         lhs.hatID        != rhs.hatID        ? lhs.hatID        < rhs.hatID :
         lhs.hatDir       != rhs.hatDir       ? lhs.hatDir       < rhs.hatDir :
         false; // Two keys are considered equivalent if < returns false reflexively
}

void CRetroPlayerInput::Reset()
{
  memset(m_joypadState, 0, sizeof(m_joypadState));
}

int16_t CRetroPlayerInput::GetInput(unsigned port, unsigned device, unsigned index, unsigned id)
{
  if (port < GAMEPAD_MAX_CONTROLLERS)
  {
    device &= RETRO_DEVICE_MASK;

    switch (device)
    {
    case RETRO_DEVICE_JOYPAD:
      if (id <= ACTION_JOYPAD_R3)
      {
        const unsigned int offset = ACTION_JOYPAD_B - ACTION_GAME_CONTROL_START;
        return m_joypadState[port][id + offset];
      }
      else
        CLog::Log(LOGERROR, "RetroPlayerInput: RETRO_DEVICE_JOYPAD id out of bounds (%u)", id);
      break;

    case RETRO_DEVICE_MOUSE:
      if (id <= RETRO_DEVICE_ID_MOUSE_RIGHT)
      {
        const unsigned int offset = ACTION_MOUSE_CONTROLLER_X - ACTION_GAME_CONTROL_START;
        return m_joypadState[port][id + offset];
      }
      else
        CLog::Log(LOGERROR, "RetroPlayerInput: RETRO_DEVICE_MOUSE id out of bounds (%u)", id);
      break;

    case RETRO_DEVICE_LIGHTGUN:
      if (id <= RETRO_DEVICE_ID_LIGHTGUN_START)
      {
        const unsigned int offset = ACTION_LIGHTGUN_X - ACTION_GAME_CONTROL_START;
        return m_joypadState[port][id + offset];
      }
      else
        CLog::Log(LOGERROR, "RetroPlayerInput: RETRO_DEVICE_LIGHTGUN id out of bounds (%u)", id);
      break;

    case RETRO_DEVICE_ANALOG:
      if (id <= RETRO_DEVICE_ID_ANALOG_Y && index <= RETRO_DEVICE_INDEX_ANALOG_RIGHT)
      {
        unsigned int offset = RETRO_DEVICE_ID_ANALOG_Y - ACTION_GAME_CONTROL_START;
        // X (id=0) and Y (id=1) are for left analog. Right analog follows in Key.h, so use id=2 and id=3
        if (index == RETRO_DEVICE_INDEX_ANALOG_RIGHT)
          offset += 2;
        return m_joypadState[port][id + offset];
      }
      else
        CLog::Log(LOGERROR, "RetroPlayerInput: RETRO_DEVICE_ANALOG id/index out of bounds (%u/%u)", id, index);
      break;

    case RETRO_DEVICE_KEYBOARD:
      CLog::Log(LOGERROR, "RetroPlayerInput: RETRO_DEVICE_KEYBOARD not supported!");
      break;

    default:
      break;
    }
  }
  CLog::Log(LOGERROR, "RetroPlayerInput: Invalid GetInput(). Controller=%u, device=%u, index=%u, id=%u",
    port, device, index, id);
  return 0;
}

void CRetroPlayerInput::ProcessKeyDown(unsigned int controllerID, uint32_t key, const CAction &action)
{
  int id = TranslateActionID(action.GetID());
  if (0 <= id && id < (int)(RETRO_ARRAY_LENGTH(m_joypadState[0])))
  {
    CLog::Log(LOGDEBUG, "-> RetroPlayerInput: Keyboard=%u, key down=%u, Action %s, id=%d",
      controllerID, key, action.GetName().c_str(), id);

    // Record the keypress to update the joypad state later
    DeviceItem item = {controllerID, key};
    m_deviceItems[item] = id;
    m_joypadState[controllerID][id] = 1;
  }
}

void CRetroPlayerInput::ProcessKeyUp(unsigned int controllerID, uint32_t key)
{
  DeviceItem item = {controllerID, key};
  std::map<DeviceItem, int>::iterator it = m_deviceItems.find(item);
  if (it != m_deviceItems.end())
  {
    CLog::Log(LOGDEBUG, "-> RetroPlayerInput: Keyboard=%u, key up=%u, id=%d", controllerID, key, it->second);

    m_joypadState[controllerID][it->second] = 0;

    m_deviceItems.erase(it);
  }
}

void CRetroPlayerInput::ProcessButtonDown(unsigned int controllerID, unsigned int buttonID, const CAction &action)
{
  int id = TranslateActionID(action.GetID());
  if (0 <= id && id < (int)(RETRO_ARRAY_LENGTH(m_joypadState[0])))
  {
    // Got to always add 1 for cosmetics (to match keymap.xml)
    CLog::Log(LOGDEBUG, "-> RetroPlayerInput: Controller=%u, button down=%u, Action %s, id=%d",
      controllerID, buttonID + 1, action.GetName().c_str(), id);

    // Record the keypress to update the joypad state later
    DeviceItem item = {controllerID, 0, buttonID};
    m_deviceItems[item] = id;
    m_joypadState[controllerID][id] = 1;
  }
}

void CRetroPlayerInput::ProcessButtonUp(unsigned int controllerID, unsigned int buttonID)
{
  DeviceItem item = {controllerID, 0, buttonID};
  std::map<DeviceItem, int>::iterator it = m_deviceItems.find(item);
  if (it != m_deviceItems.end())
  {
    CLog::Log(LOGDEBUG, "-> RetroPlayerInput: Controller=%u, button up=%u, id=%d", controllerID, buttonID + 1, it->second);
    m_joypadState[controllerID][it->second] = 0;
    m_deviceItems.erase(it);
  }
}

void CRetroPlayerInput::ProcessHatDown(unsigned int controllerID, unsigned int hatID, unsigned char hatDir, const CAction &action)
{
  int id = TranslateActionID(action.GetID());
  if (0 <= id && id < (int)(RETRO_ARRAY_LENGTH(m_joypadState[0])))
  {
    CLog::Log(LOGDEBUG, "-> RetroPlayerInput: Controller=%u, hat down=%u, direction=%u, Action %s, id=%d",
      controllerID, hatID + 1, hatDir, action.GetName().c_str(), id);

    // Record the keypress to update the joypad state later
    DeviceItem item = {controllerID, 0, 0, hatID, hatDir};
    m_deviceItems[item] = id;
    m_joypadState[controllerID][id] = 1;
  }
}

void CRetroPlayerInput::ProcessHatUp(unsigned int controllerID, unsigned int hatID, unsigned char hatDir)
{
  DeviceItem item = {controllerID, 0, 0, hatID, hatDir};
  std::map<DeviceItem, int>::iterator it = m_deviceItems.find(item);
  if (it != m_deviceItems.end())
  {
    CLog::Log(LOGDEBUG, "-> RetroPlayerInput: Controller=%u, hat up=%u, direction=%u, id=%d",
      controllerID, hatID + 1, hatDir, it->second);
    m_joypadState[controllerID][it->second] = 0;
    m_deviceItems.erase(it);
  }
}

// Untested so far. Need an emulator with analog support.
void CRetroPlayerInput::ProcessAxis(unsigned int controllerID, unsigned int axisID, const CAction &action)
{
  // value is in the range [-0x8000, 0x7fff]
  int16_t value;
  if (RETRO_ABS(action.GetAmount(1)) <= 0.01f)
    value = 0;
  else if (action.GetAmount(1) > 0)
  {
    int32_t value2 = (int)(RETRO_ANALOG_MAX * action.GetAmount(1));
    value = (int16_t)(value2 > RETRO_ANALOG_MAX ? RETRO_ANALOG_MAX : value2);
  }
  else
  {
    int32_t value2 = (int)(RETRO_ANALOG_MIN * -action.GetAmount(1));
    value = (int16_t)(value2 < RETRO_ANALOG_MIN ? RETRO_ANALOG_MIN : value2);
  }

  if (value != 0) // Is axis non-centered?
  {
    // TODO: modify TranslateActionID() to accept a device ID, then check that
    // we're in the valid range of that device
    int id = TranslateActionID(action.GetID());
    if (0 <= id && id < (int)(RETRO_ARRAY_LENGTH(m_joypadState[0])))
    {
      // Record the axis ID to update the joypad state later
      DeviceItem item = {controllerID, 0, 0, 0, 0, (unsigned char)axisID};
      // Axis events are fired rapidly, but insertion here should be quick
      // enough as no elements are usually added or removed, and no rebalancing
      // of the map is needed.
      m_deviceItems[item] = id;
      m_joypadState[controllerID][id] = value;
    }
  }
  else
  {
    DeviceItem item = {controllerID, 0, 0, 0, 0, (unsigned char)axisID};
    // Axis cented events are usually only fired once, so the deletion here
    // won't be performed rapidly
    std::map<DeviceItem, int>::iterator it = m_deviceItems.find(item);
    if (it != m_deviceItems.end())
    {
      m_joypadState[controllerID][it->second] = 0;
      m_deviceItems.erase(it);
    }
  }
}

/*
TODO:
// Return the offset of x in the interval [low, high] from ACTION_GAME_CONTROL_START
// or -1 if x lies outside the interval
*/

// Return the nth position of x in the interval [low, high], or -1 if x lies outside
#define TRANSLATE_INTERVAL(x, low, high) (((low) <= (x) && (x) <= (high)) ? (x) - (low) : -1)

int CRetroPlayerInput::TranslateActionID(int id /* , int device */) const
{
  int m_device = RETRO_DEVICE_JOYPAD; // Until we keep track of multiple devices

  switch (m_device)
  {
  case RETRO_DEVICE_JOYPAD:
  case RETRO_DEVICE_MOUSE:
  case RETRO_DEVICE_LIGHTGUN:
  case RETRO_DEVICE_ANALOG:
    return TRANSLATE_INTERVAL(id, ACTION_GAME_CONTROL_START, ACTION_GAME_CONTROL_END);
  case RETRO_DEVICE_KEYBOARD:
    // Keyboard is poll-based, need to poll requested key
  default:
    break;
  }
  return -1; // Invalid device ID
}
