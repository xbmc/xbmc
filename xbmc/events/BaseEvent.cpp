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

#include "BaseEvent.h"
#include "guilib/LocalizeStrings.h"

CBaseEvent::CBaseEvent(const std::string& identifier, const CVariant& label, const CVariant& description, EventLevel level /* = EventLevelInformation */)
  : m_level(level),
    m_identifier(identifier),
    m_icon(),
    m_label(label),
    m_description(description),
    m_details(),
    m_executionLabel(),
    m_dateTime(CDateTime::GetCurrentDateTime())
{ }

CBaseEvent::CBaseEvent(const std::string& identifier, const CVariant& label, const CVariant& description, const std::string& icon, EventLevel level /* = EventLevelInformation */)
  : m_level(level),
    m_identifier(identifier),
    m_icon(icon),
    m_label(label),
    m_description(description),
    m_details(),
    m_executionLabel(),
    m_dateTime(CDateTime::GetCurrentDateTime())
{ }

CBaseEvent::CBaseEvent(const std::string& identifier, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, EventLevel level /* = EventLevelInformation */)
  : m_level(level),
    m_identifier(identifier),
    m_icon(icon),
    m_label(label),
    m_description(description),
    m_details(details),
    m_executionLabel(),
    m_dateTime(CDateTime::GetCurrentDateTime())
{ }

CBaseEvent::CBaseEvent(const std::string& identifier, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, const CVariant& executionLabel, EventLevel level /* = EventLevelInformation */)
  : m_level(level),
    m_identifier(identifier),
    m_icon(icon),
    m_label(label),
    m_description(description),
    m_details(details),
    m_executionLabel(executionLabel),
    m_dateTime(CDateTime::GetCurrentDateTime())
{ }

std::string CBaseEvent::GetLabel() const
{
  return VariantToLocalizedString(m_label);
}

std::string CBaseEvent::GetDescription() const
{
  return VariantToLocalizedString(m_description);
}

std::string CBaseEvent::GetDetails() const
{
  return VariantToLocalizedString(m_details);
}

std::string CBaseEvent::GetExecutionLabel() const
{
  return VariantToLocalizedString(m_executionLabel);
}

std::string CBaseEvent::VariantToLocalizedString(const CVariant& variant)
{
  if (variant.isString())
    return variant.asString();

  if (variant.isInteger() && variant.asInteger() > 0)
    return g_localizeStrings.Get(static_cast<uint32_t>(variant.asInteger()));
  if (variant.isUnsignedInteger() && variant.asUnsignedInteger() > 0)
    return g_localizeStrings.Get(static_cast<uint32_t>(variant.asUnsignedInteger()));

  return "";
}
