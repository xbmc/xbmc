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
  /// \defgroup cpp_kodi_gui_controls_CTextBox Control Text Box
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::controls::CTextBox }
  /// **Used to show a multi-page piece of text**
  ///
  /// The text box control  can be used to display  descriptions,  help texts or
  /// other larger texts.  It corresponds to the representation which is also to
  /// be seen on the CDialogTextViewer.
  ///
  /// It has the header \ref TextBox.h "#include <kodi/gui/controls/TextBox.h>"
  /// be included to enjoy it.
  ///
  /// Here you find the needed skin part for a \ref Text_Box "textbox control".
  ///
  /// @note The call  of the  control is  only possible  from the  corresponding
  /// window as its class and identification number is required.
  ///
  class CTextBox : public CAddonGUIControlBase
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CTextBox
    /// @brief Construct a new control
    ///
    /// @param[in] window               related window control class
    /// @param[in] controlId            Used skin xml control id
    ///
    CTextBox(CWindow* window, int controlId)
      : CAddonGUIControlBase(window)
    {
      m_controlHandle = m_interface->kodi_gui->window->get_control_text_box(m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
      if (!m_controlHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::controls::CTextBox can't create control class from Kodi !!!");
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CTextBox
    /// @brief Destructor
    ///
    virtual ~CTextBox() = default;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CTextBox
    /// @brief Set the control on window to visible
    ///
    /// @param[in] visible              If true visible, otherwise hidden
    ///
    void SetVisible(bool visible)
    {
      m_interface->kodi_gui->control_text_box->set_visible(m_interface->kodiBase, m_controlHandle,  visible);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CTextBox
    /// @brief To reset box an remove all the text
    ///
    void Reset()
    {
      m_interface->kodi_gui->control_text_box->reset(m_controlHandle, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CTextBox
    /// @brief To set the text on box
    ///
    /// @param[in] text                 Text to show
    ///
    void SetText(const std::string& text)
    {
      m_interface->kodi_gui->control_text_box->set_text(m_interface->kodiBase, m_controlHandle,  text.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CTextBox
    /// @brief Get the used text from control
    ///
    /// @return                         Text shown
    ///
    std::string GetText() const
    {
      std::string text;
      char* ret = m_interface->kodi_gui->control_text_box->get_text(m_interface->kodiBase, m_controlHandle);
      if (ret != nullptr)
      {
        if (std::strlen(ret))
          text = ret;
        m_interface->free_string(m_interface->kodiBase, ret);
      }
      return text;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CTextBox
    /// @brief To scroll text on other position
    ///
    /// @param[in] position             The line position to scroll to
    ///
    void Scroll(unsigned int position)
    {
      m_interface->kodi_gui->control_text_box->scroll(m_interface->kodiBase, m_controlHandle, position);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CTextBox
    /// @brief To set automatic scrolling of textbox
    ///
    /// Specifies the timing  and conditions of  any autoscrolling  this textbox
    /// should have. Times are in milliseconds.  The content is delayed  for the
    /// given delay,  then scrolls at a rate of one line per time interval until
    /// the end.  If the repeat tag is present,  it then  delays for  the repeat
    /// time,  fades out over 1 second,  and repeats.  It does not wrap or reset
    /// to the top at the end of the scroll.
    ///
    /// @param[in] delay                Content delay
    /// @param[in] time                 One line per time interval
    /// @param[in] repeat               Delays with given time, fades out over 1
    ///                                 second, and repeats
    ///
    void SetAutoScrolling(int delay, int time, int repeat)
    {
      m_interface->kodi_gui->control_text_box->set_auto_scrolling(m_interface->kodiBase, m_controlHandle, delay, time, repeat);
    }
    //--------------------------------------------------------------------------
  };

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */
