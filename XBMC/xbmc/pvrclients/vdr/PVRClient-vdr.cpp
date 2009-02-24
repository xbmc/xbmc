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

  // set the length of the epg datastore
  epg->length = 0;

  //EnterCriticalSection(&m_critSection);

  /*time_t start_epg = 0;
  time_t end_epg = 0;*/

  /*if (start.IsValid())
    start.GetAsTime(start_epg);
  if (end.IsValid())
    end.GetAsTime(end_epg);*/

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
    /*sleep(750);*/
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    string str_result = data;

    if (!m_bCharsetIsUTF8)
      /*g_charsetConverter.stringCharsetToUtf8(str_result);*/

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
  return atoll(data.c_str());
}

PVR_ERROR PVRClientVDR::GetAllChannels(PVR_CHANLIST* results, bool radio)
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
    /*sleep(750);*/
  }

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    string str_result = data;
    int found;

    PVR_CHANNEL channel;
    int m_VPID = 0;
    int m_APID1 = 0;
    int m_APID2 = 0;
    int m_DPID1 = 0;
    int m_DPID2 = 0;
    int m_CAID = 0;
    string name;
    int id;

    //if (!m_bCharsetIsUTF8)
    //  g_charsetConverter.stringCharsetToUtf8(str_result);

    // Channel number
    channel.number = atoll(str_result.c_str());
    found = str_result.find(" ", 0);
    str_result.erase(0, found + 1);

    // Channel and provider name
    found = str_result.find(":", 0);
    name.assign(str_result.c_str(), found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found == -1)
    {
      channel.name = name.c_str();
    }
    else
    {
      string temp;
      temp.assign(name.c_str(), found);
      channel.name = temp.c_str();
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
    m_VPID = atoll(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // Channel audio id's
    found = str_result.find(":", 0);
    name.assign(str_result.c_str(), found);
    str_result.erase(0, found + 1);
    found = name.find(";", 0);

    if (found == -1)
    {
      id = atoll(name.c_str());

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
          m_APID2 = atoll(name.c_str());
        }

        m_DPID1 = 0;
        m_DPID2 = 0;
      }
    }
    else
    {
      int id;
      id = atoll(name.c_str());

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
          m_APID2 = atoll(name.c_str());
        }
      }

      found = name.find(";", 0);
      name.erase(0, found + 1);
      id = atoll(name.c_str());

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
          m_DPID2 = atoll(name.c_str());
        }
      }
    }

    // Teletext id
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    // CAID id
    m_CAID = atoll(str_result.c_str());
    found = str_result.find(":", 0);
    str_result.erase(0, found + 1);

    channel.encrypted = m_CAID;

    channel.radio = ((m_VPID == 0) && (m_APID1 != 0));


    if (radio == channel.radio)
    {
      channel.number = results->length+1;
      results->channel[results->length] = channel;
    }
  }

  //LeaveCriticalSection(&m_critSection);
  return PVR_ERROR_NO_ERROR;
}

//PVR_ERROR PVRClientVDR::GetChannelSettings(CTVChannelInfoTag *result)
//{
//
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//
//  if (result->m_iClientNum < 1)
//    return PVR_ERROR_SERVER_ERROR;
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTC %d", result->m_iClientNum);
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
//    int found;
//    int i_tmp;
//
//    result->m_Settings.m_VPID = 0;
//    result->m_Settings.m_APID1 = 0;
//    result->m_Settings.m_APID2 = 0;
//    result->m_Settings.m_DPID1 = 0;
//    result->m_Settings.m_DPID2 = 0;
//    result->m_Settings.m_CAID = 0;
//    string name;
//    int id;
//
//    if (!m_bCharsetIsUTF8)
//      g_charsetConverter.stringCharsetToUtf8(str_result);
//
//    // Skip Channel number
//    found = str_result.find(" ", 0);
//    str_result.erase(0, found + 1);
//
//    // Channel and provider name
//    found = str_result.find(":", 0);
//    name.assign(str_result, found);
//    str_result.erase(0, found + 1);
//    found = name.find(";", 0);
//
//    if (found != -1)
//    {
//      name.erase(0, found + 1);
//      result->m_Settings.m_strProvider = name;
//    }
//
//    // Channel frequency
//    result->m_Settings.m_Freq = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//    found = str_result.find(":", 0);
//    result->m_Settings.m_parameter.assign(str_result, found);
//    str_result.erase(0, found);
//
//    // Source Type
//    if (str_result.compare(0, 1, ":C") != 0)
//    {
//      result->m_Settings.m_SourceType = src_DVBC;
//      result->m_Settings.m_satellite = "Cable";
//      str_result.erase(0, 3);
//    }
//    else
//      if (str_result.compare(0, 1, ":T") != 0)
//      {
//        result->m_Settings.m_SourceType = src_DVBT;
//        result->m_Settings.m_satellite = "Terrestrial";
//        str_result.erase(0, 3);
//      }
//      else
//        if (str_result.compare(0, 1, ":S") != 0)
//        {
//          result->m_Settings.m_SourceType = src_DVBS;
//          str_result.erase(0, 2);
//          found = str_result.find(":", 0);
//          result->m_Settings.m_satellite.assign(str_result, found);
//          str_result.erase(0, found + 1);
//        }
//        else
//          if (str_result.compare(0, 1, ":P") != 0)
//          {
//            result->m_Settings.m_SourceType = srcAnalog;
//            result->m_Settings.m_satellite = "Analog";
//            str_result.erase(0, 3);
//          }
//
//    // Channel symbolrate
//    result->m_Settings.m_Symbolrate = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//
//    // Channel program id
//    result->m_Settings.m_VPID = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//
//    // Channel audio id's
//    found = str_result.find(":", 0);
//    name.assign(str_result, found);
//    str_result.erase(0, found + 1);
//    found = name.find(";", 0);
//
//    if (found == -1)
//    {
//      id = atoll(name.c_str());
//
//      if (id == 0)
//      {
//        result->m_Settings.m_APID1 = 0;
//        result->m_Settings.m_APID2 = 0;
//        result->m_Settings.m_DPID1 = 0;
//        result->m_Settings.m_DPID2 = 0;
//      }
//      else
//      {
//        result->m_Settings.m_APID1 = id;
//        found = name.find(",", 0);
//
//        if (found == -1)
//        {
//          result->m_Settings.m_APID2 = 0;
//        }
//        else
//        {
//          name.erase(0, found + 1);
//          result->m_Settings.m_APID2 = atoll(name.c_str());
//        }
//        result->m_Settings.m_DPID1 = 0;
//        result->m_Settings.m_DPID2 = 0;
//      }
//    }
//    else
//    {
//      int id;
//
//      id = atoll(name.c_str());
//
//      if (id == 0)
//      {
//        result->m_Settings.m_APID1 = 0;
//        result->m_Settings.m_APID2 = 0;
//      }
//      else
//      {
//        result->m_Settings.m_APID1 = id;
//        found = name.find(",", 0);
//
//        if (found == -1)
//        {
//          result->m_Settings.m_APID2 = 0;
//        }
//        else
//        {
//          name.erase(0, found + 1);
//          result->m_Settings.m_APID2 = atoll(name.c_str());
//        }
//      }
//
//      found = name.find(";", 0);
//
//      name.erase(0, found + 1);
//      id = atoll(name.c_str());
//
//      if (id == 0)
//      {
//        result->m_Settings.m_DPID1 = 0;
//        result->m_Settings.m_DPID2 = 0;
//      }
//      else
//      {
//        result->m_Settings.m_DPID1 = id;
//        found = name.find(",", 0);
//
//        if (found == -1)
//        {
//          result->m_Settings.m_DPID2 = 0;
//        }
//        else
//        {
//          name.erase(0, found + 1);
//          result->m_Settings.m_DPID2 = atoll(name.c_str());
//        }
//      }
//    }
//
//    // Teletext id
//    result->m_Settings.m_TPID = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//
//    // CAID id
//    result->m_Settings.m_CAID = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//
//    // Service id
//    result->m_Settings.m_SID = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//
//    // Network id
//    result->m_Settings.m_NID = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//
//    // Transport id
//    result->m_Settings.m_TID = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//
//    // Radio id
//    result->m_Settings.m_RID = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//
//    // DVB-S2 ?
//    if (result->m_Settings.m_SourceType == src_DVBS)
//    {
//      str_result = result->m_Settings.m_parameter;
//      found = str_result.find("S", 0);
//
//      if (found != -1)
//      {
//        str_result.erase(0, found + 1);
//        i_tmp = atoll(str_result.c_str());
//
//        if (i_tmp == 1)
//        {
//          result->m_Settings.m_SourceType = src_DVBS2;
//        }
//      }
//    }
//
//    // Inversion
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("I", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 0)
//      {
//        result->m_Settings.m_Inversion = InvOff;
//      }
//      else
//        if (i_tmp == 1)
//        {
//          result->m_Settings.m_Inversion = InvOn;
//        }
//        else
//          if (i_tmp == 999)
//          {
//            result->m_Settings.m_Inversion = InvAuto;
//          }
//    }
//    else
//    {
//      result->m_Settings.m_Inversion = InvAuto;
//    }
//
//    // CoderateL
//    if (result->m_Settings.m_SourceType == src_DVBT)
//    {
//      str_result = result->m_Settings.m_parameter;
//      found = str_result.find("D", 0);
//
//      if (found != -1)
//      {
//        str_result.erase(0, found + 1);
//        i_tmp = atoll(str_result.c_str());
//
//        if (i_tmp == 0)
//        {
//          result->m_Settings.m_CoderateL = Coderate_None;
//        }
//        else
//          if (i_tmp == 12)
//          {
//            result->m_Settings.m_CoderateL = Coderate_1_2;
//          }
//          else
//            if (i_tmp == 23)
//            {
//              result->m_Settings.m_CoderateL = Coderate_2_3;
//            }
//            else
//              if (i_tmp == 34)
//              {
//                result->m_Settings.m_CoderateL = Coderate_3_4;
//              }
//              else
//                if (i_tmp == 45)
//                {
//                  result->m_Settings.m_CoderateL = Coderate_4_5;
//                }
//                else
//                  if (i_tmp == 56)
//                  {
//                    result->m_Settings.m_CoderateL = Coderate_5_6;
//                  }
//                  else
//                    if (i_tmp == 67)
//                    {
//                      result->m_Settings.m_CoderateL = Coderate_6_7;
//                    }
//                    else
//                      if (i_tmp == 78)
//                      {
//                        result->m_Settings.m_CoderateL = Coderate_7_8;
//                      }
//                      else
//                        if (i_tmp == 89)
//                        {
//                          result->m_Settings.m_CoderateL = Coderate_8_9;
//                        }
//                        else
//                          if (i_tmp == 910)
//                          {
//                            result->m_Settings.m_CoderateL = Coderate_9_10;
//                          }
//                          else
//                            if (i_tmp == 999 || i_tmp == 910)
//                            {
//                              result->m_Settings.m_CoderateL = Coderate_Auto;
//                            }
//      }
//      else
//      {
//        result->m_Settings.m_CoderateL = Coderate_None;
//      }
//    }
//    else
//    {
//      result->m_Settings.m_CoderateL = Coderate_None;
//    }
//
//    // CoderateH
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("C", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 0)
//      {
//        result->m_Settings.m_CoderateH = Coderate_None;
//      }
//      else
//        if (i_tmp == 12)
//        {
//          result->m_Settings.m_CoderateH = Coderate_1_2;
//        }
//        else
//          if (i_tmp == 23)
//          {
//            result->m_Settings.m_CoderateH = Coderate_2_3;
//          }
//          else
//            if (i_tmp == 34)
//            {
//              result->m_Settings.m_CoderateH = Coderate_3_4;
//            }
//            else
//              if (i_tmp == 45)
//              {
//                result->m_Settings.m_CoderateH = Coderate_4_5;
//              }
//              else
//                if (i_tmp == 56)
//                {
//                  result->m_Settings.m_CoderateH = Coderate_5_6;
//                }
//                else
//                  if (i_tmp == 67)
//                  {
//                    result->m_Settings.m_CoderateH = Coderate_6_7;
//                  }
//                  else
//                    if (i_tmp == 78)
//                    {
//                      result->m_Settings.m_CoderateH = Coderate_7_8;
//                    }
//                    else
//                      if (i_tmp == 89)
//                      {
//                        result->m_Settings.m_CoderateH = Coderate_8_9;
//                      }
//                      else
//                        if (i_tmp == 910)
//                        {
//                          result->m_Settings.m_CoderateL = Coderate_9_10;
//                        }
//                        else
//                          if (i_tmp == 999 || i_tmp == 910)
//                          {
//                            result->m_Settings.m_CoderateH = Coderate_Auto;
//                          }
//    }
//    else
//    {
//      result->m_Settings.m_CoderateH = Coderate_None;
//    }
//
//    // Modulation
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("M", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 0)
//      {
//        result->m_Settings.m_Modulation = modNone;
//      }
//      else
//        if (i_tmp == 4)
//        {
//          result->m_Settings.m_Modulation = modQAM4;
//        }
//        else
//          if (i_tmp == 16)
//          {
//            result->m_Settings.m_Modulation = modQAM16;
//          }
//          else
//            if (i_tmp == 32)
//            {
//              result->m_Settings.m_Modulation = modQAM32;
//            }
//            else
//              if (i_tmp == 64)
//              {
//                result->m_Settings.m_Modulation = modQAM64;
//              }
//              else
//                if (i_tmp == 128)
//                {
//                  result->m_Settings.m_Modulation = modQAM128;
//                }
//                else
//                  if (i_tmp == 256)
//                  {
//                    result->m_Settings.m_Modulation = modQAM256;
//                  }
//                  else
//                    if (i_tmp == 512)
//                    {
//                      result->m_Settings.m_Modulation = modQAM512;
//                    }
//                    else
//                      if (i_tmp == 1024)
//                      {
//                        result->m_Settings.m_Modulation = modQAM1024;
//                      }
//                      else
//                        if (i_tmp == 998)
//                        {
//                          result->m_Settings.m_Modulation = modQAMAuto;
//                        }
//                        else
//                          if (i_tmp == 1)
//                          {
//                            result->m_Settings.m_Modulation = modBPSK;
//                          }
//                          else
//                            if (i_tmp == 2)
//                            {
//                              result->m_Settings.m_Modulation = modQPSK;
//                            }
//                            else
//                              if (i_tmp == 3)
//                              {
//                                result->m_Settings.m_Modulation = modOQPSK;
//                              }
//                              else
//                                if (i_tmp == 5)
//                                {
//                                  result->m_Settings.m_Modulation = mod8PSK;
//                                }
//                                else
//                                  if (i_tmp == 6)
//                                  {
//                                    result->m_Settings.m_Modulation = mod16APSK;
//                                  }
//                                  else
//                                    if (i_tmp == 7)
//                                    {
//                                      result->m_Settings.m_Modulation = mod32APSK;
//                                    }
//                                    else
//                                      if (i_tmp == 8)
//                                      {
//                                        result->m_Settings.m_Modulation = modOFDM;
//                                      }
//                                      else
//                                        if (i_tmp == 9)
//                                        {
//                                          result->m_Settings.m_Modulation = modCOFDM;
//                                        }
//                                        else
//                                          if (i_tmp == 10)
//                                          {
//                                            result->m_Settings.m_Modulation = modVSB8;
//                                          }
//                                          else
//                                            if (i_tmp == 11)
//                                            {
//                                              result->m_Settings.m_Modulation = modVSB16;
//                                            }
//    }
//    else
//    {
//      result->m_Settings.m_Modulation = modNone;
//    }
//
//    // Bandwith
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("B", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 5)
//      {
//        result->m_Settings.m_Bandwidth = bw_5MHz;
//      }
//      else
//        if (i_tmp == 6)
//        {
//          result->m_Settings.m_Bandwidth = bw_6MHz;
//        }
//        else
//          if (i_tmp == 7)
//          {
//            result->m_Settings.m_Bandwidth = bw_7MHz;
//          }
//          else
//            if (i_tmp == 8)
//            {
//              result->m_Settings.m_Bandwidth = bw_8MHz;
//            }
//            else
//              if (i_tmp == 999)
//              {
//                result->m_Settings.m_Bandwidth = bw_Auto;
//              }
//    }
//    else
//    {
//      result->m_Settings.m_Bandwidth = bw_Auto;
//    }
//
//    // Hierarchie
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("Y", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 0)
//      {
//        result->m_Settings.m_Hierarchie = false;
//      }
//      else
//        if (i_tmp == 1)
//        {
//          result->m_Settings.m_Hierarchie = true;
//        }
//    }
//    else
//    {
//      result->m_Settings.m_Hierarchie = false;
//    }
//
//    // Alpha
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("A", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 0)
//      {
//        result->m_Settings.m_Alpha = alpha_0;
//      }
//      else
//        if (i_tmp == 1)
//        {
//          result->m_Settings.m_Alpha = alpha_1;
//        }
//        else
//          if (i_tmp == 2)
//          {
//            result->m_Settings.m_Alpha = alpha_2;
//          }
//          else
//            if (i_tmp == 4)
//            {
//              result->m_Settings.m_Alpha = alpha_4;
//            }
//    }
//    else
//    {
//      result->m_Settings.m_Alpha = alpha_0;
//    }
//
//    // Guard
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("G", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 4)
//      {
//        result->m_Settings.m_Guard = guard_1_4;
//      }
//      else
//        if (i_tmp == 8)
//        {
//          result->m_Settings.m_Guard = guard_1_8;
//        }
//        else
//          if (i_tmp == 16)
//          {
//            result->m_Settings.m_Guard = guard_1_16;
//          }
//          else
//            if (i_tmp == 32)
//            {
//              result->m_Settings.m_Guard = guard_1_32;
//            }
//            else
//              if (i_tmp == 999)
//              {
//                result->m_Settings.m_Guard = guard_Auto;
//              }
//    }
//    else
//    {
//      result->m_Settings.m_Guard = guard_Auto;
//    }
//
//    // Transmission
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("T", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 2)
//      {
//        result->m_Settings.m_Transmission = transmission_2K;
//      }
//      else
//        if (i_tmp == 4)
//        {
//          result->m_Settings.m_Transmission = transmission_4K;
//        }
//        else
//          if (i_tmp == 8)
//          {
//            result->m_Settings.m_Transmission = transmission_8K;
//          }
//          else
//            if (i_tmp == 999)
//            {
//              result->m_Settings.m_Transmission = transmission_Auto;
//            }
//    }
//    else
//    {
//      result->m_Settings.m_Transmission = transmission_Auto;
//    }
//
//    // Priority
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("P", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 0)
//      {
//        result->m_Settings.m_Priority = false;
//      }
//      else
//        if (i_tmp == 1)
//        {
//          result->m_Settings.m_Priority = true;
//        }
//    }
//    else
//    {
//      result->m_Settings.m_Priority = false;
//    }
//
//    // Rolloff
//    str_result = result->m_Settings.m_parameter;
//
//    found = str_result.find("O", 0);
//
//    if (found != -1)
//    {
//      str_result.erase(0, found + 1);
//      i_tmp = atoll(str_result.c_str());
//
//      if (i_tmp == 0)
//      {
//        result->m_Settings.m_Rolloff = rolloff_Unknown;
//      }
//      else
//        if (i_tmp == 20)
//        {
//          result->m_Settings.m_Rolloff = rolloff_20;
//        }
//        else
//          if (i_tmp == 25)
//          {
//            result->m_Settings.m_Rolloff = rolloff_25;
//          }
//          else
//            if (i_tmp == 35)
//            {
//              result->m_Settings.m_Rolloff = rolloff_35;
//            }
//    }
//    else
//    {
//      result->m_Settings.m_Rolloff = rolloff_Unknown;
//    }
//
//    // Polarization
//    str_result = result->m_Settings.m_parameter;
//
//    if ((int) str_result.find("H", 0) != -1)
//      result->m_Settings.m_Polarization = pol_H;
//
//    if ((int) str_result.find("h", 0) != -1)
//      result->m_Settings.m_Polarization = pol_H;
//
//    if ((int) str_result.find("V", 0) != -1)
//      result->m_Settings.m_Polarization = pol_V;
//
//    if ((int) str_result.find("v", 0) != -1)
//      result->m_Settings.m_Polarization = pol_V;
//
//    if ((int) str_result.find("L", 0) != -1)
//      result->m_Settings.m_Polarization = pol_L;
//
//    if ((int) str_result.find("l", 0) != -1)
//      result->m_Settings.m_Polarization = pol_L;
//
//    if ((int) str_result.find("R", 0) != -1)
//      result->m_Settings.m_Polarization = pol_R;
//
//    if ((int) str_result.find("r", 0) != -1)
//      result->m_Settings.m_Polarization = pol_R;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::UpdateChannelSettings(const CTVChannelInfoTag &chaninfo)
//{
//
//
//  string     m_Summary;
//  string     m_Summary_2;
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//
//  EnterCriticalSection(&m_critSection);
//
//  if (chaninfo.m_iClientNum == -1)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SAVED;
//  }
//
//  m_Summary.Format("%d %s;%s:%i:" , chaninfo.m_iClientNum
//
//                   , chaninfo.m_strChannel.c_str()
//                   , chaninfo.m_Settings.m_strProvider.c_str()
//                   , chaninfo.m_Settings.m_Freq);
//
//  if ((chaninfo.m_Settings.m_SourceType == src_DVBS) ||
//      (chaninfo.m_Settings.m_SourceType == src_DVBS2))
//  {
//    if      (chaninfo.m_Settings.m_Polarization == pol_H)
//      m_Summary += "h";
//    else
//      if (chaninfo.m_Settings.m_Polarization == pol_V)
//        m_Summary += "v";
//      else
//        if (chaninfo.m_Settings.m_Polarization == pol_L)
//          m_Summary += "l";
//        else
//          if (chaninfo.m_Settings.m_Polarization == pol_R)
//            m_Summary += "r";
//  }
//
//  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
//  {
//    if      (chaninfo.m_Settings.m_Inversion == InvOff)
//      m_Summary += "I0";
//    else
//      if (chaninfo.m_Settings.m_Inversion == InvOn)
//        m_Summary += "I1";
//      else
//        if (chaninfo.m_Settings.m_Inversion == InvAuto)
//          m_Summary += "I999";
//  }
//
//  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
//  {
//    if      (chaninfo.m_Settings.m_Bandwidth == bw_5MHz)
//      m_Summary += "B5";
//    else
//      if (chaninfo.m_Settings.m_Bandwidth == bw_6MHz)
//        m_Summary += "B6";
//      else
//        if (chaninfo.m_Settings.m_Bandwidth == bw_7MHz)
//          m_Summary += "B7";
//        else
//          if (chaninfo.m_Settings.m_Bandwidth == bw_8MHz)
//            m_Summary += "B8";
//          else
//            if (chaninfo.m_Settings.m_Bandwidth == bw_Auto)
//              m_Summary += "B999";
//  }
//
//  if      (chaninfo.m_Settings.m_CoderateH == Coderate_None)
//    m_Summary += "C0";
//  else
//    if (chaninfo.m_Settings.m_CoderateH == Coderate_1_2)
//      m_Summary += "C12";
//    else
//      if (chaninfo.m_Settings.m_CoderateH == Coderate_2_3)
//        m_Summary += "C23";
//      else
//        if (chaninfo.m_Settings.m_CoderateH == Coderate_3_4)
//          m_Summary += "C34";
//        else
//          if (chaninfo.m_Settings.m_CoderateH == Coderate_4_5)
//            m_Summary += "C45";
//          else
//            if (chaninfo.m_Settings.m_CoderateH == Coderate_5_6)
//              m_Summary += "C56";
//            else
//              if (chaninfo.m_Settings.m_CoderateH == Coderate_6_7)
//                m_Summary += "C67";
//              else
//                if (chaninfo.m_Settings.m_CoderateH == Coderate_7_8)
//                  m_Summary += "C78";
//                else
//                  if (chaninfo.m_Settings.m_CoderateH == Coderate_8_9)
//                    m_Summary += "C89";
//                  else
//                    if (chaninfo.m_Settings.m_CoderateH == Coderate_9_10)
//                      m_Summary += "C910";
//                    else
//                      if (chaninfo.m_Settings.m_CoderateH == Coderate_Auto)
//                        m_Summary += "C999";
//
//  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
//  {
//    if      (chaninfo.m_Settings.m_CoderateL == Coderate_None)
//      m_Summary += "D0";
//    else
//      if (chaninfo.m_Settings.m_CoderateL == Coderate_1_2)
//        m_Summary += "D12";
//      else
//        if (chaninfo.m_Settings.m_CoderateL == Coderate_2_3)
//          m_Summary += "D23";
//        else
//          if (chaninfo.m_Settings.m_CoderateL == Coderate_3_4)
//            m_Summary += "D34";
//          else
//            if (chaninfo.m_Settings.m_CoderateL == Coderate_4_5)
//              m_Summary += "D45";
//            else
//              if (chaninfo.m_Settings.m_CoderateL == Coderate_5_6)
//                m_Summary += "D56";
//              else
//                if (chaninfo.m_Settings.m_CoderateL == Coderate_6_7)
//                  m_Summary += "D67";
//                else
//                  if (chaninfo.m_Settings.m_CoderateL == Coderate_7_8)
//                    m_Summary += "D78";
//                  else
//                    if (chaninfo.m_Settings.m_CoderateL == Coderate_8_9)
//                      m_Summary += "D89";
//                    else
//                      if (chaninfo.m_Settings.m_CoderateL == Coderate_9_10)
//                        m_Summary += "D910";
//                      else
//                        if (chaninfo.m_Settings.m_CoderateL == Coderate_Auto)
//                          m_Summary += "D999";
//  }
//
//  if      (chaninfo.m_Settings.m_Modulation == modNone)
//    m_Summary += "M0";
//  else
//    if (chaninfo.m_Settings.m_Modulation == modQAM4)
//      m_Summary += "M4";
//    else
//      if (chaninfo.m_Settings.m_Modulation == modQAM16)
//        m_Summary += "M16";
//      else
//        if (chaninfo.m_Settings.m_Modulation == modQAM32)
//          m_Summary += "M32";
//        else
//          if (chaninfo.m_Settings.m_Modulation == modQAM64)
//            m_Summary += "M64";
//          else
//            if (chaninfo.m_Settings.m_Modulation == modQAM128)
//              m_Summary += "M128";
//            else
//              if (chaninfo.m_Settings.m_Modulation == modQAM256)
//                m_Summary += "M256";
//              else
//                if (chaninfo.m_Settings.m_Modulation == modQAM512)
//                  m_Summary += "M512";
//                else
//                  if (chaninfo.m_Settings.m_Modulation == modQAM1024)
//                    m_Summary += "M1024";
//                  else
//                    if (chaninfo.m_Settings.m_Modulation == modQAMAuto)
//                      m_Summary += "M998";
//                    else
//                      if (chaninfo.m_Settings.m_Modulation == modBPSK)
//                        m_Summary += "M1";
//                      else
//                        if (chaninfo.m_Settings.m_Modulation == modQPSK)
//                          m_Summary += "M2";
//                        else
//                          if (chaninfo.m_Settings.m_Modulation == modOQPSK)
//                            m_Summary += "M3";
//                          else
//                            if (chaninfo.m_Settings.m_Modulation == mod8PSK)
//                              m_Summary += "M5";
//                            else
//                              if (chaninfo.m_Settings.m_Modulation == mod16APSK)
//                                m_Summary += "M6";
//                              else
//                                if (chaninfo.m_Settings.m_Modulation == mod32APSK)
//                                  m_Summary += "M7";
//                                else
//                                  if (chaninfo.m_Settings.m_Modulation == modOFDM)
//                                    m_Summary += "M8";
//                                  else
//                                    if (chaninfo.m_Settings.m_Modulation == modCOFDM)
//                                      m_Summary += "M9";
//                                    else
//                                      if (chaninfo.m_Settings.m_Modulation == modVSB8)
//                                        m_Summary += "M10";
//                                      else
//                                        if (chaninfo.m_Settings.m_Modulation == modVSB16)
//                                          m_Summary += "M11";
//
//  if (chaninfo.m_Settings.m_SourceType == src_DVBT)
//  {
//    if      (chaninfo.m_Settings.m_Transmission == transmission_2K)
//      m_Summary += "T2";
//    else
//      if (chaninfo.m_Settings.m_Transmission == transmission_4K)
//        m_Summary += "T4";
//      else
//        if (chaninfo.m_Settings.m_Transmission == transmission_8K)
//          m_Summary += "T8";
//        else
//          if (chaninfo.m_Settings.m_Transmission == transmission_Auto)
//            m_Summary += "T999";
//
//    if      (chaninfo.m_Settings.m_Guard == guard_1_4)
//      m_Summary += "G4";
//    else
//      if (chaninfo.m_Settings.m_Guard == guard_1_8)
//        m_Summary += "G8";
//      else
//        if (chaninfo.m_Settings.m_Guard == guard_1_16)
//          m_Summary += "G16";
//        else
//          if (chaninfo.m_Settings.m_Guard == guard_1_32)
//            m_Summary += "G32";
//          else
//            if (chaninfo.m_Settings.m_Guard == guard_Auto)
//              m_Summary += "G999";
//
//    if      (chaninfo.m_Settings.m_Hierarchie)
//      m_Summary += "Y1";
//    else
//      m_Summary += "Y0";
//
//    if      (chaninfo.m_Settings.m_Alpha == alpha_0)
//      m_Summary += "A0";
//    else
//      if (chaninfo.m_Settings.m_Alpha == alpha_1)
//        m_Summary += "A1";
//      else
//        if (chaninfo.m_Settings.m_Alpha == alpha_2)
//          m_Summary += "A2";
//        else
//          if (chaninfo.m_Settings.m_Alpha == alpha_4)
//            m_Summary += "A4";
//
//    if      (chaninfo.m_Settings.m_Priority)
//      m_Summary += "P1";
//    else
//      m_Summary += "P0";
//  }
//
//  if (chaninfo.m_Settings.m_SourceType == src_DVBS2)
//  {
//    if      (chaninfo.m_Settings.m_Rolloff == rolloff_Unknown)
//      m_Summary += "O0";
//    else
//      if (chaninfo.m_Settings.m_Rolloff == rolloff_20)
//        m_Summary += "O20";
//      else
//        if (chaninfo.m_Settings.m_Rolloff == rolloff_25)
//          m_Summary += "O25";
//        else
//          if (chaninfo.m_Settings.m_Rolloff == rolloff_25)
//            m_Summary += "O35";
//  }
//
//  if      (chaninfo.m_Settings.m_SourceType == src_DVBS)
//    m_Summary += "O35S0:S";
//  else
//    if (chaninfo.m_Settings.m_SourceType == src_DVBS2)
//      m_Summary += "S1:S";
//    else
//      if (chaninfo.m_Settings.m_SourceType == src_DVBC)
//        m_Summary += ":C";
//      else
//        if (chaninfo.m_Settings.m_SourceType == src_DVBT)
//          m_Summary += ":T";
//
//  m_Summary_2.Format(":%i:%i:%i,%i;%i,%i:%i:%i:%i:%i:%i:%i" , chaninfo.m_Settings.m_Symbolrate
//                     , chaninfo.m_Settings.m_VPID
//                     , chaninfo.m_Settings.m_APID1
//                     , chaninfo.m_Settings.m_APID2
//                     , chaninfo.m_Settings.m_DPID1
//                     , chaninfo.m_Settings.m_DPID2
//                     , chaninfo.m_Settings.m_TPID
//                     , chaninfo.m_Settings.m_CAID
//                     , chaninfo.m_Settings.m_SID
//                     , chaninfo.m_Settings.m_NID
//                     , chaninfo.m_Settings.m_TID
//                     , chaninfo.m_Settings.m_RID);
//
//  m_Summary += m_Summary_2;
//
//  sprintf(buffer, "LSTC %d", chaninfo.m_iClientNum);
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SYNC;
//  }
//
//  sprintf(buffer, "MODC %s", m_Summary.c_str());
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SAVED;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::AddChannel(const CTVChannelInfoTag &info)
//{
//
//  string m_Summary;
//  string m_Summary_2;
//  bool update_channel;
//  int iChannelNum;
//
//  if (!m_session->IsOpen())
//  {
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (info.m_iClientNum == -1)
//  {
//    int new_number = GetNumChannels();
//
//    if (new_number == -1)
//    {
//      new_number = 1;
//    }
//
//    iChannelNum = new_number + 1;
//
//    update_channel = false;
//  }
//  else
//  {
//    iChannelNum = info.m_iClientNum;
//    update_channel = true;
//  }
//
//  m_Summary.Format("%d %s;%s:%i:" , iChannelNum
//
//                   , info.m_strChannel.c_str()
//                   , info.m_Settings.m_strProvider.c_str()
//                   , info.m_Settings.m_Freq);
//
//  if ((info.m_Settings.m_SourceType == src_DVBS) ||
//      (info.m_Settings.m_SourceType == src_DVBS2))
//  {
//    if      (info.m_Settings.m_Polarization == pol_H)
//      m_Summary += "h";
//    else
//      if (info.m_Settings.m_Polarization == pol_V)
//        m_Summary += "v";
//      else
//        if (info.m_Settings.m_Polarization == pol_L)
//          m_Summary += "l";
//        else
//          if (info.m_Settings.m_Polarization == pol_R)
//            m_Summary += "r";
//  }
//
//  if (info.m_Settings.m_SourceType == src_DVBT)
//  {
//    if      (info.m_Settings.m_Inversion == InvOff)
//      m_Summary += "I0";
//    else
//      if (info.m_Settings.m_Inversion == InvOn)
//        m_Summary += "I1";
//      else
//        if (info.m_Settings.m_Inversion == InvAuto)
//          m_Summary += "I999";
//  }
//
//  if (info.m_Settings.m_SourceType == src_DVBT)
//  {
//    if      (info.m_Settings.m_Bandwidth == bw_5MHz)
//      m_Summary += "B5";
//    else
//      if (info.m_Settings.m_Bandwidth == bw_6MHz)
//        m_Summary += "B6";
//      else
//        if (info.m_Settings.m_Bandwidth == bw_7MHz)
//          m_Summary += "B7";
//        else
//          if (info.m_Settings.m_Bandwidth == bw_8MHz)
//            m_Summary += "B8";
//          else
//            if (info.m_Settings.m_Bandwidth == bw_Auto)
//              m_Summary += "B999";
//  }
//
//  if      (info.m_Settings.m_CoderateH == Coderate_None)
//    m_Summary += "C0";
//  else
//    if (info.m_Settings.m_CoderateH == Coderate_1_2)
//      m_Summary += "C12";
//    else
//      if (info.m_Settings.m_CoderateH == Coderate_2_3)
//        m_Summary += "C23";
//      else
//        if (info.m_Settings.m_CoderateH == Coderate_3_4)
//          m_Summary += "C34";
//        else
//          if (info.m_Settings.m_CoderateH == Coderate_4_5)
//            m_Summary += "C45";
//          else
//            if (info.m_Settings.m_CoderateH == Coderate_5_6)
//              m_Summary += "C56";
//            else
//              if (info.m_Settings.m_CoderateH == Coderate_6_7)
//                m_Summary += "C67";
//              else
//                if (info.m_Settings.m_CoderateH == Coderate_7_8)
//                  m_Summary += "C78";
//                else
//                  if (info.m_Settings.m_CoderateH == Coderate_8_9)
//                    m_Summary += "C89";
//                  else
//                    if (info.m_Settings.m_CoderateH == Coderate_9_10)
//                      m_Summary += "C910";
//                    else
//                      if (info.m_Settings.m_CoderateH == Coderate_Auto)
//                        m_Summary += "C999";
//
//  if (info.m_Settings.m_SourceType == src_DVBT)
//  {
//    if      (info.m_Settings.m_CoderateL == Coderate_None)
//      m_Summary += "D0";
//    else
//      if (info.m_Settings.m_CoderateL == Coderate_1_2)
//        m_Summary += "D12";
//      else
//        if (info.m_Settings.m_CoderateL == Coderate_2_3)
//          m_Summary += "D23";
//        else
//          if (info.m_Settings.m_CoderateL == Coderate_3_4)
//            m_Summary += "D34";
//          else
//            if (info.m_Settings.m_CoderateL == Coderate_4_5)
//              m_Summary += "D45";
//            else
//              if (info.m_Settings.m_CoderateL == Coderate_5_6)
//                m_Summary += "D56";
//              else
//                if (info.m_Settings.m_CoderateL == Coderate_6_7)
//                  m_Summary += "D67";
//                else
//                  if (info.m_Settings.m_CoderateL == Coderate_7_8)
//                    m_Summary += "D78";
//                  else
//                    if (info.m_Settings.m_CoderateL == Coderate_8_9)
//                      m_Summary += "D89";
//                    else
//                      if (info.m_Settings.m_CoderateL == Coderate_9_10)
//                        m_Summary += "D910";
//                      else
//                        if (info.m_Settings.m_CoderateL == Coderate_Auto)
//                          m_Summary += "D999";
//  }
//
//  if      (info.m_Settings.m_Modulation == modNone)
//    m_Summary += "M0";
//  else
//    if (info.m_Settings.m_Modulation == modQAM4)
//      m_Summary += "M4";
//    else
//      if (info.m_Settings.m_Modulation == modQAM16)
//        m_Summary += "M16";
//      else
//        if (info.m_Settings.m_Modulation == modQAM32)
//          m_Summary += "M32";
//        else
//          if (info.m_Settings.m_Modulation == modQAM64)
//            m_Summary += "M64";
//          else
//            if (info.m_Settings.m_Modulation == modQAM128)
//              m_Summary += "M128";
//            else
//              if (info.m_Settings.m_Modulation == modQAM256)
//                m_Summary += "M256";
//              else
//                if (info.m_Settings.m_Modulation == modQAM512)
//                  m_Summary += "M512";
//                else
//                  if (info.m_Settings.m_Modulation == modQAM1024)
//                    m_Summary += "M1024";
//                  else
//                    if (info.m_Settings.m_Modulation == modQAMAuto)
//                      m_Summary += "M998";
//                    else
//                      if (info.m_Settings.m_Modulation == modBPSK)
//                        m_Summary += "M1";
//                      else
//                        if (info.m_Settings.m_Modulation == modQPSK)
//                          m_Summary += "M2";
//                        else
//                          if (info.m_Settings.m_Modulation == modOQPSK)
//                            m_Summary += "M3";
//                          else
//                            if (info.m_Settings.m_Modulation == mod8PSK)
//                              m_Summary += "M5";
//                            else
//                              if (info.m_Settings.m_Modulation == mod16APSK)
//                                m_Summary += "M6";
//                              else
//                                if (info.m_Settings.m_Modulation == mod32APSK)
//                                  m_Summary += "M7";
//                                else
//                                  if (info.m_Settings.m_Modulation == modOFDM)
//                                    m_Summary += "M8";
//                                  else
//                                    if (info.m_Settings.m_Modulation == modCOFDM)
//                                      m_Summary += "M9";
//                                    else
//                                      if (info.m_Settings.m_Modulation == modVSB8)
//                                        m_Summary += "M10";
//                                      else
//                                        if (info.m_Settings.m_Modulation == modVSB16)
//                                          m_Summary += "M11";
//
//  if (info.m_Settings.m_SourceType == src_DVBT)
//  {
//    if      (info.m_Settings.m_Transmission == transmission_2K)
//      m_Summary += "T2";
//    else
//      if (info.m_Settings.m_Transmission == transmission_4K)
//        m_Summary += "T4";
//      else
//        if (info.m_Settings.m_Transmission == transmission_8K)
//          m_Summary += "T8";
//        else
//          if (info.m_Settings.m_Transmission == transmission_Auto)
//            m_Summary += "T999";
//
//    if      (info.m_Settings.m_Guard == guard_1_4)
//      m_Summary += "G4";
//    else
//      if (info.m_Settings.m_Guard == guard_1_8)
//        m_Summary += "G8";
//      else
//        if (info.m_Settings.m_Guard == guard_1_16)
//          m_Summary += "G16";
//        else
//          if (info.m_Settings.m_Guard == guard_1_32)
//            m_Summary += "G32";
//          else
//            if (info.m_Settings.m_Guard == guard_Auto)
//              m_Summary += "G999";
//
//    if      (info.m_Settings.m_Hierarchie)
//      m_Summary += "Y1";
//    else
//      m_Summary += "Y0";
//
//    if      (info.m_Settings.m_Alpha == alpha_0)
//      m_Summary += "A0";
//    else
//      if (info.m_Settings.m_Alpha == alpha_1)
//        m_Summary += "A1";
//      else
//        if (info.m_Settings.m_Alpha == alpha_2)
//          m_Summary += "A2";
//        else
//          if (info.m_Settings.m_Alpha == alpha_4)
//            m_Summary += "A4";
//
//    if      (info.m_Settings.m_Priority)
//      m_Summary += "P1";
//    else
//      m_Summary += "P0";
//  }
//
//  if (info.m_Settings.m_SourceType == src_DVBS2)
//  {
//    if      (info.m_Settings.m_Rolloff == rolloff_Unknown)
//      m_Summary += "O0";
//    else
//      if (info.m_Settings.m_Rolloff == rolloff_20)
//        m_Summary += "O20";
//      else
//        if (info.m_Settings.m_Rolloff == rolloff_25)
//          m_Summary += "O25";
//        else
//          if (info.m_Settings.m_Rolloff == rolloff_25)
//            m_Summary += "O35";
//  }
//
//  if      (info.m_Settings.m_SourceType == src_DVBS)
//    m_Summary += "O35S0:S";
//  else
//    if (info.m_Settings.m_SourceType == src_DVBS2)
//      m_Summary += "S1:S";
//    else
//      if (info.m_Settings.m_SourceType == src_DVBC)
//        m_Summary += ":C";
//      else
//        if (info.m_Settings.m_SourceType == src_DVBT)
//          m_Summary += ":T";
//
//  m_Summary_2.Format(":%i:%i:%i,%i;%i,%i:%i:%i:%i:%i:%i:%i" , info.m_Settings.m_Symbolrate
//                     , info.m_Settings.m_VPID
//                     , info.m_Settings.m_APID1
//                     , info.m_Settings.m_APID2
//                     , info.m_Settings.m_DPID1
//                     , info.m_Settings.m_DPID2
//                     , info.m_Settings.m_TPID
//                     , info.m_Settings.m_CAID
//                     , info.m_Settings.m_SID
//                     , info.m_Settings.m_NID
//                     , info.m_Settings.m_TID
//                     , info.m_Settings.m_RID);
//
//  m_Summary += m_Summary_2;
//
//  fprintf(stderr, "<<<<<<<<<<< '%s' \n", m_Summary.c_str());
//
//  vector<string> lines;
//
//  int            code;
//
//  char           buffer[1024];
//
//  EnterCriticalSection(&m_critSection);
//
//  if (!update_channel)
//  {
//    sprintf(buffer, "NEWC %s", m_Summary.c_str());
//
//    if (!m_session->SendCommand(buffer, code, lines))
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVR_ERROR_SERVER_ERROR;
//    }
//
//    if (code != 250)
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVRErr_NOT_SAVED;
//    }
//  }
//  else
//  {
//    // Modified channel
//    sprintf(buffer, "LSTC %d", iChannelNum);
//
//    if (!m_session->SendCommand(buffer, code, lines))
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVR_ERROR_SERVER_ERROR;
//    }
//
//    if (code != 250)
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVRErr_NOT_SYNC;
//    }
//
//    sprintf(buffer, "MODC %s", m_Summary.c_str());
//
//    if (!m_session->SendCommand(buffer, code, lines))
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVR_ERROR_SERVER_ERROR;
//    }
//
//    if (code != 250)
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVRErr_NOT_SAVED;
//    }
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::DeleteChannel(unsigned int number)
//{
//
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//
//  if (!m_session->IsOpen())
//  {
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTC %d", number);
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    return PVRErr_NOT_SYNC;
//  }
//
//  sprintf(buffer, "DELC %d", number);
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    return PVRErr_NOT_DELETED;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::RenameChannel(unsigned int number, string &newname)
//{
//
//  string     str_part1;
//  string     str_part2;
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//  int            found;
//
//  if (!m_session->IsOpen())
//  {
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTC %d", number);
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SYNC;
//  }
//
//  vector<string>::iterator it = lines.begin();
//
//  string& data(*it);
//  string str_result = data;
//
//  found = str_result.find(" ", 0);
//  str_part1.assign(str_result, found + 1);
//  str_result.erase(0, found + 1);
//
//  /// Channel and provider name
//  found = str_result.find(":", 0);
//  str_part2.assign(str_result, found);
//  str_result.erase(0, found);
//  found = str_part2.find(";", 0);
//
//  if (found == -1)
//  {
//    str_part2 = newname;
//  }
//  else
//  {
//    str_part2.erase(0, found);
//    str_part2.insert(0, newname);
//  }
//
//  sprintf(buffer, "MODC %s %s %s", str_part1.c_str(), str_part2.c_str(), str_result.c_str());
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SAVED;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::MoveChannel(unsigned int number, unsigned int newnumber)
//{
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//
//  if (!m_session->IsOpen())
//      return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTC %d", number);
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SYNC;
//  }
//
//  sprintf(buffer, "MOVC %d %d", number, newnumber);
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SAVED;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//
///************************************************************/
///** Record handling **/
//
//int PVRClientVDR::GetNumRecordings(void)
//{
//  vector<string>  lines;
//  int             code;
//
//  if (!m_session->IsOpen())
//    return -1;
//
//  EnterCriticalSection(&m_critSection);
//
//  if (!m_session->SendCommand("STAT records", code, lines))
//  {
//    CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get recordings count");
//    LeaveCriticalSection(&m_critSection);
//    return -1;
//  }
//
//  vector<string>::iterator it = lines.begin();
//
//  string& data(*it);
//  LeaveCriticalSection(&m_critSection);
//  return atoll(data.c_str());
//}
//
//PVR_ERROR PVRClientVDR::GetAllRecordings(VECRECORDINGS *results)
//{
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//  int            found, cnt = 0;
//  string     strbuffer;
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  if (!m_session->SendCommand("LSTR", code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  for (vector<string>::iterator it2 = lines.begin(); it2 != lines.end(); it2++)
//  {
//    string& data(*it2);
//    string str_result = data;
//    CTVRecordingInfoTag broadcast;
//
//    /* Convert to UTF8 string format */
//
//    if (!m_bCharsetIsUTF8)
//      g_charsetConverter.stringCharsetToUtf8(str_result);
//
//    /* Get recording ID */
//    broadcast.m_Index = atoll(str_result.c_str());
//
//    str_result.erase(0, 18);
//
//    /* Get recording name */
//    broadcast.m_strTitle = str_result.c_str();
//
//    /* Set file string for replay devices */
//    broadcast.m_strFileNameAndPath.Format("record://%i", broadcast.m_Index);
//
//    /* Save it inside list */
//    results->push_back(broadcast);
//
//    cnt++;
//  }
//
///// Commented out the description downloading, due to frezze XBMC
///// TODO: Fix Streamdev-Plugin LSTR function
//  std::vector<CTVRecordingInfoTag>::iterator it;
//
//  for (int i = 0; i < cnt; i++)
//  {
//    it = results->begin() + i;
//
//    lines.erase(lines.begin(), lines.end());
//    sprintf(buffer, "LSTR %d", (*it).m_Index);
//
//    if (!m_session->SendCommand(buffer, code, lines))
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVR_ERROR_SERVER_ERROR;
//    }
//
//    for (vector<string>::iterator it2 = lines.begin(); it2 != lines.end(); it2++)
//    {
//      string str_result;
//      string& data(*it2);
//      string strbuffer = data;
//
//      /* Convert to UTF8 string format */
//      g_charsetConverter.stringCharsetToUtf8(strbuffer);
//
//      /* Get Channelname */
//      str_result = strbuffer;
//      found = str_result.find("C", 0);
//
//      if (found == 0)
//      {
//        str_result.erase(0, 2);
//        found = str_result.find(" ", 0);
//        str_result.erase(0, found + 1);
//        (*it).m_strChannel = str_result.c_str();
//        continue;
//      }
//
//      /* Get Title */
//      str_result = strbuffer;
//
//      found = str_result.find("T", 0);
//
//      if (found == 0)
//      {
//        str_result.erase(0, 2);
//        //   (*it).m_strTitle = str_result.c_str();
//        continue;
//      }
//
//      /* Get short description */
//      str_result = strbuffer;
//
//      found = str_result.find("S", 0);
//
//      if (found == 0)
//      {
//        str_result.erase(0, 2);
//        (*it).m_strPlotOutline   = str_result.c_str();
//        continue;
//      }
//
//      /* Get description */
//      str_result = strbuffer;
//
//      found = str_result.find("D", 0);
//
//      if (found == 0)
//      {
//        str_result.erase(0, 2);
//        int pos = 0;
//
//        while (1)
//        {
//          pos = str_result.find("|", pos);
//
//          if (pos < 0)
//            break;
//
//          str_result.replace(pos, 1, 1, '\n');
//        }
//
//        (*it).m_strPlot = str_result.c_str();
//        continue;
//      }
//
//      /* Get ID, date and length */
//      str_result = strbuffer;
//
//      found = str_result.find("E ", 0);
//
//      if (found == 0)
//      {
//        time_t rec_time;
//        int duration;
//        str_result.erase(0, 2);
//        // (*it).m_uniqueID = atoll(str_result.c_str());
//
//        found = str_result.find(" ", 0);
//        str_result.erase(0, found + 1);
//
//        rec_time = atoll(str_result.c_str());
//        found = str_result.find(" ", 0);
//        str_result.erase(0, found + 1);
//        duration = atoll(str_result.c_str());
//
//        (*it).m_startTime = CDateTime((time_t)rec_time);
//        (*it).m_endTime = CDateTime((time_t)rec_time + duration);
//        (*it).m_duration = CDateTimeSpan(0, 0, duration / 60, duration % 60);
//        (*it).m_Summary.Format("%s", (*it).m_startTime.GetAsLocalizedDateTime(false, false));
//        continue;
//      }
//
////            /* Get Video description */
////            str_result = strbuffer;
////            found = str_result.find("X 1", 0);
////            if (found == 0) {
////                str_result.erase(0,4);
////                if (str_result.find("16:9", 0) == 0) {
////                    (*it).m_videoProps.push_back(VID_WIDESCREEN);
////                }
////                else if (str_result.find("Breitwand", 0) == 0) {
////                    (*it).m_videoProps.push_back(VID_WIDESCREEN);
////                }
////                else if (str_result.find("4:3", 0) == 0) {
////                    (*it).m_videoProps.push_back(VID_AVC);
////                }
////                else {
////                    (*it).m_videoProps.push_back(VID_UNKNOWN);
////                }
////            }
////
////            /* Get Audio description */
////            str_result = strbuffer;
////            str_result.erase(0,4);
////            found = str_result.find("X 2", 0);
////            if (found == 0) {
////                str_result.erase(0,4);
////                if (str_result.find("Dolby Digital", 0) == 0) {
////                    (*it).m_audioProps.push_back(AUD_DOLBY);
////                }
////                else if (str_result.find("Stereo", 0) == 0) {
////                    (*it).m_audioProps.push_back(AUD_STEREO);
////                }
////                else if (str_result.find("mono/Hörfilm", 0) == 0) {
////                    (*it).m_audioProps.push_back(AUD_MONO);
////                }
////                else if (str_result.find("Audio Description", 0) == 0) {
////                    (*it).m_audioProps.push_back(AUD_UNKNOWN);
////                }
////                else {
////                    (*it).m_audioProps.push_back(AUD_UNKNOWN);
////                }
////            }
//    }
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::DeleteRecording(const CTVRecordingInfoTag &recinfo)
//{
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTR %d", recinfo.m_Index);
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 215)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SYNC;
//  }
//
//  sprintf(buffer, "DELR %d", recinfo.m_Index);
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_DELETED;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::RenameRecording(const CTVRecordingInfoTag &recinfo, string &newname)
//{
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTR %d", recinfo.m_Index);
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 215)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SYNC;
//  }
//
//  sprintf(buffer, "RENR %d %s", recinfo.m_Index, newname.c_str());
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_DELETED;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//
///************************************************************/
///** Timer handling */
//
//int PVRClientVDR::GetNumTimers(void)
//{
//  vector<string>  lines;
//  int             code;
//
//  if (!m_session->IsOpen())
//    return -1;
//
//  EnterCriticalSection(&m_critSection);
//
//  if (!m_session->SendCommand("STAT timers", code, lines))
//  {
//    CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get timers count");
//    LeaveCriticalSection(&m_critSection);
//    return -1;
//  }
//
//  vector<string>::iterator it = lines.begin();
//
//  string& data(*it);
//  LeaveCriticalSection(&m_critSection);
//  return atoll(data.c_str());
//}
//
//PVR_ERROR PVRClientVDR::GetAllTimers(VECTVTIMERS *results)
//{
//  vector<string> lines;
//  int            code;
//  int            found;
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  if (!m_session->SendCommand("LSTT", code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  for (vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
//  {
//    string& data(*it);
//    string str_result = data;
//    string name;
//    CTVTimerInfoTag timerinfo;
//
//    /**
//     * VDR Format given by LSTT:
//     * 250-1 1:6:2008-10-27:0013:0055:50:99:Zeiglers wunderbare Welt des Fußballs:
//     * 250 2 0:15:2008-10-26:2000:2138:50:99:ZDFtheaterkanal:
//     * 250 3 1:6:MTWTFS-:2000:2129:50:99:WDR Köln:
//     */
//
//    if (!m_bCharsetIsUTF8)
//      g_charsetConverter.stringCharsetToUtf8(str_result);
//
//    /* Id */
//    timerinfo.m_Index = atoll(str_result.c_str());
//
//    found = str_result.find(" ", 0);
//
//    str_result.erase(0, found + 1);
//
//    /* Active */
//    timerinfo.m_Active = atoll(str_result.c_str());
//
//    str_result.erase(0, 2);
//
//    /* Channel number */
//    timerinfo.m_clientNum = atoll(str_result.c_str());
//
//    found = str_result.find(":", 0);
//
//    str_result.erase(0, found + 1);
//
//    /* Start/end time */
//    int year  = atoll(str_result.c_str());
//
//    int month = 0;
//
//    int day   = 0;
//
//    timerinfo.m_FirstDay = NULL;
//
//    if (year != 0)
//    {
//      timerinfo.m_Repeat = false;
//      found = str_result.find("-", 0);
//      str_result.erase(0, found + 1);
//      month = atoll(str_result.c_str());
//      found = str_result.find("-", 0);
//      str_result.erase(0, found + 1);
//      day   = atoll(str_result.c_str());
//      found = str_result.find(":", 0);
//      str_result.erase(0, found + 1);
//
//      timerinfo.m_Repeat_Mon = false;
//      timerinfo.m_Repeat_Tue = false;
//      timerinfo.m_Repeat_Wed = false;
//      timerinfo.m_Repeat_Thu = false;
//      timerinfo.m_Repeat_Fri = false;
//      timerinfo.m_Repeat_Sat = false;
//      timerinfo.m_Repeat_Sun = false;
//    }
//    else
//    {
//      timerinfo.m_Repeat = true;
//
//      timerinfo.m_Repeat_Mon = str_result.compare(0, 1, "-") ? true : false;
//      timerinfo.m_Repeat_Tue = str_result.compare(1, 1, "-") ? true : false;
//      timerinfo.m_Repeat_Wed = str_result.compare(2, 1, "-") ? true : false;
//      timerinfo.m_Repeat_Thu = str_result.compare(3, 1, "-") ? true : false;
//      timerinfo.m_Repeat_Fri = str_result.compare(4, 1, "-") ? true : false;
//      timerinfo.m_Repeat_Sat = str_result.compare(5, 1, "-") ? true : false;
//      timerinfo.m_Repeat_Sun = str_result.compare(6, 1, "-") ? true : false;
//
//      str_result.erase(0, 7);
//      found = str_result.find("@", 0);
//
//      if (found != -1)
//      {
//        str_result.erase(0, 1);
//        year  = atoll(str_result.c_str());
//        found = str_result.find("-", 0);
//        str_result.erase(0, found + 1);
//
//        month = atoll(str_result.c_str());
//        found = str_result.find("-", 0);
//        str_result.erase(0, found + 1);
//
//        day   = atoll(str_result.c_str());
//      }
//
//      found = str_result.find(":", 0);
//
//      str_result.erase(0, found + 1);
//    }
//
//    name.assign(str_result, 2);
//
//    str_result.erase(0, 2);
//    int start_hour = atoll(name.c_str());
//
//    name.assign(str_result, 2);
//    str_result.erase(0, 3);
//    int start_minute = atoll(name.c_str());
//
//    name.assign(str_result, 2);
//    str_result.erase(0, 2);
//    int end_hour = atoll(name.c_str());
//
//    name.assign(str_result, 2);
//    str_result.erase(0, 3);
//    int end_minute = atoll(name.c_str());
//
//    if (!timerinfo.m_Repeat)
//    {
//      int end_day = (start_hour > end_hour ? day + 1 : day);
//      timerinfo.m_StartTime = CDateTime(year, month, day, start_hour, start_minute, 0);
//      timerinfo.m_StopTime = CDateTime(year, month, end_day, end_hour, end_minute, 0);
//    }
//    else
//      if (year != 0)
//      {
//        timerinfo.m_FirstDay = CDateTime(year, month, day, start_hour, start_minute, 0);
//        timerinfo.m_StartTime = CDateTime(year, month, day, start_hour, start_minute, 0);
//        timerinfo.m_StopTime = CDateTime(year, month, day, end_hour, end_minute, 0);
//      }
//      else
//      {
//        timerinfo.m_StartTime = CDateTime(CDateTime::GetCurrentDateTime().GetYear(),
//                                          CDateTime::GetCurrentDateTime().GetMonth(),
//                                          CDateTime::GetCurrentDateTime().GetDay(),
//                                          start_hour, start_minute, 0);
//        timerinfo.m_StopTime = CDateTime(CDateTime::GetCurrentDateTime().GetYear(),
//                                         CDateTime::GetCurrentDateTime().GetMonth(),
//                                         CDateTime::GetCurrentDateTime().GetDay(),
//                                         end_hour, end_minute, 0);
//      }
//
//    /* Priority */
//    timerinfo.m_Priority = atoll(str_result.c_str());
//
//    found = str_result.find(":", 0);
//
//    str_result.erase(0, found + 1);
//
//    /* Lifetime */
//    timerinfo.m_Lifetime = atoll(str_result.c_str());
//
//    found = str_result.find(":", 0);
//
//    str_result.erase(0, found + 1);
//
//    /* Title */
//    found = str_result.find(":", 0);
//
//    str_result.erase(found, found + 1);
//
//    timerinfo.m_strTitle = str_result.c_str();
//
//    if (!timerinfo.m_Repeat)
//    {
//      timerinfo.m_Summary.Format("%s %s %s %s %s", timerinfo.m_StartTime.GetAsLocalizedDate()
//                                 , g_localizeStrings.Get(18078)
//                                 , timerinfo.m_StartTime.GetAsLocalizedTime("", false)
//                                 , g_localizeStrings.Get(18079)
//                                 , timerinfo.m_StopTime.GetAsLocalizedTime("", false));
//    }
//    else
//      if (timerinfo.m_FirstDay != NULL)
//      {
//        timerinfo.m_Summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s %s %s"
//                                   , timerinfo.m_Repeat_Mon ? g_localizeStrings.Get(18080) : "__"
//                                   , timerinfo.m_Repeat_Tue ? g_localizeStrings.Get(18081) : "__"
//                                   , timerinfo.m_Repeat_Wed ? g_localizeStrings.Get(18082) : "__"
//                                   , timerinfo.m_Repeat_Thu ? g_localizeStrings.Get(18083) : "__"
//                                   , timerinfo.m_Repeat_Fri ? g_localizeStrings.Get(18084) : "__"
//                                   , timerinfo.m_Repeat_Sat ? g_localizeStrings.Get(18085) : "__"
//                                   , timerinfo.m_Repeat_Sun ? g_localizeStrings.Get(18086) : "__"
//                                   , g_localizeStrings.Get(18087)
//                                   , timerinfo.m_FirstDay.GetAsLocalizedDate(false)
//                                   , g_localizeStrings.Get(18078)
//                                   , timerinfo.m_StartTime.GetAsLocalizedTime("", false)
//                                   , g_localizeStrings.Get(18079)
//                                   , timerinfo.m_StopTime.GetAsLocalizedTime("", false));
//      }
//      else
//      {
//        timerinfo.m_Summary.Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s"
//                                   , timerinfo.m_Repeat_Mon ? g_localizeStrings.Get(18080) : "__"
//                                   , timerinfo.m_Repeat_Tue ? g_localizeStrings.Get(18081) : "__"
//                                   , timerinfo.m_Repeat_Wed ? g_localizeStrings.Get(18082) : "__"
//                                   , timerinfo.m_Repeat_Thu ? g_localizeStrings.Get(18083) : "__"
//                                   , timerinfo.m_Repeat_Fri ? g_localizeStrings.Get(18084) : "__"
//                                   , timerinfo.m_Repeat_Sat ? g_localizeStrings.Get(18085) : "__"
//                                   , timerinfo.m_Repeat_Sun ? g_localizeStrings.Get(18086) : "__"
//                                   , g_localizeStrings.Get(18078)
//                                   , timerinfo.m_StartTime.GetAsLocalizedTime("", false)
//                                   , g_localizeStrings.Get(18079)
//                                   , timerinfo.m_StopTime.GetAsLocalizedTime("", false));
//      }
//
//    if ((timerinfo.m_StartTime <= CDateTime::GetCurrentDateTime()) &&
//        (timerinfo.m_Active == true))
//    {
//      timerinfo.m_recStatus = true;
//      timerinfo.m_strFileNameAndPath.Format("timer://%i #", timerinfo.m_Index);
//    }
//    else
//    {
//      timerinfo.m_strFileNameAndPath.Format("timer://%i %s", timerinfo.m_Index, timerinfo.m_Active ? "*" : " ");
//      timerinfo.m_recStatus = false;
//    }
//
//    results->push_back(timerinfo);
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::GetTimerInfo(unsigned int timernumber, CTVTimerInfoTag &timerinfo)
//{
//  vector<string>  lines;
//  int             code;
//  char            buffer[1024];
//  string      name;
//  int             found;
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTT %d", timernumber);
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  vector<string>::iterator it = lines.begin();
//
//  string& data(*it);
//  string str_result = data;
//
//  if (!m_bCharsetIsUTF8)
//    g_charsetConverter.stringCharsetToUtf8(str_result);
//
//  /* Id */
//  timerinfo.m_Index = atoll(str_result.c_str());
//
//  found = str_result.find(" ", 0);
//
//  str_result.erase(0, found + 1);
//
//  /* Active */
//  timerinfo.m_Active = atoll(str_result.c_str());
//
//  str_result.erase(0, 2);
//
//  /* Channel number */
//  timerinfo.m_clientNum = atoll(str_result.c_str());
//
//  found = str_result.find(":", 0);
//
//  str_result.erase(0, found + 1);
//
//  /* Start/end time */
//  int year  = atoll(str_result.c_str());
//
//  int month = 0;
//
//  int day   = 0;
//
//  timerinfo.m_FirstDay = NULL;
//
//  if (year != 0)
//  {
//    timerinfo.m_Repeat = false;
//    found = str_result.find("-", 0);
//    str_result.erase(0, found + 1);
//    month = atoll(str_result.c_str());
//    found = str_result.find("-", 0);
//    str_result.erase(0, found + 1);
//    day   = atoll(str_result.c_str());
//    found = str_result.find(":", 0);
//    str_result.erase(0, found + 1);
//
//    timerinfo.m_Repeat_Mon = false;
//    timerinfo.m_Repeat_Tue = false;
//    timerinfo.m_Repeat_Wed = false;
//    timerinfo.m_Repeat_Thu = false;
//    timerinfo.m_Repeat_Fri = false;
//    timerinfo.m_Repeat_Sat = false;
//    timerinfo.m_Repeat_Sun = false;
//  }
//  else
//  {
//    timerinfo.m_Repeat = true;
//    timerinfo.m_Repeat_Mon = str_result.compare(0, 1, "-") ? true : false;
//    timerinfo.m_Repeat_Tue = str_result.compare(1, 1, "-") ? true : false;
//    timerinfo.m_Repeat_Wed = str_result.compare(2, 1, "-") ? true : false;
//    timerinfo.m_Repeat_Thu = str_result.compare(3, 1, "-") ? true : false;
//    timerinfo.m_Repeat_Fri = str_result.compare(4, 1, "-") ? true : false;
//    timerinfo.m_Repeat_Sat = str_result.compare(5, 1, "-") ? true : false;
//    timerinfo.m_Repeat_Sun = str_result.compare(6, 1, "-") ? true : false;
//
//    str_result.erase(0, 7);
//    found = str_result.find("@", 0);
//
//    if (found != -1)
//    {
//      year  = atoll(str_result.c_str());
//      found = str_result.find("-", 0);
//      str_result.erase(0, found + 1);
//
//      month = atoll(str_result.c_str());
//      found = str_result.find("-", 0);
//      str_result.erase(0, found + 1);
//
//      day   = atoll(str_result.c_str());
//    }
//
//    found = str_result.find(":", 0);
//
//    str_result.erase(0, found + 1);
//  }
//
//  name.assign(str_result, 2);
//
//  str_result.erase(0, 2);
//  int start_hour = atoll(name.c_str());
//
//  name.assign(str_result, 2);
//  str_result.erase(0, 3);
//  int start_minute = atoll(name.c_str());
//
//  name.assign(str_result, 2);
//  str_result.erase(0, 2);
//  int end_hour = atoll(name.c_str());
//
//  name.assign(str_result, 2);
//  str_result.erase(0, 3);
//  int end_minute = atoll(name.c_str());
//
//  if (!timerinfo.m_Repeat)
//  {
//    int end_day = (start_hour > end_hour ? day + 1 : day);
//    timerinfo.m_StartTime = CDateTime(year, month, day, start_hour, start_minute, 0);
//    timerinfo.m_StopTime = CDateTime(year, month, end_day, end_hour, end_minute, 0);
//  }
//  else
//    if (year != 0)
//    {
//      timerinfo.m_FirstDay = CDateTime(year, month, day, start_hour, start_minute, 0);
//      timerinfo.m_StartTime = CDateTime(year, month, day, start_hour, start_minute, 0);
//      timerinfo.m_StopTime = CDateTime(year, month, day, end_hour, end_minute, 0);
//    }
//    else
//    {
//      timerinfo.m_StartTime = CDateTime(CDateTime::GetCurrentDateTime().GetYear(),
//                                        CDateTime::GetCurrentDateTime().GetMonth(),
//                                        CDateTime::GetCurrentDateTime().GetDay(),
//                                        start_hour, start_minute, 0);
//      timerinfo.m_StopTime = CDateTime(CDateTime::GetCurrentDateTime().GetYear(),
//                                       CDateTime::GetCurrentDateTime().GetMonth(),
//                                       CDateTime::GetCurrentDateTime().GetDay(),
//                                       end_hour, end_minute, 0);
//    }
//
//  /* Priority */
//  timerinfo.m_Priority = atoll(str_result.c_str());
//
//  found = str_result.find(":", 0);
//
//  str_result.erase(0, found + 1);
//
//  /* Lifetime */
//  timerinfo.m_Lifetime = atoll(str_result.c_str());
//
//  found = str_result.find(":", 0);
//
//  str_result.erase(0, found + 1);
//
//  /* Title */
//  found = str_result.find(":", 0);
//
//  str_result.erase(found, found + 1);
//
//  timerinfo.m_strTitle = str_result.c_str();
//
//  timerinfo.m_strPath.Format("timer://%i%s", timerinfo.m_Index, timerinfo.m_Active ? " *" : "");
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::AddTimer(const CTVTimerInfoTag &timerinfo)
//{
//  string     m_Summary;
//  string     m_Summary_2;
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  if (!timerinfo.m_Repeat)
//  {
//    m_Summary.Format("%d:%d:%04d-%02d-%02d:%02d%02d:%02d%02d:%d:%d:%s"
//                     , timerinfo.m_Active
//                     , timerinfo.m_clientNum
//                     , timerinfo.m_StartTime.GetYear()
//                     , timerinfo.m_StartTime.GetMonth()
//                     , timerinfo.m_StartTime.GetDay()
//                     , timerinfo.m_StartTime.GetHour()
//                     , timerinfo.m_StartTime.GetMinute()
//                     , timerinfo.m_StopTime.GetHour()
//                     , timerinfo.m_StopTime.GetMinute()
//                     , timerinfo.m_Priority
//                     , timerinfo.m_Lifetime
//                     , timerinfo.m_strTitle.c_str());
//  }
//  else
//    if (timerinfo.m_FirstDay != NULL)
//    {
//      m_Summary.Format("%d:%d:%c%c%c%c%c%c%c@"
//                       , timerinfo.m_Active
//                       , timerinfo.m_clientNum
//                       , timerinfo.m_Repeat_Mon ? 'M' : '-'
//                       , timerinfo.m_Repeat_Tue ? 'T' : '-'
//                       , timerinfo.m_Repeat_Wed ? 'W' : '-'
//                       , timerinfo.m_Repeat_Thu ? 'T' : '-'
//                       , timerinfo.m_Repeat_Fri ? 'F' : '-'
//                       , timerinfo.m_Repeat_Sat ? 'S' : '-'
//                       , timerinfo.m_Repeat_Sun ? 'S' : '-');
//      m_Summary_2.Format("%04d-%02d-%02d:%02d%02d:%02d%02d:%d:%d:%s"
//                         , timerinfo.m_FirstDay.GetYear()
//                         , timerinfo.m_FirstDay.GetMonth()
//                         , timerinfo.m_FirstDay.GetDay()
//                         , timerinfo.m_StartTime.GetHour()
//                         , timerinfo.m_StartTime.GetMinute()
//                         , timerinfo.m_StopTime.GetHour()
//                         , timerinfo.m_StopTime.GetMinute()
//                         , timerinfo.m_Priority
//                         , timerinfo.m_Lifetime
//                         , timerinfo.m_strTitle.c_str());
//      m_Summary = m_Summary + m_Summary_2;
//    }
//    else
//    {
//      m_Summary.Format("%d:%d:%c%c%c%c%c%c%c:%02d%02d:%02d%02d:%d:%d:%s"
//                       , timerinfo.m_Active
//                       , timerinfo.m_clientNum
//                       , timerinfo.m_Repeat_Mon ? 'M' : '-'
//                       , timerinfo.m_Repeat_Tue ? 'T' : '-'
//                       , timerinfo.m_Repeat_Wed ? 'W' : '-'
//                       , timerinfo.m_Repeat_Thu ? 'T' : '-'
//                       , timerinfo.m_Repeat_Fri ? 'F' : '-'
//                       , timerinfo.m_Repeat_Sat ? 'S' : '-'
//                       , timerinfo.m_Repeat_Sun ? 'S' : '-'
//                       , timerinfo.m_StartTime.GetHour()
//                       , timerinfo.m_StartTime.GetMinute()
//                       , timerinfo.m_StopTime.GetHour()
//                       , timerinfo.m_StopTime.GetMinute()
//                       , timerinfo.m_Priority
//                       , timerinfo.m_Lifetime
//                       , timerinfo.m_strTitle.c_str());
//    }
//
//  EnterCriticalSection(&m_critSection);
//
//  if (timerinfo.m_Index == -1)
//  {
//    sprintf(buffer, "NEWT %s", m_Summary.c_str());
//
//    if (!m_session->SendCommand(buffer, code, lines))
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVRErr_NOT_SAVED;
//    }
//
//    if (code != 250)
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVRErr_NOT_SYNC;
//    }
//  }
//  else
//  {
//    // Modified timer
//    sprintf(buffer, "LSTT %d", timerinfo.m_Index);
//
//    if (!m_session->SendCommand(buffer, code, lines))
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVR_ERROR_SERVER_ERROR;
//    }
//
//    if (code != 250)
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVRErr_NOT_SYNC;
//    }
//
//    sprintf(buffer, "MODT %d %s", timerinfo.m_Index, m_Summary.c_str());
//
//    if (!m_session->SendCommand(buffer, code, lines))
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVRErr_NOT_SAVED;
//    }
//
//    if (code != 250)
//    {
//      LeaveCriticalSection(&m_critSection);
//      return PVRErr_NOT_SYNC;
//    }
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force)
//{
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTT %d", timerinfo.m_Index);
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SYNC;
//  }
//
//  lines.erase(lines.begin(), lines.end());
//
//  if (force)
//  {
//    sprintf(buffer, "DELT %d FORCE", timerinfo.m_Index);
//  }
//  else
//  {
//    sprintf(buffer, "DELT %d", timerinfo.m_Index);
//  }
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    vector<string>::iterator it = lines.begin();
//    string& data(*it);
//    string str_result = data;
//    LeaveCriticalSection(&m_critSection);
//
//    int found = str_result.find("is recording", 0);
//
//    if (found != -1)
//    {
//      return PVRErr_RECORDING_RUNNING;
//    }
//    else
//    {
//      return PVRErr_NOT_DELETED;
//    }
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SYNC;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//PVR_ERROR PVRClientVDR::RenameTimer(const CTVTimerInfoTag &timerinfo, string &newname)
//{
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  EnterCriticalSection(&m_critSection);
//
//  CTVTimerInfoTag timerinfo1;
//  PVR_ERROR ret = GetTimerInfo(timerinfo.m_Index, timerinfo1);
//
//  if (ret != PVR_ERROR_NO_ERROR)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return ret;
//  }
//
//  timerinfo1.m_strTitle = newname;
//
//  LeaveCriticalSection(&m_critSection);
//  return UpdateTimer(timerinfo1);
//}
//
//PVR_ERROR PVRClientVDR::UpdateTimer(const CTVTimerInfoTag &timerinfo)
//{
//  string     m_Summary;
//  string     m_Summary_2;
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//
//  if (!m_session->IsOpen())
//    return PVR_ERROR_SERVER_ERROR;
//
//  if (timerinfo.m_Index == -1)
//    return PVRErr_NOT_SAVED;
//
//  if (!timerinfo.m_Repeat)
//  {
//    m_Summary.Format("%d:%d:%04d-%02d-%02d:%02d%02d:%02d%02d:%d:%d:%s"
//                     , timerinfo.m_Active
//                     , timerinfo.m_clientNum
//                     , timerinfo.m_StartTime.GetYear()
//                     , timerinfo.m_StartTime.GetMonth()
//                     , timerinfo.m_StartTime.GetDay()
//                     , timerinfo.m_StartTime.GetHour()
//                     , timerinfo.m_StartTime.GetMinute()
//                     , timerinfo.m_StopTime.GetHour()
//                     , timerinfo.m_StopTime.GetMinute()
//                     , timerinfo.m_Priority
//                     , timerinfo.m_Lifetime
//                     , timerinfo.m_strTitle.c_str());
//  }
//  else
//    if (timerinfo.m_FirstDay != NULL)
//    {
//      m_Summary.Format("%d:%d:%c%c%c%c%c%c%c@"
//                       , timerinfo.m_Active
//                       , timerinfo.m_clientNum
//                       , timerinfo.m_Repeat_Mon ? 'M' : '-'
//                       , timerinfo.m_Repeat_Tue ? 'T' : '-'
//                       , timerinfo.m_Repeat_Wed ? 'W' : '-'
//                       , timerinfo.m_Repeat_Thu ? 'T' : '-'
//                       , timerinfo.m_Repeat_Fri ? 'F' : '-'
//                       , timerinfo.m_Repeat_Sat ? 'S' : '-'
//                       , timerinfo.m_Repeat_Sun ? 'S' : '-');
//      m_Summary_2.Format("%04d-%02d-%02d:%02d%02d:%02d%02d:%d:%d:%s"
//                         , timerinfo.m_FirstDay.GetYear()
//                         , timerinfo.m_FirstDay.GetMonth()
//                         , timerinfo.m_FirstDay.GetDay()
//                         , timerinfo.m_StartTime.GetHour()
//                         , timerinfo.m_StartTime.GetMinute()
//                         , timerinfo.m_StopTime.GetHour()
//                         , timerinfo.m_StopTime.GetMinute()
//                         , timerinfo.m_Priority
//                         , timerinfo.m_Lifetime
//                         , timerinfo.m_strTitle.c_str());
//      m_Summary = m_Summary + m_Summary_2;
//    }
//    else
//    {
//      m_Summary.Format("%d:%d:%c%c%c%c%c%c%c:%02d%02d:%02d%02d:%d:%d:%s"
//                       , timerinfo.m_Active
//                       , timerinfo.m_clientNum
//                       , timerinfo.m_Repeat_Mon ? 'M' : '-'
//                       , timerinfo.m_Repeat_Tue ? 'T' : '-'
//                       , timerinfo.m_Repeat_Wed ? 'W' : '-'
//                       , timerinfo.m_Repeat_Thu ? 'T' : '-'
//                       , timerinfo.m_Repeat_Fri ? 'F' : '-'
//                       , timerinfo.m_Repeat_Sat ? 'S' : '-'
//                       , timerinfo.m_Repeat_Sun ? 'S' : '-'
//                       , timerinfo.m_StartTime.GetHour()
//                       , timerinfo.m_StartTime.GetMinute()
//                       , timerinfo.m_StopTime.GetHour()
//                       , timerinfo.m_StopTime.GetMinute()
//                       , timerinfo.m_Priority
//                       , timerinfo.m_Lifetime
//                       , timerinfo.m_strTitle.c_str());
//    }
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "LSTT %d", timerinfo.m_Index);
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVR_ERROR_SERVER_ERROR;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SYNC;
//  }
//
//  sprintf(buffer, "MODT %d %s", timerinfo.m_Index, m_Summary.c_str());
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SAVED;
//  }
//
//  if (code != 250)
//  {
//    LeaveCriticalSection(&m_critSection);
//    return PVRErr_NOT_SYNC;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return PVR_ERROR_NO_ERROR;
//}
//
//
///************************************************************/
///** Live stream handling */
//
//bool PVRClientVDR::OpenLiveStream(unsigned int channel)
//{
//  if (!m_session->IsOpen())
//    return false;
//
//  EnterCriticalSection(&m_critSection);
//
//  if (!m_session->CanStreamLive(channel))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return false;
//  }
//
//  /* Check if a another stream is opened, if yes first close them */
//  if (m_socket != INVALID_SOCKET)
//  {
//    shutdown(m_socket, SD_BOTH);
//
//    if (m_iCurrentChannel < 0)
//      m_session->AbortStreamRecording();
//    else
//      m_session->AbortStreamLive();
//
//    closesocket(m_socket);
//  }
//
//  /* Get Stream socked from VDR Backend */
//  m_socket = m_session->GetStreamLive(channel);
//
//  /* If received socket is invalid, return */
//  if (m_socket == INVALID_SOCKET)
//  {
//    CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get socket for live tv");
//    LeaveCriticalSection(&m_critSection);
//    return false;
//  }
//
//  m_iCurrentChannel = channel;
//
//  LeaveCriticalSection(&m_critSection);
//  return true;
//}
//
//void PVRClientVDR::CloseLiveStream()
//{
//  if (!m_session->IsOpen())
//    return;
//
//  EnterCriticalSection(&m_critSection);
//
//  if (m_socket != INVALID_SOCKET)
//  {
//    m_session->AbortStreamLive();
//    closesocket(m_socket);
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return;
//}
//
//int PVRClientVDR::ReadLiveStream(BYTE* buf, int buf_size)
//{
//  if (!m_session->IsOpen())
//    return 0;
//
//  if (m_socket == INVALID_SOCKET)
//    return 0;
//
//  fd_set         set_r, set_e;
//
//  struct timeval tv;
//
//  int            res;
//
//  tv.tv_sec = 10;
//  tv.tv_usec = 0;
//  FD_ZERO(&set_r);
//  FD_ZERO(&set_e);
//  FD_SET(m_socket, &set_r);
//  FD_SET(m_socket, &set_e);
//  res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
//
//  if (res < 0)
//  {
//    CLog::Log(LOGERROR, "PVRClientVDR::Read - select failed");
//    return 0;
//  }
//
//  if (res == 0)
//  {
//    CLog::Log(LOGERROR, "PVRClientVDR::Read - timeout waiting for data");
//    return 0;
//  }
//
//  res = recv(m_socket, (char*)buf, (size_t)buf_size, 0);
//
//  if (res < 0)
//  {
//    CLog::Log(LOGERROR, "PVRClientVDR::Read - failed");
//    return 0;
//  }
//
//  if (res == 0)
//  {
//    CLog::Log(LOGERROR, "PVRClientVDR::Read - eof");
//    return 0;
//  }
//
//  return res;
//}
//
//int PVRClientVDR::GetCurrentClientChannel()
//{
//  return m_iCurrentChannel;
//}
//
//bool PVRClientVDR::SwitchChannel(unsigned int channel)
//{
//  if (!m_session->IsOpen())
//    return false;
//
//  EnterCriticalSection(&m_critSection);
//
//  if (!m_session->CanStreamLive(channel))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return false;
//  }
//
//  if (m_socket != INVALID_SOCKET)
//  {
//    shutdown(m_socket, SD_BOTH);
//    m_session->AbortStreamLive();
//    closesocket(m_socket);
//  }
//
//  m_socket = m_session->GetStreamLive(channel);
//
//  if (m_socket == INVALID_SOCKET)
//  {
//    CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get socket for live tv");
//    LeaveCriticalSection(&m_critSection);
//    return false;
//  }
//
//  m_iCurrentChannel = channel;
//
//  LeaveCriticalSection(&m_critSection);
//  return true;
//}
//
//
///************************************************************/
///** Record stream handling */
//
//bool PVRClientVDR::OpenRecordedStream(const CTVRecordingInfoTag &recinfo)
//{
//
//  vector<string>  lines;
//  int             code;
//  char            buffer[1024];
//
//  if (!m_session->IsOpen())
//  {
//    return false;
//  }
//
//  EnterCriticalSection(&m_critSection);
//
//  /* Check if a another stream is opened, if yes first close them */
//
//  if (m_socket != INVALID_SOCKET)
//  {
//    shutdown(m_socket, SD_BOTH);
//
//    if (m_iCurrentChannel < 0)
//      m_session->AbortStreamRecording();
//    else
//      m_session->AbortStreamLive();
//
//    closesocket(m_socket);
//  }
//
//  /* Get Stream socked from VDR Backend */
//  m_socket = m_session->GetStreamRecording(recinfo.m_Index, &currentPlayingRecordBytes, &currentPlayingRecordFrames);
//
//  LeaveCriticalSection(&m_critSection);
//
//  if (!currentPlayingRecordBytes)
//  {
//    return false;
//  }
//
//  /* If received socket is invalid, return */
//  if (m_socket == INVALID_SOCKET)
//  {
//    CLog::Log(LOGERROR, "PCRClient-vdr: Couldn't get socket for recording");
//    return false;
//  }
//
//  m_iCurrentChannel               = -1;
//
//  currentPlayingRecordPosition    = 0;
//  return true;
//}
//
//void PVRClientVDR::CloseRecordedStream(void)
//{
//
//  if (!m_session->IsOpen())
//  {
//    return;
//  }
//
//  EnterCriticalSection(&m_critSection);
//
//  if (m_socket != INVALID_SOCKET)
//  {
//    m_session->AbortStreamRecording();
//    closesocket(m_socket);
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  return;
//}
//
//int PVRClientVDR::ReadRecordedStream(BYTE* buf, int buf_size)
//{
//
//  vector<string> lines;
//  int            code;
//  char           buffer[1024];
//  unsigned int   amountReceived;
//
//  if (!m_session->IsOpen())
//  {
//    return 0;
//  }
//
//  if (m_socket == INVALID_SOCKET)
//    return 0;
//
//  if (currentPlayingRecordPosition + buf_size > currentPlayingRecordBytes)
//  {
//    return 0;
//  }
//
//  EnterCriticalSection(&m_critSection);
//
//  sprintf(buffer, "READ %llu %u", currentPlayingRecordPosition, buf_size);
//
//  if (!m_session->SendCommand(buffer, code, lines))
//  {
//    LeaveCriticalSection(&m_critSection);
//    return 0;
//  }
//
//  LeaveCriticalSection(&m_critSection);
//
//  vector<string>::iterator it = lines.begin();
//  string& data(*it);
//
//  amountReceived = atoll(data.c_str());
//
//  fd_set         set_r, set_e;
//
//  struct timeval tv;
//  int            res;
//
//  tv.tv_sec = 3;
//  tv.tv_usec = 0;
//
//  FD_ZERO(&set_r);
//  FD_ZERO(&set_e);
//  FD_SET(m_socket, &set_r);
//  FD_SET(m_socket, &set_e);
//  res = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
//
//  if (res < 0)
//  {
//    CLog::Log(LOGERROR, "PVRClientVDR::ReadRecordedStream - select failed");
//    return 0;
//  }
//
//  if (res == 0)
//  {
//    CLog::Log(LOGERROR, "PVRClientVDR::ReadRecordedStream - timeout waiting for data");
//    return 0;
//  }
//
//  res = recv(m_socket, (char*)buf, (size_t)buf_size, 0);
//
//  if (res < 0)
//  {
//    CLog::Log(LOGERROR, "PVRClientVDR::ReadRecordedStream - failed");
//    return 0;
//  }
//
//  if (res == 0)
//  {
//    CLog::Log(LOGERROR, "PVRClientVDR::ReadRecordedStream - eof");
//    return 0;
//  }
//
//  currentPlayingRecordPosition += res;
//
//  return res;
//}
//
//__int64 PVRClientVDR::SeekRecordedStream(__int64 pos, int whence)
//{
//
//  if (!m_session->IsOpen())
//  {
//    return 0;
//  }
//
//  __int64 nextPos = currentPlayingRecordPosition;
//
//  switch (whence)
//  {
//
//    case SEEK_SET:
//      nextPos = pos;
//      break;
//
//    case SEEK_CUR:
//      nextPos += pos;
//      break;
//
//    case SEEK_END:
//
//      if (currentPlayingRecordBytes)
//        nextPos = currentPlayingRecordBytes - pos;
//      else
//        return -1;
//
//      break;
//
//    case SEEK_POSSIBLE:
//      return 1;
//
//    default:
//      return -1;
//  }
//
//  if (nextPos > currentPlayingRecordBytes)
//  {
//    return 0;
//  }
//
//  currentPlayingRecordPosition = nextPos;
//
//  return currentPlayingRecordPosition;
//}
//
//__int64 PVRClientVDR::LengthRecordedStream(void)
//{
//  return currentPlayingRecordBytes;
//}
