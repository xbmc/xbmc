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

#if !defined(AFX_PVRClient_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_PVRClient_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "Key.h"
#include "DllPVRClient.h"

class CPVRClient
{
public:
  CPVRClient(struct PVRClient* pVisz, DllPVRClient* pDll, const CStdString& strPVRClientName);
  ~CPVRClient();

  void GetProps(PVR_SERVERPROPS *props);
  void GetSettings(std::vector<PVRSetting> **vecSettings);
  void UpdateSetting(int num);

  // some helper functions
  static CStdString GetFriendlyName(const char* strClient);

protected:
  std::auto_ptr<struct PVRClient> m_pClient;
  std::auto_ptr<DllPVRClient> m_pDll;
  CStdString m_strPVRClientName;
};


#endif // !defined(AFX_PVRClient_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
