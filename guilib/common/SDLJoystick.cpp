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
    m_Amount[i] = 0;
  }
  SetSafeRange(5000);
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
        CalibrateAxis(joy);
        m_JoystickNames.push_back(string(SDL_JoystickName(i)));
        CLog::Log(LOGNOTICE, "Enabled Joystick: %s", SDL_JoystickName(i));
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

void CJoystick::CalibrateAxis(SDL_Joystick* joy)
{
  SDL_JoystickUpdate();

  int numax = SDL_JoystickNumAxes(joy);
  numax = (numax>MAX_AXES)?MAX_AXES:numax;

  // get default axis states
  for (int a = 0 ; a<numax ; a++)
  {
    m_DefaultAmount[a+1] = SDL_JoystickGetAxis(joy, a);
    CLog::Log(LOGDEBUG, "Calibrated Axis: %d , default amount %d\n", a, m_DefaultAmount[a+1]);
  }
}

void CJoystick::Reset(bool axis)
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
  int buttonId = -1;
  int axisId = -1;
  int numj = m_Joysticks.size();
  if (numj<=0)
    return;

  // update the state of all opened joysticks
  SDL_JoystickUpdate();

  // go through all joysticks
  for (int j = 0 ; j<numj ; j++)
  {
    SDL_Joystick *joy = m_Joysticks[j];
    int numb = SDL_JoystickNumButtons(joy);
    int numax = SDL_JoystickNumAxes(joy);
    numax = (numax>MAX_AXES)?MAX_AXES:numax;
    int axisval;

    // get button states first, they take priority over axis
    for (int b = 0 ; b<numb ; b++)
    {
      if (SDL_JoystickGetButton(joy, b))
      {
        m_JoyId = j;
        buttonId = b+1;
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
        if (axisval!=m_DefaultAmount[axisId])
        {
          m_JoyId = j;
        }
      }
    }
    m_AxisId = GetAxisWithMaxAmount();
  }

  if (buttonId==-1)
  {
    if (m_ButtonId!=0)
    {
      CLog::Log(LOGDEBUG, "Joystick %d button %d Up", m_JoyId, m_ButtonId);
    }
    m_pressTicks = 0;
    SetButtonActive(false);
    m_ButtonId = 0;
  }
  else
  {
    if (buttonId!=m_ButtonId)
    {
      CLog::Log(LOGDEBUG, "Joystick %d button %d Down", m_JoyId, buttonId);
      m_ButtonId = buttonId;
      m_pressTicks = SDL_GetTicks();
    }
    SetButtonActive();
  }

}

void CJoystick::Update(SDL_Event& joyEvent)
{
  int buttonId = -1;
  int axisId = -1;
  int joyId = -1;
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
      m_Amount[axisId] = 0;
    }
    else
    {
      m_Amount[axisId] = joyEvent.jaxis.value; //[-32768 to 32767]
    }
    m_AxisId = GetAxisWithMaxAmount();
    CLog::Log(LOGDEBUG, "Joystick %d Axis %d Amount %d", joyId, axisId, m_Amount[axisId]);
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
  static int maxAmount;
  static int axis;
  axis = 0;
  maxAmount = m_SafeRange;
  int tempf;
  for (int i = 1 ; i<=m_NumAxes ; i++)
  {
    tempf = abs(m_DefaultAmount[i] - m_Amount[i]);
    if (tempf>maxAmount)
    {
      maxAmount = tempf;
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
