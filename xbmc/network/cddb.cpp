/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#ifdef HAS_DVD_DRIVE

#include <taglib/id3v1genres.h>
#include "cddb.h"
#include "CompileInfo.h"
#include "network/DNSNameCache.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "filesystem/File.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"

#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace MEDIA_DETECT;
using namespace CDDB;

//-------------------------------------------------------------------------------------------------------------------
Xcddb::Xcddb()
#if defined(TARGET_WINDOWS)
    : m_cddb_socket(closesocket, INVALID_SOCKET)
#else
    : m_cddb_socket(close, -1)
#endif
    , m_cddb_ip_adress(g_advancedSettings.m_cddbAddress)
{
  m_lastError = 0;
}

//-------------------------------------------------------------------------------------------------------------------
Xcddb::~Xcddb()
{
  closeSocket();
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::openSocket()
{
  char     namebuf[NI_MAXHOST], portbuf[NI_MAXSERV];
  struct   addrinfo hints;
  struct   addrinfo *result, *addr;
  char     service[33];
  int      res;
  SOCKET   fd = INVALID_SOCKET;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  sprintf(service, "%d", CDDB_PORT);

  res = getaddrinfo(m_cddb_ip_adress.c_str(), service, &hints, &result);
  if(res)
  {
    CLog::Log(LOGERROR, "Xcddb::openSocket - failed to lookup %s with error %s", m_cddb_ip_adress.c_str(), gai_strerror(res));
    res = getaddrinfo("130.179.31.49", service, &hints, &result);
    if(res)
      return false;
  }

  for(addr = result; addr; addr = addr->ai_next)
  {
    if(getnameinfo(addr->ai_addr, addr->ai_addrlen, namebuf, sizeof(namebuf), portbuf, sizeof(portbuf),NI_NUMERICHOST))
    {
      strcpy(namebuf, "[unknown]");
      strcpy(portbuf, "[unknown]");
	}
    CLog::Log(LOGDEBUG, "Xcddb::openSocket - connecting to: %s:%s ...", namebuf, portbuf);

    fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if(fd == INVALID_SOCKET)
      continue;

    if(connect(fd, addr->ai_addr, addr->ai_addrlen) != SOCKET_ERROR)
      break;

    closesocket(fd);
    fd = INVALID_SOCKET;
  }

  freeaddrinfo(result);
  if(fd == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "Xcddb::openSocket - failed to connect to cddb");
    return false;
  }

  m_cddb_socket.attach(fd);
  return true;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::closeSocket()
{
  if (m_cddb_socket)
  {
    m_cddb_socket.reset();
  }
  return true;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::Send( const void *buffer, int bytes )
{
  std::unique_ptr<char[]> tmp_buffer(new char[bytes + 10]);
  strcpy(tmp_buffer.get(), (const char*)buffer);
  tmp_buffer.get()[bytes] = '.';
  tmp_buffer.get()[bytes + 1] = 0x0d;
  tmp_buffer.get()[bytes + 2] = 0x0a;
  tmp_buffer.get()[bytes + 3] = 0x00;
  int iErr = send((SOCKET)m_cddb_socket, (const char*)tmp_buffer.get(), bytes + 3, 0);
  if (iErr <= 0)
  {
    return false;
  }
  return true;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::Send( const char *buffer)
{
  int iErr = Send(buffer, strlen(buffer));
  if (iErr <= 0)
  {
    return false;
  }
  return true;
}

//-------------------------------------------------------------------------------------------------------------------
std::string Xcddb::Recv(bool wait4point)
{
  char tmpbuffer[1];
  char prevChar;
  int counter = 0;
  std::string str_buffer;


  //##########################################################
  // Read the buffer. Character by character
  tmpbuffer[0]=0;
  do
  {
    int lenRead;

    prevChar=tmpbuffer[0];
    lenRead = recv((SOCKET)m_cddb_socket, (char*) & tmpbuffer, 1, 0);

    //Check if there was any error reading the buffer
    if(lenRead == 0 || lenRead == SOCKET_ERROR  || WSAGetLastError() == WSAECONNRESET)
    {
      CLog::Log(LOGERROR, "Xcddb::Recv Error reading buffer. lenRead = [%d] and WSAGetLastError = [%d]", lenRead, WSAGetLastError());
      break;
    }

    //Write received data to the return string
    str_buffer.push_back(tmpbuffer[0]);
    counter++;
  }while(wait4point ? prevChar != '\n' || tmpbuffer[0] != '.' : tmpbuffer[0] != '\n');


  //##########################################################
  // Write captured data information to the xbmc log file
  CLog::Log(LOGDEBUG,"Xcddb::Recv Captured %d bytes // Buffer= %" PRIdS" bytes. Captured data follows on next line\n%s", counter, str_buffer.size(),(char *)str_buffer.c_str());


  return str_buffer;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::queryCDinfo(CCdInfo* pInfo, int inexact_list_select)
{
  if ( pInfo == NULL )
  {
    m_lastError = E_PARAMETER_WRONG;
    return false;
  }

  uint32_t discid = pInfo->GetCddbDiscId();


  //##########################################################
  // Compose the cddb query string
  std::string read_buffer = getInexactCommand(inexact_list_select);
  if (read_buffer.empty())
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select Size of inexaxt_list_select are 0");
    m_lastError = E_PARAMETER_WRONG;
    return false;
  }


  //##########################################################
  // Read the data from cddb
  Recv(false); // Clear pending data on our connection
  if (!Send(read_buffer.c_str()))
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select Error sending \"%s\"", read_buffer.c_str());
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select pInfo == NULL");
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  std::string recv_buffer = Recv(true);
  m_lastError = atoi(recv_buffer.c_str());
  switch(m_lastError)
  {
  case 210: //OK, CDDB database entry follows (until terminating marker)
    // Cool, I got it ;-)
    writeCacheFile( recv_buffer.c_str(), discid );
    parseData(recv_buffer.c_str());
    break;

  case 401: //Specified CDDB entry not found.
  case 402: //Server error.
  case 403: //Database entry is corrupt.
  case 409: //No handshake.
  default:
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select Error: \"%s\"", recv_buffer.c_str());
    return false;
  }


  //##########################################################
  // Quit
  if ( ! Send("quit") )
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select Error sending \"%s\"", "quit");
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  recv_buffer = Recv(false);
  m_lastError = atoi(recv_buffer.c_str());
  switch(m_lastError)
  {
  case 0: //By some reason, also 0 is a valid value. This is not documented, and might depend on that no string was found and atoi then returns 0
  case 230: //Closing connection.  Goodbye.
    break;

  case 530: //error, closing connection.
  default:
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select Error: \"%s\"", recv_buffer.c_str());
    return false;
  }


  //##########################################################
  // Close connection
  if ( !closeSocket() )
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select Error closing socket");
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  return true;
}


//-------------------------------------------------------------------------------------------------------------------
int Xcddb::getLastError() const
{
  return m_lastError;
}


//-------------------------------------------------------------------------------------------------------------------
const char *Xcddb::getLastErrorText() const
{
  switch (getLastError())
  {
  case E_TOC_INCORRECT:
    return "TOC Incorrect";
    break;
  case E_NETWORK_ERROR_OPEN_SOCKET:
    return "Error open Socket";
    break;
  case E_NETWORK_ERROR_SEND:
    return "Error send PDU";
    break;
  case E_WAIT_FOR_INPUT:
    return "Wait for Input";
    break;
  case E_PARAMETER_WRONG:
    return "Error Parameter Wrong";
    break;
  case 202: return "No match found";
  case 210: return "Found exact matches, list follows (until terminating marker)";
  case 211: return "Found inexact matches, list follows (until terminating marker)";
  case 401: return "Specified CDDB entry not found";
  case 402: return "Server error";
  case 403: return "Database entry is corrupt";
  case 408: return "CGI environment error";
  case 409: return "No handshake";
  case 431: return "Handshake not successful, closing connection";
  case 432: return "No connections allowed: permission denied";
  case 433: return "No connections allowed: X users allowed, Y currently active";
  case 434: return "No connections allowed: system load too high";
  case 500: return "Command syntax error, command unknown, command unimplemented";
  case 501: return "Illegal protocol level";
  case 530: return "error, closing connection, Server error, server timeout";
  default:  return "Unknown Error";
  }
}


//-------------------------------------------------------------------------------------------------------------------
int Xcddb::cddb_sum(int n)
{
  int ret;

  /* For backward compatibility this algorithm must not change */

  ret = 0;

  while (n > 0)
  {
    ret = ret + (n % 10);
    n = n / 10;
  }

  return (ret);
}

//-------------------------------------------------------------------------------------------------------------------
uint32_t Xcddb::calc_disc_id(int tot_trks, toc cdtoc[])
{
  int i = 0, t = 0, n = 0;

  while (i < tot_trks)
  {

    n = n + cddb_sum((cdtoc[i].min * 60) + cdtoc[i].sec);
    i++;
  }

  t = ((cdtoc[tot_trks].min * 60) + cdtoc[tot_trks].sec) - ((cdtoc[0].min * 60) + cdtoc[0].sec);

  return ((n % 0xff) << 24 | t << 8 | tot_trks);
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::addTitle(const char *buffer)
{
  char value[2048];
  int trk_nr = 0;
  //TTITLEN
  if (buffer[7] == '=')
  { //Einstellig
    trk_nr = buffer[6] - 47;
    strcpy(value, buffer + 8);
  }
  else if (buffer[8] == '=')
  { //Zweistellig
    trk_nr = ((buffer[6] - 48) * 10) + buffer[7] - 47;
    strcpy(value, buffer + 9);
  }
  else if (buffer[9] == '=')
  { //Dreistellig
    trk_nr = ((buffer[6] - 48) * 100) + ((buffer[7] - 48) * 10) + buffer[8] - 47;
    strcpy(value, buffer + 10);
  }
  else
  {
    return ;
  }

  // track artist" / "track title
  std::vector<std::string> values = StringUtils::Split(value, " / ");
  if (values.size() > 1)
  {
    g_charsetConverter.unknownToUTF8(values[0]);
    m_mapArtists[trk_nr] += values[0];
    g_charsetConverter.unknownToUTF8(values[1]);
    m_mapTitles[trk_nr] += values[1];
  }
  else if (!values.empty())
  {
    g_charsetConverter.unknownToUTF8(values[0]);
    m_mapTitles[trk_nr] += values[0];
  }
}

//-------------------------------------------------------------------------------------------------------------------
const std::string& Xcddb::getInexactCommand(int select) const
{
  typedef std::map<int, std::string>::const_iterator iter;
  iter i = m_mapInexact_cddb_command_list.find(select);
  if (i == m_mapInexact_cddb_command_list.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
const std::string& Xcddb::getInexactArtist(int select) const
{
  typedef std::map<int, std::string>::const_iterator iter;
  iter i = m_mapInexact_artist_list.find(select);
  if (i == m_mapInexact_artist_list.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
const std::string& Xcddb::getInexactTitle(int select) const
{
  typedef std::map<int, std::string>::const_iterator iter;
  iter i = m_mapInexact_title_list.find(select);
  if (i == m_mapInexact_title_list.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
const std::string& Xcddb::getTrackArtist(int track) const
{
  typedef std::map<int, std::string>::const_iterator iter;
  iter i = m_mapArtists.find(track);
  if (i == m_mapArtists.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
const std::string& Xcddb::getTrackTitle(int track) const
{
  typedef std::map<int, std::string>::const_iterator iter;
  iter i = m_mapTitles.find(track);
  if (i == m_mapTitles.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::getDiskTitle(std::string& strdisk_title) const
{
  strdisk_title = m_strDisk_title;
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::getDiskArtist(std::string& strdisk_artist) const
{
  strdisk_artist = m_strDisk_artist;
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::parseData(const char *buffer)
{
  //writeLog("parseData Start");

  std::map<std::string, std::string> keywords;
  std::list<std::string> keywordsOrder; // remember order of keywords as it appears in data received from CDDB

  // Collect all the keywords and put them in map. 
  // Multiple occurrences of the same keyword indicate that 
  // the data contained on those lines should be concatenated
  char *line;
  const char trenner[3] = {'\n', '\r', '\0'};
  strtok((char*)buffer, trenner); // skip first line
  while ((line = strtok(0, trenner)))
  {
    // Lines that begin with # are comments, should be ignored
    if (line[0] != '#')
    {
      char *s = strstr(line, "=");
      if (s != NULL)
      {
        std::string strKeyword(line, s - line);
        StringUtils::TrimRight(strKeyword);

        std::string strValue(s+1);
        StringUtils::Replace(strValue, "\\n", "\n");
        StringUtils::Replace(strValue, "\\t", "\t");
        StringUtils::Replace(strValue, "\\\\", "\\");

        std::map<std::string, std::string>::const_iterator it = keywords.find(strKeyword);
        if (it != keywords.end())
          strValue = it->second + strValue; // keyword occured before, concatenate
        else
          keywordsOrder.push_back(strKeyword);

        keywords[strKeyword] = strValue;
      }
    }
  }

  // parse keywords 
  for (std::list<std::string>::const_iterator it = keywordsOrder.begin(); it != keywordsOrder.end(); ++it)
  {
    std::string strKeyword = *it;
    std::string strValue = keywords[strKeyword];

    //! @todo STRING_CLEANUP
    if (strKeyword == "DTITLE")
    {
      // DTITLE may contain artist and disc title, separated with " / ",
      // for example: DTITLE=Modern Talking / Album: Victory (The 11th Album)
      bool found = false;
      for (int i = 0; i < (int)strValue.size() - 2; i++)
      {
        if (strValue[i] == ' ' && strValue[i + 1] == '/' && strValue[i + 2] == ' ')
        {
          m_strDisk_artist = TrimToUTF8(strValue.substr(0, i));
          m_strDisk_title = TrimToUTF8(strValue.substr(i+3));
          found = true;
          break;
        }
      }

      if (!found)
        m_strDisk_title = TrimToUTF8(strValue);
    }
    else if (strKeyword == "DYEAR")
      m_strYear = TrimToUTF8(strValue);
    else if (strKeyword== "DGENRE")
      m_strGenre = TrimToUTF8(strValue);
    else if (StringUtils::StartsWith(strKeyword, "TTITLE"))
      addTitle((strKeyword + "=" + strValue).c_str());
    else if (strKeyword == "EXTD")
    {
      std::string strExtd(strValue);

      if (m_strYear.empty())
      {
        // Extract Year from extended info
        // as a fallback
        size_t iPos = strExtd.find("YEAR: ");
        if (iPos != std::string::npos) // You never know if you really get UTF-8 strings from cddb
          g_charsetConverter.unknownToUTF8(strExtd.substr(iPos + 6, 4), m_strYear);
      }

      if (m_strGenre.empty())
      {
        // Extract ID3 Genre
        // as a fallback
        size_t iPos = strExtd.find("ID3G: ");
        if (iPos != std::string::npos)
        {
          std::string strGenre = strExtd.substr(iPos + 5, 4);
          StringUtils::TrimLeft(strGenre);
          if (StringUtils::IsNaturalNumber(strGenre))
          {
            int iGenre = strtol(strGenre.c_str(), NULL, 10);
            m_strGenre = TagLib::ID3v1::genre(iGenre).to8Bit(true);
          }
        }
      }
    }
    else if (StringUtils::StartsWith(strKeyword, "EXTT"))
      addExtended((strKeyword + "=" + strValue).c_str());
  }

  //writeLog("parseData Ende");
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::addExtended(const char *buffer)
{
  char value[2048];
  int trk_nr = 0;
  //TTITLEN
  if (buffer[5] == '=')
  { //Einstellig
    trk_nr = buffer[4] - 47;
    strcpy(value, buffer + 6);
  }
  else if (buffer[6] == '=')
  { //Zweistellig
    trk_nr = ((buffer[4] - 48) * 10) + buffer[5] - 47;
    strcpy(value, buffer + 7);
  }
  else if (buffer[7] == '=')
  { //Dreistellig
    trk_nr = ((buffer[4] - 48) * 100) + ((buffer[5] - 48) * 10) + buffer[6] - 47;
    strcpy(value, buffer + 8);
  }
  else
  {
    return ;
  }

  std::string strValue;
  std::string strValueUtf8=value;
  // You never know if you really get UTF-8 strings from cddb
  g_charsetConverter.unknownToUTF8(strValueUtf8, strValue);
  m_mapExtended_track[trk_nr] = strValue;
}

//-------------------------------------------------------------------------------------------------------------------
const std::string& Xcddb::getTrackExtended(int track) const
{
  typedef std::map<int, std::string>::const_iterator iter;
  iter i = m_mapExtended_track.find(track);
  if (i == m_mapExtended_track.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::addInexactList(const char *list)
{
  /*
  211 Found inexact matches, list follows (until terminating `.')
  soundtrack bf0cf90f Modern Talking / Victory - The 11th Album
  rock c90cf90f Modern Talking / Album: Victory (The 11th Album)
  misc de0d020f Modern Talking / Ready for the victory
  rock e00d080f Modern Talking / Album: Victory (The 11th Album)
  rock c10d150f Modern Talking / Victory (The 11th Album)
  .
  */

  /*
  m_mapInexact_cddb_command_list;
  m_mapInexact_artist_list;
  m_mapInexact_title_list;
  */
  int start = 0;
  int end = 0;
  bool found = false;
  int line_counter = 0;
  // //writeLog("addInexactList Start");
  for (unsigned int i = 0;i < strlen(list);i++)
  {
    if (list[i] == '\n')
    {
      end = i;
      found = true;
    }
    if (found)
    {
      if (line_counter > 0)
      {
        addInexactListLine(line_counter, list + start, end - start - 1);
      }
      start = i + 1;
      line_counter++;
      found = false;
    }
  }
  // //writeLog("addInexactList End");
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::addInexactListLine(int line_cnt, const char *line, int len)
{
  // rock c90cf90f Modern Talking / Album: Victory (The 11th Album)
  int search4 = 0;
  char genre[100]; // 0
  char discid[10]; // 1
  char artist[1024]; // 2
  char title[1024];
  char cddb_command[1024];
  int start = 0;
  // //writeLog("addInexactListLine Start");
  for (int i = 0;i < len;i++)
  {
    switch (search4)
    {
    case 0:
      if (line[i] == ' ')
      {
        strncpy(genre, line, i);
        genre[i] = 0x00;
        search4 = 1;
        start = i + 1;
      }
      break;
    case 1:
      if (line[i] == ' ')
      {
        strncpy(discid, line + start, i - start);
        discid[i - start] = 0x00;
        search4 = 2;
        start = i + 1;
      }
      break;
    case 2:
      if (i + 2 <= len && line[i] == ' ' && line[i + 1] == '/' && line[i + 2] == ' ')
      {
        strncpy(artist, line + start, i - start);
        artist[i - start] = 0x00;
        strncpy(title, line + (i + 3), len - (i + 3));
        title[len - (i + 3)] = 0x00;
      }
      break;
    }
  }
  sprintf(cddb_command, "cddb read %s %s", genre, discid);

  m_mapInexact_cddb_command_list[line_cnt] = cddb_command;

  std::string strArtist=artist;
  // You never know if you really get UTF-8 strings from cddb
  g_charsetConverter.unknownToUTF8(artist, strArtist);
  m_mapInexact_artist_list[line_cnt] = strArtist;

  std::string strTitle=title;
  // You never know if you really get UTF-8 strings from cddb
  g_charsetConverter.unknownToUTF8(title, strTitle);
  m_mapInexact_title_list[line_cnt] = strTitle;
  // char log_string[1024];
  // sprintf(log_string,"%u: %s - %s",line_cnt,artist,title);
  // //writeLog(log_string);
  // //writeLog("addInexactListLine End");
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::setCDDBIpAdress(const std::string& ip_adress)
{
  m_cddb_ip_adress = ip_adress;
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::setCacheDir(const std::string& pCacheDir )
{
  cCacheDir = pCacheDir;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::queryCache( uint32_t discid )
{
  if (cCacheDir.empty())
    return false;

  XFILE::CFile file;
  if (file.Open(GetCacheFile(discid)))
  {
    // Got a cachehit
    char buffer[4096];
    file.Read(buffer, 4096);
    file.Close();
    parseData( buffer );
    return true;
  }

  return false;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::writeCacheFile( const char* pBuffer, uint32_t discid )
{
  if (cCacheDir.empty())
    return false;

  XFILE::CFile file;
  if (file.OpenForWrite(GetCacheFile(discid), true))
  {
    const bool ret = ( (size_t) file.Write((void*)pBuffer, strlen(pBuffer) + 1) == strlen(pBuffer) + 1);
    file.Close();
    return ret;
  }

  return false;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::isCDCached( int nr_of_tracks, toc cdtoc[] )
{
  if (cCacheDir.empty())
    return false;

  return XFILE::CFile::Exists(GetCacheFile(calc_disc_id(nr_of_tracks, cdtoc)));
}

//-------------------------------------------------------------------------------------------------------------------
const std::string& Xcddb::getYear() const
{
  return m_strYear;
}

//-------------------------------------------------------------------------------------------------------------------
const std::string& Xcddb::getGenre() const
{
  return m_strGenre;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::queryCDinfo(CCdInfo* pInfo)
{
  if ( pInfo == NULL )
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo pInfo == NULL");
    m_lastError = E_PARAMETER_WRONG;
    return false;
  }

  int lead_out = pInfo->GetTrackCount();
  int real_track_count = pInfo->GetTrackCount();
  uint32_t discid = pInfo->GetCddbDiscId();
  unsigned long frames[100];


  //##########################################################
  //
  if ( queryCache(discid) )
  {
    CLog::Log(LOGDEBUG, "Xcddb::queryCDinfo discid [%08x] already cached", discid);
    return true;
  }

  //##########################################################
  //
  for (int i = 0;i < lead_out;i++)
  {
    frames[i] = pInfo->GetTrackInformation( i + 1 ).nFrames;
    if (i > 0 && frames[i] < frames[i - 1])
    {
      CLog::Log(LOGERROR, "Xcddb::queryCDinfo E_TOC_INCORRECT");
      m_lastError = E_TOC_INCORRECT;
      return false;
    }
  }
  unsigned long complete_length = pInfo->GetDiscLength();


  //##########################################################
  // Open socket to cddb database
  if ( !openSocket() )
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error opening socket");
    m_lastError = E_NETWORK_ERROR_OPEN_SOCKET;
    return false;
  }
  std::string recv_buffer = Recv(false);
  m_lastError = atoi(recv_buffer.c_str());
  switch(m_lastError)
  {
  case 200: //OK, read/write allowed
  case 201: //OK, read only
    break;

  case 432: //No connections allowed: permission denied
  case 433: //No connections allowed: X users allowed, Y currently active
  case 434: //No connections allowed: system load too high
  default:
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error: \"%s\"", recv_buffer.c_str());
    return false;
  }


  //##########################################################
  // Send the Hello message
  std::string version = CSysInfo::GetVersion();
  std::string lcAppName = CCompileInfo::GetAppName();
  StringUtils::ToLower(lcAppName);
  if (version.find(" ") != std::string::npos)
    version = version.substr(0, version.find(" "));
  std::string strGreeting = "cddb hello " + lcAppName + " kodi.tv " + CCompileInfo::GetAppName() + " " + version;
  if ( ! Send(strGreeting.c_str()) )
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error sending \"%s\"", strGreeting.c_str());
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  recv_buffer = Recv(false);
  m_lastError = atoi(recv_buffer.c_str());
  switch(m_lastError)
  {
  case 200: //Handshake successful
  case 402: //Already shook hands
    break;

  case 431: //Handshake not successful, closing connection
  default:
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error: \"%s\"", recv_buffer.c_str());
    return false;
  }


  //##########################################################
  // Set CDDB protocol-level to 5
  if ( ! Send("proto 5"))
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error sending \"%s\"", "proto 5");
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  recv_buffer = Recv(false);
  m_lastError = atoi(recv_buffer.c_str());
  switch(m_lastError)
  {
  case 200: //CDDB protocol level: current cur_level, supported supp_level
  case 201: //OK, protocol version now: cur_level
  case 502: //Protocol level already cur_level
    break;

  case 501: //Illegal protocol level.
  default:
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error: \"%s\"", recv_buffer.c_str());
    return false;
  }


  //##########################################################
  // Compose the cddb query string
  char query_buffer[1024];
  strcpy(query_buffer, "");
  strcat(query_buffer, "cddb query");
  {
    char tmp_buffer[256];
    sprintf(tmp_buffer, " %08x", discid);
    strcat(query_buffer, tmp_buffer);
  }
  {
    char tmp_buffer[256];
    sprintf(tmp_buffer, " %i", real_track_count);
    strcat(query_buffer, tmp_buffer);
  }
  for (int i = 0;i < lead_out;i++)
  {
    char tmp_buffer[256];
    sprintf(tmp_buffer, " %lu", frames[i]);
    strcat(query_buffer, tmp_buffer);
  }
  {
    char tmp_buffer[256];
    sprintf(tmp_buffer, " %lu", complete_length);
    strcat(query_buffer, tmp_buffer);
  }


  //##########################################################
  // Query for matches
  if ( ! Send(query_buffer))
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error sending \"%s\"", query_buffer);
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  // 200 rock d012180e Soundtrack / Hackers
  std::string read_buffer;
  recv_buffer = Recv(false);
  m_lastError = atoi(recv_buffer.c_str());
  switch(m_lastError)
  {
  case 200: //Found exact match
    strtok((char *)recv_buffer.c_str(), " ");
    read_buffer = StringUtils::Format("cddb read %s %08x", strtok(NULL, " "), discid);
    break;

  case 210: //Found exact matches, list follows (until terminating marker)
  case 211: //Found inexact matches, list follows (until terminating marker)
    /*
    soundtrack bf0cf90f Modern Talking / Victory - The 11th Album
    rock c90cf90f Modern Talking / Album: Victory (The 11th Album)
    misc de0d020f Modern Talking / Ready for the victory
    rock e00d080f Modern Talking / Album: Victory (The 11th Album)
    rock c10d150f Modern Talking / Victory (The 11th Album)
    .
    */
    recv_buffer += Recv(true);
    addInexactList(recv_buffer.c_str());
    m_lastError=E_WAIT_FOR_INPUT;
    return false; //This is actually good. The calling method will handle this

  case 202: //No match found
    CLog::Log(LOGNOTICE, "Xcddb::queryCDinfo No match found in CDDB database when doing the query shown below:\n%s",query_buffer);
  case 403: //Database entry is corrupt
  case 409: //No handshake
  default:
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error: \"%s\"", recv_buffer.c_str());
    return false;
  }


  //##########################################################
  // Read the data from cddb
  if ( !Send(read_buffer.c_str()) )
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error sending \"%s\"", read_buffer.c_str());
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  recv_buffer = Recv(true);
  m_lastError = atoi(recv_buffer.c_str());
  switch(m_lastError)
  {
  case 210: //OK, CDDB database entry follows (until terminating marker)
    // Cool, I got it ;-)
    writeCacheFile( recv_buffer.c_str(), discid );
    parseData(recv_buffer.c_str());
    break;

  case 401: //Specified CDDB entry not found.
  case 402: //Server error.
  case 403: //Database entry is corrupt.
  case 409: //No handshake.
  default:
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error: \"%s\"", recv_buffer.c_str());
    return false;
  }


  //##########################################################
  // Quit
  if ( ! Send("quit") )
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error sending \"%s\"", "quit");
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  recv_buffer = Recv(false);
  m_lastError = atoi(recv_buffer.c_str());
  switch(m_lastError)
  {
  case 0: //By some reason, also 0 is a valid value. This is not documented, and might depend on that no string was found and atoi then returns 0
  case 230: //Closing connection.  Goodbye.
    break;

  case 530: //error, closing connection.
  default:
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error: \"%s\"", recv_buffer.c_str());
    return false;
  }


  //##########################################################
  // Close connection
  if ( !closeSocket() )
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error closing socket");
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  return true;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::isCDCached( CCdInfo* pInfo )
{
  if (cCacheDir.empty())
    return false;
  if ( pInfo == NULL )
    return false;

  return XFILE::CFile::Exists(GetCacheFile(pInfo->GetCddbDiscId()));
}

std::string Xcddb::GetCacheFile(uint32_t disc_id) const
{
  std::string strFileName;
  strFileName = StringUtils::Format("%x.cddb", disc_id);
  return URIUtils::AddFileToFolder(cCacheDir, strFileName);
}

std::string Xcddb::TrimToUTF8(const std::string &untrimmedText)
{
  std::string text(untrimmedText);
  StringUtils::Trim(text);
  // You never know if you really get UTF-8 strings from cddb
  g_charsetConverter.unknownToUTF8(text);
  return text;
}

#endif

