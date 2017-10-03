/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "MouseTranslator.h"
#include "MouseStat.h"
#include "input/Key.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <string>

using namespace KODI;
using namespace MOUSE;

namespace
{

using ActionName = std::string;
using KeyID = uint32_t;

static const std::map<ActionName, KeyID> MouseKeys =
{
    { "click"                    , KEY_MOUSE_CLICK },
    { "leftclick"                , KEY_MOUSE_CLICK },
    { "rightclick"               , KEY_MOUSE_RIGHTCLICK },
    { "middleclick"              , KEY_MOUSE_MIDDLECLICK },
    { "doubleclick"              , KEY_MOUSE_DOUBLE_CLICK },
    { "longclick"                , KEY_MOUSE_LONG_CLICK },
    { "wheelup"                  , KEY_MOUSE_WHEEL_UP },
    { "wheeldown"                , KEY_MOUSE_WHEEL_DOWN },
    { "mousemove"                , KEY_MOUSE_MOVE },
    { "mousedrag"                , KEY_MOUSE_DRAG },
    { "mousedragstart"           , KEY_MOUSE_DRAG_START },
    { "mousedragend"             , KEY_MOUSE_DRAG_END },
    { "mouserdrag"               , KEY_MOUSE_RDRAG },
    { "mouserdragstart"          , KEY_MOUSE_RDRAG_START },
    { "mouserdragend"            , KEY_MOUSE_RDRAG_END }
};

} // anonymous namespace

uint32_t CMouseTranslator::TranslateCommand(const TiXmlElement *pButton)
{
  uint32_t buttonId = 0;

  if (pButton != nullptr)
  {
    std::string szKey = pButton->ValueStr();
    if (!szKey.empty())
    {
      StringUtils::ToLower(szKey);

      auto it = MouseKeys.find(szKey);
      if (it != MouseKeys.end())
        buttonId = it->second;

      if (buttonId == 0)
      {
        CLog::Log(LOGERROR, "Unknown mouse action (%s), skipping", pButton->Value());
      }
      else
      {
        int id = 0;
        if ((pButton->QueryIntAttribute("id", &id) == TIXML_SUCCESS))
        {
          if (0 <= id && id < MOUSE_MAX_BUTTON)
            buttonId += id;
        }
      }
    }
  }

  return buttonId;
}

bool CMouseTranslator::TranslateEventID(unsigned int eventId, BUTTON_ID &buttonId)
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
