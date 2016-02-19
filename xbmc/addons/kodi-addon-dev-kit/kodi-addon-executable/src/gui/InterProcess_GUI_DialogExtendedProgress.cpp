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

#include "InterProcess_GUI_DialogExtendedProgress.h"
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

  GUIHANDLE CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_New(const std::string& title)
  {
    try
    {
      uint64_t retPtr;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_New, session);
      vrp.push(API_STRING,    title.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_POINTER(session, vrp, &retPtr);
      if (retCode != API_SUCCESS)
        throw retCode;
      void* ptr = (void*)retPtr;
      return ptr;
    }
    PROCESS_HANDLE_EXCEPTION;
    return nullptr;
  }

  void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Delete(GUIHANDLE handle)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_Delete, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  std::string CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Title(GUIHANDLE handle) const
  {
    try
    {
      char* title;
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_Title, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, title);
      if (retCode != API_SUCCESS)
        throw retCode;
      return title;
    }
    PROCESS_HANDLE_EXCEPTION;
    return "";
  }

  void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetTitle(GUIHANDLE handle, const std::string& title)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_SetTitle, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_STRING,    title.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  std::string CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Text(GUIHANDLE handle) const
  {
    try
    {
      char* text;
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_Text, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, text);
      if (retCode != API_SUCCESS)
        throw retCode;
      return text;
    }
    PROCESS_HANDLE_EXCEPTION;
    return "";
  }

  void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetText(GUIHANDLE handle, const std::string& text)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_SetText, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_STRING,    text.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  bool CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_IsFinished(GUIHANDLE handle) const
  {
    try
    {
      bool ret;
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_IsFinished, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_MarkFinished(GUIHANDLE handle)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_MarkFinished, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  float CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Percentage(GUIHANDLE handle) const
  {
    try
    {
      float percentage;
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_SetPercentage, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_FLOAT(session, vrp, &percentage);
      if (retCode != API_SUCCESS)
        throw retCode;
      return percentage;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0.0f;
  }

  void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetPercentage(GUIHANDLE handle, float fPercentage)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_SetPercentage, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_FLOAT,    &fPercentage);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetProgress(GUIHANDLE handle, int currentItem, int itemCount)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_ExtendedProgress_SetProgress, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_INT,      &currentItem);
      vrp.push(API_INT,      &itemCount);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

}; /* extern "C" */
