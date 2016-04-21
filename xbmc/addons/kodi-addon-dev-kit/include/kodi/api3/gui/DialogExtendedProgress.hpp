#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_GUI_CDialogExtendedProgress Dialog Extended Progress
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>Progress dialog shown for background work</b>
  ///
  /// The with \ref DialogExtendedProgress.hpp "#include <kodi/api3/addon/DialogExtendedProgress.hpp>"
  /// given class are basically used to create Kodi's extended progress.
  ///
  /// \image html help.GUIDialogExtendedProgress.png
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api3/addon/DialogExtendedProgress.h>
  ///
  /// KodiAPI::GUI::CDialogExtendedProgress *ext_progress = new KodiAPI::GUI::CDialogExtendedProgress("Test Extended progress");
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
  class CDialogExtendedProgress
  {
  public:
    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// Construct a new dialog
    ///
    /// @param[in] title  Title string
    ///
    CDialogExtendedProgress(const std::string& title = "");
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// Destructor
    ///
    ~CDialogExtendedProgress();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// @brief Get the used title
    ///
    /// @return Title string
    ///
    std::string Title() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// @brief To set the title of dialog
    ///
    /// @param[in] strTitle     Title string
    ///
    void SetTitle(const std::string& strTitle);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// @brief Get the used text information string
    ///
    /// @return Text string
    ///
    std::string Text() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// @brief To set the used text information string
    ///
    /// @param[in] strText    information text to set
    ///
    void SetText(const std::string& strText);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// @brief To ask dialog is finished
    ///
    /// @return True if on end
    ///
    bool IsFinished() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// @brief Mark progress finished
    ///
    void MarkFinished();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// @brief Get the current progress position as percent
    ///
    /// @return Position
    ///
    float Percentage() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// @brief To set the current progress position as percent
    ///
    /// @param[in] fPercentage    Position to use from 0.0 to 100.0
    ///
    void SetPercentage(float fPercentage);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogExtendedProgress
    /// @brief To set progress position with predefined places
    ///
    /// @param[in] currentItem    Place position to use
    /// @param[in] itemCount      Amount of used places
    ///
    void SetProgress(int currentItem, int itemCount);
    //--------------------------------------------------------------------------

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_GUI_EXTENDED_PROGRESS_DIALOG;
  #endif
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
