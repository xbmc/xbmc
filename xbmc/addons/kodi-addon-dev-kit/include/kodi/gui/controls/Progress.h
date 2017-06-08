#pragma once
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
  /// \defgroup cpp_kodi_gui_controls_CProgress Control Progress
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::controls::CProgress }
  /// **Window control to show the progress of a particular operation**
  ///
  /// The progress control is used to show the progress of an item that may take
  /// a long time,  or to show how far  through a movie you are.  You can choose
  /// the position, size, and look of the progress control.
  ///
  /// It has the header \ref Progress.h "#include <kodi/gui/controls/Progress.h>"
  /// be included to enjoy it.
  ///
  /// Here you find the needed skin part for a \ref Progress_Control "progress control"
  ///
  /// @note The call of the control is only possible from the corresponding
  /// window as its class and identification number is required.
  ///
  class CProgress : public CAddonGUIControlBase
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CProgress
    /// @brief Construct a new control
    ///
    /// @param[in] window               related window control class
    /// @param[in] controlId            Used skin xml control id
    ///
    CProgress(CWindow* window, int controlId)
      : CAddonGUIControlBase(window)
    {
      m_controlHandle = m_interface->kodi_gui->window->get_control_progress(m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
      if (!m_controlHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::controls::CProgress can't create control class from Kodi !!!");
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CProgress
    /// @brief Destructor
    ///
    virtual ~CProgress() = default;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CProgress
    /// @brief Set the control on window to visible
    ///
    /// @param[in] visible              If true visible, otherwise hidden
    ///
    void SetVisible(bool visible)
    {
      m_interface->kodi_gui->control_progress->set_visible(m_interface->kodiBase, m_controlHandle, visible);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CProgress
    /// @brief To set Percent position of control
    ///
    /// @param[in] percent              The percent position to use
    ///
    void SetPercentage(float percent)
    {
      m_interface->kodi_gui->control_progress->set_percentage(m_interface->kodiBase, m_controlHandle, percent);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CProgress
    /// @brief Get the active percent position of progress bar
    ///
    /// @return                         Progress position as percent
    ///
    float GetPercentage() const
    {
      return m_interface->kodi_gui->control_progress->get_percentage(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------
  };

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */
