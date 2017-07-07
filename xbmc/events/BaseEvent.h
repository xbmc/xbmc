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

#include "XBDateTime.h"
#include "events/IEvent.h"
#include "utils/Variant.h"

class CBaseEvent : public IEvent
{
public:
  ~CBaseEvent() override = default;

  std::string GetIdentifier() const override { return m_identifier; }
  EventLevel GetLevel() const override { return m_level; }
  std::string GetLabel() const override;
  std::string GetIcon() const override { return m_icon; }
  std::string GetDescription() const override;
  std::string GetDetails() const override;
  std::string GetExecutionLabel() const override;
  CDateTime GetDateTime() const override { return m_dateTime; }

  bool CanExecute() const override { return !GetExecutionLabel().empty(); }

  void ToSortable(SortItem& sortable, Field field) const override;

protected:
  CBaseEvent(const std::string& identifier, const CVariant& label, const CVariant& description, EventLevel level = EventLevel::Information);
  CBaseEvent(const std::string& identifier, const CVariant& label, const CVariant& description, const std::string& icon, EventLevel level = EventLevel::Information);
  CBaseEvent(const std::string& identifier, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, EventLevel level = EventLevel::Information);
  CBaseEvent(const std::string& identifier, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, const CVariant& executionLabel, EventLevel level = EventLevel::Information);

  EventLevel m_level;
  std::string m_identifier;
  std::string m_icon;
  CVariant m_label;
  CVariant m_description;
  CVariant m_details;
  CVariant m_executionLabel;

private:
  static std::string VariantToLocalizedString(const CVariant& variant);
  static uint64_t GetInternalTimestamp();

  uint64_t m_timestamp; // high res internal time stamp
  CDateTime m_dateTime; // user interface time stamp
};
