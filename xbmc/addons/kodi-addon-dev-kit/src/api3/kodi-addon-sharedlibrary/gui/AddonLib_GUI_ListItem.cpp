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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ListItem.hpp)

#include <cstring>

API_NAMESPACE

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
    m_ListItemHandle = g_interProcess.m_Callbacks->GUI.ListItem.Create(g_interProcess.m_Handle, label.c_str(),
                                               label2.c_str(), iconImage.c_str(),
                                               thumbnailImage.c_str(), path.c_str());
  }

  CListItem::CListItem(GUIHANDLE listItemHandle)
  {
    m_ListItemHandle = listItemHandle;
  }

  CListItem::~CListItem()
  {
    g_interProcess.m_Callbacks->GUI.ListItem.Destroy(g_interProcess.m_Handle, m_ListItemHandle);
  }

  std::string CListItem::GetLabel()
  {
    std::string label;
    label.resize(1024);
    unsigned int size = (unsigned int)label.capacity();
    g_interProcess.m_Callbacks->GUI.ListItem.GetLabel(g_interProcess.m_Handle, m_ListItemHandle, label[0], size);
    label.resize(size);
    label.shrink_to_fit();
    return label.c_str();
  }

  void CListItem::SetLabel(const std::string &label)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetLabel(g_interProcess.m_Handle, m_ListItemHandle, label.c_str());
  }

  std::string CListItem::GetLabel2()
  {
    std::string label;
    label.resize(1024);
    unsigned int size = (unsigned int)label.capacity();
    g_interProcess.m_Callbacks->GUI.ListItem.GetLabel2(g_interProcess.m_Handle, m_ListItemHandle, label[0], size);
    label.resize(size);
    label.shrink_to_fit();
    return label.c_str();
  }

  void CListItem::SetLabel2(const std::string &label)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetLabel2(g_interProcess.m_Handle, m_ListItemHandle, label.c_str());
  }

  std::string CListItem::GetIconImage()
  {
    std::string image;
    image.resize(1024);
    unsigned int size = (unsigned int)image.capacity();
    g_interProcess.m_Callbacks->GUI.ListItem.GetIconImage(g_interProcess.m_Handle, m_ListItemHandle, image[0], size);
    image.resize(size);
    image.shrink_to_fit();
    return image.c_str();
  }

  void CListItem::SetIconImage(const std::string &image)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetIconImage(g_interProcess.m_Handle, m_ListItemHandle, image.c_str());
  }

  std::string CListItem::GetOverlayImage()
  {
    std::string image;
    image.resize(1024);
    unsigned int size = (unsigned int)image.capacity();
    g_interProcess.m_Callbacks->GUI.ListItem.GetOverlayImage(g_interProcess.m_Handle, m_ListItemHandle, image[0], size);
    image.resize(size);
    image.shrink_to_fit();
    return image.c_str();
  }

  void CListItem::SetOverlayImage(unsigned int image, bool bOnOff /*= false */)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetOverlayImage(g_interProcess.m_Handle, m_ListItemHandle, image, bOnOff);
  }

  void CListItem::SetThumbnailImage(const std::string &image)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetThumbnailImage(g_interProcess.m_Handle, m_ListItemHandle, image.c_str());
  }

  void CListItem::SetArt(const std::string &type, const std::string &url)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetArt(g_interProcess.m_Handle, m_ListItemHandle, type.c_str(), url.c_str());
  }

  std::string CListItem::GetArt(const std::string &type)
  {
    std::string strReturn;
    char* strMsg = g_interProcess.m_Callbacks->GUI.ListItem.GetArt(g_interProcess.m_Handle, m_ListItemHandle, type.c_str());
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        strReturn = strMsg;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
    }
    return strReturn;
  }

  void CListItem::SetArtFallback(const std::string &from, const std::string &to)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetArtFallback(g_interProcess.m_Handle, m_ListItemHandle, from.c_str(), to.c_str());
  }

  bool CListItem::HasArt(const std::string &type)
  {
    return g_interProcess.m_Callbacks->GUI.ListItem.HasArt(g_interProcess.m_Handle, m_ListItemHandle, type.c_str());
  }

  void CListItem::Select(bool bOnOff)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.Select(g_interProcess.m_Handle, m_ListItemHandle, bOnOff);
  }

  bool CListItem::IsSelected()
  {
    return g_interProcess.m_Callbacks->GUI.ListItem.IsSelected(g_interProcess.m_Handle, m_ListItemHandle);
  }

  bool CListItem::HasIcon()
  {
    return g_interProcess.m_Callbacks->GUI.ListItem.HasIcon(g_interProcess.m_Handle, m_ListItemHandle);
  }

  bool CListItem::HasOverlay()
  {
    return g_interProcess.m_Callbacks->GUI.ListItem.HasOverlay(g_interProcess.m_Handle, m_ListItemHandle);
  }

  bool CListItem::IsFileItem()
  {
    return g_interProcess.m_Callbacks->GUI.ListItem.IsFileItem(g_interProcess.m_Handle, m_ListItemHandle);
  }

  bool CListItem::IsFolder()
  {
    return g_interProcess.m_Callbacks->GUI.ListItem.IsFolder(g_interProcess.m_Handle, m_ListItemHandle);
  }

  void CListItem::SetProperty(const std::string &key, const std::string &value)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetProperty(g_interProcess.m_Handle, m_ListItemHandle, key.c_str(), value.c_str());
  }

  std::string CListItem::GetProperty(const std::string &key)
  {
    std::string property;
    property.resize(1024);
    unsigned int size = (unsigned int)property.capacity();
    g_interProcess.m_Callbacks->GUI.ListItem.GetProperty(g_interProcess.m_Handle, m_ListItemHandle, key.c_str(), property[0], size);
    property.resize(size);
    property.shrink_to_fit();
    return property;
  }

  void CListItem::ClearProperty(const std::string &key)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.ClearProperty(g_interProcess.m_Handle, m_ListItemHandle, key.c_str());
  }

  void CListItem::ClearProperties()
  {
    g_interProcess.m_Callbacks->GUI.ListItem.ClearProperties(g_interProcess.m_Handle, m_ListItemHandle);
  }

  bool CListItem::HasProperties()
  {
    return g_interProcess.m_Callbacks->GUI.ListItem.HasProperties(g_interProcess.m_Handle, m_ListItemHandle);
  }

  bool CListItem::HasProperty(const std::string &key)
  {
    return g_interProcess.m_Callbacks->GUI.ListItem.HasProperty(g_interProcess.m_Handle, m_ListItemHandle, key.c_str());
  }

  void CListItem::SetPath(const std::string &path)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetPath(g_interProcess.m_Handle, m_ListItemHandle, path.c_str());
  }

  std::string CListItem::GetPath()
  {
    std::string strReturn;
    char* strMsg = g_interProcess.m_Callbacks->GUI.ListItem.GetPath(g_interProcess.m_Handle, m_ListItemHandle);
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        strReturn = strMsg;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
    }
    return strReturn;
  }

  int CListItem::GetDuration() const
  {
    return g_interProcess.m_Callbacks->GUI.ListItem.GetDuration(g_interProcess.m_Handle, m_ListItemHandle);
  }

  void CListItem::SetSubtitles(const std::vector<std::string>& subtitleFiles)
  {
    unsigned int size = subtitleFiles.size();
    if (size == 0)
      return;

    const char** subtitles = (const char**)malloc(size);
    for (unsigned int i = 0; i < size; ++i)
      subtitles[i] = subtitleFiles[i].c_str();

    g_interProcess.m_Callbacks->GUI.ListItem.SetSubtitles(g_interProcess.m_Handle, m_ListItemHandle, subtitles, size);
    free(subtitles);
  }

  void CListItem::SetMimeType(const std::string& mimetype)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetMimeType(g_interProcess.m_Handle, m_ListItemHandle, mimetype.c_str());
  }

  void CListItem::SetContentLookup(bool enable)
  {
    g_interProcess.m_Callbacks->GUI.ListItem.SetContentLookup(g_interProcess.m_Handle, m_ListItemHandle, enable);
  }

  void CListItem::AddContextMenuItems(const std::vector<std::pair<std::string, std::string> >& items, bool replaceItems)
  {
    const char*** entries = nullptr;
    unsigned int size = items.size();
    if (size != 0)
    {
      entries = (const char***)malloc(size*sizeof(const char***));
      for (unsigned int i = 0; i < size; ++i)
      {
        entries[i][0] = items.at(i).first.c_str();
        entries[i][1] = items.at(i).second.c_str();
      }
    }

    g_interProcess.m_Callbacks->GUI.ListItem.AddContextMenuItems(g_interProcess.m_Handle, m_ListItemHandle, entries, size, replaceItems);
    if (entries)
      free(entries);
  }

  void CListItem::AddStreamInfo(const std::string& cType, const std::vector<std::pair<std::string, std::string> >& dictionary)
  {
    const char*** entries = nullptr;
    unsigned int size = dictionary.size();
    if (size == 0)
      return;

    entries = (const char***)malloc(size*sizeof(const char***));
    for (unsigned int i = 0; i < size; ++i)
    {
      entries[i][0] = dictionary.at(i).first.c_str();
      entries[i][1] = dictionary.at(i).second.c_str();
    }

    g_interProcess.m_Callbacks->GUI.ListItem.AddStreamInfo(g_interProcess.m_Handle, m_ListItemHandle, cType.c_str(), entries, size);
    free(entries);
  }

  void CListItem::SetMusicInfo_BOOL(ADDON_MusicInfoTag type, bool value)
  {   
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(bool));
  }

  void CListItem::SetMusicInfo_INT(ADDON_MusicInfoTag type, int value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(int));
  }

  void CListItem::SetMusicInfo_UINT(ADDON_MusicInfoTag type, unsigned int value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(unsigned int));
  }

  void CListItem::SetMusicInfo_FLOAT(ADDON_MusicInfoTag type, float value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(float));
  }

  void CListItem::SetMusicInfo_STRING(ADDON_MusicInfoTag type, std::string value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle, m_ListItemHandle, type, (void*)value.c_str(), sizeof(const char*));
  } 

  void CListItem::SetMusicInfo_STRING_LIST(ADDON_MusicInfoTag type, std::vector<std::string> values)
  {
    unsigned int size = values.size();
    if (type == 0 || size == 0)
      return;

    const char** entries = (const char**)malloc(size*sizeof(const char**));
    for (unsigned int i = 0; i < size; ++i)
      entries[i] = values[i].c_str();

    g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &entries, size);
    free(entries);
  }

  void CListItem::SetVideoInfo_BOOL(ADDON_VideoInfoTag type, bool value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(bool));
  }

  void CListItem::SetVideoInfo_INT(ADDON_VideoInfoTag type, int value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(int));
  }

  void CListItem::SetVideoInfo_UINT(ADDON_VideoInfoTag type, unsigned int value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(unsigned int));
  }

  void CListItem::SetVideoInfo_FLOAT(ADDON_VideoInfoTag type, float value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(float));
  }

  void CListItem::SetVideoInfo_STRING(ADDON_VideoInfoTag type, std::string value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle, m_ListItemHandle, type, (void*)value.c_str(), sizeof(const char*));
  }

  void CListItem::SetVideoInfo_STRING_LIST(ADDON_VideoInfoTag type, std::vector<std::string> values)
  {
    unsigned int size = values.size();
    if (type == 0 || size == 0)
      return;

    const char** entries = (const char**)malloc(size*sizeof(const char**));
    for (unsigned int i = 0; i < size; ++i)
      entries[i] = values[i].c_str();

    g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle, m_ListItemHandle, type, (void*)entries, size);
    free(entries);
  } 

  void CListItem::SetVideoInfo_Resume(ADDON_VideoInfoTag_Resume &resume)
  { 
    g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle, m_ListItemHandle, ADDON_VideoInfoTag_resume_DATA, &resume, sizeof(ADDON_VideoInfoTag_resume_DATA));
  }

  void CListItem::SetVideoInfo_Cast(std::vector<ADDON_VideoInfoTag_Cast> &cast)
  {
    unsigned int size = cast.size();
    if (size == 0)
      return;

    ADDON_VideoInfoTag_cast_DATA_STRUCT** entries = (ADDON_VideoInfoTag_cast_DATA_STRUCT**)malloc(size*sizeof(ADDON_VideoInfoTag_cast_DATA_STRUCT**));
    for (unsigned int i = 0; i < size; ++i)
    {
      entries[i] = (ADDON_VideoInfoTag_cast_DATA_STRUCT*)malloc(sizeof(ADDON_VideoInfoTag_cast_DATA_STRUCT*));
      entries[i]->name       = cast[i].name.c_str();
      entries[i]->role       = cast[i].role.c_str();
      entries[i]->order      = cast[i].order;
      entries[i]->thumbnail  = cast[i].thumbnail.c_str();
    }

    g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle, m_ListItemHandle, ADDON_VideoInfoTag_cast_DATA_LIST, (void*)entries, size);
    free(entries);
  }

  void CListItem::SetPictureInfo_BOOL(ADDON_PictureInfoTag type, bool value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(bool));
  }

  void CListItem::SetPictureInfo_INT(ADDON_PictureInfoTag type, int value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(int));
  }

  void CListItem::SetPictureInfo_UINT(ADDON_PictureInfoTag type, unsigned int value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(unsigned int));
  }

  void CListItem::SetPictureInfo_FLOAT(ADDON_PictureInfoTag type, float value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle, m_ListItemHandle, type, &value, sizeof(float));
  }

  void CListItem::SetPictureInfo_STRING(ADDON_PictureInfoTag type, std::string value)
  {
    if (type != 0)
      g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle, m_ListItemHandle, type, (void*)value.c_str(), sizeof(const char*));
  }

  void CListItem::SetPictureInfo_STRING_LIST(ADDON_PictureInfoTag type, std::vector<std::string> values)
  {
    unsigned int size = values.size();
    if (type == 0 || size == 0)
      return;

    const char** entries = (const char**)malloc(size*sizeof(const char**));
    for (unsigned int i = 0; i < size; ++i)
      entries[i] = values[i].c_str();

    g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle, m_ListItemHandle, type, (void*)entries, size);
    free(entries);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
