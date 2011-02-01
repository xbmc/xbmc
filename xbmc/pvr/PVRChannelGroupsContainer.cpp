/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PVRChannelGroupsContainer.h"

CPVRChannelGroupsContainer g_PVRChannelGroups;

CPVRChannelGroupsContainer::CPVRChannelGroupsContainer(void)
{
}

CPVRChannelGroupsContainer::~CPVRChannelGroupsContainer(void)
{
  Unload();
}

bool CPVRChannelGroupsContainer::Update(void)
{
  return m_groupsRadio->Update() &&
         m_groupsTV->Update();
}

bool CPVRChannelGroupsContainer::Load(void)
{
  Unload();

  m_groupsRadio = new CPVRChannelGroups(true);
  m_groupsTV    = new CPVRChannelGroups(false);

  return m_groupsRadio->Load() &&
         m_groupsTV->Load();
}

void CPVRChannelGroupsContainer::Unload(void)
{
  if (m_groupsRadio)
  {
    delete m_groupsRadio;
    m_groupsRadio = NULL;
  }

  if (m_groupsTV)
  {
    delete m_groupsTV;
    m_groupsTV = NULL;
  }
}

const CPVRChannelGroups *CPVRChannelGroupsContainer::Get(bool bRadio) const
{
  return bRadio ? m_groupsRadio : m_groupsTV;
}

const CPVRChannelGroup *CPVRChannelGroupsContainer::GetGroupAll(bool bRadio) const
{
  const CPVRChannelGroup *group = NULL;

  const CPVRChannelGroups *groups = Get(bRadio);
  if (groups)
    group = groups->GetGroupAll();

  return group;
}

const CPVRChannel *CPVRChannelGroupsContainer::GetChannelById(int iChannelId) const
{
  const CPVRChannel *channel = m_groupsTV->GetGroupAll()->GetByChannelID(iChannelId);

  if (!channel)
    channel = m_groupsRadio->GetGroupAll()->GetByChannelID(iChannelId);

  return channel;
}
