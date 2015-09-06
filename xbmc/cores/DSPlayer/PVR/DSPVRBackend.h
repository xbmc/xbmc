#pragma once
/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
*
*      Copyright (C) 2015 Romank
*      https://github.com/Romank1
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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "utils/log.h"
#include "filesystem\File.h"
#include "threads/CriticalSection.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/DSFileUtils.h"
#include "pvr/PVRManager.h"
#include "DSSocket.h"
#include "URL.h"
#include "utils/URIUtils.h"

using XFILE::CFile;

/* Http Request Method type*/
enum HttpRequestMethod {
    REQUEST_GET,
    REQUEST_POST
};

#ifdef TARGET_WINDOWS // disable 4355: 'this' used in base member initializer list
#pragma warning(push)
#pragma warning(disable: 4355)
#endif

class CDSPVRBackend
{
public:
  CDSPVRBackend(const CStdString& strBackendBaseAddress, const CStdString& strBackendName);
  virtual ~CDSPVRBackend();
  virtual bool        ConvertStreamURLToTimeShiftFilePath(const CStdString& strUrl, CStdString& strTimeShiftFile) = 0;
  virtual bool        SupportsStreamConversion(const CStdString& strUrl) const = 0;
  virtual bool        SupportsFastChannelSwitch() const = 0;
  virtual bool        GetRecordingStreamURL(const CStdString& strRecordingId, CStdString& strRecordingUrl, bool bGetUNCPath) { return false; };
  
  const CStdString&   GetBackendName() const { return m_strBackendName; };

protected:
  bool                JSONRPCSendCommand(HttpRequestMethod requestType, const CStdString& strCommand, const CStdString& strArguments, CVariant &json_response);
  bool                TCPClientIsConnected();
  bool                TCPClientConnect();
  bool                TCPClientDisconnect();
  bool                TCPClientSendCommand(const CStdString& strCommand, CStdString & strResponse);
  bool                IsFileExistAndAccessible(const CStdString& strFilePath);
  bool                ResolveHostName(const CStdString& strUrl, CStdString& strResolvedUrl);
                    
private:              
  bool                HttpRequestGET(const CStdString& strCommand, CStdString& strResponse);
  bool                HttpRequestPOST(const CStdString& strCommand, const CStdString& strArguments, CStdString& strResponse);
  
  CStdString          m_strBaseURL;
  CStdString          m_strBackendName;
  CDSSocket          *m_tcpclient;
  CCriticalSection    m_ObjectLock;
};

