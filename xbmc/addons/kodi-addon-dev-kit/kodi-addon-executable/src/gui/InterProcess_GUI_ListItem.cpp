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

#include "InterProcess_GUI_ListItem.h"
#include "InterProcess.h"
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <cstring>
#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range

using namespace P8PLATFORM;

extern "C"
{

GUIHANDLE CKODIAddon_InterProcess_GUI_ListItem::ListItem_Create(
const std::string&      label,
const std::string&      label2,
const std::string&      iconImage,
const std::string&      thumbnailImage,
const std::string&      path)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_Destroy(GUIHANDLE handle)
{

}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetLabel(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetLabel(GUIHANDLE handle, const std::string &label)
{

}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetLabel2(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetLabel2(GUIHANDLE handle, const std::string &label)
{

}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetIconImage(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetIconImage(GUIHANDLE handle, const std::string &image)
{

}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetOverlayImage(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetOverlayImage(GUIHANDLE handle, unsigned int image, bool bOnOff /*= false */)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetThumbnailImage(GUIHANDLE handle, const std::string &image)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetArt(GUIHANDLE handle, const std::string &type, const std::string &url)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetArtFallback(GUIHANDLE handle, const std::string &from, const std::string &to)
{

}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasArt(GUIHANDLE handle, const std::string &type)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_Select(GUIHANDLE handle, bool bOnOff)
{

}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_IsSelected(GUIHANDLE handle)
{

}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasIcon(GUIHANDLE handle)
{

}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasOverlay(GUIHANDLE handle)
{

}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_IsFileItem(GUIHANDLE handle)
{

}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_IsFolder(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetProperty(GUIHANDLE handle, const std::string &key, const std::string &value)
{

}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetProperty(GUIHANDLE handle, const std::string &key)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_ClearProperty(GUIHANDLE handle, const std::string &key)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_ClearProperties(GUIHANDLE handle)
{

}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasProperties(GUIHANDLE handle)
{

}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasProperty(GUIHANDLE handle, const std::string &key)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPath(GUIHANDLE handle, const std::string &path)
{

}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetPath(GUIHANDLE handle)
{

}

int CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetDuration(GUIHANDLE handle) const
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetSubtitles(GUIHANDLE handle, const std::vector<std::string>& subtitleFiles)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMimeType(GUIHANDLE handle, const std::string& mimetype)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetContentLookup(GUIHANDLE handle, bool enable)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_AddContextMenuItems(GUIHANDLE handle, const std::vector<std::pair<std::string, std::string> >& items, bool replaceItems)
{

}


void CKODIAddon_InterProcess_GUI_ListItem::ListItem_AddStreamInfo(GUIHANDLE handle, const std::string& cType, const std::vector<std::pair<std::string, std::string> >& dictionary)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_BOOL(GUIHANDLE handle, ADDON_MusicInfoTag type, bool value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_INT(GUIHANDLE handle,ADDON_MusicInfoTag type, int value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_UINT(GUIHANDLE handle, ADDON_MusicInfoTag type, unsigned int value)

{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_FLOAT(GUIHANDLE handle,ADDON_MusicInfoTag type, float value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_STRING(GUIHANDLE handle, ADDON_MusicInfoTag type, std::string value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_STRING_LIST(GUIHANDLE handle,ADDON_MusicInfoTag type, std::vector<std::string> values)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_BOOL(GUIHANDLE handle,ADDON_VideoInfoTag type, bool value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_INT(GUIHANDLE handle, ADDON_VideoInfoTag type, int value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_UINT(GUIHANDLE handle,ADDON_VideoInfoTag type, unsigned int value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_FLOAT(GUIHANDLE handle, ADDON_VideoInfoTag type, float value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_STRING(GUIHANDLE handle,ADDON_VideoInfoTag type, std::string value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_STRING_LIST(GUIHANDLE handle, ADDON_VideoInfoTag type, std::vector<std::string> values)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_Resume(GUIHANDLE handle, ADDON_VideoInfoTag_Resume &resume)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_Cast(GUIHANDLE handle, std::vector<ADDON_VideoInfoTag_Cast> &cast)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_BOOL(GUIHANDLE handle, ADDON_PictureInfoTag type, bool value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_INT(GUIHANDLE handle, ADDON_PictureInfoTag type, int value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_UINT(GUIHANDLE handle, ADDON_PictureInfoTag type, unsigned int value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_FLOAT(GUIHANDLE handle, ADDON_PictureInfoTag type, float value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_STRING(GUIHANDLE handle, ADDON_PictureInfoTag type, std::string value)
{

}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_STRING_LIST(GUIHANDLE handle, ADDON_PictureInfoTag type, std::vector<std::string> values)
{

}

}; /* extern "C" */
