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
  /// \defgroup cpp_kodi_gui_controls_CImage Control Image
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::controls::CImage }
  /// **Window control used to show an image.**
  ///
  /// The  image control is used  for displaying  images in Kodi. You can choose
  /// the position, size, transparency and contents of the image to be displayed.
  ///
  /// It has the header \ref Image.h "#include <kodi/gui/controls/Image.h>"
  /// be included to enjoy it.
  ///
  /// Here you find the needed skin part for a \ref Image_Control "image control"
  ///
  /// @note The  call of  the control is  only possible  from the  corresponding
  /// window as its class and identification number is required.
  ///
  class CImage : public CAddonGUIControlBase
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CImage
    /// @brief Construct a new control
    ///
    /// @param[in] window               related window control class
    /// @param[in] controlId            Used skin xml control id
    ///
    CImage(CWindow* window, int controlId)
      : CAddonGUIControlBase(window)
    {
      m_controlHandle = m_interface->kodi_gui->window->get_control_image(m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
      if (!m_controlHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::controls::CImage can't create control class from Kodi !!!");
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CImage
    /// @brief Destructor
    ///
    ~CImage() override = default;
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CImage
    /// @brief Set the control on window to visible
    ///
    /// @param[in] visible              If true visible, otherwise hidden
    ///
    void SetVisible(bool visible)
    {
      m_interface->kodi_gui->control_image->set_visible(m_interface->kodiBase, m_controlHandle, visible);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CImage
    /// @brief To set the filename used on image control.
    ///
    /// @param[in] filename             Image file to use
    /// @param[in] useCache             To define  storage of image,  default is
    ///                                 in  cache,  if false  becomes it  loaded
    ///                                 always on changes again
    ///
    void SetFileName(const std::string& filename, bool useCache = true)
    {
      m_interface->kodi_gui->control_image->set_filename(m_interface->kodiBase, m_controlHandle, filename.c_str(), useCache);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_controls_CImage
    /// @brief To set set the diffuse color on image.
    ///
    /// @param[in] colorDiffuse         Color to use for diffuse
    ///
    void SetColorDiffuse(uint32_t colorDiffuse)
    {
      m_interface->kodi_gui->control_image->set_color_diffuse(m_interface->kodiBase, m_controlHandle, colorDiffuse);
    }
    //--------------------------------------------------------------------------
  };

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */
