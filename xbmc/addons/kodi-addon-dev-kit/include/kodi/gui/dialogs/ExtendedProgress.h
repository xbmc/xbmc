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
  /// \defgroup cpp_kodi_gui_dialogs_CExtendedProgress Dialog Extended Progress
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::dialogs::ExtendedProgress }
  /// **Progress dialog shown for background work**
  ///
  /// The with \ref ExtendedProgress.h "#include <kodi/gui/dialogs/ExtendedProgress.h>"
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
  class CExtendedProgress
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// Construct a new dialog
    ///
    /// @param[in] title  Title string
    ///
    CExtendedProgress(const std::string& title = "")
    {
      using namespace ::kodi::addon;
      m_DialogHandle = CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->new_dialog(CAddonBase::m_interface->toKodi->kodiBase, title.c_str());
      if (!m_DialogHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CDialogExtendedProgress can't create window class from Kodi !!!");
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// Destructor
    ///
    ~CExtendedProgress()
    {
      using namespace ::kodi::addon;
      if (m_DialogHandle)
        CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->delete_dialog(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// @brief Get the used title
    ///
    /// @return Title string
    ///
    std::string Title() const
    {
      using namespace ::kodi::addon;
      std::string text;
      char* strMsg = CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->get_title(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
      if (strMsg != nullptr)
      {
        if (std::strlen(strMsg))
          text = strMsg;
        CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, strMsg);
      }
      return text;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// @brief To set the title of dialog
    ///
    /// @param[in] title     Title string
    ///
    void SetTitle(const std::string& title)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->set_title(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, title.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// @brief Get the used text information string
    ///
    /// @return Text string
    ///
    std::string Text() const
    {
      using namespace ::kodi::addon;
      std::string text;
      char* strMsg = CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->get_text(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
      if (strMsg != nullptr)
      {
        if (std::strlen(strMsg))
          text = strMsg;
        CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, strMsg);
      }
      return text;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// @brief To set the used text information string
    ///
    /// @param[in] text         information text to set
    ///
    void SetText(const std::string& text)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->set_text(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, text.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// @brief To ask dialog is finished
    ///
    /// @return True if on end
    ///
    bool IsFinished() const
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->is_finished(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// @brief Mark progress finished
    ///
    void MarkFinished()
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->mark_finished(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// @brief Get the current progress position as percent
    ///
    /// @return Position
    ///
    float Percentage() const
    {
      using namespace ::kodi::addon;
      return CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->get_percentage(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// @brief To set the current progress position as percent
    ///
    /// @param[in] percentage   Position to use from 0.0 to 100.0
    ///
    void SetPercentage(float percentage)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->set_percentage(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, percentage);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_CExtendedProgress
    /// @brief To set progress position with predefined places
    ///
    /// @param[in] currentItem    Place position to use
    /// @param[in] itemCount      Amount of used places
    ///
    void SetProgress(int currentItem, int itemCount)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogExtendedProgress->set_progress(CAddonBase::m_interface->toKodi->kodiBase, m_DialogHandle, currentItem, itemCount);
    }
    //--------------------------------------------------------------------------

  private:
    void* m_DialogHandle;
  };

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */
