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

#include "InterProcess.h"
#include "kodi/api2/gui/ListItem.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CListItem::CListItem(
    const std::string&      label,
    const std::string&      label2,
    const std::string&      iconImage,
    const std::string&      thumbnailImage,
    const std::string&      path)
  {
    m_ListItemHandle = g_interProcess.ListItem_Create(label,
                                             label2, iconImage,
                                             thumbnailImage, path);
  }

  CListItem::CListItem(GUIHANDLE listItemHandle)
  {
    m_ListItemHandle = listItemHandle;
  }

  CListItem::~CListItem()
  {
    g_interProcess.ListItem_Destroy(m_ListItemHandle);
  }

  std::string CListItem::GetLabel()
  {
    return g_interProcess.ListItem_GetLabel(m_ListItemHandle);
  }

  void CListItem::SetLabel(const std::string &label)
  {
    g_interProcess.ListItem_SetLabel(m_ListItemHandle, label);
  }
  
  std::string CListItem::GetLabel2()
  {
    return g_interProcess.ListItem_GetLabel2(m_ListItemHandle);
  }

  void CListItem::SetLabel2(const std::string &label)
  {
    g_interProcess.ListItem_SetLabel2(m_ListItemHandle, label);
  }

  std::string CListItem::GetIconImage()
  {
    return g_interProcess.ListItem_GetIconImage(m_ListItemHandle);
  }

  void CListItem::SetIconImage(const std::string &image)
  {
    g_interProcess.ListItem_SetIconImage(m_ListItemHandle, image);
  }

  std::string CListItem::GetOverlayImage()
  {
    return g_interProcess.ListItem_GetOverlayImage(m_ListItemHandle);
  }

  void CListItem::SetOverlayImage(unsigned int image, bool bOnOff /*= false */)
  {
    g_interProcess.ListItem_SetOverlayImage(m_ListItemHandle, image, bOnOff);
  }

  void CListItem::SetThumbnailImage(const std::string &image)
  {
    g_interProcess.ListItem_SetThumbnailImage(m_ListItemHandle, image);
  }

  void CListItem::SetArt(const std::string &type, const std::string &url)
  {
    g_interProcess.ListItem_SetArt(m_ListItemHandle, type, url);
  }

  void CListItem::SetArtFallback(const std::string &from, const std::string &to)
  {
    g_interProcess.ListItem_SetArtFallback(m_ListItemHandle, from, to);
  }

  bool CListItem::HasArt(const std::string &type)
  {
    return g_interProcess.ListItem_HasArt(m_ListItemHandle, type);
  }

  void CListItem::Select(bool bOnOff)
  {
    g_interProcess.ListItem_Select(m_ListItemHandle, bOnOff);
  }

  bool CListItem::IsSelected()
  {
    return g_interProcess.ListItem_IsSelected(m_ListItemHandle);
  }

  bool CListItem::HasIcon()
  {
    return g_interProcess.ListItem_HasIcon(m_ListItemHandle);
  }

  bool CListItem::HasOverlay()
  {
    return g_interProcess.ListItem_HasOverlay(m_ListItemHandle);
  }

  bool CListItem::IsFileItem()
  {
    return g_interProcess.ListItem_IsFileItem(m_ListItemHandle);
  }

  bool CListItem::IsFolder()
  {
    return g_interProcess.ListItem_IsFolder(m_ListItemHandle);
  }

  void CListItem::SetProperty(const std::string &key, const std::string &value)
  {
    g_interProcess.ListItem_SetProperty(m_ListItemHandle, key, value);
  }

  std::string CListItem::GetProperty(const std::string &key)
  {
    return g_interProcess.ListItem_GetProperty(m_ListItemHandle, key);
  }

  void CListItem::ClearProperty(const std::string &key)
  {
    g_interProcess.ListItem_ClearProperty(m_ListItemHandle, key);
  }

  void CListItem::ClearProperties()
  {
    g_interProcess.ListItem_ClearProperties(m_ListItemHandle);
  }

  bool CListItem::HasProperties()
  {
    return g_interProcess.ListItem_HasProperties(m_ListItemHandle);
  }

  bool CListItem::HasProperty(const std::string &key)
  {
    return g_interProcess.ListItem_HasProperty(m_ListItemHandle, key);
  }

  void CListItem::SetPath(const std::string &path)
  {
    g_interProcess.ListItem_SetPath(m_ListItemHandle, path);
  }

  std::string CListItem::GetPath()
  {
    return g_interProcess.ListItem_GetPath(m_ListItemHandle);
  }

  int CListItem::GetDuration() const
  {
    return g_interProcess.ListItem_GetDuration(m_ListItemHandle);
  }

  void CListItem::SetSubtitles(const std::vector<std::string>& subtitleFiles)
  {
    g_interProcess.ListItem_SetSubtitles(m_ListItemHandle, subtitleFiles);
  }

  void CListItem::SetMimeType(const std::string& mimetype)
  {
    g_interProcess.ListItem_SetMimeType(m_ListItemHandle, mimetype);
  }

  void CListItem::SetContentLookup(bool enable)
  {
    g_interProcess.ListItem_SetContentLookup(m_ListItemHandle, enable);
  }

  void CListItem::AddContextMenuItems(const std::vector<std::pair<std::string, std::string> >& items, bool replaceItems)
  {
    g_interProcess.ListItem_AddContextMenuItems(m_ListItemHandle, items, replaceItems);
  }

  void CListItem::AddStreamInfo(const std::string& cType, const std::vector<std::pair<std::string, std::string> >& dictionary)
  {
    g_interProcess.ListItem_AddStreamInfo(m_ListItemHandle, cType, dictionary);
  }   
    
  void CListItem::SetMusicInfo_BOOL(ADDON_MusicInfoTag type, bool value)
  {   
    if (type != 0)
      g_interProcess.ListItem_SetMusicInfo_BOOL(m_ListItemHandle, type, value);
  }   
    
  void CListItem::SetMusicInfo_INT(ADDON_MusicInfoTag type, int value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetMusicInfo_INT(m_ListItemHandle, type, value);
  } 
    
  void CListItem::SetMusicInfo_UINT(ADDON_MusicInfoTag type, unsigned int value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetMusicInfo_UINT(m_ListItemHandle, type, value);
  } 
    
  void CListItem::SetMusicInfo_FLOAT(ADDON_MusicInfoTag type, float value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetMusicInfo_FLOAT(m_ListItemHandle, type, value);
  } 
    
  void CListItem::SetMusicInfo_STRING(ADDON_MusicInfoTag type, std::string value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetMusicInfo_STRING(m_ListItemHandle, type, value);
  } 
    
  void CListItem::SetMusicInfo_STRING_LIST(ADDON_MusicInfoTag type, std::vector<std::string> values)
  {
    g_interProcess.ListItem_SetMusicInfo_STRING_LIST(m_ListItemHandle, type, values);
  } 

  void CListItem::SetVideoInfo_BOOL(ADDON_VideoInfoTag type, bool value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetVideoInfo_BOOL(m_ListItemHandle, type, value);
  }

  void CListItem::SetVideoInfo_INT(ADDON_VideoInfoTag type, int value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetVideoInfo_INT(m_ListItemHandle, type, value);
  }

  void CListItem::SetVideoInfo_UINT(ADDON_VideoInfoTag type, unsigned int value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetVideoInfo_UINT(m_ListItemHandle, type, value);
  }

  void CListItem::SetVideoInfo_FLOAT(ADDON_VideoInfoTag type, float value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetVideoInfo_FLOAT(m_ListItemHandle, type, value);
  }

  void CListItem::SetVideoInfo_STRING(ADDON_VideoInfoTag type, std::string value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetVideoInfo_STRING(m_ListItemHandle, type, value);
  }

  void CListItem::SetVideoInfo_STRING_LIST(ADDON_VideoInfoTag type, std::vector<std::string> values)
  {
    g_interProcess.ListItem_SetVideoInfo_STRING_LIST(m_ListItemHandle, type, values);
  } 
    
  void CListItem::SetVideoInfo_Resume(ADDON_VideoInfoTag_Resume &resume)
  { 
    g_interProcess.ListItem_SetVideoInfo_Resume(m_ListItemHandle, resume);
  }
  
  void CListItem::SetVideoInfo_Cast(std::vector<ADDON_VideoInfoTag_Cast> &cast)
  {
    g_interProcess.ListItem_SetVideoInfo_Cast(m_ListItemHandle, cast);
  }

  void CListItem::SetPictureInfo_BOOL(ADDON_PictureInfoTag type, bool value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetPictureInfo_BOOL(m_ListItemHandle, type, value);
  }

  void CListItem::SetPictureInfo_INT(ADDON_PictureInfoTag type, int value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetPictureInfo_INT(m_ListItemHandle, type, value);
  }

  void CListItem::SetPictureInfo_UINT(ADDON_PictureInfoTag type, unsigned int value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetPictureInfo_UINT(m_ListItemHandle, type, value);
  }

  void CListItem::SetPictureInfo_FLOAT(ADDON_PictureInfoTag type, float value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetPictureInfo_FLOAT(m_ListItemHandle, type, value);
  }

  void CListItem::SetPictureInfo_STRING(ADDON_PictureInfoTag type, std::string value)
  {
    if (type != 0)
      g_interProcess.ListItem_SetPictureInfo_STRING(m_ListItemHandle, type, value);
  }

  void CListItem::SetPictureInfo_STRING_LIST(ADDON_PictureInfoTag type, std::vector<std::string> values)
  {
    g_interProcess.ListItem_SetPictureInfo_STRING_LIST(m_ListItemHandle, type, values);
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
