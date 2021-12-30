/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/gui/dialogs/progress.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace dialogs
{

//==============================================================================
/// @defgroup cpp_kodi_gui_dialogs_CProgress Dialog Progress
/// @ingroup cpp_kodi_gui_dialogs
/// @brief @cpp_class{ kodi::gui::dialogs::CProgress }
/// **Progress dialog shown in center**\n
/// The with @ref Progress.h "#include <kodi/gui/dialogs/Progress.h>"
/// given class are basically used to create Kodi's progress dialog with named
/// text fields.
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/Progress.h>
///
/// kodi::gui::dialogs::CProgress *progress = new kodi::gui::dialogs::CProgress;
/// progress->SetHeading("Test progress");
/// progress->SetLine(1, "line 1");
/// progress->SetLine(2, "line 2");
/// progress->SetLine(3, "line 3");
/// progress->SetCanCancel(true);
/// progress->ShowProgressBar(true);
/// progress->Open();
/// for (unsigned int i = 0; i < 100; i += 10)
/// {
///   progress->SetPercentage(i);
///   sleep(1);
/// }
/// delete progress;
/// ~~~~~~~~~~~~~
///
class ATTR_DLL_LOCAL CProgress
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief Construct a new dialog
  ///
  CProgress()
  {
    using namespace ::kodi::addon;
    m_DialogHandle = CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->new_dialog(
        CPrivateBase::m_interface->toKodi->kodiBase);
    if (!m_DialogHandle)
      kodi::Log(ADDON_LOG_FATAL,
                "kodi::gui::dialogs::CProgress can't create window class from Kodi !!!");
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief Destructor
  ///
  ~CProgress()
  {
    using namespace ::kodi::addon;
    if (m_DialogHandle)
      CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->delete_dialog(
          CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief To open the dialog
  ///
  void Open()
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->open(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief Set the heading title of dialog
  ///
  /// @param[in] heading Title string to use
  ///
  void SetHeading(const std::string& heading)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->set_heading(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, heading.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief To set the line text field on dialog from 0 - 2
  ///
  /// @param[in] iLine Line number
  /// @param[in] line Text string
  ///
  void SetLine(unsigned int iLine, const std::string& line)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->set_line(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, iLine, line.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief To enable and show cancel button on dialog
  ///
  /// @param[in] canCancel if true becomes it shown
  ///
  void SetCanCancel(bool canCancel)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->set_can_cancel(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, canCancel);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief To check dialog for clicked cancel button
  ///
  /// @return True if canceled
  ///
  bool IsCanceled() const
  {
    using namespace ::kodi::addon;
    return CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->is_canceled(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief Get the current progress position as percent
  ///
  /// @param[in] percentage Position to use from 0 to 100
  ///
  void SetPercentage(int percentage)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->set_percentage(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, percentage);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief To set the current progress position as percent
  ///
  /// @return Current Position used from 0 to 100
  ///
  int GetPercentage() const
  {
    using namespace ::kodi::addon;
    return CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->get_percentage(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief To show or hide progress bar dialog
  ///
  /// @param[in] onOff If true becomes it shown
  ///
  void ShowProgressBar(bool onOff)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->show_progress_bar(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, onOff);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief Set the maximum position of progress, needed if `SetProgressAdvance(...)` is used
  ///
  /// @param[in] max Biggest usable position to use
  ///
  void SetProgressMax(int max)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->set_progress_max(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, max);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief To increase progress bar by defined step size until reach of maximum position
  ///
  /// @param[in] steps Step size to increase, default is 1
  ///
  void SetProgressAdvance(int steps = 1)
  {
    using namespace ::kodi::addon;
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->set_progress_advance(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle, steps);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_dialogs_CProgress
  /// @brief To check progress was canceled on work
  ///
  /// @return True if aborted
  ///
  bool Abort()
  {
    using namespace ::kodi::addon;
    return CPrivateBase::m_interface->toKodi->kodi_gui->dialogProgress->abort(
        CPrivateBase::m_interface->toKodi->kodiBase, m_DialogHandle);
  }
  //----------------------------------------------------------------------------

private:
  KODI_GUI_HANDLE m_DialogHandle;
};

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
