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

#include "InterProcess_GUI_DialogSelect.h"
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

int CKODIAddon_InterProcess_GUI_DialogSelect::Dialogs_Select_Show(
          const std::string&              heading,
          const std::vector<std::string>& entries,
          int                             selected)
{
  try
  {
    int selected;
    uint32_t size = entries.size();
    CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
    CRequestPacket vrp(KODICall_GUI_Dialogs_Select_Show, session);
    vrp.push(API_STRING,    heading.c_str());
    vrp.push(API_INT,      &selected);
    vrp.push(API_UINT32_T, &size);
    for (uint32_t i = 0; i < size; ++i)
      vrp.push(API_STRING, entries[i].c_str());
    uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_INTEGER(session, vrp, &selected);
    if (retCode != API_SUCCESS)
      throw retCode;
    return selected;
  }
  PROCESS_HANDLE_EXCEPTION;
  return -1;
}

}; /* extern "C" */
