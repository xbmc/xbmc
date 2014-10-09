/* 
 *  Copyright (C) 2010-2013 Eduard Kytmanov
 *  http://www.avmedia.su
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_DS_PLAYER

#include "DSInputStreamPVRManager.h"
#include "URL.h"

#include "filesystem/PVRFile.h "
#include "pvr/addons/PVRClients.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

CDSInputStreamPVRManager* g_pPVRStream = NULL;

CDSInputStreamPVRManager::CDSInputStreamPVRManager(CDSPlayer *pPlayer)
	: m_pPlayer(pPlayer)
	, m_pFile(NULL)
	, m_pLiveTV(NULL)
	, m_pRecordable(NULL)
{
}


CDSInputStreamPVRManager::~CDSInputStreamPVRManager(void)
{
	Close();
}

void CDSInputStreamPVRManager::Close()
{
	if(m_pFile)
	{
		m_pFile->Close();
		SAFE_DELETE(m_pFile);
	}

	m_pLiveTV = NULL;
	m_pRecordable = NULL;
}

bool CDSInputStreamPVRManager::Open(const CFileItem& file)
{
	Close();

	m_pFile = new CPVRFile;

	CURL url(file.GetPath());
	if (!m_pFile->Open(url))
	{
		SAFE_DELETE(m_pFile);
		return false;
	}

	m_pLiveTV     = ((CPVRFile*)m_pFile)->GetLiveTV();
	m_pRecordable = ((CPVRFile*)m_pFile)->GetRecordable();

	std::string transFile = XFILE::CPVRFile::TranslatePVRFilename(file.GetPath());
	CFileItem fileItem = file;
	fileItem.SetPath(transFile);
	
	return m_pPlayer->OpenFileInternal(fileItem);
}

bool CDSInputStreamPVRManager::SelectChannel(const CPVRChannel &channel)
{
	if (!SupportsChannelSwitch())
	{
		CFileItem item(channel);
		return Open(item);
	}
	else if(m_pLiveTV)
	{
		return m_pLiveTV->SelectChannel(channel.ChannelNumber());
	}

	return false;
}

bool CDSInputStreamPVRManager::NextChannel(bool preview /* = false */)
{
	PVR_CLIENT client;
	if (!preview && !SupportsChannelSwitch())
	{
		CPVRChannelPtr channel;
		g_PVRManager.GetCurrentChannel(channel);
		CFileItemPtr item = g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelUp(*channel);
		if (item.get())
			return Open(*item.get());
	}
	else if (m_pLiveTV)
		return m_pLiveTV->PrevChannel(preview);

	return false;
}

bool CDSInputStreamPVRManager::PrevChannel(bool preview/* = false*/)
{
	PVR_CLIENT client;
	if (!preview && !SupportsChannelSwitch())
	{
		CPVRChannelPtr channel;
		g_PVRManager.GetCurrentChannel(channel);
		CFileItemPtr item = g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelDown(*channel);
		if (item.get())
			return Open(*item.get());
	}
	else if (m_pLiveTV)
		return m_pLiveTV->PrevChannel(preview);
	return false;
}

bool CDSInputStreamPVRManager::SelectChannelByNumber(unsigned int iChannelNumber)
{
	PVR_CLIENT client;
	if (!SupportsChannelSwitch())
	{
		CPVRChannelPtr channel;
		g_PVRManager.GetCurrentChannel(channel);
		CFileItemPtr item = g_PVRChannelGroups->Get(channel->IsRadio())->GetSelectedGroup()->GetByChannelNumber(iChannelNumber);
		if (item.get())
			return Open(*item.get());
	}
	else if (m_pLiveTV)
		return m_pLiveTV->SelectChannel(iChannelNumber);

	return false;
}

bool CDSInputStreamPVRManager::SupportsChannelSwitch()const
{
	PVR_CLIENT client;
	return g_PVRClients->GetPlayingClient(client) && client->HandlesInputStream();}

bool CDSInputStreamPVRManager::UpdateItem(CFileItem& item)
{
	if (m_pLiveTV)
		return m_pLiveTV->UpdateItem(item);
	return false;
}

uint64_t CDSInputStreamPVRManager::GetTotalTime()
{
	if (m_pLiveTV)
		return m_pLiveTV->GetTotalTime();
	return 0;
}

uint64_t CDSInputStreamPVRManager::GetTime()
{
	if (m_pLiveTV)
		return m_pLiveTV->GetStartTime();
	return 0;
}

#endif
