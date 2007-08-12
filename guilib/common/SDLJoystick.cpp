#include "../include.h"
#include "../Key.h"
#include "SDLJoystick.h"
#include "ButtonTranslator.h"

#ifdef HAS_SDL_JOYSTICK

CJoystick g_Joystick; // global

CJoystick::CJoystick()
{
  Reset();
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

    // enable joystick event capture
    SDL_JoystickEventState(SDL_ENABLE);

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
  }
  else
  {
    // no joysticks, disable joystick event capture
    SDL_JoystickEventState(SDL_DISABLE);
  }
}

void CJoystick::Reset()
{
  m_Amount = 0.0f;
  m_Button = 0;
  m_Axis = 0;
}

void CJoystick::Update(SDL_Event& joyEvent)
{  
  int buttonId = -1;
  int axisId = -1;
  int joyId = -1;
  const char* joyName = NULL;
  bool ignore = false;
  bool axis = false;

  m_Amount = 0.0f;
  
  switch(joyEvent.type)
  {
  case SDL_JOYBUTTONDOWN:  
    joyId = joyEvent.jbutton.which;
    buttonId = joyEvent.jbutton.button;
    CLog::Log(LOGNOTICE, "Joystick %d button %d", joyId, buttonId);
    break;

  case SDL_JOYAXISMOTION:
    joyId = joyEvent.jaxis.which;
    axisId = joyEvent.jaxis.axis;
    axis = true;
    m_Amount = (float)joyEvent.jaxis.value; //[-32768 to 32767]
    ignore = true; // FIXME!!
    //CLog::Log(LOGNOTICE, "Joystick %d axis %d amount %d", joyId, axisId, (int)m_Amount);
    break;

  case SDL_JOYBALLMOTION:
  case SDL_JOYHATMOTION:
  case SDL_JOYBUTTONUP:
  default:
    ignore = true;
    break;
  }

  if (ignore)
    return;

  if (axis)
  {
    if (m_Axis = g_buttonTranslator.TranslateJoystickString(m_JoystickNames[joyId].c_str(), 
                                                            axisId, m_Action, true))
    {
      m_Button = 0;
    }
  }
  else
  {
    if (m_Button = g_buttonTranslator.TranslateJoystickString(m_JoystickNames[joyId].c_str(), 
                                                              buttonId, m_Action, false))
    {
      m_Axis = 0;
    }
  }
}

#endif
