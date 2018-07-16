/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "definitions.h"

namespace kodi
{
namespace gui
{

  //============================================================================
  ///
  // \defgroup cpp_kodi_gui ::general
  /// \addtogroup cpp_kodi_gui
  /// @{
  /// @brief **Allow use of binary classes and function to use on add-on's**
  ///
  /// Permits the use of the required functions of the add-on to Kodi. This class
  /// also contains some functions to the control.
  ///
  /// These are pure functions them no other initialization need.
  ///
  /// It has the header \ref kodi/gui/General.h "#include <kodi/gui/General.h>" be included
  /// to enjoy it.
  ///

  //==========================================================================
  ///
  /// \ingroup cpp_kodi_gui
  /// @brief Performs a graphical lock of rendering engine
  ///
  inline void Lock()
  {
    using namespace ::kodi::addon;
    CAddonBase::m_interface->toKodi->kodi_gui->general->lock();
  }

  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// \ingroup cpp_kodi_gui
  /// @brief Performs a graphical unlock of previous locked rendering engine
  ///
  inline void Unlock()
  {
    using namespace ::kodi::addon;
    CAddonBase::m_interface->toKodi->kodi_gui->general->unlock();
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// \ingroup cpp_kodi_gui
  /// @brief Return the the current screen height with pixel
  ///
  inline int GetScreenHeight()
  {
    using namespace ::kodi::addon;
    return CAddonBase::m_interface->toKodi->kodi_gui->general->get_screen_height(CAddonBase::m_interface->toKodi->kodiBase);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// \ingroup cpp_kodi_gui
  /// @brief Return the the current screen width with pixel
  ///
  inline int GetScreenWidth()
  {
    using namespace ::kodi::addon;
    return CAddonBase::m_interface->toKodi->kodi_gui->general->get_screen_width(CAddonBase::m_interface->toKodi->kodiBase);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// \ingroup cpp_kodi_gui
  /// @brief Return the the current screen rendering resolution
  ///
  inline int GetVideoResolution()
  {
    using namespace ::kodi::addon;
    return CAddonBase::m_interface->toKodi->kodi_gui->general->get_video_resolution(CAddonBase::m_interface->toKodi->kodiBase);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// \ingroup cpp_kodi_gui
  /// @brief Returns the id for the current 'active' dialog as an integer.
  ///
  /// @return                        The currently active dialog Id
  ///
  ///
  ///-------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ..
  /// int wid = kodi::gui::GetCurrentWindowDialogId()
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  inline int GetCurrentWindowDialogId()
  {
    using namespace ::kodi::addon;
    return CAddonBase::m_interface->toKodi->kodi_gui->general->get_current_window_dialog_id(CAddonBase::m_interface->toKodi->kodiBase);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// \ingroup cpp_kodi_gui
  /// @brief Returns the id for the current 'active' window as an integer.
  ///
  /// @return                        The currently active window Id
  ///
  ///
  ///-------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ..
  /// int wid = kodi::gui::GetCurrentWindowId()
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  inline int GetCurrentWindowId()
  {
    using namespace ::kodi::addon;
    return CAddonBase::m_interface->toKodi->kodi_gui->general->get_current_window_id(CAddonBase::m_interface->toKodi->kodiBase);
  }
  //--------------------------------------------------------------------------

} /* namespace gui */
} /* namespace kodi */
