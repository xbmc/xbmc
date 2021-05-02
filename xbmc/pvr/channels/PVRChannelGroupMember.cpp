/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupMember.h"

using namespace PVR;

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
