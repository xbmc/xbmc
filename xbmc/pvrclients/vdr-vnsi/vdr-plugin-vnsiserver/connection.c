/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/resource.h>

#include <vdr/plugin.h>
#include <vdr/device.h>
#include <vdr/channels.h>

#include "connection.h"
#include "receiver.h"
#include "server.h"
#include "tools.h"
#include "vdrcommand.h"
#include "recplayer.h"

cConnection::cConnection(cServer *server, int fd, unsigned int id, const char *ClientAdr)
{
  m_Id            = id;
  m_server        = server;
  m_Streamer      = NULL;
  m_isStreaming   = false;
  m_Channel       = NULL;
  m_NetLogFile    = NULL;
  m_ClientAddress = ClientAdr;
  m_socket.set_handle(fd);

  Start();
}

cConnection::~cConnection()
{
  Cancel(-1);

  StopChannelStreaming();

  if (m_NetLogFile)
  {
    fclose(m_NetLogFile);
    m_NetLogFile = NULL;
  }
  Cancel();
}

void cConnection::Action(void)
{
  uint32_t kaTimeStamp;
  uint32_t logStringLen;
  uint32_t channelID;
  uint32_t requestID;
  uint32_t opcode;
  uint32_t dataLength;
  uint8_t* data;

  while (Running())
  {
    if (!m_socket.read((uint8_t*)&channelID, sizeof(uint32_t))) break;
    channelID = ntohl(channelID);

    if (channelID == 1)
    {
      if (!m_socket.read((uint8_t*)&requestID, sizeof(uint32_t), 1000)) break;
      requestID = ntohl(requestID);

      if (!m_socket.read((uint8_t*)&opcode, sizeof(uint32_t), 1000)) break;
      opcode = ntohl(opcode);

      if (!m_socket.read((uint8_t*)&dataLength, sizeof(uint32_t), 1000)) break;
      dataLength = ntohl(dataLength);
      if (dataLength > 200000) // a random sanity limit
      {
        esyslog("VNSI-Error: dataLength > 200000!");
        break;
      }

      if (dataLength)
      {
        data = (uint8_t*)malloc(dataLength);
        if (!data)
        {
          esyslog("VNSI-Error: Extra data buffer malloc error");
          break;
        }

        if (!m_socket.read(data, dataLength, 1000))
        {
          esyslog("VNSI-Error: Could not read data");
          free(data);
          break;
        }
      }
      else
      {
        data = NULL;
      }

      //LOGCONSOLE("Received chan=%lu, ser=%lu, op=%lu, edl=%lu", channelID, requestID, opcode, dataLength);

      if (!m_loggedIn && (opcode != 1))
      {
        esyslog("VNSI-Error: Not logged in and opcode != 1");
        if (data) free(data);
        break;
      }

      /* Handle channel open and close inside this thread */
      if (opcode == VDR_CHANNELSTREAM_OPEN)
      {
        cResponsePacket *resp = new cResponsePacket();
        if (!resp->init(requestID))
        {
          esyslog("VNSI-Error: response packet init fail");
          delete resp;
          continue;
        }

        uint32_t number = ntohl(*(uint32_t*)&data[0]);
        free(data);

        if (m_isStreaming)
          StopChannelStreaming();

        const cChannel *channel = Channels.GetByNumber(number);
        if (channel != NULL)
        {
          if (StartChannelStreaming(channel, resp))
          {
            isyslog("VNSI: Started streaming of channel %i - %s", number, channel->Name());
            continue;
          }
          else
          {
            LOGCONSOLE("Can't stream channel %i - %s", number, channel->Name());
            resp->add_U32(VDR_RET_DATALOCKED);
          }
        }
        else
        {
          esyslog("VNSI-Error: Can't find channel %i", number);
          resp->add_U32(VDR_RET_DATAINVALID);
        }

        resp->finalise();
        m_socket.write(resp->getPtr(), resp->getLen());
      }
      else if (opcode == VDR_CHANNELSTREAM_CLOSE)
      {
        if (m_isStreaming)
          StopChannelStreaming();
      }
      else
      {
        cRequestPacket* req = new cRequestPacket(requestID, opcode, data, dataLength, this);
        m_server->GetCmdControl()->recvRequest(req);
      }
    }
    else if (channelID == 3)
    {
      if (!m_socket.read((uint8_t*)&kaTimeStamp, sizeof(uint32_t), 1000)) break;
      kaTimeStamp = ntohl(kaTimeStamp);

      //LOGCONSOLE("Received chan=%lu kats=%lu", channelID, kaTimeStamp);

      uint8_t buffer[8];
      *(uint32_t*)&buffer[0] = htonl(3); // KA CHANNEL
      *(uint32_t*)&buffer[4] = htonl(kaTimeStamp);
      if (!m_socket.write(buffer, 8))
      {
        esyslog("VNSI-Error: Could not send back KA reply");
        break;
      }
    }
    else if (channelID == 4)
    {
      if (!m_socket.read((uint8_t*)&logStringLen, sizeof(uint32_t), 1000)) break;
      logStringLen = ntohl(logStringLen);

      LOGCONSOLE("Received chan=%lu loglen=%lu", channelID, logStringLen);

      uint8_t buffer[logStringLen + 1];
      if (!m_socket.read((uint8_t*)&buffer, logStringLen, 1000)) break;
      buffer[logStringLen] = '\0';

      LOGCONSOLE("Client said: '%s'", buffer);
      if (m_NetLogFile)
      {
        if (fputs((const char*)buffer, m_NetLogFile) == EOF)
        {
          fclose(m_NetLogFile);
          m_NetLogFile = NULL;
        }
        fflush(NULL);
      }
    }
    else
    {
      esyslog("VNSI-Error: Incoming channel number unknown");
      break;
    }
  }

  /* If thread is ended due to closed connection delete a
     possible running stream here */
  StopChannelStreaming();
}

void cConnection::EnableNetLog(bool yesNo, const char* ClientName)
{
  if (yesNo)
  {
    cString Base(cPlugin::ConfigDirectory());
    Base = cString::sprintf("%s/vnsi-server/%s-%s.log", *Base, *m_ClientAddress, ClientName);

    m_NetLogFile = fopen(*Base, "a");
    if (m_NetLogFile)
      isyslog("VNSI: Client network logging started");
  }
  else
  {
    if (m_NetLogFile)
    {
      fclose(m_NetLogFile);
      m_NetLogFile = NULL;
      isyslog("VNSI: Client network logging stopped");
    }
  }
}

bool cConnection::StartChannelStreaming(const cChannel *channel, cResponsePacket *resp)
{
  m_Channel     = channel;
  m_Streamer    = new cLiveStreamer;
  m_isStreaming = m_Streamer->StreamChannel(m_Channel, 50, &m_socket, resp);
  return m_isStreaming;
}

void cConnection::StopChannelStreaming()
{
  m_isStreaming = false;
  if (m_Streamer)
  {
    delete m_Streamer;
    m_Streamer = NULL;
    m_Channel  = NULL;
  }
}
