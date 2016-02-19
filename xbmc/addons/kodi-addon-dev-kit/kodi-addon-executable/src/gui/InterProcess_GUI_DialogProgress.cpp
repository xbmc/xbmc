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

#include "InterProcess_GUI_DialogProgress.h"
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

  GUIHANDLE CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_New()
  {
    try
    {
      uint64_t retPtr;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_New, session);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_POINTER(session, vrp, &retPtr);
      if (retCode != API_SUCCESS)
        throw retCode;
      void* ptr = (void*)retPtr;
      return ptr;
    }
    PROCESS_HANDLE_EXCEPTION;
    return nullptr;
  }

  void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_Delete(GUIHANDLE handle)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_Delete, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_Open(GUIHANDLE handle)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_Open, session);
      vrp.push(API_UINT64_T, &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetHeading(GUIHANDLE handle, const std::string& title)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_SetHeading, session);
      vrp.push(API_UINT64_T, &ptr);
      vrp.push(API_STRING,    title.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetLine(GUIHANDLE handle, unsigned int iLine, const std::string& line)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_SetLine, session);
      vrp.push(API_UINT64_T,     &ptr);
      vrp.push(API_UNSIGNED_INT, &iLine);
      vrp.push(API_STRING,        line.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetCanCancel(GUIHANDLE handle, bool bCanCancel)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_SetCanCancel, session);
      vrp.push(API_UINT64_T,   &ptr);
      vrp.push(API_BOOLEAN,    &bCanCancel);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  bool CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_IsCanceled(GUIHANDLE handle) const
  {
    try
    {
      bool ret;
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_IsCanceled, session);
      vrp.push(API_UINT64_T,   &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetPercentage(GUIHANDLE handle, int iPercentage)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_SetPercentage, session);
      vrp.push(API_UINT64_T,   &ptr);
      vrp.push(API_INT,        &iPercentage);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  int CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_GetPercentage(GUIHANDLE handle) const
  {
    try
    {
      int ret;
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_GetPercentage, session);
      vrp.push(API_UINT64_T,   &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_INTEGER(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return -1;
  }

  void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_ShowProgressBar(GUIHANDLE handle, bool bOnOff)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_ShowProgressBar, session);
      vrp.push(API_UINT64_T,   &ptr);
      vrp.push(API_BOOLEAN,    &bOnOff);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetProgressMax(GUIHANDLE handle, int iMax)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_SetProgressMax, session);
      vrp.push(API_UINT64_T,   &ptr);
      vrp.push(API_INT,        &iMax);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_SetProgressAdvance(GUIHANDLE handle, int nSteps)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_SetProgressAdvance, session);
      vrp.push(API_UINT64_T,   &ptr);
      vrp.push(API_INT,        &nSteps);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  bool CKODIAddon_InterProcess_GUI_DialogProgress::Dialogs_Progress_Abort(GUIHANDLE handle)
  {
    try
    {
      bool ret;
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_GUI_Dialogs_Progress_Abort, session);
      vrp.push(API_UINT64_T,   &ptr);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

}; /* extern "C" */
