#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_GUI_ListItem
  {
    GUIHANDLE ListItem_Create(
    const std::string&      label,
    const std::string&      label2,
    const std::string&      iconImage,
    const std::string&      thumbnailImage,
    const std::string&      path);
    void ListItem_Destroy(GUIHANDLE handle);
    std::string ListItem_GetLabel(GUIHANDLE handle);
    void ListItem_SetLabel(GUIHANDLE handle, const std::string &label);
    std::string ListItem_GetLabel2(GUIHANDLE handle);
    void ListItem_SetLabel2(GUIHANDLE handle, const std::string &label);
    std::string ListItem_GetIconImage(GUIHANDLE handle);
    void ListItem_SetIconImage(GUIHANDLE handle, const std::string &image);
    std::string ListItem_GetOverlayImage(GUIHANDLE handle);
    void ListItem_SetOverlayImage(GUIHANDLE handle, unsigned int image, bool bOnOff /*= false */);
    void ListItem_SetThumbnailImage(GUIHANDLE handle, const std::string &image);
    void ListItem_SetArt(GUIHANDLE handle, const std::string &type, const std::string &url);
    void ListItem_SetArtFallback(GUIHANDLE handle, const std::string &from, const std::string &to);
    bool ListItem_HasArt(GUIHANDLE handle, const std::string &type);
    void ListItem_Select(GUIHANDLE handle, bool bOnOff);
    bool ListItem_IsSelected(GUIHANDLE handle);
    bool ListItem_HasIcon(GUIHANDLE handle);
    bool ListItem_HasOverlay(GUIHANDLE handle);
    bool ListItem_IsFileItem(GUIHANDLE handle);
    bool ListItem_IsFolder(GUIHANDLE handle);
    void ListItem_SetProperty(GUIHANDLE handle, const std::string &key, const std::string &value);
    std::string ListItem_GetProperty(GUIHANDLE handle, const std::string &key);
    void ListItem_ClearProperty(GUIHANDLE handle, const std::string &key);
    void ListItem_ClearProperties(GUIHANDLE handle);
    bool ListItem_HasProperties(GUIHANDLE handle);
    bool ListItem_HasProperty(GUIHANDLE handle, const std::string &key);
    void ListItem_SetPath(GUIHANDLE handle, const std::string &path);
    std::string ListItem_GetPath(GUIHANDLE handle);
    int ListItem_GetDuration(GUIHANDLE handle) const;
    void ListItem_SetSubtitles(GUIHANDLE handle, const std::vector<std::string>& subtitleFiles);
    void ListItem_SetMimeType(GUIHANDLE handle, const std::string& mimetype);
    void ListItem_SetContentLookup(GUIHANDLE handle, bool enable);
    void ListItem_AddContextMenuItems(GUIHANDLE handle, const std::vector<std::pair<std::string, std::string> >& items, bool replaceItems);
    void ListItem_AddStreamInfo(GUIHANDLE handle, const std::string& cType, const std::vector<std::pair<std::string, std::string> >& dictionary);
    void ListItem_SetMusicInfo_BOOL(GUIHANDLE handle, ADDON_MusicInfoTag type, bool value);
    void ListItem_SetMusicInfo_INT(GUIHANDLE handle, ADDON_MusicInfoTag type, int value);
    void ListItem_SetMusicInfo_UINT(GUIHANDLE handle, ADDON_MusicInfoTag type, unsigned int value);
    void ListItem_SetMusicInfo_FLOAT(GUIHANDLE handle,ADDON_MusicInfoTag type, float value);
    void ListItem_SetMusicInfo_STRING(GUIHANDLE handle, ADDON_MusicInfoTag type, std::string value);
    void ListItem_SetMusicInfo_STRING_LIST(GUIHANDLE handle,ADDON_MusicInfoTag type, std::vector<std::string> values);
    void ListItem_SetVideoInfo_BOOL(GUIHANDLE handle, ADDON_VideoInfoTag type, bool value);
    void ListItem_SetVideoInfo_INT(GUIHANDLE handle, ADDON_VideoInfoTag type, int value);
    void ListItem_SetVideoInfo_UINT(GUIHANDLE handle,ADDON_VideoInfoTag type, unsigned int value);
    void ListItem_SetVideoInfo_FLOAT(GUIHANDLE handle, ADDON_VideoInfoTag type, float value);
    void ListItem_SetVideoInfo_STRING(GUIHANDLE handle, ADDON_VideoInfoTag type, std::string value);
    void ListItem_SetVideoInfo_STRING_LIST(GUIHANDLE handle, ADDON_VideoInfoTag type, std::vector<std::string> values);
    void ListItem_SetVideoInfo_Resume(GUIHANDLE handle, ADDON_VideoInfoTag_Resume &resume);
    void ListItem_SetVideoInfo_Cast(GUIHANDLE handle, std::vector<ADDON_VideoInfoTag_Cast> &cast);
    void ListItem_SetPictureInfo_BOOL(GUIHANDLE handle, ADDON_PictureInfoTag type, bool value);
    void ListItem_SetPictureInfo_INT(GUIHANDLE handle, ADDON_PictureInfoTag type, int value);
    void ListItem_SetPictureInfo_UINT(GUIHANDLE handle, ADDON_PictureInfoTag type, unsigned int value);
    void ListItem_SetPictureInfo_FLOAT(GUIHANDLE handle, ADDON_PictureInfoTag type, float value);
    void ListItem_SetPictureInfo_STRING(GUIHANDLE handle, ADDON_PictureInfoTag type, std::string value);
    void ListItem_SetPictureInfo_STRING_LIST(GUIHANDLE handle, ADDON_PictureInfoTag type, std::vector<std::string> values);

  };

}; /* extern "C" */
