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

#include <cstring>

extern "C"
{

GUIHANDLE CKODIAddon_InterProcess_GUI_ListItem::ListItem_Create(
  const std::string&      label,
  const std::string&      label2,
  const std::string&      iconImage,
  const std::string&      thumbnailImage,
  const std::string&      path)
{
  return g_interProcess.m_Callbacks->GUI.ListItem.Create(g_interProcess.m_Handle->addonData, label.c_str(),
                                             label2.c_str(), iconImage.c_str(),
                                             thumbnailImage.c_str(), path.c_str());
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_Destroy(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.ListItem.Destroy(g_interProcess.m_Handle->addonData, handle);
}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetLabel(GUIHANDLE handle)
{
  std::string label;
  label.resize(1024);
  unsigned int size = (unsigned int)label.capacity();
  g_interProcess.m_Callbacks->GUI.ListItem.GetLabel(g_interProcess.m_Handle->addonData, handle, label[0], size);
  label.resize(size);
  label.shrink_to_fit();
  return label.c_str();
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetLabel(GUIHANDLE handle, const std::string &label)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetLabel(g_interProcess.m_Handle->addonData, handle, label.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetLabel2(GUIHANDLE handle)
{
  std::string label;
  label.resize(1024);
  unsigned int size = (unsigned int)label.capacity();
  g_interProcess.m_Callbacks->GUI.ListItem.GetLabel2(g_interProcess.m_Handle->addonData, handle, label[0], size);
  label.resize(size);
  label.shrink_to_fit();
  return label.c_str();
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetLabel2(GUIHANDLE handle, const std::string &label)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetLabel2(g_interProcess.m_Handle->addonData, handle, label.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetIconImage(GUIHANDLE handle)
{
  std::string image;
  image.resize(1024);
  unsigned int size = (unsigned int)image.capacity();
  g_interProcess.m_Callbacks->GUI.ListItem.GetIconImage(g_interProcess.m_Handle->addonData, handle, image[0], size);
  image.resize(size);
  image.shrink_to_fit();
  return image.c_str();
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetIconImage(GUIHANDLE handle, const std::string &image)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetIconImage(g_interProcess.m_Handle->addonData, handle, image.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetOverlayImage(GUIHANDLE handle)
{
  std::string image;
  image.resize(1024);
  unsigned int size = (unsigned int)image.capacity();
  g_interProcess.m_Callbacks->GUI.ListItem.GetOverlayImage(g_interProcess.m_Handle->addonData, handle, image[0], size);
  image.resize(size);
  image.shrink_to_fit();
  return image.c_str();
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetOverlayImage(GUIHANDLE handle, unsigned int image, bool bOnOff /*= false */)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetOverlayImage(g_interProcess.m_Handle->addonData, handle, image, bOnOff);
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetThumbnailImage(GUIHANDLE handle, const std::string &image)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetThumbnailImage(g_interProcess.m_Handle->addonData, handle, image.c_str());
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetArt(GUIHANDLE handle, const std::string &type, const std::string &url)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetArt(g_interProcess.m_Handle->addonData, handle, type.c_str(), url.c_str());
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetArtFallback(GUIHANDLE handle, const std::string &from, const std::string &to)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetArtFallback(g_interProcess.m_Handle->addonData, handle, from.c_str(), to.c_str());
}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasArt(GUIHANDLE handle, const std::string &type)
{
  return g_interProcess.m_Callbacks->GUI.ListItem.HasArt(g_interProcess.m_Handle->addonData, handle, type.c_str());
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_Select(GUIHANDLE handle, bool bOnOff)
{
  g_interProcess.m_Callbacks->GUI.ListItem.Select(g_interProcess.m_Handle->addonData, handle, bOnOff);
}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_IsSelected(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.ListItem.IsSelected(g_interProcess.m_Handle->addonData, handle);
}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasIcon(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.ListItem.HasIcon(g_interProcess.m_Handle->addonData, handle);
}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasOverlay(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.ListItem.HasOverlay(g_interProcess.m_Handle->addonData, handle);
}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_IsFileItem(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.ListItem.IsFileItem(g_interProcess.m_Handle->addonData, handle);
}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_IsFolder(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.ListItem.IsFolder(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetProperty(GUIHANDLE handle, const std::string &key, const std::string &value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetProperty(g_interProcess.m_Handle->addonData, handle, key.c_str(), value.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetProperty(GUIHANDLE handle, const std::string &key)
{
  std::string property;
  property.resize(1024);
  unsigned int size = (unsigned int)property.capacity();
  g_interProcess.m_Callbacks->GUI.ListItem.GetProperty(g_interProcess.m_Handle->addonData, handle, key.c_str(), property[0], size);
  property.resize(size);
  property.shrink_to_fit();
  return property;
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_ClearProperty(GUIHANDLE handle, const std::string &key)
{
  g_interProcess.m_Callbacks->GUI.ListItem.ClearProperty(g_interProcess.m_Handle->addonData, handle, key.c_str());
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_ClearProperties(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.ListItem.ClearProperties(g_interProcess.m_Handle->addonData, handle);
}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasProperties(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.ListItem.HasProperties(g_interProcess.m_Handle->addonData, handle);
}

bool CKODIAddon_InterProcess_GUI_ListItem::ListItem_HasProperty(GUIHANDLE handle, const std::string &key)
{
  return g_interProcess.m_Callbacks->GUI.ListItem.HasProperty(g_interProcess.m_Handle->addonData, handle, key.c_str());
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPath(GUIHANDLE handle, const std::string &path)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetPath(g_interProcess.m_Handle->addonData, handle, path.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetPath(GUIHANDLE handle)
{
  std::string strReturn;
  char* strMsg = g_interProcess.m_Callbacks->GUI.ListItem.GetPath(g_interProcess.m_Handle->addonData, handle);
  if (strMsg != nullptr)
  {
    if (std::strlen(strMsg))
      strReturn = strMsg;
    g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
  }
  return strReturn;
}

int CKODIAddon_InterProcess_GUI_ListItem::ListItem_GetDuration(GUIHANDLE handle) const
{
  return g_interProcess.m_Callbacks->GUI.ListItem.GetDuration(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetSubtitles(GUIHANDLE handle, const std::vector<std::string>& subtitleFiles)
{
  unsigned int size = subtitleFiles.size();
  if (size == 0)
    return;

  const char** subtitles = (const char**)malloc(size);
  for (unsigned int i = 0; i < size; ++i)
    subtitles[i] = subtitleFiles[i].c_str();

  g_interProcess.m_Callbacks->GUI.ListItem.SetSubtitles(g_interProcess.m_Handle->addonData, handle, subtitles, size);
  free(subtitles);
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMimeType(GUIHANDLE handle, const std::string& mimetype)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetMimeType(g_interProcess.m_Handle->addonData, handle, mimetype.c_str());
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetContentLookup(GUIHANDLE handle, bool enable)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetContentLookup(g_interProcess.m_Handle->addonData, handle, enable);
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_AddContextMenuItems(GUIHANDLE handle, const std::vector<std::pair<std::string, std::string> >& items, bool replaceItems)
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

  g_interProcess.m_Callbacks->GUI.ListItem.AddContextMenuItems(g_interProcess.m_Handle->addonData, handle, entries, size, replaceItems);
  if (entries)
    free(entries);
}


void CKODIAddon_InterProcess_GUI_ListItem::ListItem_AddStreamInfo(GUIHANDLE handle, const std::string& cType, const std::vector<std::pair<std::string, std::string> >& dictionary)
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

  g_interProcess.m_Callbacks->GUI.ListItem.AddStreamInfo(g_interProcess.m_Handle->addonData, handle, cType.c_str(), entries, size);
  free(entries);
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_BOOL(GUIHANDLE handle, ADDON_MusicInfoTag type, bool value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(bool));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_INT(GUIHANDLE handle,ADDON_MusicInfoTag type, int value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(int));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_UINT(GUIHANDLE handle, ADDON_MusicInfoTag type, unsigned int value)

{
  g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(unsigned int));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_FLOAT(GUIHANDLE handle,ADDON_MusicInfoTag type, float value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(float));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_STRING(GUIHANDLE handle, ADDON_MusicInfoTag type, std::string value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle->addonData, handle, type, (void*)value.c_str(), sizeof(const char*));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetMusicInfo_STRING_LIST(GUIHANDLE handle,ADDON_MusicInfoTag type, std::vector<std::string> values)
{
  unsigned int size = values.size();
  if (type == 0 || size == 0)
    return;

  const char** entries = (const char**)malloc(size*sizeof(const char**));
  for (unsigned int i = 0; i < size; ++i)
    entries[i] = values[i].c_str();

  g_interProcess.m_Callbacks->GUI.ListItem.SetMusicInfo(g_interProcess.m_Handle->addonData, handle, type, &entries, size);
  free(entries);
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_BOOL(GUIHANDLE handle,ADDON_VideoInfoTag type, bool value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(bool));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_INT(GUIHANDLE handle, ADDON_VideoInfoTag type, int value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(int));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_UINT(GUIHANDLE handle,ADDON_VideoInfoTag type, unsigned int value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(unsigned int));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_FLOAT(GUIHANDLE handle, ADDON_VideoInfoTag type, float value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(float));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_STRING(GUIHANDLE handle,ADDON_VideoInfoTag type, std::string value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle->addonData, handle, type, (void*)value.c_str(), sizeof(const char*));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_STRING_LIST(GUIHANDLE handle, ADDON_VideoInfoTag type, std::vector<std::string> values)
{
  unsigned int size = values.size();
  if (type == 0 || size == 0)
    return;

  const char** entries = (const char**)malloc(size*sizeof(const char**));
  for (unsigned int i = 0; i < size; ++i)
    entries[i] = values[i].c_str();

  g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle->addonData, handle, type, (void*)entries, size);
  free(entries);
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_Resume(GUIHANDLE handle, ADDON_VideoInfoTag_Resume &resume)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle->addonData, handle, ADDON_VideoInfoTag____resume____________________DATA, &resume, sizeof(ADDON_VideoInfoTag____resume____________________DATA));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetVideoInfo_Cast(GUIHANDLE handle, std::vector<ADDON_VideoInfoTag_Cast> &cast)
{
  unsigned int size = cast.size();
  if (size == 0)
    return;

  ADDON_VideoInfoTag__cast__DATA_STRUCT** entries = (ADDON_VideoInfoTag__cast__DATA_STRUCT**)malloc(size*sizeof(ADDON_VideoInfoTag__cast__DATA_STRUCT**));
  for (unsigned int i = 0; i < size; ++i)
  {
    entries[i] = (ADDON_VideoInfoTag__cast__DATA_STRUCT*)malloc(sizeof(ADDON_VideoInfoTag__cast__DATA_STRUCT*));
    entries[i]->name       = cast[i].name.c_str();
    entries[i]->role       = cast[i].role.c_str();
    entries[i]->order      = cast[i].order;
    entries[i]->thumbnail  = cast[i].thumbnail.c_str();
  }

  g_interProcess.m_Callbacks->GUI.ListItem.SetVideoInfo(g_interProcess.m_Handle->addonData, handle, ADDON_VideoInfoTag____cast______________________DATA_LIST, (void*)entries, size);
  free(entries);
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_BOOL(GUIHANDLE handle, ADDON_PictureInfoTag type, bool value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(bool));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_INT(GUIHANDLE handle, ADDON_PictureInfoTag type, int value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(int));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_UINT(GUIHANDLE handle, ADDON_PictureInfoTag type, unsigned int value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(unsigned int));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_FLOAT(GUIHANDLE handle, ADDON_PictureInfoTag type, float value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle->addonData, handle, type, &value, sizeof(float));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_STRING(GUIHANDLE handle, ADDON_PictureInfoTag type, std::string value)
{
  g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle->addonData, handle, type, (void*)value.c_str(), sizeof(const char*));
}

void CKODIAddon_InterProcess_GUI_ListItem::ListItem_SetPictureInfo_STRING_LIST(GUIHANDLE handle, ADDON_PictureInfoTag type, std::vector<std::string> values)
{
  unsigned int size = values.size();
  if (type == 0 || size == 0)
    return;

  const char** entries = (const char**)malloc(size*sizeof(const char**));
  for (unsigned int i = 0; i < size; ++i)
    entries[i] = values[i].c_str();

  g_interProcess.m_Callbacks->GUI.ListItem.SetPictureInfo(g_interProcess.m_Handle->addonData, handle, type, (void*)entries, size);
  free(entries);
}

}; /* extern "C" */
