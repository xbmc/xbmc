/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "threads/Thread.h"

namespace JOYSTICK
{
  class IInputReceiver;

  class CRumbleGenerator : public CThread
  {
  public:
    CRumbleGenerator(const std::string& controllerId);

    virtual ~CRumbleGenerator(void) { AbortRumble(); }

    void NotifyUser(IInputReceiver* receiver);
    bool DoTest(IInputReceiver* receiver);

    void AbortRumble(void) { StopThread(); }

  protected:
    // implementation of CThread
    void Process(void);

  private:
    enum RUMBLE_TYPE
    {
      RUMBLE_UNKNOWN,
      RUMBLE_NOTIFICATION,
      RUMBLE_TEST,
    };

    static std::vector<std::string> GetMotors(const std::string& controllerId);

    // Construction param
    const std::vector<std::string> m_motors;

    // Test param
    IInputReceiver* m_receiver;
    RUMBLE_TYPE     m_type;
  };
}
