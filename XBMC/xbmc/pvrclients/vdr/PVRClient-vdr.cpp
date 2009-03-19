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

/*
 * DESCRIPTION:
 *
 * PVRClientVDR is part of the PVRManager to support backend access.
 *
 * The code included inside this file is to access VDR's streamdev-server
 * Plugin as TV/PVR Backend.
 *
 */

#include "PVRClient-vdr.h"

#ifndef _LINUX
extern "C" long long atoll(const char *ca)
{
  long long ig=0;
  int       sign=1;
  /* test for prefixing white space */
  while (*ca == ' ' || *ca == '\t' ) 
    ca++;
  /* Check sign entered or no */
  if ( *ca == '-' )       
    sign = -1;
  /* convert string to int */
  while (*ca != '\0')
    if (*ca >= '0' && *ca <= '9')
      ig = ig * 10LL + *ca++ - '0';
    else
      ca++;
  return (ig*(long long)sign);
}
#endif


#ifdef _LINUX
#define SD_BOTH SHUT_RDWR
#endif

using namespace std;

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

/************************************************************/
/** Class interface */

PVRClientVDR::PVRClientVDR(PVRCallbacks *callback)
{
  m_xbmc              = callback;
  m_iCurrentChannel   = 1;
  m_session           = new CVTPSession();
  m_bConnected        = false;
  m_bCharsetIsUTF8    = false;

 // InitializeCriticalSection(&m_critSection);
}

PVRClientVDR::~PVRClientVDR()
{
  Disconnect();
  //DeleteCriticalSection(&m_critSection);
}


/************************************************************/
/** Server handling */

PVR_ERROR PVRClientVDR::GetProperties(PVR_SERVERPROPS *props)
{
  props->Name                      = "VDR";
  props->DefaultHostname           = "127.0.0.1";
  props->DefaultPort               = 2004;
  props->DefaultUser               = "vdr";
  props->DefaultPassword           = "vdr";
  props->SupportChannelLogo        = false;
  props->SupportTimeShift          = false;
  props->SupportEPG                = true;
  props->SupportRecordings         = true;
  props->SupportTimers             = true;
  props->SupportRadio              = true;
  props->SupportChannelSettings    = true;
  props->SupportTeletext           = false;
  props->SupportDirector           = false;
  props->SupportBouquets           = false;
  
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR PVRClientVDR::Connect()
{
  vector<string>  lines;
  int             code;

  /* Open Streamdev-Server VTP-Connection to VDR Backend Server */

  if (!m_session->Open("192.168.1.3", 2004))
    return PVR_ERROR_SERVER_ERROR;

  /* vdr version checking */
  ///* Check VDR streamdev is patched by calling a newly added command */
  //if (GetNumChannels() == -1)
  //{
  //  CGUIDialogOK::ShowAndGetInput(18100, 18091, 18106, 18092);
  //  return PVR_ERROR_SERVER_ERROR;
  //}

  if (!m_session->SendCommand("STAT charset", code, lines))
  {
    //CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get backend character set");
    return PVR_ERROR_SERVER_ERROR;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  if (data == "UTF-8" || data == "UTF8")
    m_bCharsetIsUTF8 = true;

  m_bConnected = true;
  return PVR_ERROR_NO_ERROR;
}

void PVRClientVDR::Disconnect()
{
  m_bConnected = false;

  /* Close Streamdev-Server Connection */
  Close();
  return;
}

bool PVRClientVDR::IsUp()
{
  return m_bConnected;
}

void PVRClientVDR::Close()
{
  /* Check if a stream socket is open, if yes close stream */
  if (m_socket != INVALID_SOCKET)
  {
    m_session->AbortStreamLive();
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
  }

  /* Close Streamdev-Server VTP Backend Connection */
  m_session->Close();
}


/************************************************************/
/** General handling */

const char* PVRClientVDR::GetBackendName()
{
  if (!m_session->IsOpen())
    return "";

  //EnterCriticalSection(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_session->SendCommand("STAT name", code, lines))
  {
    //CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get backend name");
//    LeaveCriticalSection(&m_critSection);
    return "";
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);

  //LeaveCriticalSection(&m_critSection);
  return data.c_str();
}

const char* PVRClientVDR::GetBackendVersion()
{
  if (!m_session->IsOpen())
    return "";

  //EnterCriticalSection(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_session->SendCommand("STAT version", code, lines))
  {
    //CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get backend version");
 //   LeaveCriticalSection(&m_critSection);
    return "";
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);

 // LeaveCriticalSection(&m_critSection);
  return data.c_str();
}

PVR_ERROR PVRClientVDR::GetDriveSpace(long long *total, long long *used, int *percent)
{

  if (!m_session->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  //EnterCriticalSection(&m_critSection);

  vector<string>  lines;
  int             code;

  if (!m_session->SendCommand("STAT disk", code, lines))
  {
    //CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get disk statistics");
  //  LeaveCriticalSection(&m_critSection);
    return PVR_ERROR_SERVER_ERROR;
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  size_t found = data.find("MB");

  if (found != string::npos)
  {
    *total = atoll(data.c_str()) * 1024;
    data.erase(0, found + 3);
    *used = atoll(data.c_str()) * 1024;
    *percent = (float)(((float) * used / (float) * total) * 100);
  }

  //LeaveCriticalSection(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** EPG handling */

PVR_ERROR PVRClientVDR::GetEPGForChannel(const unsigned int number, PVR_PROGLIST *epg, time_t start, time_t end)
{

  vector<string> lines;
  int            code;
  char           buffer[1024];
  int            found;
  PVR_PROGINFO   broadcast;

  if (!m_session->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  //EnterCriticalSection(&m_critSection);

  if (start != 0)
    sprintf(buffer, "LSTE %d from %d to %d", number, (int)start, (int)end);
  else
    sprintf(buffer, "LSTE %d", number);
  while (!m_session->SendCommand(buffer, code, lines))
  {
    if (code != 451)
    {
    //  LeaveCriticalSection(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {

    string& data(*it);
    string str_result = data;

    /** Get Channelname **/
    found = str_result.find("C", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
//    = str_result.c_str();
      continue;
    }

    /** Get Title **/
    found = str_result.find("T", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      broadcast.title = str_result.c_str();
      continue;
    }

    /** Get short description **/
    found = str_result.find("S", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      broadcast.subtitle = str_result.c_str();
      continue;
    }

    /** Get description **/
    found = str_result.find("D", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);

      int pos = 0;

      while (1)
      {
        pos = str_result.find("|", pos);

        if (pos < 0)
          break;

        str_result.replace(pos, 1, 1, '\n');
      }
      broadcast.description = str_result.c_str();
      continue;
    }

    /** Get Genre **/
    found = str_result.find("G", 0);
    if (found == 0)
    {
      str_result.erase(0, 2);
      /*broadcast.category = atoll(str_result.c_str());*/
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      /*broadcast.m_GenreSubType = atoll(str_result.c_str());*/
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      broadcast.category = str_result.c_str();
      continue;
    }

    /** Get ID, date and length**/
    found = str_result.find("E ", 0);
    if (found == 0)
    {
      time_t rec_time;
      int duration;
      str_result.erase(0, 2);
//    = atoll(str_result.c_str());

      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);

      rec_time = atoll(str_result.c_str());
      found = str_result.find(" ", 0);
      str_result.erase(0, found + 1);
      duration = atoll(str_result.c_str());

      broadcast.starttime = rec_time;
      broadcast.endtime  = rec_time + duration;
      continue;
    }

    /** end tag **/
    found = str_result.find("e", 0);
    if (found == 0)
    {
      epg->progInfo[epg->length] = broadcast;
      epg->length++;
    }
  }

  //LeaveCriticalSection(&m_critSection);

  return PVR_ERROR_NO_ERROR;
}

//PVR_ERROR PVRClientVDR::GetEPGNowInfo(unsigned int number, CTVEPGInfoTag *result)
//{
//
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//  int            found;
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTE %d NOW", number);
//  while (!m_session->SendCommand(buffer, code, lines))
//  {
//    if (code != 451)
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVR_ERROR_SERVER_ERROR;
//    }
//    Sleep(750);
//  }
//
//  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
//  {
//    string& data(*it);
//    string str_result = data;
//
//    if (!m_bCharsetIsUTF8)
//      g_charsetConverter.stringCharsetToUtf8(str_result);
//
//    /** Get Channelname **/
//    found = str_result.find("C", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//      result->m_strChannel = str_result.c_str();
//      continue;
//    }
//
//    /** Get Title **/
//    found = str_result.find("T", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//      result->m_strTitle = str_result.c_str();
//      continue;
//    }
//
//    /** Get short description **/
//    found = str_result.find("S", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//      result->m_strPlotOutline = str_result.c_str();
//      continue;
//    }
//
//    /** Get description **/
//    found = str_result.find("D", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//      int pos = 0;
//
//      while (1)
//      {
//        pos = str_result.find("|", pos);
//
//        if (pos < 0)
//          break;
//
//        str_result.replace(pos, 1, 1, '\n');
//      }
//
//      result->m_strPlot = str_result.c_str();
//      continue;
//    }
//
//    /** Get Genre **/
//    found = str_result.find("G", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//      result->m_GenreType = atoll(str_result.c_str());
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//      result->m_GenreSubType = atoll(str_result.c_str());
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//      result->m_strGenre = str_result.c_str();
//      continue;
//    }
//
//    /** Get ID, date and length**/
//    found = str_result.find("E ", 0);
//    if (found == 0)
//    {
//      time_t rec_time;
//      int duration;
//      str_result.erase(0, 2);
////                broadcast.m_bouquetNum = atoll(str_result.c_str());
//
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//
//      rec_time = atoll(str_result.c_str());
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//      duration = atoll(str_result.c_str());
//
//      result->m_startTime = CDateTime((time_t)rec_time);
//      result->m_endTime = CDateTime((time_t)rec_time + duration);
//      result->m_duration = CDateTimeSpan(0, 0, duration / 60, duration % 60);
//      continue;
//    }
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::GetEPGNextInfo(unsigned int number, CTVEPGInfoTag *result)
//{
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//  int            found;
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTE %d NEXT", number);
//  while (!m_session->SendCommand(buffer, code, lines))
//  {
//    if (code != 451)
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVR_ERROR_SERVER_ERROR;
//    }
//    Sleep(750);
//  }
//
//  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
//  {
//    string& data(*it);
//    string str_result = data;
//
//    if (!m_bCharsetIsUTF8)
//      g_charsetConverter.stringCharsetToUtf8(str_result);
//
//    /** Get Channelname **/
//    found = str_result.find("C", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//      result->m_strChannel = str_result.c_str();
//      continue;
//    }
//
//    /** Get Title **/
//    found = str_result.find("T", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//      result->m_strTitle = str_result.c_str();
//      continue;
//    }
//
//    /** Get short description **/
//    found = str_result.find("S", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//      result->m_strPlotOutline = str_result.c_str();
//      continue;
//    }
//
//    /** Get description **/
//    found = str_result.find("D", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//
//      int pos = 0;
//
//      while (1)
//      {
//        pos = str_result.find("|", pos);
//
//        if (pos < 0)
//          break;
//
//        str_result.replace(pos, 1, 1, '\n');
//      }
//
//      result->m_strPlot = str_result.c_str();
//      continue;
//    }
//
//    /** Get Genre **/
//    found = str_result.find("G", 0);
//    if (found == 0)
//    {
//      str_result.erase(0, 2);
//      result->m_GenreType = atoll(str_result.c_str());
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//      result->m_GenreSubType = atoll(str_result.c_str());
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//      result->m_strGenre = str_result.c_str();
//      continue;
//    }
//
//    /** Get ID, date and length**/
//    found = str_result.find("E ", 0);
//    if (found == 0)
//    {
//      time_t rec_time;
//      int duration;
//      str_result.erase(0, 2);
////                broadcast.m_bouquetNum = atoll(str_result.c_str());
//
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//
//      rec_time = atoll(str_result.c_str());
//      found = str_result.find(" ", 0);
//      str_result.erase(0, found + 1);
//      duration = atoll(str_result.c_str());
//
//      result->m_startTime = CDateTime((time_t)rec_time);
//      result->m_endTime = CDateTime((time_t)rec_time + duration);
//      result->m_duration = CDateTimeSpan(0, 0, duration / 60, duration % 60);
//      continue;
//    }
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//

/************************************************************/
/** Channel handling */

int PVRClientVDR::GetNumChannels()
{
  vector<string>  lines;
  int             code;

  if (!m_session->IsOpen())
    return -1;

  //EnterCriticalSection(&m_critSection);

  if (!m_session->SendCommand("STAT channels", code, lines))
  {
    //CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get channel count");
    //LeaveCriticalSection(&m_critSection);
    return -1;
  }

  vector<string>::iterator it = lines.begin();

  string& data(*it);
  //LeaveCriticalSection(&m_critSection);
  return atol(data.c_str());
}

PVR_ERROR PVRClientVDR::GetAllChannels(PVR_CHANLIST* results)
{
  vector<string> lines;
  int            code;
  unsigned int   number = 1;

  if (!m_session->IsOpen())
    return PVR_ERROR_SERVER_ERROR;

  //EnterCriticalSection(&m_critSection);

  results->length = 0;
  results->channel = NULL;

  while (!m_session->SendCommand("LSTC", code, lines))
  {
    if (code != 451)
    {
     // LeaveCriticalSection(&m_critSection);
      return PVR_ERROR_SERVER_ERROR;
    }
    Sleep(750);
  }

  // allocate the required array size
  results->channel = MALLOC(PVR_CHANNEL, lines.size());
  if (results->channel = NULL)
  {
    // couldn't allocate enough memory
    free(results);
    return PVR_ERROR_NOT_IMPLEMENTED;
  }

  // now step through the response from streamdev-server
  int pos = 0;
  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    string str_result = data;
    int found;

    PVR_CHANNEL *channel = MALLOC(PVR_CHANNEL, 1);

    int m_VPID = 0;
    int m_APID1 = 0;
    int m_APID2 = 0;
    int m_DPID1 = 0;
    int m_DPID2 = 0;
    int m_CAID = 0;
    string name;
    int id;

    //if (!m_bCharsetIsUTF8)
    //  need to convert to UTF8

    // Channel number
    channel->number = atol(str_result.c_str());
    found = str_result.find(" ", 0);
    str_result.erase(0, found + 1);

    // Channel and provider name
    found = str_result.find(":", 0);
    name.assign(str_result.c_str(), found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found == -1)
    {
      channel->name = name.c_str();
    }
    else
    {
      string temp;
      temp.assign(name.c_str(), found);
      channel->name = temp.c_str();
      name.erase(0, found + 1);
    }

    // Channel frequency
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);
    found = str_result.find(":", 0);
    str_result.erase(0, found);

    // Source Type
    if (str_result.compare(0, 1, ":C") != 0)
    {
      str_result.erase(0, 3);
    }
    else
      if (str_result.compare(0, 1, ":T") != 0)
      {
        str_result.erase(0, 3);
      }
      else
        if (str_result.compare(0, 1, ":S") != 0)
        {
          str_result.erase(0, 2);
          found = str_result.find(":", 0);
          str_result.erase(0, found + 1);
        }
        else
          if (str_result.compare(0, 1, ":P") != 0)
          {
            str_result.erase(0, 3);
          }

    // Channel symbolrate
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Channel program id
    m_VPID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Channel audio id's
    found = str_result.find(":", 0);
    name.assign(str_result.c_str(), found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found == -1)
    {
      id = atol(name.c_str());

      if (id == 0)
      {
        m_APID1 = 0;
        m_APID2 = 0;
        m_DPID1 = 0;
        m_DPID2 = 0;
      }
      else
      {
        m_APID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          m_APID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          m_APID2 = atol(name.c_str());
        }

        m_DPID1 = 0;
        m_DPID2 = 0;
      }
    }
    else
    {
      int id;
      id = atol(name.c_str());

      if (id == 0)
      {
        m_APID1 = 0;
        m_APID2 = 0;
      }
      else
      {
        m_APID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          m_APID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          m_APID2 = atol(name.c_str());
        }
      }

      found = name.find(";", 0);
      name.erase(0, found + 1);
      id = atol(name.c_str());

      if (id == 0)
      {
        m_DPID1 = 0;
        m_DPID2 = 0;
      }
      else
      {
        m_DPID1 = id;
        found = name.find(",", 0);

        if (found == -1)
        {
          m_DPID2 = 0;
        }
        else
        {
          name.erase(0, found + 1);
          m_DPID2 = atol(name.c_str());
        }
      }
    }

    // Teletext id
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // CAID id
    m_CAID = atol(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // is this channel encrypted?
    channel->encrypted = m_CAID;

    // is this a radio channel?
    channel->radio = ((m_VPID == 0) && (m_APID1 != 0));

    // increment the channel counters
    pos++;
    results->length++;

  }

  //LeaveCriticalSection(&m_critSection);
  return PVR_ERROR_NO_ERROR;
}
