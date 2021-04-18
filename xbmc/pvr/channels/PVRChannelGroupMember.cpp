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
  m_channelNumber = channelNumber;
}

void CPVRChannelGroupMember::SetClientChannelNumber(const CPVRChannelNumber& clientChannelNumber)
{
  m_clientChannelNumber = clientChannelNumber;
}

void CPVRChannelGroupMember::SetClientPriority(int iClientPriority)
{
  m_iClientPriority = iClientPriority;
}

void CPVRChannelGroupMember::SetOrder(int iOrder)
{
  m_iOrder = iOrder;
}
