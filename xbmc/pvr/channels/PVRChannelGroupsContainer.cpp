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

CPVRChannelGroupsContainer::CPVRChannelGroupsContainer(void)
{

  m_groupsRadio = new CPVRChannelGroups(true);
  m_groupsTV    = new CPVRChannelGroups(false);
}

CPVRChannelGroupsContainer::~CPVRChannelGroupsContainer(void)
{
  delete m_groupsRadio;
  delete m_groupsTV;
}

bool CPVRChannelGroupsContainer::Update(void)
{
  return m_groupsRadio->Update() &&
         m_groupsTV->Update();
}

bool CPVRChannelGroupsContainer::Load(void)
{
  Unload();

  return m_groupsRadio->Load() &&
         m_groupsTV->Load();
}

void CPVRChannelGroupsContainer::Unload(void)
{
  m_groupsRadio->Clear();
  m_groupsTV->Clear();
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

const CPVRChannelGroup *CPVRChannelGroupsContainer::GetById(bool bRadio, int iGroupId) const
{
  const CPVRChannelGroup *group = NULL;
  const CPVRChannelGroups *groups = Get(bRadio);
  if (groups)
    group = (iGroupId == XBMC_INTERNAL_GROUPID) ?
        groups->GetGroupAll() :
        groups->GetById(iGroupId);

  return group;
}

const CPVRChannel *CPVRChannelGroupsContainer::GetChannelById(int iChannelId) const
{
  const CPVRChannel *channel = m_groupsTV->GetGroupAll()->GetByChannelID(iChannelId);

  if (!channel)
    channel = m_groupsRadio->GetGroupAll()->GetByChannelID(iChannelId);

  return channel;
}
