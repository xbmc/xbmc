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

#include "InterProcess_GUI_DialogYesNo.h"
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

bool CKODIAddon_InterProcess_GUI_DialogYesNo::Dialogs_YesNo_ShowAndGetInputSingleText(
          const std::string&      heading,
          const std::string&      text,
          bool&                   bCanceled,
          const std::string&      noLabel,
          const std::string&      yesLabel)
{
  try
  {
    bool ret = false;
    CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
    CRequestPacket vrp(KODICall_GUI_Dialogs_YesNo_ShowAndGetInputSingleText, session);
    vrp.push(API_STRING,  heading.c_str());
    vrp.push(API_STRING,  text.c_str());
    vrp.push(API_STRING,  noLabel.c_str());
    vrp.push(API_STRING,  yesLabel.c_str());
    uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_TWO_BOOLEAN(session, vrp, &ret, &bCanceled);
    if (retCode != API_SUCCESS)
      throw retCode;
    return ret;
  }
  PROCESS_HANDLE_EXCEPTION;
  return false;
}

bool CKODIAddon_InterProcess_GUI_DialogYesNo::Dialogs_YesNo_ShowAndGetInputLineText(
          const std::string&      heading,
          const std::string&      line0,
          const std::string&      line1,
          const std::string&      line2,
          const std::string&      noLabel,
          const std::string&      yesLabel)
{
  try
  {
    bool ret = false;
    CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
    CRequestPacket vrp(KODICall_GUI_Dialogs_YesNo_ShowAndGetInputLineText, session);
    vrp.push(API_STRING,  heading.c_str());
    vrp.push(API_STRING,  line0.c_str());
    vrp.push(API_STRING,  line1.c_str());
    vrp.push(API_STRING,  line2.c_str());
    vrp.push(API_STRING,  noLabel.c_str());
    vrp.push(API_STRING,  yesLabel.c_str());
    uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &ret);
    if (retCode != API_SUCCESS)
      throw retCode;
    return ret;
  }
  PROCESS_HANDLE_EXCEPTION;
  return false;
}

bool CKODIAddon_InterProcess_GUI_DialogYesNo::Dialogs_YesNo_ShowAndGetInputLineButtonText(
          const std::string&      heading,
          const std::string&      line0,
          const std::string&      line1,
          const std::string&      line2,
          bool&                   bCanceled,
          const std::string&      noLabel,
          const std::string&      yesLabel)
{
  try
  {
    bool ret = false;
    CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
    CRequestPacket vrp(KODICall_GUI_Dialogs_YesNo_ShowAndGetInputLineButtonText, session);
    vrp.push(API_STRING,  heading.c_str());
    vrp.push(API_STRING,  line0.c_str());
    vrp.push(API_STRING,  line1.c_str());
    vrp.push(API_STRING,  line2.c_str());
    vrp.push(API_STRING,  noLabel.c_str());
    vrp.push(API_STRING,  yesLabel.c_str());
    uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_TWO_BOOLEAN(session, vrp, &ret, &bCanceled);
    if (retCode != API_SUCCESS)
      throw retCode;
    return ret;
  }
  PROCESS_HANDLE_EXCEPTION;
  return false;
}

}; /* extern "C" */
