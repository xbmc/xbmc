 /*     Copyright (C) 2009 by Cory Fields
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#include "WiiUse_WiiRemote.h"

void CWiiController::get_keys(wiimote* wm)
{
  m_buttonHeld = 0;
  m_buttonPressed = 0;
  m_buttonReleased = 0;

  m_repeatableHeld = (wm->btns & (m_repeatFlags));
  m_holdableHeld = (wm->btns & (m_holdFlags));
  m_repeatableReleased = (wm->btns_released & (m_repeatFlags));
  m_holdableReleased = (wm->btns_released & (m_holdFlags));

  for (int i = 1;i <= WIIMOTE_NUM_BUTTONS; i++)
  {
    if (IS_PRESSED(wm,convert_code(i)))
      m_buttonPressed = i;
    if (IS_RELEASED(wm,convert_code(i)))
      m_buttonReleased = i;
    if (IS_HELD(wm,convert_code(i)))
      m_buttonHeld = i;
  }
}

void CWiiController::handleKeyPress() 
{
  if ((m_holdableReleased && m_buttonDownTime < g_hold_button_timeout))
    EventClient.SendButton(m_buttonReleased, m_joyString, BTN_QUEUE | BTN_NO_REPEAT);
  if (m_buttonPressed && !m_holdableHeld && !m_buttonHeld)  
    EventClient.SendButton(m_buttonPressed, m_joyString, BTN_QUEUE | BTN_NO_REPEAT);
}

void CWiiController::handleRepeat()
{
  if (m_repeatableHeld)
    EventClient.SendButton(m_buttonPressed, m_joyString, BTN_QUEUE | BTN_NO_REPEAT, 5);
}

void CWiiController::handleACC(float currentRoll, float currentPitch)
{
  int rollWeight = 0;
  int tiltWeight = 0;

  if (m_start_roll == 0)
    m_start_roll = currentRoll;
  m_abs_roll = smoothDeg(m_abs_roll, currentRoll);
  m_rel_roll = m_abs_roll - m_start_roll;
  rollWeight = int((m_rel_roll*m_rel_roll));
  if (rollWeight > 65000)
    rollWeight = 65000;

 if (m_start_pitch == 0) 
    m_start_pitch = currentPitch;
  m_abs_pitch = smoothDeg(m_abs_pitch, currentPitch);
  m_rel_pitch = m_start_pitch - m_abs_pitch;
  tiltWeight = int((m_rel_pitch*m_rel_pitch*16));
  if (tiltWeight > 65000)
    tiltWeight = 65000;

  if (m_currentAction == ACTION_NONE)
  {
    if ((g_deadzone - (abs((int)m_rel_roll)) < 5) && (abs((int)m_abs_pitch) < (g_deadzone / 1.5)))
     // crossed the roll deadzone threshhold while inside the pitch deadzone
      {
        m_currentAction = ACTION_ROLL;
      }
    else if ((g_deadzone - (abs((int)m_rel_pitch)) < 5) && (abs((int)m_abs_roll) < (g_deadzone / 1.5))) 
    // crossed the pitch deadzone threshhold while inside the roll deadzone
      {
        m_currentAction = ACTION_PITCH;
      }
  }

  if (m_currentAction == ACTION_ROLL)
  {
    if (m_rel_roll < -g_deadzone)
      EventClient.SendButton(KEYCODE_ROLL_NEG, m_joyString, BTN_QUEUE | BTN_NO_REPEAT, rollWeight);
    if (m_rel_roll > g_deadzone)
      EventClient.SendButton(KEYCODE_ROLL_POS, m_joyString, BTN_QUEUE | BTN_NO_REPEAT, rollWeight);
//    printf("Roll: %f\n",m_rel_roll);
  }

  if (m_currentAction == ACTION_PITCH)
  {
    if (m_rel_pitch > g_deadzone)
      EventClient.SendButton(KEYCODE_PITCH_POS, m_joyString, BTN_QUEUE | BTN_NO_REPEAT,tiltWeight);
    if (m_rel_pitch < -g_deadzone)
      EventClient.SendButton(KEYCODE_PITCH_NEG, m_joyString, BTN_QUEUE | BTN_NO_REPEAT,tiltWeight);
//    printf("Pitch: %f\n",m_rel_pitch);
  }
}

void CWiiController::handleIR()
{
//TODO
}

int connectWiimote(wiimote** wiimotes)
{
  int found = 0;
  int connected = 0;
  wiimote* wm;
  while(!found)
  {
    //Look for Wiimotes. Keep searching until one is found. Wiiuse provides no way to search forever, so search for 5 secs and repeat.
    found = wiiuse_find(wiimotes, MAX_WIIMOTES, 5);
  }
  connected = wiiuse_connect(wiimotes, MAX_WIIMOTES);
  wm = wiimotes[0];
  if (connected)
  {
    EventClient.SendHELO("Wii Remote", ICON_PNG, NULL); 
    wiiuse_set_leds(wm, WIIMOTE_LED_1);
    wiiuse_rumble(wm, 1);
    wiiuse_set_orient_threshold(wm,1);
    #ifndef WIN32
      usleep(200000);
    #else
      Sleep(200);
    #endif
    wiiuse_rumble(wm, 0);

    return 1;
  }
  return 0;
}

int handle_disconnect(wiimote* wm)
{
  EventClient.SendNOTIFICATION("Wii Remote disconnected", "", ICON_PNG, NULL);
  return 0;
}

unsigned short convert_code(unsigned short client_code)
{
//WIIMOTE variables are from wiiuse. We need them to be 1-11 for the eventclient.
//For that we use this ugly conversion.

  switch (client_code)
  {
  case KEYCODE_BUTTON_UP: return WIIMOTE_BUTTON_UP;
  case KEYCODE_BUTTON_DOWN: return WIIMOTE_BUTTON_DOWN;
  case KEYCODE_BUTTON_LEFT: return WIIMOTE_BUTTON_LEFT;
  case KEYCODE_BUTTON_RIGHT: return WIIMOTE_BUTTON_RIGHT;
  case KEYCODE_BUTTON_A: return WIIMOTE_BUTTON_A;
  case KEYCODE_BUTTON_B: return WIIMOTE_BUTTON_B;
  case KEYCODE_BUTTON_MINUS: return WIIMOTE_BUTTON_MINUS;
  case KEYCODE_BUTTON_HOME: return WIIMOTE_BUTTON_HOME;
  case KEYCODE_BUTTON_PLUS: return WIIMOTE_BUTTON_PLUS;
  case KEYCODE_BUTTON_ONE: return WIIMOTE_BUTTON_ONE;
  case KEYCODE_BUTTON_TWO: return WIIMOTE_BUTTON_TWO;
  default : break;
  }
return 0;
}

float smoothDeg(float oldVal, float newVal)
{
//If values go over 180 or under -180, weird things happen. This tranforms the values to -360 to 360 instead.

  if (newVal - oldVal > 300)
    return (newVal - 360);
  if (oldVal - newVal > 300)
    return(newVal + 360);
  return(newVal);
}

int32_t getTicks(void)
{
  int32_t ticks;
  struct timeval now;
  gettimeofday(&now, NULL);
  ticks = now.tv_sec * 1000l;
  ticks += now.tv_usec / 1000l;
  return ticks;
}

int main(int argc, char** argv) 
{
  int32_t timeout = 0;
  bool connected = 0;
  wiimote** wiimotes;
  wiimotes =  wiiuse_init(MAX_WIIMOTES);
  CWiiController controller;
  wiimote* wm;
  {
  //Main Loop
  while (1)
    {
    if (!connected) connected = (connectWiimote(wiimotes));
    wm = wiimotes[0]; // Only worry about 1 controller. No need for more?
    controller.m_buttonDownTime = getTicks() - timeout;

//Handle ACC, Repeat, and IR outside of the Event loop so that buttons can be continuously sent
    if (timeout)
    {
      if ((controller.m_buttonDownTime > g_hold_button_timeout) && controller.m_holdableHeld)
        controller.handleACC(wm->orient.roll, wm->orient.pitch);
      if ((controller.m_buttonDownTime  > g_repeat_rate) && controller.m_repeatableHeld)
        controller.handleRepeat();
    }
    if (wiiuse_poll(wiimotes, MAX_WIIMOTES)) 
    {
      for (int i = 0; i < MAX_WIIMOTES; ++i)
      //MAX_WIIMOTES hardcoded at 1.
      {
        switch (wiimotes[i]->event) 
        {
        case WIIUSE_EVENT:

	  controller.get_keys(wm);  //Load up the CWiiController
          controller.handleKeyPress();
          if (!controller.m_buttonHeld && (controller.m_holdableHeld || controller.m_repeatableHeld))
          {
            //Prepare to repeat or hold. Do this only once.
            timeout = getTicks();
            EnableMotionSensing(wm);
            controller.m_abs_roll = 0;
            controller.m_abs_pitch = 0;
            controller.m_start_roll = 0;
            controller.m_start_pitch = 0;
          }
          if (controller.m_buttonReleased)
          {
            DisableMotionSensing(wm);
            controller.m_currentAction = ACTION_NONE;
          }
        break;
        case WIIUSE_STATUS:
        break;
        case WIIUSE_DISCONNECT:
        case WIIUSE_UNEXPECTED_DISCONNECT:
        handle_disconnect(wm);
	connected = 0;
        break;
        case WIIUSE_READ_DATA:
        break;
        default:
        break;
	}
      }
    }
  }
  }
  wiiuse_cleanup(wiimotes, MAX_WIIMOTES);
  return 0;
}
