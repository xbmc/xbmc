#pragma once
/*
* SAP-Announcement Support for XBMC
* Copyright (c) 2004 Forza (Chris Barnett)
* Portions Copyright (c) by the authors of libOpenDAAP
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "IDirectory.h"
#include "utils/Thread.h"



namespace SDP
{
  struct sdp_desc_time
  {
    std::string active;
    std::string repeat;
  };

  struct sdp_desc_media
  {
    std::string name;
    std::string title;
    std::string connection;

    std::vector<std::string> attributes;
  };

  struct sdp_desc
  {
    std::string version;
    std::string origin;
    std::string name;
    std::string title;
    std::string bandwidth;

    std::vector<std::string>    attributes;
    std::vector<sdp_desc_time>  times;
    std::vector<sdp_desc_media> media;
  };

  struct sap_desc
  {
    int version;
    int addrtype;
    int msgtype;
    int encrypted;
    int compressed;
    int authlen;
    int msgid;

    std::string origin;
    std::string payload_type;
  };
  int parse_sap(const char* data, int len, struct sap_desc *h);
  int parse_sdp(const char* data, int len, struct sdp_desc *sdp);
}

namespace DIRECTORY
{


class CSAPSessions
  : CThread
{
public:
  CSAPSessions();
  ~CSAPSessions();

protected:
  friend class CSAPDirectory;
  friend class CSAPFile;

  struct CSession
  {
    std::string origin;
    int         msgid;
    DWORD       timeout;
    std::string payload_origin;
    std::string payload_type;
    std::string payload;
  };

  std::vector<CSession> m_sessions;
  CCriticalSection      m_section;

private:
  void Process();
  bool ParseAnnounce(char* data, int len);

};


class CSAPDirectory
  : public IDirectory
{
public:
  CSAPDirectory(void);
  virtual ~CSAPDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
};



extern CSAPSessions g_sapsessions;


}