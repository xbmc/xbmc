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
#include "ErrorCodeNames.h"
#include KITINCLUDE(ADDON_API_LEVEL, .internal/AddonLib_internal.hpp)

API_NAMESPACE

namespace KodiAPI
{
extern "C"
{

/*!
 * @brief Last error code.
 */
uint32_t KODI_API_lasterror = API_SUCCESS;

const KODI_API_ErrorTranslator errorTranslator[API_ERR_LASTCODE] =
{
  { API_SUCCESS,          "Successful return code" },
  { API_ERR_BUFFER,       "Invalid buffer pointer" },
  { API_ERR_COUNT,        "Invalid count argument" },
  { API_ERR_TYPE,         "Invalid datatype argument" },
  { API_ERR_TAG,          "Invalid tag argument" },
  { API_ERR_COMM,         "Invalid communicator" },
  { API_ERR_RANK,         "Invalid rank" },
  { API_ERR_ROOT,         "Invalid root" },
  { API_ERR_GROUP,        "Null group passed to function" },
  { API_ERR_OP,           "Invalid operation" },
  { API_ERR_TOPOLOGY,     "Invalid topology" },
  { API_ERR_DIMS,         "Illegal dimension argument" },
  { API_ERR_ARG,          "Invalid argument" },
  { API_ERR_UNKNOWN,      "Unknown error" },
  { API_ERR_TRUNCATE,     "message truncated on receive" },
  { API_ERR_OTHER,        "Other error; use Error_string" },
  { API_ERR_INTERN,       "internal error code" },
  { API_ERR_IN_STATUS,    "Look in status for error value" },
  { API_ERR_PENDING,      "Pending request" },
  { API_ERR_REQUEST,      "illegal API_request handle" },
  { API_ERR_CONNECTION,   "Failed to connect"},
  { API_ERR_VALUE_NOT_AVAILABLE, "Requested value not available" }
};

const char* KodiAPI_ErrorCodeToString(uint32_t code)
{
  if (code < API_ERR_LASTCODE)
    return errorTranslator[code].errorName;
  else
    return "Invalid Error code!";
}

} /* extern "C" */
} /* namespace KodiAPI */

END_NAMESPACE()
