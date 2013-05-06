#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "Addon.h"
#include "input/ButtonTranslator.h"

namespace ADDON
{
  class CPeripheral;
  typedef boost::shared_ptr<CPeripheral> PeripheralPtr;

  /*! \brief Encapsulates a peripheral.
   * \details A peripheral can consist of keymaps, lircmaps and irssmaps
   */
  class CPeripheral : public CAddon
  {
  public:
    /*! \brief Constructor
     * \param[in] ext The c-pluff extension descriptor
     */
    CPeripheral(const cp_extension_t *ext);

    /*! \brief Constructor
     * \param[in] ext The pre-initialized addon props structure
     * \details Note that the peripheral is not considered loaded after construction
     */
    CPeripheral(const AddonProps& props) : CAddon(props), 
                                           m_conditional(false), m_reference(false) {}

    /* \brief Empty destructor */
    virtual ~CPeripheral() {}

    /*! \brief Load all keymaps into memory
     * \return False if nothing was loaded, true otherwise
     */
    bool LoadKeymaps();


    const std::map<int, CButtonTranslator::buttonMap>& GetTranslatorMap() const
    {
      return m_translatorMap;
    }

    const std::map<int, CButtonTranslator::buttonMap>& GetTouchMap() const
    {
      return m_touchMap;
    }

    const CButtonTranslator::JoystickMappings& GetJoyButtonMap() const
    {
      return m_joystickButtonMap;
    }

    const CButtonTranslator::JoystickMappings& GetJoyAxisMap() const
    {
      return m_joystickAxisMap;
    }

    const CButtonTranslator::JoystickMappings& GetJoyHatMap() const
    {
      return m_joystickHatMap;
    }

    /*! \brief Returns whether an add-on should be conditionally loaded
     * \details This is used with peripherals supported by the peripheral manager.
     *          Prevents auto-loading keymaps which may only be suitable for 
     *          particular devices.
     */
    bool IsConditional() const
    {
      return m_conditional;
    }

    /*! \brief Returns whether an add-on represents a reference mapping.
     * \details Device such as keyboard and mouse have reference mappings.
     *          Peripherals with this flag set are loaded first, before other
     *          add-ons.
     */
    bool IsReference() const
    {
      return m_reference;
    }
  protected:
    void MapWindowActions(TiXmlNode *pWindow, int windowID,
                          const std::string& type);
    void MapTouchActions(int windowID, TiXmlNode *pTouch);
    void MapJoystickActions(int windowID, TiXmlNode *pJoystick,
                            const std::vector<std::string>& names);

    //! \brief Maps from window to button mappings
    std::map<int, CButtonTranslator::buttonMap> m_touchMap;

    //! \brief Maps from window to button mappings
    std::map<int, CButtonTranslator::buttonMap> m_translatorMap;

#if defined(HAS_SDL_JOYSTICK) || defined(HAS_EVENT_SERVER)
    CButtonTranslator::JoystickMappings m_joystickButtonMap;
    CButtonTranslator::JoystickMappings m_joystickAxisMap;
    CButtonTranslator::JoystickMappings m_joystickHatMap;
#endif

    bool m_conditional; ///< Whether or not add-on should be conditionally loaded.
    bool m_reference; ///< Whether or not add-on represents a reference device mapping.
  };
}
