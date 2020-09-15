/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/gui/dialogs/ok.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace dialogs
{

//==============================================================================
/// @defgroup cpp_kodi_gui_dialogs_OK Dialog OK
/// @ingroup cpp_kodi_gui_dialogs
/// @{
/// @brief @cpp_namespace{ kodi::gui::dialogs::OK }
/// **OK dialog**\n
/// The functions listed below permit the call of a dialogue of information, a
/// confirmation of the user by press from OK required.
///
/// It has the header @ref OK.h "#include <kodi/gui/dialogs/OK.h>"
/// be included to enjoy it.
///
namespace OK
{
//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_OK
/// @brief Use dialog to inform user with text and confirmation with OK with
/// continued string.
///
/// @param[in] heading Dialog heading.
/// @param[in] text Multi-line text.
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/OK.h>
/// ...
/// kodi::gui::dialogs::OK::ShowAndGetInput("Test dialog", "Hello World!\nI'm a call from add-on\n :) :D");
/// ~~~~~~~~~~~~~
///
inline void ATTRIBUTE_HIDDEN ShowAndGetInput(const std::string& heading, const std::string& text)
{
  using namespace ::kodi::addon;
  CAddonBase::m_interface->toKodi->kodi_gui->dialogOK->show_and_get_input_single_text(
      CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), text.c_str());
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_OK
/// @brief Use dialog to inform user with text and confirmation with OK with
/// strings separated to the lines.
///
/// @param[in] heading Dialog heading.
/// @param[in] line0 Line #1 text.
/// @param[in] line1 Line #2 text.
/// @param[in] line2 Line #3 text.
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/OK.h>
/// ...
/// kodi::gui::dialogs::OK::ShowAndGetInput("Test dialog", "Hello World!", "I'm a call from add-on", " :) :D");
/// ~~~~~~~~~~~~~
///
inline void ATTRIBUTE_HIDDEN ShowAndGetInput(const std::string& heading,
                                             const std::string& line0,
                                             const std::string& line1,
                                             const std::string& line2)
{
  using namespace ::kodi::addon;
  CAddonBase::m_interface->toKodi->kodi_gui->dialogOK->show_and_get_input_line_text(
      CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), line0.c_str(), line1.c_str(),
      line2.c_str());
}
//------------------------------------------------------------------------------
} // namespace OK
/// @}

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
