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

#include "AddonExe_GUI_DialogFileBrowser.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/GUI/AddonGUIDialogFileBrowser.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetDirectory(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* shares;
  char* heading;
  bool  bWriteOnly;
  req.pop(API_STRING,  &shares);
  req.pop(API_STRING,  &heading);
  req.pop(API_BOOLEAN, &bWriteOnly);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* path = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_FileBrowser::ShowAndGetDirectory(shares, heading, *path, iMaxStringSize, bWriteOnly);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, path);
  free(path);
  return true;
}

bool CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* shares;
  char* mask;
  char* heading;
  bool  useThumbs;
  bool  useFileDirectories;
  req.pop(API_STRING,  &shares);
  req.pop(API_STRING,  &mask);
  req.pop(API_STRING,  &heading);
  req.pop(API_BOOLEAN, &useThumbs);
  req.pop(API_BOOLEAN, &useFileDirectories);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* path = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_FileBrowser::ShowAndGetFile(shares, mask, heading, *path, iMaxStringSize, useThumbs, useFileDirectories);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, path);
  free(path);
  return true;
}

bool CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetFileFromDir(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* directory;
  char* mask;
  char* heading;
  bool  useThumbs;
  bool  useFileDirectories;
  bool  singleList;
  req.pop(API_STRING,  &directory);
  req.pop(API_STRING,  &mask);
  req.pop(API_STRING,  &heading);
  req.pop(API_BOOLEAN, &useThumbs);
  req.pop(API_BOOLEAN, &useFileDirectories);
  req.pop(API_BOOLEAN, &singleList);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* path = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_FileBrowser::ShowAndGetFileFromDir(directory, mask, heading, *path, iMaxStringSize, useThumbs, useFileDirectories, singleList);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, path);
  free(path);
  return true;
}

bool CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetFileList(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* shares;
  char* mask;
  char* heading;
  bool useThumbs;
  bool useFileDirectories;
  req.pop(API_STRING,  &shares);
  req.pop(API_STRING,  &mask);
  req.pop(API_STRING,  &heading);
  req.pop(API_BOOLEAN, &useThumbs);
  req.pop(API_BOOLEAN, &useFileDirectories);

  char** list;
  unsigned int listSize = 0;
  bool ret = GUI::CAddOnDialog_FileBrowser::ShowAndGetFileList(shares, mask, heading, list, listSize, useThumbs, useFileDirectories);
  uint32_t retValue = API_SUCCESS;
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &retValue);
  resp.push(API_BOOLEAN, &ret);
  resp.push(API_UNSIGNED_INT, &listSize);
  if (ret)
  {
    for (unsigned int i = 0; i < listSize; ++i)
      resp.push(API_STRING, list[i]);
  }
  resp.finalise();
  if (ret)
    GUI::CAddOnDialog_FileBrowser::ClearList(list, listSize);
  return true;
}

bool CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetSource(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool  allowNetworkShares;
  char* additionalShare;
  char* strType;
  req.pop(API_BOOLEAN, &allowNetworkShares);
  req.pop(API_STRING,  &additionalShare);
  req.pop(API_STRING,  &strType);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* path = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_FileBrowser::ShowAndGetSource(*path, iMaxStringSize, allowNetworkShares, additionalShare, strType);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, path);
  free(path);
  return true;
}

bool CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetImage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* shares;
  char* heading;
  req.pop(API_STRING,  &shares);
  req.pop(API_STRING,  &heading);
  unsigned int iMaxStringSize = 1024*sizeof(char);
  char* path = (char*)malloc(iMaxStringSize);
  bool ret = GUI::CAddOnDialog_FileBrowser::ShowAndGetImage(shares, heading, *path, iMaxStringSize);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(API_SUCCESS, &ret, path);
  free(path);
  return true;
}

bool CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetImageList(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* shares;
  char* heading;
  req.pop(API_STRING,  &shares);
  req.pop(API_STRING,  &heading);

  char** list;
  unsigned int listSize = 0;
  bool ret = GUI::CAddOnDialog_FileBrowser::ShowAndGetImageList(shares, heading, list, listSize);
  uint32_t retValue = API_SUCCESS;
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &retValue);
  resp.push(API_BOOLEAN, &ret);
  resp.push(API_UNSIGNED_INT, &listSize);
  if (ret)
  {
    for (unsigned int i = 0; i < listSize; ++i)
      resp.push(API_STRING, list[i]);
  }
  resp.finalise();
  if (ret)
    GUI::CAddOnDialog_FileBrowser::ClearList(list, listSize);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
