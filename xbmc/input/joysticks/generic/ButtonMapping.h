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

#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMapCallback.h"
#include "input/joysticks/IDriverHandler.h"

#include <vector>

namespace JOYSTICK
{
  class IActionMap;
  class IButtonMap;
  class IButtonMapper;

  /*!
   * \ingroup joystick
   * \brief Generic implementation of a class that provides button mapping by
   *        translating driver events to button mapping commands
   *
   * Button mapping commands are invoked instantly for buttons and hats.
   *
   * Button mapping commands are deferred for a short while after an axis is
   * activated, and only one command will be invoked per activation.
   */
  class CButtonMapping : public IDriverHandler,
                         public IButtonMapCallback
  {
  public:
    /*!
     * \brief Constructor for CButtonMapping
     *
     * \param buttonMapper Carries out button-mapping commands using <buttonMap>
     * \param buttonMap The button map given to <buttonMapper> on each command
     */
    CButtonMapping(IButtonMapper* buttonMapper, IButtonMap* buttonMap, IActionMap* actionMap);

    virtual ~CButtonMapping(void) { }

    // implementation of IDriverHandler
    virtual bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
    virtual bool OnHatMotion(unsigned int hatIndex, HAT_STATE state) override;
    virtual bool OnAxisMotion(unsigned int axisIndex, float position) override;
    virtual void ProcessAxisMotions(void) override;

    // implementation of IButtonMapCallback
    virtual void SaveButtonMap() override;
    virtual void ResetIgnoredPrimitives() override;
    virtual void RevertButtonMap() override;

  private:
    /*!
     * \brief Process the primitive mapping command
     *
     * First, this function checks if the input should be dropped. This can
     * happen if the input is ignored or the cooldown period is active. If the
     * input is dropped, this returns true with no effect, effectively absorbing
     * the input. Otherwise, the mapping command is sent to m_buttonMapper.
     *
     * \param primitive The primitive being mapped
     * \return True if the mapping command was handled, false otherwise
     */
    bool MapPrimitive(const CDriverPrimitive& primitive);

    // Motion functions
    void OnMotion(const CDriverPrimitive& semiaxis);
    void OnMotionless(const CDriverPrimitive& semiaxis);
    bool IsMoving() const;

    // Action functions
    void Activate(const CDriverPrimitive& semiaxis);
    void Deactivate(const CDriverPrimitive& semiaxis);
    bool IsActive(const CDriverPrimitive& semiaxis) const;

    // Construction parameters
    IButtonMapper* const m_buttonMapper;
    IButtonMap* const    m_buttonMap;
    IActionMap* const    m_actionMap;

    struct ActivatedAxis
    {
      CDriverPrimitive driverPrimitive;
      bool             bEmitted; // true if this axis has emited a button-mapping command
    };

    std::vector<CDriverPrimitive> m_movingAxes;
    std::vector<ActivatedAxis> m_activatedAxes;
    unsigned int               m_lastAction;
  };
}
