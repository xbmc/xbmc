#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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

#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_LibFunc_Base.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

struct CB_AddOnLib;

struct CAddOnListItem
{
  static void Init(::V2::KodiAPI::CB_AddOnLib *callbacks);

  static GUIHANDLE Create(void *addonData, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path);
  static void Destroy(void *addonData, GUIHANDLE handle);
  static void GetLabel(void *addonData, GUIHANDLE handle, char &label, unsigned int &iMaxStringSize);
  static void SetLabel(void *addonData, GUIHANDLE handle, const char *label);
  static void GetLabel2(void *addonData, GUIHANDLE handle, char &label, unsigned int &iMaxStringSize);
  static void SetLabel2(void *addonData, GUIHANDLE handle, const char *label);
  static void GetIconImage(void *addonData, GUIHANDLE handle, char &image, unsigned int &iMaxStringSize);
  static void SetIconImage(void *addonData, GUIHANDLE handle, const char *image);
  static void GetOverlayImage(void *addonData, GUIHANDLE handle, char &image, unsigned int &iMaxStringSize);
  static void SetOverlayImage(void *addonData, GUIHANDLE handle, unsigned int image, bool bOnOff /* = false*/);
  static void SetThumbnailImage(void *addonData, GUIHANDLE handle, const char *image);
  static void SetArt(void *addonData, GUIHANDLE handle, const char *type, const char *url);
  static void SetArtFallback(void *addonData, GUIHANDLE handle, const char *from, const char *to);
  static bool HasArt(void *addonData, GUIHANDLE handle, const char *type);
  static void Select(void *addonData, GUIHANDLE handle, bool bOnOff);
  static bool IsSelected(void *addonData, GUIHANDLE handle);
  static bool HasIcon(void *addonData, GUIHANDLE handle);
  static bool HasOverlay(void *addonData, GUIHANDLE handle);
  static bool IsFileItem(void *addonData, GUIHANDLE handle);
  static bool IsFolder(void *addonData, GUIHANDLE handle);
  static void SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value);
  static void GetProperty(void *addonData, GUIHANDLE handle, const char *key, char &property, unsigned int &iMaxStringSize);
  static void ClearProperty(void *addonData, GUIHANDLE handle, const char *key);
  static void ClearProperties(void *addonData, GUIHANDLE handle);
  static bool HasProperties(void *addonData, GUIHANDLE handle);
  static bool HasProperty(void *addonData, GUIHANDLE handle, const char *key);
  static void SetPath(void *addonData, GUIHANDLE handle, const char *path);
  static char* GetPath(void *addonData, GUIHANDLE handle);
  static int  GetDuration(void *addonData, GUIHANDLE handle);
  static void SetSubtitles(void *addonData, GUIHANDLE handle, const char** streams, unsigned int entries);
  static void SetMimeType(void* addonData, GUIHANDLE handle, const char* mimetype);
  static void SetContentLookup(void* addonData, GUIHANDLE handle, bool enable);
  static void AddContextMenuItems(void* addonData, GUIHANDLE handle, const char** menus[2], unsigned int entries, bool replaceItems);
  static void AddStreamInfo(void* addonData, GUIHANDLE handle, const char* cType, const char** dictionary[2], unsigned int entries);
  static void SetMusicInfo(void* addonData, GUIHANDLE handle, unsigned int type, void* data, unsigned int entries);
  static void SetVideoInfo(void* addonData, GUIHANDLE handle, unsigned int type, void* data, unsigned int entries);
  static void SetPictureInfo(void* addonData, GUIHANDLE handle, unsigned int type, void* data, unsigned int entries);
};

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
