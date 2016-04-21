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
  /// \defgroup CPP_KodiAPI_GUI_CDialogProgress Dialog Progress
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>Progress dialog shown in center</b>
  ///
  /// The with \ref DialogProgress.hpp "#include <kodi/api2/gui/DialogProgress.hpp>"
  /// given class are basically used to create Kodi's progress dialog with named
  /// text fields.
  ///
  /// \image html help.GUIDialogProgress.png
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/addon/DialogProgress.h>
  ///
  /// KodiAPI::GUI::CDialogProgress *progress = new KodiAPI::GUI::CDialogProgress;
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
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief Construct a new dialog
    ///
    CDialogProgress();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief Destructor
    ///
    ~CDialogProgress();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief To open the dialog
    ///
    void Open();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief Set the heading title of dialog
    ///
    /// @param[in] heading Title string to use
    ///
    void SetHeading(const std::string& heading);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief To set the line text field on dialog from 0 - 2
    ///
    /// @param[in] iLine Line number
    /// @param[in] line Text string
    ///
    void SetLine(unsigned int iLine, const std::string& line);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief To enable and show cancel button on dialog
    ///
    /// @param[in] bCanCancel if true becomes it shown
    ///
    void SetCanCancel(bool bCanCancel);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief To check dialog for clicked cancel button
    ///
    /// @return True if canceled
    ///
    bool IsCanceled() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief Get the current progress position as percent
    ///
    /// @param[in] iPercentage Position to use from 0.0 to 100.0
    ///
    void SetPercentage(int iPercentage);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief To set the current progress position as percent
    ///
    /// @return Current Position used from 0.0 to 100.0
    ///
    int GetPercentage() const;
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief To show or hide progress bar dialog
    ///
    /// @param[in] bOnOff If true becomes it shown
    ///
    void ShowProgressBar(bool bOnOff);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief Set the maximum position of progress, needed if `SetProgressAdvance(...)` is used
    ///
    /// @param[in] iMax Biggest usable position to use
    ///
    void SetProgressMax(int iMax);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief To increase progress bar by defined step size until reach of maximum position
    ///
    /// @param[in] nSteps Step size to increase, default is 1
    ///
    void SetProgressAdvance(int nSteps=1);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_CDialogProgress
    /// @brief To check progress was canceled on work
    ///
    /// @return True if aborted
    ///
    bool Abort();
    //--------------------------------------------------------------------------

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_GUI_PROGRESS_DIALOG;
  #endif
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
