/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/gui/dialogs/extended_progress.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace dialogs
{

//==============================================================================
/// @defgroup cpp_kodi_gui_dialogs_CExtendedProgress Dialog Extended Progress
/// @ingroup cpp_kodi_gui_dialogs
/// @brief @cpp_class{ kodi::gui::dialogs::ExtendedProgress }
/// **Progress dialog shown for background work**
///
/// The with @ref ExtendedProgress.h "#include <kodi/gui/dialogs/ExtendedProgress.h>"
/// given class are basically used to create Kodi's extended progress.
///
///
/// --------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/ExtendedProgress.h>
///
/// kodi::gui::dialogs::CExtendedProgress *ext_progress = new kodi::gui::dialogs::CExtendedProgress("Test Extended progress");
/// ext_progress->SetText("Test progress");
/// for (unsigned int i = 0; i < 50; i += 10)
/// {
///   ext_progress->SetProgress(i, 100);
///   sleep(1);
/// }
///
/// ext_progress->SetTitle("Test Extended progress - Second round");
/// ext_progress->SetText("Test progress - Step 2");
///
/// for (unsigned int i = 50; i < 100; i += 10)
/// {
///   ext_progress->SetProgress(i, 100);
///   sleep(1);
/// }
/// delete ext_progress;
/// ~~~~~~~~~~~~~
///
class ATTR_DLL_LOCAL CExtendedProgress
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// Construct a new dialog.
  ///
  /// @param[in] title [opt] Title string
  ///
  explicit CExtendedProgress(const std::string& title = "")
  {
    using namespace ::kodi::addon;
    m_DialogHandle =
        CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->new_dialog(
            CPrivateBase::m_interface->toKodi->kodiBase, title.c_str());
    if (!m_DialogHandle)
      kodi::Log(ADDON_LOG_FATAL,
                "kodi::gui::CDialogExtendedProgress can't create window class from Kodi !!!");
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// Destructor.
  ///
  ~CExtendedProgress()
  {
    using namespace ::kodi::addon;
    if (m_DialogHandle)
      CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->delete_dialog(
          CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// @brief Get the used title.
  ///
  /// @return Title string
  ///
  std::string Title() const
  {
    using namespace ::kodi::addon;
    std::string text;
    char* strMsg = CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->get_title(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        text = strMsg;
      CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                     strMsg);
    }
    return text;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// @brief To set the title of dialog.
  ///
  /// @param[in] title Title string
  ///
  void SetTitle(const std::string& title)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->set_title(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, title.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// @brief Get the used text information string.
  ///
  /// @return Text string
  ///
  std::string Text() const
  {
    using namespace ::kodi::addon;
    std::string text;
    char* strMsg = CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->get_text(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        text = strMsg;
      CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                     strMsg);
    }
    return text;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// @brief To set the used text information string.
  ///
  /// @param[in] text Information text to set
  ///
  void SetText(const std::string& text)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->set_text(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, text.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// @brief To ask dialog is finished.
  ///
  /// @return True if on end
  ///
  bool IsFinished() const
  {
    using namespace ::kodi::addon;
    return CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->is_finished(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// @brief Mark progress finished.
  ///
  void MarkFinished()
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->mark_finished(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// @brief Get the current progress position as percent.
  ///
  /// @return Position
  ///
  float Percentage() const
  {
    using namespace ::kodi::addon;
    return CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->get_percentage(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// @brief To set the current progress position as percent.
  ///
  /// @param[in] percentage Position to use from 0.0 to 100.0
  ///
  void SetPercentage(float percentage)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->set_percentage(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, percentage);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CExtendedProgress
  /// @brief To set progress position with predefined places.
  ///
  /// @param[in] currentItem Place position to use
  /// @param[in] itemCount Amount of used places
  ///
  void SetProgress(int currentItem, int itemCount)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->set_progress(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, currentItem, itemCount);
  }
  //----------------------------------------------------------------------------

private:
  KODI_GUI_HANDLE m_DialogHandle;
};

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
