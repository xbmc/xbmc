#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://xbmc.org
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

#ifndef __PVRCLIENT_TVHEADEND_H__
#define __PVRCLIENT_TVHEADEND_H__

//#include <list>
//#include "FileSystem/HTSPDirectory.h"
#include "client.h"
//
//extern "C" {
//#include "lib/libhts/htsmsg.h"
//}

#define DEFAULT_HOST        "127.0.0.1"
#define DEFAULT_PORT        9982
//
//using namespace HTSP;

class cPVRClientTvheadend
{
  public:
    /* Class interface */
    cPVRClientTvheadend();
    ~cPVRClientTvheadend();

//    /* connect functions */
//    bool Connect(std::string sHostname = DEFAULT_HOST, int iPort = DEFAULT_PORT);
//    void Disconnect();
//    bool IsConnected();
//
//    /* Server handling */
//    PVR_ERROR GetProperties(PVR_SERVERPROPS *props);
//    PVR_ERROR GetStreamProperties(PVR_STREAMPROPS *props);
//
//    /* General handling */
//    const char* GetBackendName();
//    const char* GetBackendVersion();
//    PVR_ERROR   GetBackendTime(time_t *localTime, int *gmtOffset);
//    const char* GetConnectionString();
//
//    /* Channel handling */
//    int GetNumChannels();
//    int GetNumBouquets();
//    PVR_ERROR RequestChannelList(PVRHANDLE handle, bool radio = false);
//    PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end);
//
//    /* Live stream handling */
//    bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
//    void CloseLiveStream();
//    int ReadLiveStream(unsigned char* buf, int buf_size);
//
//    bool SwitchChannel(const PVR_CHANNEL &channelinfo);
//
//  private:
//    /* host session */
//    CURL                    m_url;
//    CHTSPDirectorySession*  m_pSession;
};
//
//inline bool cPVRClientTvheadend::IsConnected()
//{
//  return (m_pSession != NULL);
//}
//
//inline int cPVRClientTvheadend::GetNumChannels()
//{
//  return (int)(m_pSession->GetChannels().size());
//}
//
//inline int cPVRClientTvheadend::GetNumBouquets()
//{
//  return (int)(m_pSession->GetTags().size());
//}
//
//inline const char* cPVRClientTvheadend::GetConnectionString()
//{
//  return m_url.Get().c_str();
//}

#endif // __PVRCLIENT_TVHEADEND_H__

