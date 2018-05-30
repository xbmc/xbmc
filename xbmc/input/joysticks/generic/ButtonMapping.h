/*
 *      Copyright (C) 2014-2017 Team Kodi
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

#include "input/joysticks/interfaces/IButtonMapCallback.h"
#include "input/joysticks/interfaces/IDriverHandler.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"
#include "input/mouse/MouseTypes.h"

#include <map>
#include <memory>
#include <stdint.h>

class IKeymap;

namespace KODI
{
namespace JOYSTICK
{
  class CButtonMapping;
  class IButtonMap;
  class IButtonMapper;

  /*!
   * \brief Detects and dispatches mapping events
   *
   * A mapping event usually occurs when a driver primitive is pressed or
   * exceeds a certain threshold.
   *
   * Detection can be quite complicated due to driver bugs, so each type of
   * driver primitive is given its own detector class inheriting from this one.
   */
  class CPrimitiveDetector
  {
  protected:
    CPrimitiveDetector(CButtonMapping* buttonMapping);

    /*!
     * \brief Dispatch a mapping event
     *
     * \return True if the primitive was mapped, false otherwise
     */
    bool MapPrimitive(const CDriverPrimitive &primitive);

  private:
    CButtonMapping* const m_buttonMapping;
  };

  /*!
   * \brief Detects when a button should be mapped
   */
  class CButtonDetector : public CPrimitiveDetector
  {
  public:
    CButtonDetector(CButtonMapping* buttonMapping, unsigned int buttonIndex);

    /*!
     * \brief Button state has been updated
     *
     * \param bPressed The new state
     *
     * \return True if this press was handled, false if it should fall through
     *         to the next driver handler
     */
    bool OnMotion(bool bPressed);

  private:
    // Construction parameters
    const unsigned int m_buttonIndex;
  };

  /*!
   * \brief Detects when a D-pad direction should be mapped
   */
  class CHatDetector : public CPrimitiveDetector
  {
  public:
    CHatDetector(CButtonMapping* buttonMapping, unsigned int hatIndex);

    /*!
     * \brief Hat state has been updated
     *
     * \param state The new state
     *
     * \return True if state is a cardinal direction, false otherwise
     */
    bool OnMotion(HAT_STATE state);

  private:
    // Construction parameters
    const unsigned int m_hatIndex;
  };

  struct AxisConfiguration
  {
    bool bKnown = false;
    int center = 0;
    unsigned int range = 1;
    bool bLateDiscovery = false;
  };

  /*!
   * \brief Detects when an axis should be mapped
   */
  class CAxisDetector : public CPrimitiveDetector
  {
  public:
    CAxisDetector(CButtonMapping* buttonMapping, unsigned int axisIndex, const AxisConfiguration& config);

    /*!
     * \brief Axis state has been updated
     *
     * \param position The new state
     *
     * \return Always true - axis motion events are always absorbed while button mapping
     */
    bool OnMotion(float position);

    /*!
     * \brief Called once per frame
     *
     * If an axis was activated, the button mapping command will be emitted
     * here.
     */
    void ProcessMotion();

    /*!
     * \brief Check if the axis was mapped and is still in motion
     *
     * \return True between when the axis is mapped and when it crosses zero
     */
    bool IsMapping() const { return m_state == AXIS_STATE::MAPPED; }

    /*!
     * \brief Set the state such that this axis has generated a mapping event
     *
     * If an axis is mapped to the Select action, it may be pressed when button
     * mapping begins. This function is used to indicate that the axis shouldn't
     * be mapped until after it crosses zero again.
     */
    void SetEmitted(const CDriverPrimitive& activePrimitive);

  private:
    enum class AXIS_STATE
    {
      /*!
       * \brief Axis is inactive (position is less than threshold)
       */
      INACTIVE,

      /*!
       * \brief Axis is activated (position has exceeded threshold)
       */
      ACTIVATED,

      /*!
       * \brief Axis has generated a mapping event, but has not been centered yet
       */
      MAPPED,
    };

    enum class AXIS_TYPE
    {
      /*!
       * \brief Axis type is initially unknown
       */
      UNKNOWN,

      /*!
       * \brief Axis is centered about 0
       *
       *   - If the axis is an analog stick, it can travel to -1 or +1.
       *   - If the axis is a pressure-sensitive button or a normal trigger,
       *     it can travel to +1.
       *   - If the axis is a DirectInput trigger, then it is possible that two
       *     triggers can be on the same axis in opposite directions.
       *   - Normally, D-pads  appear as a hat or four buttons. However, some
       *     D-pads are reported as two axes that can have the discrete values
       *     -1, 0 or 1. This is called a "discrete D-pad".
       */
      NORMAL,

      /*!
       * \brief Axis is centered about -1 or 1
       *
       *   - On OSX, with the cocoa driver, triggers are centered about -1 and
       *     travel to +1. In this case, the range is 2 and the direction is
       *     positive.
       *   - The author of SDL has observed triggers centered at +1 and travel
       *     to 0. In this case, the range is 1 and the direction is negative.
       */
      OFFSET,
    };

    void DetectType(float position);

    // Construction parameters
    const unsigned int m_axisIndex;
    AxisConfiguration m_config; // mutable

    // State variables
    AXIS_STATE m_state;
    CDriverPrimitive m_activatedPrimitive;
    AXIS_TYPE m_type;
    bool m_initialPositionKnown; // set to true on first motion
    float m_initialPosition; // set to position of first motion
    bool m_initialPositionChanged; // set to true when position differs from the initial position
    unsigned int m_activationTimeMs; // only used to delay anomalous trigger mapping to detect full range
  };

  /*!
   * \brief Detects when a keyboard key should be mapped
   */
  class CKeyDetector : public CPrimitiveDetector
  {
  public:
    CKeyDetector(CButtonMapping* buttonMapping, XBMCKey keycode);

    /*!
     * \brief Key state has been updated
     *
     * \param bPressed The new state
     *
     * \return True if this press was handled, false if it should fall through
     *         to the next driver handler
     */
    bool OnMotion(bool bPressed);

  private:
    // Construction parameters
    const XBMCKey m_keycode;
  };

  /*!
   * \brief Detects when a mouse button should be mapped
   */
  class CMouseButtonDetector : public CPrimitiveDetector
  {
  public:
    CMouseButtonDetector(CButtonMapping* buttonMapping, MOUSE::BUTTON_ID buttonIndex);

    /*!
     * \brief Button state has been updated
     *
     * \param bPressed The new state
     *
     * \return True if this press was handled, false if it should fall through
     *         to the next driver handler
     */
    bool OnMotion(bool bPressed);

  private:
    // Construction parameters
    const MOUSE::BUTTON_ID m_buttonIndex;
  };

  /*!
   * \brief Detects when a mouse button should be mapped
   */
  class CPointerDetector : public CPrimitiveDetector
  {
  public:
    CPointerDetector(CButtonMapping* buttonMapping);

    /*!
     * \brief Pointer position has been updated
     *
     * \param x The new x coordinate
     * \param y The new y coordinate
     *
     * \return Always true - pointer motion events are always absorbed while
     *         button mapping
     */
    bool OnMotion(int x, int y);

  private:
    // Utility function
    static INPUT::INTERCARDINAL_DIRECTION GetPointerDirection(int x, int y);

    static const unsigned int MIN_FRAME_COUNT = 10;

    // State variables
    bool m_bStarted = false;
    int m_startX = 0;
    int m_startY = 0;
    unsigned int m_frameCount = 0;
  };

  /*!
   * \ingroup joystick
   * \brief Generic implementation of a class that provides button mapping by
   *        translating driver events to button mapping commands
   *
   * Button mapping commands are invoked instantly for buttons and hats.
   *
   * Button mapping commands are deferred for a short while after an axis is
   * activated, and only one button mapping command will be invoked per
   * activation.
   */
  class CButtonMapping : public IDriverHandler,
                         public KEYBOARD::IKeyboardDriverHandler,
                         public MOUSE::IMouseDriverHandler,
                         public IButtonMapCallback
  {
  public:
    /*!
     * \brief Constructor for CButtonMapping
     *
     * \param buttonMapper Carries out button-mapping commands using <buttonMap>
     * \param buttonMap The button map given to <buttonMapper> on each command
     */
    CButtonMapping(IButtonMapper* buttonMapper, IButtonMap* buttonMap, IKeymap* keymap);

    virtual ~CButtonMapping() = default;

    // implementation of IDriverHandler
    virtual bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
    virtual bool OnHatMotion(unsigned int hatIndex, HAT_STATE state) override;
    virtual bool OnAxisMotion(unsigned int axisIndex, float position, int center, unsigned int range) override;
    virtual void ProcessAxisMotions(void) override;

    // implementation of IKeyboardDriverHandler
    bool OnKeyPress(const CKey& key) override;
    void OnKeyRelease(const CKey& key) override { }

    // implementation of IMouseDriverHandler
    bool OnPosition(int x, int y) override;
    bool OnButtonPress(MOUSE::BUTTON_ID button) override;
    void OnButtonRelease(MOUSE::BUTTON_ID button) override;

    // implementation of IButtonMapCallback
    virtual void SaveButtonMap() override;
    virtual void ResetIgnoredPrimitives() override;
    virtual void RevertButtonMap() override;

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

  private:
    bool IsMapping() const;

    void OnLateDiscovery(unsigned int axisIndex);

    CButtonDetector& GetButton(unsigned int buttonIndex);
    CHatDetector& GetHat(unsigned int hatIndex);
    CAxisDetector& GetAxis(unsigned int axisIndex, float position, const AxisConfiguration& initialConfig = AxisConfiguration());
    CKeyDetector& GetKey(XBMCKey keycode);
    CMouseButtonDetector& GetMouseButton(MOUSE::BUTTON_ID buttonIndex);
    CPointerDetector &GetPointer();

    // Construction parameters
    IButtonMapper* const m_buttonMapper;
    IButtonMap* const    m_buttonMap;
    IKeymap* const       m_keymap;

    std::map<unsigned int, CButtonDetector> m_buttons;
    std::map<unsigned int, CHatDetector> m_hats;
    std::map<unsigned int, CAxisDetector> m_axes;
    std::map<XBMCKey, CKeyDetector> m_keys;
    std::map<MOUSE::BUTTON_ID, CMouseButtonDetector> m_mouseButtons;
    std::unique_ptr<CPointerDetector> m_pointer;
    unsigned int m_lastAction;
    uint64_t m_frameCount;
  };
}
}
