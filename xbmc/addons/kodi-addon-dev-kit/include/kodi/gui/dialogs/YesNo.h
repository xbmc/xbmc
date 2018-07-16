/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../definitions.h"
#include "../../AddonBase.h"

namespace kodi
{
namespace gui
{
namespace dialogs
{

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_dialogs_YesNo Dialog Yes/No
  /// \ingroup cpp_kodi_gui
  /// @{
  /// @brief \cpp_namespace{ kodi::gui::dialogs::YesNo }
  /// **Yes / No dialog**
  ///
  /// The Yes / No dialog can be used to inform the user about questions and get
  /// the answer.
  ///
  /// In order to achieve a line break is a <b>\\n</b> directly in the text or
  /// in the <em>"./resources/language/resource.language.??_??/strings.po"</em>
  /// to call with <b>std::string kodi::general::GetLocalizedString(...);</b>.
  ///
  /// It has the header \ref YesNo.h "#include <kodi/gui/dialogs/YesNo.h>"
  /// be included to enjoy it.
  ///
  ///
  namespace YesNo
  {
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_YesNo
    /// @brief Use dialog to get numeric new password with one text string shown
    /// everywhere and cancel return field
    ///
    /// @param[in] heading    Dialog heading
    /// @param[in] text       Multi-line text
    /// @param[out] canceled  Return value about cancel button
    /// @param[in] noLabel    [opt] label to put on the no button
    /// @param[in] yesLabel   [opt] label to put on the yes button
    /// @return           Returns True if 'Yes' was pressed, else False
    ///
    /// @note It is preferred to only use this as it is actually a multi-line text.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/gui/dialogs/YesNo.h>
    ///
    /// bool canceled;
    /// bool ret = kodi::gui::dialogs::YesNo::ShowAndGetInput(
    ///    "Yes / No test call",   /* The Header */
    ///    "You has opened Yes / No dialog for test\n\nIs this OK for you?",
    ///    canceled,               /* return value about cancel button */
    ///    "Not really",           /* No label, is optional and if empty "No" */
    ///    "Ohhh yes");            /* Yes label, also optional and if empty "Yes" */
    /// fprintf(stderr, "You has called Yes/No, returned '%s' and was %s\n",
    ///          ret ? "yes" : "no",
    ///          canceled ? "canceled" : "not canceled");
    /// ~~~~~~~~~~~~~
    ///
    inline bool ShowAndGetInput(const std::string& heading, const std::string& text,
                                bool& canceled, const std::string& noLabel = "",
                                const std::string& yesLabel = "")
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogYesNo->show_and_get_input_single_text(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                                    heading.c_str(), text.c_str(), &canceled,
                                                                                                    noLabel.c_str(), yesLabel.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_YesNo
    /// @brief Use dialog to get numeric new password with separated line strings
    ///
    /// @param[in] heading      Dialog heading
    /// @param[in] line0        Line #0 text
    /// @param[in] line1        Line #1 text
    /// @param[in] line2        Line #2 text
    /// @param[in] noLabel      [opt] label to put on the no button.
    /// @param[in] yesLabel     [opt] label to put on the yes button.
    /// @return                 Returns True if 'Yes' was pressed, else False.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/gui/dialogs/YesNo.h>
    ///
    /// bool ret = kodi::gui::dialogs::YesNo::ShowAndGetInput(
    ///    "Yes / No test call",   // The Header
    ///    "You has opened Yes / No dialog for test",
    ///    "",
    ///    "Is this OK for you?",
    ///    "Not really",           // No label, is optional and if empty "No"
    ///    "Ohhh yes");            // Yes label, also optional and if empty "Yes"
    /// fprintf(stderr, "You has called Yes/No, returned '%s'\n",
    ///          ret ? "yes" : "no");
    /// ~~~~~~~~~~~~~
    ///
    inline bool ShowAndGetInput(const std::string& heading, const std::string& line0, const std::string& line1,
                                const std::string& line2, const std::string& noLabel = "",
                                const std::string& yesLabel = "")
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogYesNo->show_and_get_input_line_text(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                                  heading.c_str(), line0.c_str(), line1.c_str(), line2.c_str(),
                                                                                                  noLabel.c_str(), yesLabel.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_YesNo
    /// @brief Use dialog to get numeric new password with separated line strings and cancel return field
    ///
    /// @param[in] heading      Dialog heading
    /// @param[in] line0        Line #0 text
    /// @param[in] line1        Line #1 text
    /// @param[in] line2        Line #2 text
    /// @param[out] canceled    Return value about cancel button
    /// @param[in] noLabel      [opt] label to put on the no button
    /// @param[in] yesLabel     [opt] label to put on the yes button
    /// @return                 Returns True if 'Yes' was pressed, else False
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/gui/dialogs/YesNo.h>
    ///
    /// bool canceled;
    /// bool ret = kodi::gui::dialogs::YesNo::ShowAndGetInput(
    ///    "Yes / No test call",   // The Header
    ///    "You has opened Yes / No dialog for test",
    ///    "",
    ///    "Is this OK for you?",
    ///    canceled,               // return value about cancel button
    ///    "Not really",           // No label, is optional and if empty "No"
    ///    "Ohhh yes");            // Yes label, also optional and if empty "Yes"
    /// fprintf(stderr, "You has called Yes/No, returned '%s' and was %s\n",
    ///          ret ? "yes" : "no",
    ///          canceled ? "canceled" : "not canceled");
    /// ~~~~~~~~~~~~~
    ///
    inline bool ShowAndGetInput(const std::string& heading, const std::string& line0, const std::string& line1,
                                const std::string& line2, bool& canceled, const std::string& noLabel = "",
                                const std::string& yesLabel = "")
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogYesNo->show_and_get_input_line_button_text(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                                         heading.c_str(), line0.c_str(), line1.c_str(), line2.c_str(),
                                                                                                         &canceled, noLabel.c_str(), yesLabel.c_str());
    }
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */
