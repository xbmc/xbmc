/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "IConfigurationWindow.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMapper.h"
#include "input/keyboard/IKeyboardHandler.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include <set>
#include <string>
#include <vector>

namespace GAME
{
  class CGUIConfigurationWizard : public IConfigurationWizard,
                                  public JOYSTICK::IButtonMapper,
                                  public KEYBOARD::IKeyboardHandler,
                                  public Observer,
                                  protected CThread
  {
  public:
    CGUIConfigurationWizard();

    virtual ~CGUIConfigurationWizard(void) { }

    // implementation of IConfigurationWizard
    virtual void Run(const std::string& strControllerId, const std::vector<IFeatureButton*>& buttons) override;
    virtual void OnUnfocus(IFeatureButton* button) override;
    virtual bool Abort(bool bWait = true) override;

    // implementation of IButtonMapper
    virtual std::string ControllerID(void) const override { return m_strControllerId; }
    virtual bool MapPrimitive(JOYSTICK::IButtonMap* buttonMap, const JOYSTICK::CDriverPrimitive& primitive) override;

    // implementation of IKeyboardHandler
    virtual bool OnKeyPress(const CKey& key) override;
    virtual void OnKeyRelease(const CKey& key) override { }

    // implementation of Observer
    virtual void Notify(const Observable& obs, const ObservableMessage msg) override;

  protected:
    // implementation of CThread
    virtual void Process(void) override;

  private:
    void InitializeState(void);

    void InstallHooks(void);
    void RemoveHooks(void);

    // Run() parameters
    std::string                          m_strControllerId;
    std::vector<IFeatureButton*>         m_buttons;

    // State variables and mutex
    IFeatureButton*                      m_currentButton;
    JOYSTICK::CARDINAL_DIRECTION         m_currentDirection;
    std::set<JOYSTICK::CDriverPrimitive> m_history; // History to avoid repeated features
    CCriticalSection                     m_stateMutex;

    // Synchronization event
    CEvent                               m_inputEvent;
  };
}
