#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "events/AddonEvent.h"

class CAddonManagementEvent : public CAddonEvent
{
public:
  CAddonManagementEvent(ADDON::AddonPtr addon, const CVariant& description);
  CAddonManagementEvent(ADDON::AddonPtr addon, const CVariant& description, const CVariant& details);
  CAddonManagementEvent(ADDON::AddonPtr addon, const CVariant& description, const CVariant& details, const CVariant& executionLabel);
  CAddonManagementEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description);
  CAddonManagementEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description, const CVariant& details);
  CAddonManagementEvent(ADDON::AddonPtr addon, EventLevel level, const CVariant& description, const CVariant& details, const CVariant& executionLabel);
  virtual ~CAddonManagementEvent() { }

  virtual const char* GetType() const { return "AddonManagementEvent"; }
  virtual std::string GetExecutionLabel() const;

  virtual bool CanExecute() const { return m_addon != NULL; }
  virtual bool Execute() const;
};
