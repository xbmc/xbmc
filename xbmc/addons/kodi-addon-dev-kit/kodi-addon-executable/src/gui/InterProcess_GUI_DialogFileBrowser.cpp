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

#include "InterProcess_GUI_DialogFileBrowser.h"
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

  bool CKODIAddon_InterProcess_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetDirectory(
            const std::string&      shares,
            const std::string&      heading,
            std::string&            path,
            bool                    bWriteOnly)
  {
    try
    {
      bool ret;
      char* givenPath;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_FileBrowser_ShowAndGetDirectory, session);
      vrp.push(API_STRING,   shares.c_str());
      vrp.push(API_STRING,   heading.c_str());
      vrp.push(API_BOOLEAN, &bWriteOnly);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &givenPath);
      if (retCode != API_SUCCESS)
        throw retCode;
      path = givenPath;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetFile(
            const std::string&      shares,
            const std::string&      mask,
            const std::string&      heading,
            std::string&            file,
            bool                    useThumbs,
            bool                    useFileDirectories)
  {
    try
    {
      bool ret;
      bool givenFile;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_FileBrowser_ShowAndGetFile, session);
      vrp.push(API_STRING,   shares.c_str());
      vrp.push(API_STRING,   mask.c_str());
      vrp.push(API_STRING,   heading.c_str());
      vrp.push(API_BOOLEAN, &useThumbs);
      vrp.push(API_BOOLEAN, &useFileDirectories);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &givenFile);
      if (retCode != API_SUCCESS)
        throw retCode;
      file = givenFile;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetFileFromDir(
            const std::string&      directory,
            const std::string&      mask,
            const std::string&      heading,
            std::string&            path,
            bool                    useThumbs,
            bool                    useFileDirectories,
            bool                    singleList)
  {
    try
    {
      bool ret;
      char* givenPath;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_FileBrowser_ShowAndGetFileFromDir, session);
      vrp.push(API_STRING,   directory.c_str());
      vrp.push(API_STRING,   mask.c_str());
      vrp.push(API_STRING,   heading.c_str());
      vrp.push(API_BOOLEAN, &useThumbs);
      vrp.push(API_BOOLEAN, &useFileDirectories);
      vrp.push(API_BOOLEAN, &singleList);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &givenPath);
      if (retCode != API_SUCCESS)
        throw retCode;
      path = givenPath;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetFileList(
            const std::string&      shares,
            const std::string&      mask,
            const std::string&      heading,
            std::vector<std::string> &path,
            bool                    useThumbs,
            bool                    useFileDirectories)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_FileBrowser_ShowAndGetFileList, session);
      vrp.push(API_STRING,   shares.c_str());
      vrp.push(API_STRING,   mask.c_str());
      vrp.push(API_STRING,   heading.c_str());
      vrp.push(API_BOOLEAN, &useThumbs);
      vrp.push(API_BOOLEAN, &useFileDirectories);

      bool ret;
      char* text;
      unsigned int length;
      uint32_t retCode;
      CLockObject lock(session->m_callMutex);
      std::unique_ptr<CResponsePacket> vresp(session->ReadResult(&vrp));
      if (!vresp)
        throw API_ERR_BUFFER;
      vresp->pop(API_UINT32_T,     &retCode);
      vresp->pop(API_BOOLEAN,      &ret);
      vresp->pop(API_UNSIGNED_INT, &length);
      for (unsigned int i = 0; i < length; ++i)
        path.push_back((char*)vresp->pop(API_STRING, &text));
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetSource(
            std::string&            path,
            bool                    allowNetworkShares,
            const std::string&      additionalShare,
            const std::string&      strType)
  {
    try
    {
      bool ret;
      char* givenPath;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_FileBrowser_ShowAndGetSource, session);
      vrp.push(API_BOOLEAN, &allowNetworkShares);
      vrp.push(API_STRING,   additionalShare.c_str());
      vrp.push(API_STRING,   strType.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &givenPath);
      if (retCode != API_SUCCESS)
        throw retCode;
      path = givenPath;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetImage(
            const std::string&      shares,
            const std::string&      heading,
            std::string&            path)
  {
    try
    {
      bool ret;
      char* givenPath;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_FileBrowser_ShowAndGetImage, session);
      vrp.push(API_STRING,   shares.c_str());
      vrp.push(API_STRING,   heading.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(session, vrp, &ret, &givenPath);
      if (retCode != API_SUCCESS)
        throw retCode;
      path = givenPath;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetImageList(
            const std::string&      shares,
            const std::string&      heading,
            std::vector<std::string> &path)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_FileBrowser_ShowAndGetImageList, session);
      vrp.push(API_STRING,   shares.c_str());
      vrp.push(API_STRING,   heading.c_str());

      bool ret;
      char* text;
      unsigned int length;
      uint32_t retCode;
      CLockObject lock(session->m_callMutex);
      std::unique_ptr<CResponsePacket> vresp(session->ReadResult(&vrp));
      if (!vresp)
        throw API_ERR_BUFFER;
      vresp->pop(API_UINT32_T,     &retCode);
      vresp->pop(API_BOOLEAN,      &ret);
      vresp->pop(API_UNSIGNED_INT, &length);
      for (unsigned int i = 0; i < length; ++i)
        path.push_back((char*)vresp->pop(API_STRING, &text));
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

}; /* extern "C" */
