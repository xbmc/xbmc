#include "../include.h"
#include "../Key.h"
#include "SDLJoystick.h"
#include "ButtonTranslator.h"
#include <math.h>

#ifdef HAS_SDL_JOYSTICK

CJoystick g_Joystick; // global

CJoystick::CJoystick()
{
  Reset();
  m_JoyId = -1;
  for (int i = 0 ; i<MAX_AXES ; i++)
  {
    m_Amount[i] = 0.0f;
  }
}

void CJoystick::Initialize(HWND hWnd)
{
  // clear old joystick names
  m_JoystickNames.clear();

  // any open ones? if so, close them.
  if (m_Joysticks.size()>0)
  {
    vector<SDL_Joystick*>::iterator iter = m_Joysticks.begin();
    while (iter != m_Joysticks.end())
    {
      SDL_JoystickClose(*iter);
      iter++;
    }
    m_Joysticks.clear();
  }

  // any joysticks connected?
  if (SDL_NumJoysticks()>0)
  {
    // load joystick names and open all connected joysticks
    for (int i = 0 ; i<SDL_NumJoysticks() ; i++)
    {
      SDL_Joystick *joy = SDL_JoystickOpen(i);
      m_Joysticks.push_back(joy);
      if (joy)
      {
        m_JoystickNames.push_back(string(SDL_JoystickName(i)));
        CLog::Log(LOGNOTICE, "Enabled Joystick: %s", SDL_JoystickName(i));
      }
      else
      {
        m_JoystickNames.push_back(string(""));
      }
    }
    // enable joystick event capture
    SDL_JoystickEventState(SDL_ENABLE);    
  }
  else
  {
    // no joysticks, disable joystick event capture
    SDL_JoystickEventState(SDL_DISABLE);
  }
}

void CJoystick::Reset(bool axis)
{
  m_ButtonId = 0;
  if (axis)
  {
    m_AxisId = 0;
    for (int i = 0 ; i<MAX_AXES ; i++)
    {
      m_Amount[i] = 0.0f;
    }
  }
}

void CJoystick::Update(SDL_Event& joyEvent)
{  
  int buttonId = -1;
  int axisId = -1;
  int joyId = -1;
  const char* joyName = NULL;
  bool ignore = false;
  bool axis = false;

  switch(joyEvent.type)
  {
  case SDL_JOYBUTTONDOWN:  
    m_JoyId = joyId = joyEvent.jbutton.which;
    m_ButtonId = buttonId = joyEvent.jbutton.button;
    CLog::Log(LOGDEBUG, "Joystick %d button %d", joyId, buttonId);
    break;

  case SDL_JOYAXISMOTION:
    joyId = joyEvent.jaxis.which;
    axisId = joyEvent.jaxis.axis;
    m_NumAxes = SDL_JoystickNumAxes(m_Joysticks[joyId]);
    if (axisId<0 || axis>=MAX_AXES)
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
      m_Amount[axisId] = 0.0f;
    }
    else
    {
      m_Amount[axisId] = ((float)joyEvent.jaxis.value / 32768.0f); //[-32768 to 32767]
    }
    m_AxisId = GetAxisWithMaxAmount();
    CLog::Log(LOGDEBUG, "Joystick %d axis %d amount %f", joyId, axisId, m_Amount[axisId]);
    break;

  case SDL_JOYBALLMOTION:
  case SDL_JOYHATMOTION:
    ignore = true;
    break;

  case SDL_JOYBUTTONUP:
    m_ButtonId = 0;
  default:
    ignore = true;
    break;
  }

  if (ignore)
    return;

  if (axis)
  {
    m_ButtonId = 0;
  }
  else
  {
    m_AxisId = 0;
  }
}

int CJoystick::GetAxisWithMaxAmount()
{
  static float maxAmount;
  static int axis;
  axis = 0;
  maxAmount = 0;
  for (int i = 0 ; i<m_NumAxes ; i++)
  {
    if ((float)fabs(m_Amount[i])>maxAmount)
    {
      maxAmount = (float)fabs(m_Amount[i]);
      axis = i;
    }
  }
  return axis;
}

#endif
