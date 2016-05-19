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

#include <vector>

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_GUI_DialogFileBrowser Dialog File Browser
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>File browser dialog</b>
  ///
  /// The functions listed below of the class "DialogFileBrowser" offer
  /// the possibility to select to a file by the user of the add-on.
  ///
  /// It allows all the options that are possible in Kodi itself and offers all
  /// support file types.
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref DialogFileBrowser.hpp "#include <kodi/api3/gui/DialogFileBrowser.hpp>"
  /// be included to enjoy it.
  ///
  namespace DialogFileBrowser
  {
    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogFileBrowser
    /// @brief Directory selection dialog
    ///
    /// @param shares With Shares becomes the available start folders be set.
    /// @param heading Dialog header name
    /// @param path Return value about selected directory
    /// @param bWriteOnly If set only writeable folders are shown.
    /// @return False if selection becomes canceled.
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/gui/DialogFileBrowser.hpp>
    ///
    /// /*
    ///  * Example show directory selection dialog with on 'share' (first value)
    ///  * defined directory types.
    ///  *
    ///  * If this becomes leaved empty and 'directory' is empty goes it to the
    ///  * base path of the hard disk.
    ///  *
    ///  * Also can be with path written to 'directory' before the dialog forced
    ///  * to a start place.
    ///  */
    /// std::string directory;
    /// bool ret = KodiAPI::GUI::DialogFileBrowser::ShowAndGetDirectory("local|network|removable",
    ///                                                    "Test directory selection",
    ///                                                    directory,
    ///                                                    false);
    /// fprintf(stderr, "Selected directory is : %s and was %s\n", directory.c_str(), ret ? "OK" : "Canceled");
    /// ~~~~~~~~~~~~~
    ///
    bool ShowAndGetDirectory(
      const std::string&      shares,
      const std::string&      heading,
      std::string&            path,
      bool                    bWriteOnly = false);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogFileBrowser
    /// @brief File selection dialog
    ///
    /// @param[in] shares               With Shares becomes the available  start
    ///                                 folders be set.
    /// @param[in] mask                 The mask to filter visible  files,  e.g.
    ///                                 ".m3u|.pls|.b4s|.wpl".
    /// @param[in] heading              Dialog header name
    /// @param[in] path                 Return value about selected file
    /// @param[in] useThumbs            If set show thumbs if possible on dialog.
    /// @param[in] useFileDirectories   If set also  packages  (e.g. *.zip)  are
    ///                                 handled as directories.
    /// @return                         False if selection becomes canceled.
    ///
    bool ShowAndGetFile(
      const std::string&      shares,
      const std::string&      mask,
      const std::string&      heading,
      std::string&            path,
      bool                    useThumbs = false,
      bool                    useFileDirectories = false);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogFileBrowser
    /// @brief File selection from a directory
    ///
    /// @param[in] directory            The  directory  name  where  the  dialog
    ///                                 start, possible  are  normal  names  and
    ///                                 kodi's special names.
    /// @param[in] mask                 The mask to filter visible  files,  e.g.
    ///                                 ".m3u|.pls|.b4s|.wpl".
    /// @param[in] heading              Dialog header name
    /// @param[out] path                Return value about selected file
    /// @param[in] useThumbs            If set show thumbs if possible on dialog.
    /// @param[in] useFileDirectories   If set also  packages  (e.g. *.zip)  are
    ///                                 handled as directories.
    /// @param[in] singleList
    /// @return                         False if selection becomes canceled.
    ///
    bool ShowAndGetFileFromDir(
      const std::string&      directory,
      const std::string&      mask,
      const std::string&      heading,
      std::string&            path,
      bool                    useThumbs = false,
      bool                    useFileDirectories = false,
      bool                    singleList = false);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogFileBrowser
    /// @brief File selection dialog to get several in to a list
    ///
    /// @param[out] shares            With Shares becomes  the  available  start
    ///                               folders be set.
    /// @param[in] mask               The mask to  filter  visible  files,  e.g.
    ///                               ".m3u|.pls|.b4s|.wpl".
    /// @param[in] heading            Dialog header name
    /// @param[in] path               Return value about selected files
    /// @param[in] useThumbs          If set show thumbs if possible on dialog.
    /// @param[in] useFileDirectories If  set  also  packages  (e.g. *.zip)  are
    ///                               handled as directories.
    /// @return False if selection becomes canceled.
    ///
    bool ShowAndGetFileList(
      const std::string&      shares,
      const std::string&      mask,
      const std::string&      heading,
      std::vector<std::string> &path,
      bool                    useThumbs = false,
      bool                    useFileDirectories = false);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogFileBrowser
    /// @brief Source selection dialog
    ///
    /// @param[out] path              Return value about selected source
    /// @param[in] allowNetworkShares Allow also access to network
    /// @param[in] additionalShare    With additionalShare becomes the available
    ///                               start folders be set (optional).
    /// @param[in] type
    /// @return                       False if selection becomes canceled.
    ///
    bool ShowAndGetSource(
      std::string&            path,
      bool                    allowNetworkShares,
      const std::string&      additionalShare = "",
      const std::string&      type = "");
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogFileBrowser
    /// @brief Image selection dialog
    ///
    /// @param[in] shares     With Shares becomes the available start folders be
    ///                       set.
    /// @param[in] heading    Dialog header name
    /// @param[out] path      Return value about selected image
    /// @return               False if selection becomes canceled.
    ///
    bool ShowAndGetImage(
      const std::string&      shares,
      const std::string&      heading,
      std::string&            path);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogFileBrowser
    /// @brief Image selection dialog to get several in to a list
    ///
    /// @param[in] shares       With Shares becomes the available  start folders
    ///                         be set.
    /// @param[in] heading      Dialog header name
    /// @param[out] path        Return value about selected images
    /// @return                 False if selection becomes canceled.
    ///
    bool ShowAndGetImageList(
      const std::string&      shares,
      const std::string&      heading,
      std::vector<std::string> &path);
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
