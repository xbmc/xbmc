/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/gui/dialogs/filebrowser.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace dialogs
{

//==============================================================================
/// @defgroup cpp_kodi_gui_dialogs_FileBrowser Dialog File Browser
/// @ingroup cpp_kodi_gui_dialogs
/// @brief @cpp_namespace{ kodi::gui::dialogs::FileBrowser }
/// **File browser dialog**\n
/// The functions listed below of the class "FileBrowser" offer the possibility
/// to select to a file by the user of the add-on.
///
/// It allows all the options that are possible in Kodi itself and offers all
/// support file types.
///
/// It has the header @ref FileBrowser.h "#include <kodi/gui/dialogs/FileBrowser.h>"
/// be included to enjoy it.
///
namespace FileBrowser
{
//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_FileBrowser
/// @brief Directory selection dialog.
///
/// @param[in] shares With Shares becomes the available start folders be set
/// @param[in] heading Dialog header name
/// @param[in,out] path As in the path to start and return value about
///                     selected directory
/// @param[in] writeOnly [opt] If set only writeable folders are shown
/// @return False if selection becomes canceled
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/FileBrowser.h>
///
/// // Example show directory selection dialog with on 'share' (first value)
/// // defined directory types.
/// //
/// // If this becomes leaved empty and 'directory' is empty goes it to the
/// // base path of the hard disk.
/// //
/// // Also can be with path written to 'directory' before the dialog forced
/// // to a start place.
/// std::string directory;
/// bool ret = kodi::gui::dialogs::FileBrowser::ShowAndGetDirectory("local|network|removable",
///                                                    "Test directory selection",
///                                                    directory,
///                                                    false);
/// fprintf(stderr, "Selected directory is : %s and was %s\n", directory.c_str(), ret ? "OK" : "Canceled");
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL ShowAndGetDirectory(const std::string& shares,
                                               const std::string& heading,
                                               std::string& path,
                                               bool writeOnly = false)
{
  using namespace ::kodi::addon;
  char* retString = nullptr;
  bool ret = CPrivateBase::m_interface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_directory(
      CPrivateBase::m_interface->toKodi->kodiBase, shares.c_str(), heading.c_str(), path.c_str(),
      &retString, writeOnly);
  if (retString != nullptr)
  {
    if (std::strlen(retString))
      path = retString;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   retString);
  }
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_FileBrowser
/// @brief File selection dialog.
///
/// @param[in] shares With Shares becomes the available start folders be set.
/// @param[in] mask The mask to filter visible files, e.g. ".m3u|.pls|.b4s|.wpl"
/// @param[in] heading Dialog header name
/// @param[in,out] path As in the path to start and Return value about selected
///                     file
/// @param[in] useThumbs [opt] If set show thumbs if possible on dialog
/// @param[in] useFileDirectories [opt] If set also packages (e.g. *.zip) are
///                               handled as directories.
/// @return False if selection becomes canceled
///
inline bool ATTR_DLL_LOCAL ShowAndGetFile(const std::string& shares,
                                          const std::string& mask,
                                          const std::string& heading,
                                          std::string& path,
                                          bool useThumbs = false,
                                          bool useFileDirectories = false)
{
  using namespace ::kodi::addon;
  char* retString = nullptr;
  bool ret = CPrivateBase::m_interface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_file(
      CPrivateBase::m_interface->toKodi->kodiBase, shares.c_str(), mask.c_str(), heading.c_str(),
      path.c_str(), &retString, useThumbs, useFileDirectories);
  if (retString != nullptr)
  {
    if (std::strlen(retString))
      path = retString;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   retString);
  }
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_FileBrowser
/// @brief File selection from a directory.
///
/// @param[in] directory The directory name where the dialog start, possible are
///                      normal names and kodi's special names
/// @param[in] mask The mask to filter visible files, e.g. ".m3u|.pls|.b4s|.wpl"
/// @param[in] heading Dialog header name
/// @param[in,out] path As in the path to start and Return value about selected
///                     file
/// @param[in] useThumbs [opt] If set show thumbs if possible on dialog
/// @param[in] useFileDirectories [opt] If set also  packages (e.g. *.zip) are
///                               handled as directories
/// @param[in] singleList [opt]
/// @return False if selection becomes canceled
///
inline bool ATTR_DLL_LOCAL ShowAndGetFileFromDir(const std::string& directory,
                                                 const std::string& mask,
                                                 const std::string& heading,
                                                 std::string& path,
                                                 bool useThumbs = false,
                                                 bool useFileDirectories = false,
                                                 bool singleList = false)
{
  using namespace ::kodi::addon;
  char* retString = nullptr;
  bool ret =
      CPrivateBase::m_interface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_file_from_dir(
          CPrivateBase::m_interface->toKodi->kodiBase, directory.c_str(), mask.c_str(),
          heading.c_str(), path.c_str(), &retString, useThumbs, useFileDirectories, singleList);
  if (retString != nullptr)
  {
    if (std::strlen(retString))
      path = retString;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   retString);
  }
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_FileBrowser
/// @brief File selection dialog to get several in to a list.
///
/// @param[in] shares With Shares becomes the available start folders be set.
/// @param[in] mask The mask to filter visible files, e.g. ".m3u|.pls|.b4s|.wpl"
/// @param[in] heading Dialog header name
/// @param[out] fileList Return value about selected files
/// @param[in] useThumbs [opt] If set show thumbs if possible on dialog.
/// @param[in] useFileDirectories [opt] If set also packages (e.g. *.zip) are
///                               handled as directories.
/// @return False if selection becomes canceled.
///
inline bool ATTR_DLL_LOCAL ShowAndGetFileList(const std::string& shares,
                                              const std::string& mask,
                                              const std::string& heading,
                                              std::vector<std::string>& fileList,
                                              bool useThumbs = false,
                                              bool useFileDirectories = false)
{
  using namespace ::kodi::addon;
  char** list = nullptr;
  unsigned int listSize = 0;
  bool ret = CPrivateBase::m_interface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_file_list(
      CPrivateBase::m_interface->toKodi->kodiBase, shares.c_str(), mask.c_str(), heading.c_str(),
      &list, &listSize, useThumbs, useFileDirectories);
  if (ret)
  {
    for (unsigned int i = 0; i < listSize; ++i)
      fileList.emplace_back(list[i]);
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogFileBrowser->clear_file_list(
        CPrivateBase::m_interface->toKodi->kodiBase, &list, listSize);
  }
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_FileBrowser
/// @brief Source selection dialog.
///
/// @param[in,out] path As in the path to start and Return value about selected
///                     source
/// @param[in] allowNetworkShares Allow also access to network
/// @param[in] additionalShare [opt] With additionalShare becomes the available
///                            start folders be set.
/// @param[in] type [opt]
/// @return False if selection becomes canceled
///
inline bool ATTR_DLL_LOCAL ShowAndGetSource(std::string& path,
                                            bool allowNetworkShares,
                                            const std::string& additionalShare = "",
                                            const std::string& type = "")
{
  using namespace ::kodi::addon;
  char* retString = nullptr;
  bool ret = CPrivateBase::m_interface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_source(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str(), &retString, allowNetworkShares,
      additionalShare.c_str(), type.c_str());
  if (retString != nullptr)
  {
    if (std::strlen(retString))
      path = retString;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   retString);
  }
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_FileBrowser
/// @brief Image selection dialog.
///
/// @param[in] shares With Shares becomes the available start folders be set
/// @param[in] heading Dialog header name
/// @param[out] path Return value about selected image
/// @return False if selection becomes canceled
///
inline bool ATTR_DLL_LOCAL ShowAndGetImage(const std::string& shares,
                                           const std::string& heading,
                                           std::string& path)
{
  using namespace ::kodi::addon;
  char* retString = nullptr;
  bool ret = CPrivateBase::m_interface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_image(
      CPrivateBase::m_interface->toKodi->kodiBase, shares.c_str(), heading.c_str(), path.c_str(),
      &retString);
  if (retString != nullptr)
  {
    if (std::strlen(retString))
      path = retString;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   retString);
  }
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_FileBrowser
/// @brief Image selection dialog to get several in to a list.
///
/// @param[in] shares With Shares becomes the available start folders be set
/// @param[in] heading Dialog header name
/// @param[out] file_list Return value about selected images
/// @return False if selection becomes canceled
///
inline bool ATTR_DLL_LOCAL ShowAndGetImageList(const std::string& shares,
                                               const std::string& heading,
                                               std::vector<std::string>& file_list)
{
  using namespace ::kodi::addon;
  char** list = nullptr;
  unsigned int listSize = 0;
  bool ret =
      CPrivateBase::m_interface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_image_list(
          CPrivateBase::m_interface->toKodi->kodiBase, shares.c_str(), heading.c_str(), &list,
          &listSize);
  if (ret)
  {
    for (unsigned int i = 0; i < listSize; ++i)
      file_list.emplace_back(list[i]);
    CPrivateBase::m_interface->toKodi->kodi_gui->dialogFileBrowser->clear_file_list(
        CPrivateBase::m_interface->toKodi->kodiBase, &list, listSize);
  }
  return ret;
}
//------------------------------------------------------------------------------
}; // namespace FileBrowser

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
