#pragma once
/*
 *  SAP-Announcement Support for XBMC
 *      Copyright (c) 2008 elupus (Joakim Plate)
 *      Copyright (C) 2008-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IDirectory.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"


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

  struct sdp_desc_origin
  {
    std::string username;
    int         sessionid;
    int         sessionver;
    std::string nettype;
    std::string addrtype;
    std::string address;
  };

  struct sdp_desc
  {
    std::string     version;
    std::string     origin;
    std::string     name;
    std::string     info;
    std::string     bandwidth;

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

    void clear()
    {
      version    = 0;
      addrtype   = 0;
      msgtype    = 0;
      encrypted  = 0;
      compressed = 0;
      authlen    = 0;
      msgid      = 0;
      origin.clear();
      payload_type.clear();
    }
  };
  int parse_sap(const char* data, struct sap_desc *h);
  int parse_sdp(const char* data, struct sdp_desc *sdp);
  int parse_sdp_origin(const char* data, struct sdp_desc_origin *o);
}





namespace XFILE     { class CSAPFile; }
namespace XFILE { class CSAPDirectory; }

class CSAPSessions
  : CThread
{
public:
  CSAPSessions();
  ~CSAPSessions();

  void StopThread(bool bWait = true);

protected:
  friend class XFILE::CSAPDirectory;
  friend class XFILE::CSAPFile;

  struct CSession
  {
    std::string  origin;
    int          msgid;
    unsigned int timeout;
    std::string  payload_origin;
    std::string  payload_type;
    std::string  payload;
    std::string  path;
  };

  std::vector<CSession> m_sessions;
  CCriticalSection      m_section;

private:
  void Process();
  bool ParseAnnounce(char* data, int len);
  SOCKET m_socket;
};

extern CSAPSessions g_sapsessions;



namespace XFILE
{

  class CSAPDirectory
    : public IDirectory
  {
  public:
    CSAPDirectory(void);
    virtual ~CSAPDirectory(void);
    virtual bool GetDirectory(const CURL& url, CFileItemList &items);
    virtual bool AllowAll() const { return true; }
  };

}
