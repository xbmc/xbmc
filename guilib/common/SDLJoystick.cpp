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
  m_JoyId = 0;
  m_ButtonId = 0;
  m_ActiveFlags = JACTIVE_NONE;
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
    SDL_JoystickEventState(SDL_DISABLE);    
  }
  else
  {
    // no joysticks, disable joystick event capture
    SDL_JoystickEventState(SDL_DISABLE);
  }
}

void CJoystick::Reset(bool axis)
{
  SetButtonActive(false);
  if (axis)
  {
    SetAxisActive(false);
    for (int i = 0 ; i<MAX_AXES ; i++)
    {
      ResetAxis(i);
    }
  }
}

void CJoystick::Update(SDL_Event& joyEvent)
{  
  int buttonId = -1;
  int axisId = -1;
  int joyId = -1;
  const char* joyName = NULL;
  bool ignore = false; // not used for now
  bool axis = false;

  switch(joyEvent.type)
  {
  case SDL_JOYBUTTONDOWN:  
    m_JoyId = joyId = joyEvent.jbutton.which;
    m_ButtonId = buttonId = joyEvent.jbutton.button + 1;
    m_pressTicks = SDL_GetTicks();
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
      m_Amount[axisId] = 0.0f;
    }
    else
    {
      m_Amount[axisId] = ((float)joyEvent.jaxis.value / 32768.0f); //[-32768 to 32767]
    }
    m_AxisId = GetAxisWithMaxAmount();
    CLog::Log(LOGDEBUG, "Joystick %d Axis %d Amount %f", joyId, axisId, m_Amount[axisId]);
    break;

  case SDL_JOYBALLMOTION:
  case SDL_JOYHATMOTION:
    ignore = true;
    break;

  case SDL_JOYBUTTONUP:
     m_pressTicks = 0;
    SetButtonActive(false);
    CLog::Log(LOGDEBUG, "Joystick %d button %d Up", joyEvent.jbutton.which, m_ButtonId);

  default:
    ignore = true;
    break;
  }
}

bool CJoystick::GetButton(int &id, bool consider_repeat)
{ 
  if (!IsButtonActive())
    return false;
  if (!consider_repeat)
  {
    id = m_ButtonId;
    return true;
  }

  static Uint32 lastPressTicks = 0;
  static Uint32 lastTicks = 0;
  static Uint32 nowTicks = 0;

  if ((m_ButtonId>=0) && m_pressTicks)
  {
    // return the id if it's the first press
    if (lastPressTicks!=m_pressTicks)
    {
      lastPressTicks = m_pressTicks;
      id = m_ButtonId;
      return true;
    }
    nowTicks = SDL_GetTicks();
    if ((nowTicks-m_pressTicks)<500) // 500ms delay before we repeat
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

int CJoystick::GetAxisWithMaxAmount()
{
  static float maxAmount;
  static int axis;
  axis = 0;
  maxAmount = 0;
  for (int i = 1 ; i<=m_NumAxes ; i++)
  {
    if ((float)fabs(m_Amount[i])>maxAmount)
    {
      maxAmount = (float)fabs(m_Amount[i]);
      axis = i;
    }
  }
  if (maxAmount==0)
    SetAxisActive(false);
  else
    SetAxisActive();
  return axis;
}

#endif
