
//-----------------------------------------------------------------------------
// File: cddb.cpp
//
// Desc: Make a connection to the CDDB database
//
// Hist: 00.00.01
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "cddb.h"
#include "../dnsnamecache.h"
#include "../id3tag.h"
#include "../util.h"


using namespace CDDB;

//-------------------------------------------------------------------------------------------------------------------
Xcddb::Xcddb()
    : m_cddb_socket(INVALID_SOCKET)
{
  m_lastError = 0;
  m_cddb_ip_adress = g_advancedSettings.m_cddbAddress;
  cCacheDir = "";
  m_strNull = "";
}

//-------------------------------------------------------------------------------------------------------------------
Xcddb::~Xcddb()
{
  closeSocket();
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::openSocket()
{
  unsigned int port = CDDB_PORT;

  sockaddr_in service;
  service.sin_family = AF_INET;

  // connect to site directly
  CStdString strIpadres;
  CDNSNameCache::Lookup(m_cddb_ip_adress, strIpadres); 
  if (strIpadres == "") 
  {
    strIpadres = "130.179.31.49"; //"64.71.163.204";
    CLog::Log(LOGERROR, "Xcddb::openSocket DNS lookup for %s failed. Trying to use %s instead", m_cddb_ip_adress.c_str(), strIpadres.c_str());
  }
  service.sin_addr.s_addr = inet_addr(strIpadres.c_str());
  service.sin_port = htons(port);
  m_cddb_socket.attach( socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));

  // attempt to connection
  if (connect((SOCKET)m_cddb_socket, (sockaddr*) &service, sizeof(struct sockaddr)) == SOCKET_ERROR)
  {
    m_cddb_socket.reset();
    return false;
  }
  return true;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::closeSocket()
{
  if ( m_cddb_socket.isValid() )
  {
    m_cddb_socket.reset();
  }
  return true;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::Send( const void *buffer, int bytes )
{
  auto_aptr<char> tmp_buffer (new char[bytes + 10]);
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
string Xcddb::Recv(bool wait4point)
{
  char tmpbuffer[1];
  char prevChar;
  int counter = 0;
  string str_buffer;


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
  CLog::Log(LOGDEBUG,"Xcddb::Recv Captured %d bytes // Buffer= %d bytes. Captured data follows on next line\n%s", counter, str_buffer.size(),(char *)str_buffer.c_str());
  

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

  unsigned long discid = pInfo->GetCddbDiscId();


  //##########################################################
  // Compose the cddb query string
  CStdString read_buffer = getInexactCommand(inexact_list_select);
  if (read_buffer.size() == 0)
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select Size of inexaxt_list_select are 0");
    m_lastError = E_PARAMETER_WRONG;
    return false;
  }


  //##########################################################
  // Read the data from cddb
  Recv(false); //erstmal den Müll abholen
  if ( !Send(read_buffer) )
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select Error sending \"%s\"", read_buffer.c_str());
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo_inexaxt_list_select pInfo == NULL");
    m_lastError = E_NETWORK_ERROR_SEND;
    return false;
  }
  CStdString recv_buffer = Recv(true);
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
//int Xcddb::queryCDinfo(int real_track_count, toc cdtoc[])
//{
//  // //writeLog("Xcddb::queryCDinfo - Start");
//
//  /* //writeLog("getHostByName start");
//   struct hostent* hp=gethostbyname("freedb.freedb.org");
//   //writeLog("hp->h_name=%s",hp->h_name);
//   //writeLog("hp->h_aliases=%s",hp->h_aliases);
//   //writeLog("hp->h_addr_list=%s",hp->h_addr_list);
//   //writeLog("getHostByName end");
//  */
//  int lead_out = real_track_count;
//  unsigned long discid = calc_disc_id(real_track_count, cdtoc);
//  unsigned long frames[100];
//
//  bool bLoaded = queryCache( discid );
//
//  if ( bLoaded )
//    return true;
//
//  for (int i = 0;i <= lead_out;i++)
//  {
//    frames[i] = (cdtoc[i].min * 75 * 60) + (cdtoc[i].sec * 75) + cdtoc[i].frame;
//    if (i > 0 && frames[i] < frames[i - 1])
//    {
//      m_lastError = E_TOC_INCORRECT;
//      return false;
//    }
//  }
//  unsigned long complete_length = frames[lead_out] / 75;
//
//  // Socket öffnen
//  if ( !openSocket() )
//  {
//    //writeLog("openSocket Failed");
//    m_lastError = E_NETWORK_ERROR_OPEN_SOCKET;
//    return false;
//  }
//
//  // Erst mal was empfangen
//  string recv_buffer = Recv(false);
//  /*
//  200 OK, read/write allowed
//  201 OK, read only
//  432 No connections allowed: permission denied
//  433 No connections allowed: X users allowed, Y currently active
//  434 No connections allowed: system load too high
//  */
//  if (recv_buffer.c_str()[0] == '2')
//  {
//    //OK
//    //  //writeLog("Connection 2 cddb: OK");
//    m_lastError = IN_PROGRESS;
//  }
//  else if (recv_buffer.c_str()[0] == '4')
//  {
//    //No connections allowed
//    //writeLog("Connection 2 cddb: No connections allowed");
//    m_lastError = 430 + (recv_buffer.c_str()[3] - 48);
//    return false;
//  }
//
//  // Jetzt hello Senden
//  if ( ! Send("cddb hello xbox xbox xcddb 00.00.01"))
//  {
//    //writeLog("Send Failed");
//    m_lastError = E_NETWORK_ERROR_SEND;
//    return false;
//  }
//
//  // hello Antwort
//  recv_buffer = Recv(false);
//  /*
//  200 Handshake successful
//  431 Handshake not successful, closing connection
//  402 Already shook hands
//  */
//  if (recv_buffer.c_str()[0] == '2')
//  {
//    //OK
//    ////writeLog("Hello 2 cddb: OK");
//    m_lastError = IN_PROGRESS;
//  }
//  else if (recv_buffer.c_str()[0] == '4' && recv_buffer.c_str()[1] == '3')
//  {
//    //No connections allowed
//    //writeLog("Hello 2 cddb: Handshake not successful, closing connection");
//    m_lastError = E_CDDB_Handshake_not_successful;
//    return false;
//  }
//  else if (recv_buffer.c_str()[0] == '4' && recv_buffer.c_str()[1] == '0')
//  {
//    //  //writeLog("Hello 2 cddb: Already shook hands, but it's OK");
//    m_lastError = W_CDDB_already_shook_hands;
//  }
//
//
//  // Hier jetzt die CD abfragen
//  //##########################################################
//  char query_buffer[1024];
//  strcpy(query_buffer, "");
//  strcat(query_buffer, "cddb query");
//  {
//    char tmp_buffer[256];
//    sprintf(tmp_buffer, " %08x", discid);
//    strcat(query_buffer, tmp_buffer);
//  }
//  {
//    char tmp_buffer[256];
//    sprintf(tmp_buffer, " %u", real_track_count);
//    strcat(query_buffer, tmp_buffer);
//  }
//  for (int i = 0;i < lead_out;i++)
//  {
//    char tmp_buffer[256];
//    sprintf(tmp_buffer, " %u", frames[i]);
//    strcat(query_buffer, tmp_buffer);
//  }
//  {
//    char tmp_buffer[256];
//    sprintf(tmp_buffer, " %u", complete_length);
//    strcat(query_buffer, tmp_buffer);
//  }
//
//  //cddb query
//  if ( ! Send(query_buffer))
//  {
//    //writeLog("Send Failed");
//    m_lastError = E_NETWORK_ERROR_SEND;
//    return false;
//  }
//
//  // Antwort
//  // 200 rock d012180e Soundtrack / Hackers
//  char read_buffer[1024];
//  recv_buffer = Recv(false);
//  // Hier antwort auswerten
//  /*
//  200 Found exact match
//  211 Found inexact matches, list follows (until terminating marker)
//  202 No match found
//  403 Database entry is corrupt
//  409 No handshake
//  */
//  char *tmp_str;
//  tmp_str = (char *)recv_buffer.c_str();
//  switch (tmp_str[0] - 48)
//  {
//  case 2:
//    switch (tmp_str[1] - 48)
//    {
//    case 0:
//      switch (tmp_str[2] - 48)
//      {
//      case 0:  //200
//        strtok(tmp_str, " ");
//        strcpy(read_buffer, "");
//        strcat(read_buffer, "cddb read ");
//        // categ
//        strcat(read_buffer, strtok(0, " "));
//        {
//          char tmp_buffer[256];
//          sprintf(tmp_buffer, " %08x", discid);
//          strcat(read_buffer, tmp_buffer);
//        }
//        m_lastError = IN_PROGRESS;
//        break;
//      case 2:  //202
//        m_lastError = E_NO_MATCH_FOUND;
//        return false;
//        break;
//      default:
//        m_lastError = false;
//        return false;
//      }
//      break;
//    case 1:
//      switch (tmp_str[2] - 48)
//      {
//      case 1:  //211
//        m_lastError = E_INEXACT_MATCH_FOUND;
//        /*
//        211 Found inexact matches, list follows (until terminating `.')
//        soundtrack bf0cf90f Modern Talking / Victory - The 11th Album
//        rock c90cf90f Modern Talking / Album: Victory (The 11th Album)
//        misc de0d020f Modern Talking / Ready for the victory
//        rock e00d080f Modern Talking / Album: Victory (The 11th Album)
//        rock c10d150f Modern Talking / Victory (The 11th Album)
//        .
//        */
//        addInexactList(tmp_str);
//        m_lastError = E_WAIT_FOR_INPUT;
//        return false;
//        break;
//      default:
//        m_lastError = -1;
//        return false;
//      }
//      break;
//    default:
//      m_lastError = -1;
//      return false;
//    }
//    break;
//  case 4:
//    switch (tmp_str[2] - 48)
//    {
//    case 3:  //403
//      m_lastError = 403;
//      break;
//    case 9:  //409
//      m_lastError = 409;
//      break;
//    default:
//      m_lastError = -1;
//      return false;
//    }
//    break;
//  default:
//    m_lastError = -1;
//    return false;
//  }
//
//
//  //##########################################################
//  if ( !Send(read_buffer) )
//  {
//    //writeLog("Send Failed");
//    //writeLog(read_buffer);
//    return false;
//  }
//
//
//  // cddb read Antwort
//  recv_buffer = Recv(true);
//  /*
//  210 OK, CDDB database entry follows (until terminating marker)
//  401 Specified CDDB entry not found.
//  402 Server error.
//  403 Database entry is corrupt.
//  409 No handshake.
//  */
//  char *tmp_str2;
//  tmp_str2 = (char *)recv_buffer.c_str();
//  switch (tmp_str2[0] - 48)
//  {
//  case 2:
//    //   //writeLog("2-- XXXXXXXXXXXXXXXX");
//    // Cool, I got it ;-)
//    writeCacheFile( tmp_str2, discid );
//    parseData(tmp_str2);
//    break;
//  case 4:
//    //   //writeLog("4-- XXXXXXXXXXXXXXXX");
//    switch (tmp_str2[2] - 48)
//    {
//    case 1:  //401
//      //     //writeLog("401 XXXXXXXXXXXXXXXX");
//      m_lastError = 401;
//      break;
//    case 2:  //402
//      //     //writeLog("402 XXXXXXXXXXXXXXXX");
//      m_lastError = 402;
//      break;
//    case 3:  //403
//      //     //writeLog("403 XXXXXXXXXXXXXXXX");
//      m_lastError = 403;
//      break;
//    case 9:  //409
//      //     //writeLog("409 XXXXXXXXXXXXXXXX");
//      m_lastError = 409;
//      break;
//    default:
//      m_lastError = -1;
//      return false;
//    }
//    break;
//  default:
//    m_lastError = -1;
//    return false;
//  }
//
//  //##########################################################
//  // Abmelden 2x Senden kommt sonst zu fehler
//  if ( ! Send("quit") )
//  {
//    //writeLog("Send Failed");
//    return false;
//  }
//
//  // quit Antwort
//  Recv(false);
//
//  // Socket schliessen
//  if ( !closeSocket() )
//  {
//    //writeLog("closeSocket Failed");
//    return false;
//  }
//  else
//  {
//    //  //writeLog("closeSocket OK");
//  }
//  m_lastError = QUERRY_OK;
//  return true;
//}




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
//Can be removed if/when removing Xcddb::queryCDinfo(int real_track_count, toc cdtoc[])
//  case IN_PROGRESS:
//    return "in Progress";
//    break;
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
//Can be removed if/when removing Xcddb::queryCDinfo(int real_track_count, toc cdtoc[])
//  case E_NO_MATCH_FOUND:
//    return "No Match found";
//    break;
//  case E_INEXACT_MATCH_FOUND:
//    return "Inexact Match found";
//    break;
//  case W_CDDB_already_shook_hands:
//    return "Warning already shook hands";
//    break;
//  case E_CDDB_Handshake_not_successful:
//    return "Error Handshake not successful";
//    break;
//  case QUERRY_OK:
//    return "Query OK";
//    break;
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
unsigned long Xcddb::calc_disc_id(int tot_trks, toc cdtoc[])
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
  char artist[1024];
  char title[1024];
  unsigned int len = (unsigned int)strlen(value);
  bool found = false;
  unsigned int index;
  for (index = 0;index < len;index++)
  {
    if ((index + 2) <= len && value[index] == ' ' && value[index + 1] == '/' && value[index + 2] == ' ')
    {
      // Jep found
      found = true;
      break;
    }
  }
  if (found)
  {
    strncpy(artist, value, index);
    artist[index] = '\0';
    strcpy(title, value + index + 3);
  }
  else
  {
    artist[0] = '\0';
    strcpy(title, value);
  }

  CStdString strArtist=artist;
  // You never know if you really get UTF-8 strings from cddb
  if (!g_charsetConverter.isValidUtf8(strArtist)) 
    g_charsetConverter.stringCharsetToUtf8(CStdString(artist), strArtist);
  m_mapArtists[trk_nr] += strArtist;

  CStdString strTitle=title;
  // You never know if you really get UTF-8 strings from cddb
  if (!g_charsetConverter.isValidUtf8(strTitle)) 
    g_charsetConverter.stringCharsetToUtf8(CStdString(title), strTitle);
  m_mapTitles[trk_nr] += strTitle;
}

//-------------------------------------------------------------------------------------------------------------------
const CStdString& Xcddb::getInexactCommand(int select) const
{
  typedef map<int, CStdString>::const_iterator iter;
  iter i = m_mapInexact_cddb_command_list.find(select);
  if (i == m_mapInexact_cddb_command_list.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
const CStdString& Xcddb::getInexactArtist(int select) const
{
  typedef map<int, CStdString>::const_iterator iter;
  iter i = m_mapInexact_artist_list.find(select);
  if (i == m_mapInexact_artist_list.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
const CStdString& Xcddb::getInexactTitle(int select) const
{
  typedef map<int, CStdString>::const_iterator iter;
  iter i = m_mapInexact_title_list.find(select);
  if (i == m_mapInexact_title_list.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
const CStdString& Xcddb::getTrackArtist(int track) const
{
  typedef map<int, CStdString>::const_iterator iter;
  iter i = m_mapArtists.find(track);
  if (i == m_mapArtists.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
const CStdString& Xcddb::getTrackTitle(int track) const
{
  typedef map<int, CStdString>::const_iterator iter;
  iter i = m_mapTitles.find(track);
  if (i == m_mapTitles.end())
    return m_strNull;
  return i->second;
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::getDiskTitle(CStdString& strdisk_title) const
{
  strdisk_title = m_strDisk_title;
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::getDiskArtist(CStdString& strdisk_artist) const
{
  strdisk_artist = m_strDisk_artist;
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::parseData(const char *buffer)
{
  //writeLog("parseData Start");

  char *line;
  const char trenner[3] = {'\n', '\r', '\0'};
  line = strtok((char*)buffer, trenner);
  int line_cnt = 0;
  while (line = strtok(0, trenner))
  {
    if (line[0] != '#')
    {
      if (0 == strncmp(line, "DTITLE", 6))
      {
        // DTITLE=Modern Talking / Album: Victory (The 11th Album)
        unsigned int len = (unsigned int)strlen(line) - 6;
        bool found = false;
        unsigned int i = 5;
        for (;i < len;i++)
        {
          if ((i + 2) <= len && line[i] == ' ' && line[i + 1] == '/' && line[i + 2] == ' ')
          {
            // Jep found
            found = true;
            break;
          }
        }
        if (found)
        {
          CStdString strLine = (char*)(line + 7);
          CStdString strDisk_artist = strLine.Left(i - 7);
          CStdString strDisk_title = (char*)(line + i + 3);

          // You never know if you really get UTF-8 strings from cddb
          if (!g_charsetConverter.isValidUtf8(strDisk_artist))
            g_charsetConverter.stringCharsetToUtf8(strDisk_artist, m_strDisk_artist);
          else
            m_strDisk_artist=strDisk_artist;

          // You never know if you really get UTF-8 strings from cddb
          if (!g_charsetConverter.isValidUtf8(strDisk_title))
            g_charsetConverter.stringCharsetToUtf8(strDisk_title, m_strDisk_title);
          else
            m_strDisk_title=strDisk_title;
        }
        else
        {
          CStdString strDisk_title = (char*)(line + 7);
          // You never know if you really get UTF-8 strings from cddb
          if (!g_charsetConverter.isValidUtf8(strDisk_title))
            g_charsetConverter.stringCharsetToUtf8(strDisk_title, m_strDisk_title);
          else
            m_strDisk_title=strDisk_title;
        }
      }
      else if (0 == strncmp(line, "DYEAR", 5))
      {
        CStdString strYear = (char*)(line + 5);
        strYear.TrimLeft("= ");
        // You never know if you really get UTF-8 strings from cddb
        if (!g_charsetConverter.isValidUtf8(strYear))
          g_charsetConverter.stringCharsetToUtf8(strYear, m_strYear);
        else
          m_strYear=strYear;
      }
      else if (0 == strncmp(line, "DGENRE", 6))
      {
        CStdString strGenre = (char*)(line + 6);
        strGenre.TrimLeft("= ");

        // You never know if you really get UTF-8 strings from cddb
        if (!g_charsetConverter.isValidUtf8(strGenre))
          g_charsetConverter.stringCharsetToUtf8(strGenre, m_strGenre);
        else
          m_strGenre=strGenre;
      }
      else if (0 == strncmp(line, "TTITLE", 6))
      {
        addTitle(line);
      }
      else if (0 == strncmp(line, "EXTD", 4))
      {
        CStdString strExtd((char*)(line + 4));

        if (m_strYear.IsEmpty())
        {
          // Extract Year from extended info
          // as a fallback
          int iPos = strExtd.Find("YEAR:");
          if (iPos > -1)
          {
            CStdString strYear;
            strYear = strExtd.Mid(iPos + 6, 4);

            // You never know if you really get UTF-8 strings from cddb
            if (!g_charsetConverter.isValidUtf8(strYear))
              g_charsetConverter.stringCharsetToUtf8(strYear, m_strYear);
            else
              m_strYear=strYear;
          }
        }

        if (m_strGenre.IsEmpty())
        {
          // Extract ID3 Genre
          // as a fallback
          int iPos = strExtd.Find("ID3G:");
          if (iPos > -1)
          {
            CStdString strGenre;
            strGenre = strExtd.Mid(iPos + 5, 4);
            strGenre.TrimLeft(' ');
            if (StringUtils::IsNaturalNumber(strGenre))
            {
              CID3Tag tag;
              m_strGenre=tag.ParseMP3Genre(strGenre);
            }
          }
        }
      }
      else if (0 == strncmp(line, "EXTT", 4))
      {
        addExtended(line);
      }
    }
    line_cnt++;
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

  CStdString strValue;
  CStdString strValueUtf8=value;
  // You never know if you really get UTF-8 strings from cddb
  if (!g_charsetConverter.isValidUtf8(strValueUtf8))
    g_charsetConverter.stringCharsetToUtf8(strValueUtf8, strValue);
  else
    strValue=strValueUtf8;
  m_mapExtended_track[trk_nr] = strValue;
}

//-------------------------------------------------------------------------------------------------------------------
const CStdString& Xcddb::getTrackExtended(int track) const
{
  typedef map<int, CStdString>::const_iterator iter;
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

  CStdString strArtist=artist;
  // You never know if you really get UTF-8 strings from cddb
  if (!g_charsetConverter.isValidUtf8(strArtist)) 
    g_charsetConverter.stringCharsetToUtf8(CStdString(artist), strArtist);
  m_mapInexact_artist_list[line_cnt] = strArtist;

  CStdString strTitel=title;
  // You never know if you really get UTF-8 strings from cddb
  if (!g_charsetConverter.isValidUtf8(strTitel)) 
    g_charsetConverter.stringCharsetToUtf8(CStdString(artist), strTitel);
  m_mapInexact_title_list[line_cnt] = strTitel;
  // char log_string[1024];
  // sprintf(log_string,"%u: %s - %s",line_cnt,artist,title);
  // //writeLog(log_string);
  // //writeLog("addInexactListLine End");
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::setCDDBIpAdress(const CStdString& ip_adress)
{
  m_cddb_ip_adress = ip_adress;
}

//-------------------------------------------------------------------------------------------------------------------
void Xcddb::setCacheDir(const CStdString& pCacheDir )
{
  cCacheDir = pCacheDir;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::queryCache( unsigned long discid )
{
  if (cCacheDir.size() == 0)
    return false;

  CStdString strFileName;
  strFileName.Format("%s\\%x.cddb", cCacheDir.c_str(), discid);

  FILE* fd;
  fd = fopen( strFileName.c_str(), "rb");
  if (fd)
  {
    // Got a cachehit
    char buffer[4096];
    OutputDebugString ( "cddb local cache hit.\n" );
    fread( buffer, 1, 4096 , fd);
    fclose(fd);
    parseData( buffer );
    return true;
  }

  return false;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::writeCacheFile( const char* pBuffer, unsigned long discid )
{
  if (cCacheDir.size() == 0)
    return false;

  CStdString strFileName;
  strFileName.Format("%s\\%x.cddb", cCacheDir.c_str(), discid);

  FILE* fd;
  fd = fopen(strFileName.c_str(), "wb+");
  if (fd)
  {
    OutputDebugString ( "Current cd saved to local cddb.\n" );
    fwrite( (void*) pBuffer, 1, strlen( pBuffer ) + 1, fd );
    fclose(fd);
    return true;
  }

  return false;
}

//-------------------------------------------------------------------------------------------------------------------
bool Xcddb::isCDCached( int nr_of_tracks, toc cdtoc[] )
{
  if (cCacheDir.size() == 0)
    return false;

  unsigned long discid = calc_disc_id(nr_of_tracks, cdtoc);

  CStdString strFileName;
  strFileName.Format("%s\\%x.cddb", cCacheDir.c_str(), discid);

  FILE* fd;
  fd = fopen(strFileName.c_str(), "rb");
  if (fd)
  {
    fclose(fd);
    return true;
  }

  return false;
}

//-------------------------------------------------------------------------------------------------------------------
const CStdString& Xcddb::getYear() const
{
  return m_strYear;
}

//-------------------------------------------------------------------------------------------------------------------
const CStdString& Xcddb::getGenre() const
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
  unsigned long discid = pInfo->GetCddbDiscId();
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
  CStdString recv_buffer = Recv(false);
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
  if ( ! Send("cddb hello xbox xbox XboxMediaCenter pre-2.1"))
  {
    CLog::Log(LOGERROR, "Xcddb::queryCDinfo Error sending \"%s\"", "cddb hello xbox xbox XboxMediaCenter 1.1.0");
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
  case 201:	//OK, protocol version now: cur_level
  case 502: //Protocol level already cur_level
    break;
  
  case 501:	//Illegal protocol level.
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
    sprintf(tmp_buffer, " %u", real_track_count);
    strcat(query_buffer, tmp_buffer);
  }
  for (int i = 0;i < lead_out;i++)
  {
    char tmp_buffer[256];
    sprintf(tmp_buffer, " %u", frames[i]);
    strcat(query_buffer, tmp_buffer);
  }
  {
    char tmp_buffer[256];
    sprintf(tmp_buffer, " %u", complete_length);
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
  CStdString read_buffer;
  recv_buffer = Recv(false);
  m_lastError = atoi(recv_buffer.c_str());
  switch(m_lastError)
  {
  case 200: //Found exact match
    strtok((char *)recv_buffer.c_str(), " ");
    read_buffer.Format("cddb read %s %08x", strtok(NULL, " "), discid);
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
  if ( !Send(read_buffer) )
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
  if (cCacheDir.size() == 0)
    return false;
  if ( pInfo == NULL )
    return false;

  unsigned long discid = pInfo->GetCddbDiscId();

  CStdString strFileName;
  strFileName.Format("%s\\%x.cddb", cCacheDir.c_str(), discid);

  FILE* fd;
  fd = fopen(strFileName.c_str(), "rb");
  if (fd)
  {
    fclose(fd);
    return true;
  }

  return false;
}




