/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupMember.h"

#include "utils/DatabaseUtils.h"
#include "utils/SortUtils.h"
#include "utils/Variant.h"

using namespace PVR;

void CPVRChannelGroupMember::Serialize(CVariant& value) const
{
  value["channelnumber"] = m_channelNumber.GetChannelNumber();
  value["subchannelnumber"] = m_channelNumber.GetSubChannelNumber();
}

void CPVRChannelGroupMember::ToSortable(SortItem& sortable, Field field) const
{
  if (field == FieldChannelNumber)
  {
    sortable[FieldChannelNumber] = m_channelNumber.SortableChannelNumber();
  }
  else if (field == FieldClientChannelOrder)
  {
    if (m_iOrder)
      sortable[FieldClientChannelOrder] = m_iOrder;
    else
      sortable[FieldClientChannelOrder] = m_clientChannelNumber.SortableChannelNumber();
  }
}

void CPVRChannelGroupMember::SetChannelNumber(const CPVRChannelNumber& channelNumber)
{
  if (m_channelNumber != channelNumber)
  {
    m_channelNumber = channelNumber;
    m_bChanged = true;
  }
}

void CPVRChannelGroupMember::SetClientChannelNumber(const CPVRChannelNumber& clientChannelNumber)
{
  if (m_clientChannelNumber != clientChannelNumber)
  {
    m_clientChannelNumber = clientChannelNumber;
    m_bChanged = true;
  }
}

void CPVRChannelGroupMember::SetClientPriority(int iClientPriority)
{
  if (m_iClientPriority != iClientPriority)
  {
    m_iClientPriority = iClientPriority;
    m_bChanged = true;
  }
}

void CPVRChannelGroupMember::SetOrder(int iOrder)
{
  if (m_iOrder != iOrder)
  {
    m_iOrder = iOrder;
    m_bChanged = true;
  }
}
