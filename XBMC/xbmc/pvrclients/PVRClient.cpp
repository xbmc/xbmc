/*
*      Copyright (C) 2005-2009 Team XBMC
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
#include "stdafx.h"
#include <vector>

// PVRClient.cpp: implementation of the CPVRClient class.
//
//////////////////////////////////////////////////////////////////////

#include "PVRClient.h"
#include "pvrclients/PVRClientTypes.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPVRClient::CPVRClient(struct PVRClient* pClient, DllPVRClient* pDll,
                               const CStdString& strPVRClientName)
                               : m_pClient(pClient)
                               , m_pDll(pDll)
                               , m_strPVRClientName(strPVRClientName)
{}

CPVRClient::~CPVRClient()
{
}

void CPVRClient::GetProps(PVR_SERVERPROPS *props)
{
  // get info from vis
  m_pClient->GetProps(props);
}

void CPVRClient::GetSettings(std::vector<PVRSetting> **vecSettings)
{
  if (vecSettings) *vecSettings = NULL;
  if (m_pClient->GetSettings)
    m_pClient->GetSettings(vecSettings);
}

void CPVRClient::UpdateSetting(int num)
{
  if (m_pClient->UpdateSetting)
    m_pClient->UpdateSetting(num);
}