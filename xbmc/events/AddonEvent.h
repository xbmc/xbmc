#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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

#include "events/UniqueEvent.h"
#include "addons/IAddon.h"

class CAddonEvent : public CUniqueEvent
{
public:
  CAddonEvent(ADDON::AddonPtr addon, const CVariant& description);
  CAddonEvent(ADDON::AddonPtr addon, const CVariant& description, const CVariant& details);
  CAddonEvent(ADDON::AddonPtr addon, const CVariant& description, const CVariant& details, const CVariant& executionLabel);
  CAddonEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description);
  CAddonEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description, const CVariant& details);
  CAddonEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description, const CVariant& details, const CVariant& executionLabel);
  ~CAddonEvent() override = default;

  const char* GetType() const override { return "AddonEvent"; }

protected:
  ADDON::AddonPtr m_addon;
};
