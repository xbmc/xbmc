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

#include "definitions.h"
#include "../AddonBase.h"

namespace kodi
{
namespace gui
{

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_CDialogProgress Dialog Progress
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::CDialogProgress }
  /// **Progress dialog shown in center**
  ///
  /// The with \ref DialogProgress.h "#include <kodi/gui/DialogProgress.h>"
  /// given class are basically used to create Kodi's progress dialog with named
  /// text fields.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/gui/DialogProgress.h>
  ///
  /// kodi::gui::CDialogProgress *progress = new kodi::gui::CDialogProgress;
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
  class CDialogProgress
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief Construct a new dialog
    ///
    CDialogProgress()
    {
      using namespace ::kodi::addon;
      m_DialogHandle = CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->new_dialog(CAddonBase::m_interface->toKodi->kodiBase);
      if (!m_DialogHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CDialogProgress can't create window class from Kodi !!!");
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief Destructor
    ///
    ~CDialogProgress()
    {
      using namespace ::kodi::addon;
      if (m_DialogHandle)
        CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->delete_dialog(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief To open the dialog
    ///
    void Open()
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->open(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief Set the heading title of dialog
    ///
    /// @param[in] heading Title string to use
    ///
    void SetHeading(const std::string& heading)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->set_heading(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, heading.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief To set the line text field on dialog from 0 - 2
    ///
    /// @param[in] iLine Line number
    /// @param[in] line Text string
    ///
    void SetLine(unsigned int iLine, const std::string& line)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->set_line(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, iLine, line.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief To enable and show cancel button on dialog
    ///
    /// @param[in] canCancel if true becomes it shown
    ///
    void SetCanCancel(bool canCancel)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->set_can_cancel(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, canCancel);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief To check dialog for clicked cancel button
    ///
    /// @return True if canceled
    ///
    bool IsCanceled() const
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->is_canceled(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief Get the current progress position as percent
    ///
    /// @param[in] percentage Position to use from 0 to 100
    ///
    void SetPercentage(int percentage)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->set_percentage(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, percentage);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief To set the current progress position as percent
    ///
    /// @return Current Position used from 0 to 100
    ///
    int GetPercentage() const
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->get_percentage(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief To show or hide progress bar dialog
    ///
    /// @param[in] onOff If true becomes it shown
    ///
    void ShowProgressBar(bool onOff)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->show_progress_bar(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, onOff);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief Set the maximum position of progress, needed if `SetProgressAdvance(...)` is used
    ///
    /// @param[in] max Biggest usable position to use
    ///
    void SetProgressMax(int max)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->set_progress_max(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, max);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief To increase progress bar by defined step size until reach of maximum position
    ///
    /// @param[in] steps Step size to increase, default is 1
    ///
    void SetProgressAdvance(int steps=1)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->set_progress_advance(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, steps);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CDialogProgress
    /// @brief To check progress was canceled on work
    ///
    /// @return True if aborted
    ///
    bool Abort()
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogProgress->abort(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    }
    //--------------------------------------------------------------------------

  private:
    void* m_DialogHandle;
  };

} /* namespace gui */
} /* namespace kodi */
