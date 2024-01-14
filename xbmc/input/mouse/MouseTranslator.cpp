/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MouseTranslator.h"

#include "MouseStat.h"
#include "input/keyboard/KeyIDs.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <map>
#include <string>

#include <tinyxml2.h>

using namespace KODI;
using namespace MOUSE;

namespace
{

using ActionName = std::string;
using KeyID = uint32_t;

static const std::map<ActionName, KeyID> MouseKeys = {{"click", KEY_MOUSE_CLICK},
                                                      {"leftclick", KEY_MOUSE_CLICK},
                                                      {"rightclick", KEY_MOUSE_RIGHTCLICK},
                                                      {"middleclick", KEY_MOUSE_MIDDLECLICK},
                                                      {"doubleclick", KEY_MOUSE_DOUBLE_CLICK},
                                                      {"longclick", KEY_MOUSE_LONG_CLICK},
                                                      {"wheelup", KEY_MOUSE_WHEEL_UP},
                                                      {"wheeldown", KEY_MOUSE_WHEEL_DOWN},
                                                      {"mousemove", KEY_MOUSE_MOVE},
                                                      {"mousedrag", KEY_MOUSE_DRAG},
                                                      {"mousedragstart", KEY_MOUSE_DRAG_START},
                                                      {"mousedragend", KEY_MOUSE_DRAG_END},
                                                      {"mouserdrag", KEY_MOUSE_RDRAG},
                                                      {"mouserdragstart", KEY_MOUSE_RDRAG_START},
                                                      {"mouserdragend", KEY_MOUSE_RDRAG_END}};

} // anonymous namespace

uint32_t CMouseTranslator::TranslateCommand(const tinyxml2::XMLElement* pButton)
{
  uint32_t buttonId = 0;

  if (pButton != nullptr)
  {
    std::string szKey = pButton->Value();
    if (!szKey.empty())
    {
      StringUtils::ToLower(szKey);

      auto it = MouseKeys.find(szKey);
      if (it != MouseKeys.end())
        buttonId = it->second;

      if (buttonId == 0)
      {
        CLog::Log(LOGERROR, "Unknown mouse action ({}), skipping", pButton->Value());
      }
      else
      {
        int id = 0;
        if ((pButton->QueryIntAttribute("id", &id) == tinyxml2::XML_SUCCESS))
        {
          if (0 <= id && id < MOUSE_MAX_BUTTON)
            buttonId += id;
        }
      }
    }
  }

  return buttonId;
}

bool CMouseTranslator::TranslateEventID(unsigned int eventId, BUTTON_ID& buttonId)
{
  switch (eventId)
  {
    case XBMC_BUTTON_LEFT:
    {
      buttonId = BUTTON_ID::LEFT;
      return true;
    }
    case XBMC_BUTTON_MIDDLE:
    {
      buttonId = BUTTON_ID::MIDDLE;
      return true;
    }
    case XBMC_BUTTON_RIGHT:
    {
      buttonId = BUTTON_ID::RIGHT;
      return true;
    }
    case XBMC_BUTTON_WHEELUP:
    {
      buttonId = BUTTON_ID::WHEEL_UP;
      return true;
    }
    case XBMC_BUTTON_WHEELDOWN:
    {
      buttonId = BUTTON_ID::WHEEL_DOWN;
      return true;
    }
    case XBMC_BUTTON_X1:
    {
      buttonId = BUTTON_ID::BUTTON4;
      return true;
    }
    case XBMC_BUTTON_X2:
    {
      buttonId = BUTTON_ID::BUTTON5;
      return true;
    }
    case XBMC_BUTTON_X3:
    {
      buttonId = BUTTON_ID::HORIZ_WHEEL_LEFT;
      return true;
    }
    case XBMC_BUTTON_X4:
    {
      buttonId = BUTTON_ID::HORIZ_WHEEL_RIGHT;
      return true;
    }
    default:
      break;
  }

  return false;
}
