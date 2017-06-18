/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include "input/joysticks/IButtonMapper.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
  class CGUIDialogButtonCapture : public JOYSTICK::IButtonMapper,
                                  public Observer,
                                  protected CThread
  {
  public:
    CGUIDialogButtonCapture();

    virtual ~CGUIDialogButtonCapture() = default;

    // implementation of IButtonMapper
    virtual std::string ControllerID(void) const override;
    virtual bool NeedsCooldown(void) const override { return false; }
    virtual bool Emulation(void) const override { return false; }
    virtual unsigned int ControllerNumber(void) const override { return 0; }
    virtual bool MapPrimitive(JOYSTICK::IButtonMap* buttonMap,
                              IKeymap* keymap,
                              const JOYSTICK::CDriverPrimitive& primitive) override;
    virtual void OnEventFrame(const JOYSTICK::IButtonMap* buttonMap, bool bMotion) override { }
    virtual void OnLateAxis(const JOYSTICK::IButtonMap* buttonMap, unsigned int axisIndex) override { }

    // implementation of Observer
    virtual void Notify(const Observable &obs, const ObservableMessage msg) override;

    /*!
     * \brief Show the dialog
     */
    void Show();

  protected:
    // implementation of CThread
    virtual void Process() override;

    virtual std::string GetDialogText() = 0;
    virtual std::string GetDialogHeader() = 0;
    virtual bool MapPrimitiveInternal(JOYSTICK::IButtonMap* buttonMap,
                                      IKeymap* keymap,
                                      const JOYSTICK::CDriverPrimitive& primitive) = 0;
    virtual void OnClose(bool bAccepted) = 0;

    CEvent m_captureEvent;

  private:
    void InstallHooks();
    void RemoveHooks();
  };
}
}
