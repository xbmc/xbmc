/*
*      Copyright (C) 2007-2012 Team XBMC
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

#include "system.h"
#include "SDLJoystick.h"
#include "ButtonTranslator.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#include <math.h>

#ifdef HAS_SDL_JOYSTICK
#include <SDL/SDL.h>

using namespace std;

CJoystick::CJoystick()
{
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

  // clear old joystick names
  m_JoystickNames.clear();

  // any open ones? if so, close them.
  if (m_Joysticks.size()>0)
  {
    for(size_t idJoy = 0; idJoy < m_Joysticks.size(); idJoy++)
    {
      // any joysticks unplugged?
      if(SDL_JoystickOpened(idJoy))
        SDL_JoystickClose(m_Joysticks[idJoy]);
    }
    m_Joysticks.clear();
    m_JoyId = -1;
  }

  // Set deadzone range
  SetDeadzone(g_advancedSettings.m_controllerDeadzone);

  // any joysticks connected?
  if (SDL_NumJoysticks()>0)
  {
    // load joystick names and open all connected joysticks
    for (int i = 0 ; i<SDL_NumJoysticks() ; i++)
    {
      SDL_Joystick *joy = SDL_JoystickOpen(i);

#if defined(TARGET_DARWIN)
      // On OS X, the 360 controllers are handled externally, since the SDL code is
      // really buggy and doesn't handle disconnects.
      //
      if (std::string(SDL_JoystickName(i)).find("360") != std::string::npos)
      {
        CLog::Log(LOGNOTICE, "Ignoring joystick: %s", SDL_JoystickName(i));
        continue;
      }
#endif

      m_Joysticks.push_back(joy);
      if (joy)
      {
        m_JoystickNames.push_back(string(SDL_JoystickName(i)));
        CLog::Log(LOGNOTICE, "Enabled Joystick: %s", SDL_JoystickName(i));
        CLog::Log(LOGNOTICE, "Details: Total Axis: %d Total Hats: %d Total Buttons: %d",
            SDL_JoystickNumAxes(joy), SDL_JoystickNumHats(joy), SDL_JoystickNumButtons(joy));        
      }
      else
      {
        m_JoystickNames.push_back(string(""));
      }
    }
  }
  
  // disable joystick events, since we'll be polling them
  SDL_JoystickEventState(SDL_DISABLE);
}

void CJoystick::Reset(bool axis /*=false*/)
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
  int numj        = m_Joysticks.size();
  if (numj <= 0)
    return;

  // update the state of all opened joysticks
  SDL_JoystickUpdate();

  // go through all joysticks
  for (int j = 0; j<numj; j++)
  {
    SDL_Joystick *joy = m_Joysticks[j];
    int numb = SDL_JoystickNumButtons(joy);
    int numhat = SDL_JoystickNumHats(joy);
    int numax = SDL_JoystickNumAxes(joy);
    numax = (numax>MAX_AXES)?MAX_AXES:numax;
    int axisval;
    uint8_t hatval;

    // get button states first, they take priority over axis
    for (int b = 0 ; b<numb ; b++)
    {
      if (SDL_JoystickGetButton(joy, b))
      {
        m_JoyId = SDL_JoystickIndex(joy);
        buttonId = b+1;
        j = numj-1;
        break;
      }
    }
    
    for (int h = 0; h < numhat; h++)
    {
      hatval = SDL_JoystickGetHat(joy, h);
      if (hatval != SDL_HAT_CENTERED)
      {
        m_JoyId = SDL_JoystickIndex(joy);
        hatId = h + 1;
        m_HatState = hatval;
        j = numj-1;
        break; 
      }
    }

    // get axis states
    m_NumAxes = numax;
    for (int a = 0 ; a<numax ; a++)
    {
      axisval = SDL_JoystickGetAxis(joy, a);
      axisId = a+1;
      if (axisId<=0 || axisId>=MAX_AXES)
      {
        CLog::Log(LOGERROR, "Axis Id out of range. Maximum supported axis: %d", MAX_AXES);
      }
      else
      {
        m_Amount[axisId] = axisval;  //[-32768 to 32767]
      }
    }
    m_AxisId = GetAxisWithMaxAmount();
    if (m_AxisId)
    {
      m_JoyId = SDL_JoystickIndex(joy);
      j = numj-1;
      break;
    }
  }

  if(hatId==-1)
  {
    if(m_HatId!=0)
      CLog::Log(LOGDEBUG, "Joystick %d hat %u Centered", m_JoyId, hatId);
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
      m_pressTicksHat = SDL_GetTicks();
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
      m_pressTicksButton = SDL_GetTicks();
    }
    SetButtonActive();
  }

}

void CJoystick::Update(SDL_Event& joyEvent)
{
  if (!IsEnabled())
    return;

  int buttonId = -1;
  int axisId = -1;
  int joyId = -1;
  DECLARE_UNUSED(bool,ignore = false)
  DECLARE_UNUSED(bool,axis = false);

  switch(joyEvent.type)
  {
  case SDL_JOYBUTTONDOWN:
    m_JoyId = joyId = joyEvent.jbutton.which;
    m_ButtonId = buttonId = joyEvent.jbutton.button + 1;
    m_pressTicksButton = SDL_GetTicks();
    SetButtonActive();
    CLog::Log(LOGDEBUG, "Joystick %d button %d Down", joyId, buttonId);
    break;

  case SDL_JOYAXISMOTION:
    joyId = joyEvent.jaxis.which;
    axisId = joyEvent.jaxis.axis + 1;
    m_NumAxes = SDL_JoystickNumAxes(m_Joysticks[joyId]);
    if (axisId<=0 || axisId>=MAX_AXES)
    {
      CLog::Log(LOGERROR, "Axis Id out of range. Maximum supported axis: %d", MAX_AXES);
      ignore = true;
      break;
    }
    axis = true;
    m_JoyId = joyId;
    if (joyEvent.jaxis.value==0)
    {
      ignore = true;
      m_Amount[axisId] = 0;
    }
    else
    {
      m_Amount[axisId] = joyEvent.jaxis.value; //[-32768 to 32767]
    }
    m_AxisId = GetAxisWithMaxAmount();
    CLog::Log(LOGDEBUG, "Joystick %d Axis %d Amount %d", joyId, axisId, m_Amount[axisId]);
    break;

  case SDL_JOYHATMOTION:
    m_JoyId = joyId = joyEvent.jbutton.which;
    m_HatId = joyEvent.jhat.hat + 1;
    m_pressTicksHat = SDL_GetTicks();
    m_HatState = joyEvent.jhat.value;
    SetHatActive(m_HatState != SDL_HAT_CENTERED);
    CLog::Log(LOGDEBUG, "Joystick %d Hat %d Down with position %d", joyId, buttonId, m_HatState);
    break;

  case SDL_JOYBALLMOTION:
    ignore = true;
    break;
    
  case SDL_JOYBUTTONUP:
    m_pressTicksButton = 0;
    SetButtonActive(false);
    CLog::Log(LOGDEBUG, "Joystick %d button %d Up", joyEvent.jbutton.which, m_ButtonId);

  default:
    ignore = true;
    break;
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

  static uint32_t lastPressTicks = 0;
  static uint32_t lastTicks = 0;
  static uint32_t nowTicks = 0;

  if ((m_HatId>=0) && m_pressTicksHat)
  {
    // return the id if it's the first press
    if (lastPressTicks!=m_pressTicksHat)
    {
      lastPressTicks = m_pressTicksHat;
      return true;
    }
    nowTicks = SDL_GetTicks();
    if ((nowTicks-m_pressTicksHat)<500) // 500ms delay before we repeat
      return false;
    if ((nowTicks-lastTicks)<100) // 100ms delay before successive repeats
      return false;

    lastTicks = nowTicks;
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

  static uint32_t lastPressTicks = 0;
  static uint32_t lastTicks = 0;
  static uint32_t nowTicks = 0;

  if ((m_ButtonId>=0) && m_pressTicksButton)
  {
    // return the id if it's the first press
    if (lastPressTicks!=m_pressTicksButton)
    {
      lastPressTicks = m_pressTicksButton;
      id = m_ButtonId;
      return true;
    }
    nowTicks = SDL_GetTicks();
    if ((nowTicks-m_pressTicksButton)<500) // 500ms delay before we repeat
    {
      return false;
    }
    if ((nowTicks-lastTicks)<100) // 100ms delay before successive repeats
    {
      return false;
    }
    lastTicks = nowTicks;
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
  static int maxAmount;
  static int axis;
  axis = 0;
  maxAmount = 0;
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

bool CJoystick::ReleaseJoysticks()
{
  m_Joysticks.clear();
  m_JoystickNames.clear();
  m_HatId = 0;
  m_ButtonId = 0;
  m_HatState = SDL_HAT_CENTERED;
  m_ActiveFlags = JACTIVE_NONE;
  Reset(true);

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

#endif
