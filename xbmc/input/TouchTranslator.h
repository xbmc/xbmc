/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "IButtonMapper.h"

#include <map>
#include <string>

class TiXmlElement;

class CTouchTranslator : public IButtonMapper
{
public:
  CTouchTranslator() = default;

  // implementation of IButtonMapper
  virtual void MapActions(int windowID, const TiXmlNode *bDevice) override;
  virtual void Clear() override;

  bool TranslateTouchAction(int window, int touchAction, int touchPointers, int &action, std::string &actionString);

private:
  bool TranslateAction(int window, unsigned int touchCommand, int touchPointers, unsigned int &actionId, std::string &actionString);

  struct CTouchAction
  {
    unsigned int actionId;
    std::string strAction; // Needed for "ActivateWindow()" type actions
  };

  using TouchActionKey = unsigned int;
  using TouchActionMap = std::map<TouchActionKey, CTouchAction>;

  using WindowID = int;
  using TouchMap = std::map<WindowID, TouchActionMap>;

  unsigned int GetActionID(WindowID window, TouchActionKey touchActionKey, std::string &actionString);

  static unsigned int TranslateTouchCommand(const TiXmlElement *pButton, CTouchAction &action);

  static unsigned int GetTouchActionKey(unsigned int touchCommandId, int touchPointers);

  TouchMap m_touchMap;
};
