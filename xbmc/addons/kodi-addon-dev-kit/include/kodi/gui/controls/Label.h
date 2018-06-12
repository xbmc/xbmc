/*
 *      Copyright (C) 2005-2017 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "../../AddonBase.h"
#include "../Window.h"

namespace kodi
{
namespace gui
{
namespace controls
{

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_controls_CLabel Control Label
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::controls::CLabel }
  /// **Window control used to show some lines of text.**
  ///
  /// The  label control  is used for  displaying text  in Kodi.  You can choose
  /// the font, size, colour, location and contents of the text to be displayed.
  ///
  /// It has the header \ref Label.h "#include <kodi/gui/controls/Label.h>"
  /// be included to enjoy it.
  ///
  /// Here you find the needed skin part for a \ref Label_Control "label control"
  ///
  /// @note The  call  of the control  is only possible  from the  corresponding
  /// window as its class and identification number is required.
  ///
  class CLabel : public CAddonGUIControlBase
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CLabel
    /// @brief Construct a new control
    ///
    /// @param[in] window               related window control class
    /// @param[in] controlId            Used skin xml control id
    ///
    CLabel(CWindow* window, int controlId)
      : CAddonGUIControlBase(window)
    {
      m_controlHandle = m_interface->kodi_gui->window->get_control_label(m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
      if (!m_controlHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::controls::CLabel can't create control class from Kodi !!!");
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CLabel
    /// @brief Destructor
    ///
    ~CLabel() override = default;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CLabel
    /// @brief Set the control on window to visible
    ///
    /// @param[in] visible              If true visible, otherwise hidden
    ///
    void SetVisible(bool visible)
    {
      m_interface->kodi_gui->control_label->set_visible(m_interface->kodiBase, m_controlHandle, visible);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CLabel
    /// @brief To set the text string on label
    ///
    /// @param[in] text                 Text to show
    ///
    void SetLabel(const std::string& text)
    {
      m_interface->kodi_gui->control_label->set_label(m_interface->kodiBase, m_controlHandle, text.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CLabel
    /// @brief Get the used text from control
    ///
    /// @return                         Used text on label control
    ///
    std::string GetLabel() const
    {
      std::string label;
      char* ret = m_interface->kodi_gui->control_label->get_label(m_interface->kodiBase, m_controlHandle);
      if (ret != nullptr)
      {
        if (std::strlen(ret))
          label = ret;
        m_interface->free_string(m_interface->kodiBase, ret);
      }
      return label;
    }
    //--------------------------------------------------------------------------
  };

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */
