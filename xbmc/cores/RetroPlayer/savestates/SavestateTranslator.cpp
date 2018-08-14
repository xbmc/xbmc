/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateTranslator.h"
#include "SavestateDefines.h"

using namespace KODI;
using namespace RETRO;

SAVETYPE CSavestateTranslator::TranslateType(const std::string& type)
{
  if      (type == SAVESTATE_TYPE_AUTO)   return SAVETYPE::AUTO;
  else if (type == SAVESTATE_TYPE_SLOT)   return SAVETYPE::SLOT;
  else if (type == SAVESTATE_TYPE_MANUAL) return SAVETYPE::MANUAL;

  return SAVETYPE::UNKNOWN;
}

std::string CSavestateTranslator::TranslateType(const SAVETYPE& type)
{
  switch (type)
  {
    case SAVETYPE::AUTO:   return SAVESTATE_TYPE_AUTO;
    case SAVETYPE::SLOT:   return SAVESTATE_TYPE_SLOT;
    case SAVETYPE::MANUAL: return SAVESTATE_TYPE_MANUAL;
    default:
      break;
  }
  return SAVESTATE_TYPE_UNKNOWN;
}
