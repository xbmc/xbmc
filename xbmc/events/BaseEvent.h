/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
