/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  MMST implementation taken from the xine-mms plugin made by
 *  Major MMS (http://geocities.com/majormms/).
 *  Ported to MPlayer by Abhijeet Phatak <abhijeetphatak@yahoo.com>.
 *  Ported to XBMC by team-xbmc
 *
 *  Information about the MMS protocol can be found at http://get.to/sdp
 *
 *  copyright (C) 2002 Abhijeet Phatak <abhijeetphatak@yahoo.com>
 *  copyright (C) 2002 the xine project
 *  copyright (C) 2000-2001 major mms
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
 * yuvalt: GetPosition should return how far into the stream you have gotten. I how many bytes
 * you have read. GetLenght() total length of the stream you are playing in bytes. (or -1 if unknown).
 * GetContentType should return the mimetype of the stream if known, otherwise empty
 */

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _LINUX
#include <unistd.h>
#endif
#include <errno.h>
#include "FileMMS.h"
#include "Util.h"
#include "Settings.h"

using namespace XFILE;

#define HDR_BUF_SIZE 8192

void CFileMMS::put_32(command_t *cmd, uint32_t value)
{
  cmd->buf[cmd->num_bytes] = value % 256;
  value = value >> 8;
  cmd->buf[cmd->num_bytes + 1] = value % 256;
  value = value >> 8;
  cmd->buf[cmd->num_bytes + 2] = value % 256;
  value = value >> 8;
  cmd->buf[cmd->num_bytes + 3] = value % 256;
  cmd->num_bytes += 4;
}

uint32_t CFileMMS::get_32(unsigned char *cmd, int offset)
{
  uint32_t ret;

  ret = cmd[offset];
  ret |= cmd[offset + 1] << 8;
  ret |= cmd[offset + 2] << 16;
  ret |= cmd[offset + 3] << 24;

  return ret;
}

void CFileMMS::send_command(int s, int command, uint32_t switches, uint32_t extra,
    int length, char *data)
{
  command_t cmd;
  int len8;

  len8 = (length + 7) / 8;

  cmd.num_bytes = 0;

  put_32(&cmd, 0x00000001); /* start sequence */
  put_32(&cmd, 0xB00BFACE); /* #-)) */
  put_32(&cmd, len8 * 8 + 32);
  put_32(&cmd, 0x20534d4d); /* protocol type "MMS " */
  put_32(&cmd, len8 + 4);
  put_32(&cmd, seq_num);
  seq_num++;
  put_32(&cmd, 0x0); /* unknown */
  put_32(&cmd, 0x0);
  put_32(&cmd, len8 + 2);
  put_32(&cmd, 0x00030000 | command); /* dir | command */
  put_32(&cmd, switches);
  put_32(&cmd, extra);

  memcpy(&cmd.buf[48], data, length);
  if (length & 7)
    memset(&cmd.buf[48 + length], 0, 8 - (length & 7));

  if (send(s, (const char *)cmd.buf, len8 * 8 + 48, 0) != (len8 * 8 + 48))
  {
    CLog::Log(LOGERROR, "MMS: write error");
  }
}

void CFileMMS::string_utf16(char *dest, const char *src, int len)
{
  int i;
  size_t len1, len2;
  const char *ip;
  char *op;

  if (url_conv != (iconv_t) (-1))
  {
    memset(dest, 0, 1000);
    len1 = len;
    len2 = 1000;
    ip = src;
    op = dest;

    iconv_const(url_conv, &ip, &len1, &op, &len2);
  }
  else
  {
    if (len > 499)
      len = 499;
    for (i = 0; i < len; i++)
    {
      dest[i * 2] = src[i];
      dest[i * 2 + 1] = 0;
    }
    /* trailing zeroes */
    dest[i * 2] = 0;
    dest[i * 2 + 1] = 0;
  }
}

void CFileMMS::get_answer(int s)
{
  char data[MMS_BUF_SIZE];
  int command = 0x1b;

  while (command == 0x1b)
  {
    int len;

    len = recv(s, data, MMS_BUF_SIZE, 0);
    if (!len)
    {
            CLog::Log(LOGERROR, "MMS: eof reached");
      return;
    }

    command = get_32((unsigned char*) data, 36) & 0xFFFF;

    if (command == 0x1b)
      send_command(s, 0x1b, 0, 0, 0, data);
  }
}

int CFileMMS::get_data(int s, char *buf, size_t count)
{
  ssize_t len;
  size_t total = 0;

  while (total < count)
  {
    len = recv(s, &buf[total], count - total, 0);

    if (len <= 0)
    {
      perror("read error:");
      return 0;
    }

    total += len;

    if (len != 0)
    {
      fflush(stdout);
    }
  }

  return 1;
}

int CFileMMS::get_header(int s, uint8_t *header)
{
  unsigned char pre_header[8];
  int header_len;

  header_len = 0;

  while (1)
  {
    if (!get_data(s, (char*) pre_header, 8))
    {
      CLog::Log(LOGERROR, "MMS: pre-header read failed");
      return 0;
    }
    if (pre_header[4] == 0x02)
    {

      int packet_len;

      packet_len = (pre_header[7] << 8 | pre_header[6]) - 8;

      CLog::Log(LOGINFO, "MMS: asf header packet detected, len=%d", packet_len);

      if (packet_len < 0 || packet_len > HDR_BUF_SIZE - header_len)
      {
        CLog::Log(LOGERROR, "MMS: invalid header size");
        return 0;
      }

      if (!get_data(s, (char*) &header[header_len], packet_len))
      {
        CLog::Log(LOGERROR, "MMS: header data read failed");
        return 0;
      }

      header_len += packet_len;

      if ((header[header_len - 1] == 1) && (header[header_len - 2] == 1))
      {
        memcpy(out_buf, header, header_len);
                CLog::Log(LOGDEBUG, "MMS: header read successful");
        out_buf_len = header_len;
        return (header_len);
      }
    }
    else
    {
      int32_t packet_len;
      int command;
      char data[MMS_BUF_SIZE];

      if (!get_data(s, (char*) &packet_len, 4))
      {
        CLog::Log(LOGERROR, "MMS: packet_len read failed");
        return 0;
      }

      packet_len = get_32((unsigned char*) &packet_len, 0) + 4;

      if (packet_len < 0 || packet_len > MMS_BUF_SIZE)
      {
        CLog::Log(LOGERROR, "MMS: packet_len size invalid");
        return 0;
      }

      if (!get_data(s, data, packet_len))
      {
                CLog::Log(LOGERROR, "MMS: command data read failed");
        return 0;
      }

      command = get_32((unsigned char*) data, 24) & 0xFFFF;

      if (command == 0x1b)
        send_command(s, 0x1b, 0, 0, 0, data);

    }
  }
}

int CFileMMS::interp_header(uint8_t *header, int header_len)
{
  int i;
  int packet_length = -1;

  i = 30;
  while (i < header_len)
  {
    uint64_t guid_1, guid_2, length;

    guid_2 = (uint64_t) header[i] | ((uint64_t) header[i + 1] << 8)
        | ((uint64_t) header[i + 2] << 16) | ((uint64_t) header[i + 3]
        << 24) | ((uint64_t) header[i + 4] << 32)
        | ((uint64_t) header[i + 5] << 40) | ((uint64_t) header[i + 6]
        << 48) | ((uint64_t) header[i + 7] << 56);
    i += 8;

    guid_1 = (uint64_t) header[i] | ((uint64_t) header[i + 1] << 8)
        | ((uint64_t) header[i + 2] << 16) | ((uint64_t) header[i + 3]
        << 24) | ((uint64_t) header[i + 4] << 32)
        | ((uint64_t) header[i + 5] << 40) | ((uint64_t) header[i + 6]
        << 48) | ((uint64_t) header[i + 7] << 56);
    i += 8;

    CLog::Log(LOGDEBUG, "MMS: guid found: %016"PRIu64"x%016"PRIu64"", guid_1, guid_2);

    length = (uint64_t) header[i] | ((uint64_t) header[i + 1] << 8)
        | ((uint64_t) header[i + 2] << 16) | ((uint64_t) header[i + 3]
        << 24) | ((uint64_t) header[i + 4] << 32)
        | ((uint64_t) header[i + 5] << 40) | ((uint64_t) header[i + 6]
        << 48) | ((uint64_t) header[i + 7] << 56);

    i += 8;

    if ((guid_1 == 0x6cce6200aa00d9a6ULL) && (guid_2 == 0x11cf668e75b22630ULL))
    {
            CLog::Log(LOGDEBUG, "MMS guid: header object");
    }
    else if ((guid_1 == 0x6cce6200aa00d9a6ULL) && (guid_2 == 0x11cf668e75b22636ULL))
    {
            CLog::Log(LOGDEBUG, "MMS guid: data object");
    }
    else if ((guid_1 == 0x6553200cc000e48eULL) && (guid_2 == 0x11cfa9478cabdca1ULL))
    {
      packet_length = get_32(header, i + 92 - 24);

            CLog::Log(LOGDEBUG, "MMS guid: file object, packet length = %d (%d)\n",
              packet_length, get_32(header, i+96-24));
    }
    else if ((guid_1 == 0x6553200cc000e68eULL) && (guid_2 == 0x11cfa9b7b7dc0791ULL))
    {

      int stream_id = header[i + 48] | header[i + 49] << 8;

      CLog::Log(LOGDEBUG, "MMS guid: stream object, stream_id=%d\n",  stream_id);

      if (num_stream_ids < MMS_MAX_STREAMS)
      {
        stream_ids[num_stream_ids] = stream_id;
        num_stream_ids++;
      }
      else
      {
                CLog::Log(LOGERROR, "MMS: too many streams");
      }
    }
    else
    {
            CLog::Log(LOGINFO, "MMS guid: unknown object");
    }

    i += (int)(length - 24);
  }

  return packet_length;
}

int CFileMMS::get_media_packet(int s, int padding)
{
  unsigned char pre_header[8];
  char data[MMS_BUF_SIZE];

  if (!get_data(s, (char*) pre_header, 8))
  {
        CLog::Log(LOGERROR, "MMS: pre-header read failed");
    return 0;
  }

  if (pre_header[4] == 0x04)
  {

    int packet_len;

    packet_len = (pre_header[7] << 8 | pre_header[6]) - 8;

    if (packet_len < 0 || packet_len > MMS_BUF_SIZE)
    {
      CLog::Log(LOGERROR, "MMS: invalid packet size");
      return 0;
    }

    if (!get_data(s, data, packet_len))
    {
            CLog::Log(LOGERROR, "MMS: media data read failed");
      return 0;
    }

    memcpy(out_buf, data, padding);
    out_buf_len = padding;
  }
  else
  {
    int32_t packet_len;
    int command;

    if (!get_data(s, (char*) &packet_len, 4))
    {
            CLog::Log(LOGERROR, "MMS: packet_len read failed");
      return 0;
    }

    packet_len = get_32((unsigned char*) &packet_len, 0) + 4;

    if (packet_len < 0 || packet_len > MMS_BUF_SIZE)
    {
            CLog::Log(LOGERROR, "MMS: packet_len size invalid");
      return 0;
    }

    if (!get_data(s, data, packet_len))
    {
            CLog::Log(LOGERROR, "MMS: command data read failed");
      return 0;
    }

    if ((pre_header[7] != 0xb0) || (pre_header[6] != 0x0b) ||
        (pre_header[5] != 0xfa) || (pre_header[4] != 0xce))
    {
            CLog::Log(LOGERROR, "MMS: pre header missing signature");
      return -1;
    }

    command = get_32((unsigned char*) data, 24) & 0xFFFF;

    if (command == 0x1b)
      send_command(s, 0x1b, 0, 0, 0, data);
    else if (command == 0x1e)
    {
      return 0;
    }
    else if (command == 0x21)
    {
      // Looks like it's new in WMS9
      // Unknown command, but ignoring it seems to work.
      return 0;
    }
    else if (command != 0x05)
    {
            CLog::Log(LOGERROR, "MMS: unknown command %x", command);
      return -1;
    }
  }

  return 1;
}

int CFileMMS::streaming_start(char* hostname, int port, char* path)
{
  char str[1024];
  char data[MMS_BUF_SIZE];
  int len, i;
  struct sockaddr_in sa;
  struct hostent *hp;
  uint8_t asf_header[HDR_BUF_SIZE];
  int asf_header_len;


  if (s > 0)
  {
    closesocket(s);
    s = -1;
  }

  if (port == 0)
  {
    port = 1755;
  }

  if ((hp = gethostbyname(hostname)) == NULL)
  {
    CLog::Log(LOGERROR, "MMS: host lookup failure: %s", hostname);
    return -1;
  }

  /* fill socket structure */
  memmove(&sa.sin_addr, hp->h_addr, hp->h_length);
  sa.sin_family = hp->h_addrtype;
  sa.sin_port = htons(port);

  if ((s = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0)
  {
    CLog::Log(LOGERROR, "MMS: error opening socket: %d", errno);
    return -1;
  }

  /* try to connect */
  if (connect(s, (struct sockaddr *) &sa, sizeof sa) < 0)
  {
    CLog::Log(LOGERROR, "MMS: error connecting socket: %d", errno);
    return -1;
  }

    CLog::Log(LOGDEBUG, "MMS: socket connected");

  seq_num = 0;

  /*
   * Send the initial connect info including player version no. Client GUID (random) and the host address being connected to.
   * This command is sent at the very start of protocol initiation. It sends local information to the serve
   * cmd 1 0x01
   * */

  /* prepare for the url encoding conversion */

  snprintf(
      str,
      1023,
      "\034\003NSPlayer/7.0.0.1956; {33715801-BAB3-9D85-24E9-03B90328270A}; Host: %s",
      hostname);
  string_utf16(data, str, strlen(str));

  // send_command(s, commandno ....)
  send_command(s, 1, 0, 0x0004000b, strlen(str) * 2 + 2, data);

  len = recv(s, data, MMS_BUF_SIZE, 0);

  /*
   * This sends details of the local machine IP address to a Funnel system at the server.
   * Also, the TCP or UDP transport selection is sent.
   *
   * here 192.168.0.1 is local ip address TCP/UDP states the tronsport we r using
   * and 1037 is the  local TCP or UDP socket number
   * cmd 2 0x02
   */

  string_utf16(&data[8], "\002\000\\\\192.168.0.1\\TCP\\1037", 24);
  memset(data, 0, 8);
  send_command(s, 2, 0, 0, 24 * 2 + 10, data);

  len = recv(s, data, MMS_BUF_SIZE, 0);

  /*
   * This command sends file path (at server) and file name request to the server.
   * 0x5
   */

  string_utf16(&data[8], path, strlen(path));
  memset(data, 0, 8);
  send_command(s, 5, 0, 0, strlen(path) * 2 + 10, data);

  get_answer(s);

  /*
   * The ASF header chunk request. Includes ?session' variable for pre header value.
   * After this command is sent,
   * the server replies with 0x11 command and then the header chunk with header data follows.
   * 0x15
   */

  memset(data, 0, 40);
  data[32] = 2;

  send_command(s, 0x15, 1, 0, 40, data);
  num_stream_ids = 0;


  asf_header_len = get_header(s, asf_header);

  if (asf_header_len == 0)
  {
      CLog::Log(LOGERROR, "MMS: error reading ASF header");
    closesocket(s);
    return -1;
  }

  plen = interp_header(asf_header, asf_header_len);

  /*
   * This command is the media stream MBR selector. Switches are always 6 bytes in length.
   * After all switch elements, data ends with bytes [00 00] 00 20 ac 40 [02].
   * Where:
   * [00 00] shows 0x61 0x00 (on the first 33 sent) or 0xff 0xff for ASF files, and with no ending data for WMV files.
   * It is not yet understood what all this means.
   * And the last [02] byte is probably the header ?session' value.
   *
   *  0x33
   */

  memset(data, 0, 40);

  for (i = 1; i < num_stream_ids; i++)
  {
    data[(i - 1) * 6 + 2] = (char)0xFF;
    data[(i - 1) * 6 + 3] = (char)0xFF;
    data[(i - 1) * 6 + 4] = stream_ids[i];
    data[(i - 1) * 6 + 5] = 0x00;
  }

  send_command(s, 0x33, num_stream_ids, 0xFFFF | stream_ids[0] << 16,
      (num_stream_ids - 1) * 6 + 2, data);

  get_answer(s);

  /*
   * Start sending file from packet xx.
   * This command is also used for resume downloads or requesting a lost packet.
   * Also used for seeking by sending a play point value which seeks to the media time point.
   * Includes ?session' value in pre header and the maximum media stream time.
   * 0x07
   */

  memset(data, 0, 40);

  for (i = 8; i < 16; i++)
    data[i] = (char)0xFF;

  data[20] = 0x04;

  send_command(s, 0x07, 1, 0xFFFF | stream_ids[0] << 16, 24, data);

  return 0;
}

CFileMMS::CFileMMS()
{
  s = -1;
  out_buf_len = 0;
  url_conv = iconv_open("UTF-16LE", "UTF-8");
}

CFileMMS::~CFileMMS()
{
  if (url_conv != (iconv_t) (-1))
    iconv_close(url_conv);
}

__int64 CFileMMS::GetPosition()
{
  return 0;
}

__int64 CFileMMS::GetLength()
{
  return 0;
}

bool CFileMMS::Open(const CURL& url, bool bBinary)
{
  CStdString filename = url.GetFileName();
  CUtil::UrlDecode(filename);

  int result = streaming_start((char*) url.GetHostName().c_str(),
      url.GetPort(), (char*) filename.c_str());

  return (result >= 0);
}

unsigned int CFileMMS::Read(void* lpBuf, __int64 uiBufSize)
{
  int sent = 0;

  // First time there is a buffer with the header -- send it
  if (out_buf_len > 0)
  {
    memcpy(lpBuf, out_buf, out_buf_len <= uiBufSize ? (size_t)out_buf_len : (size_t)uiBufSize);
    sent = out_buf_len;
    out_buf_len = 0;
  }
  // otherwise, read normal packets from the stream
  else
  {
    // read until we actually get a packet with data
    while (out_buf_len == 0)
    {
      int ret = get_media_packet(s, plen);

      if (ret <= 0)
        return ret;
    }

    memcpy(lpBuf, out_buf, out_buf_len <= uiBufSize ? (size_t)out_buf_len : (size_t)uiBufSize);
    sent = out_buf_len;
    out_buf_len = 0;
  }

  return sent;
}

__int64 CFileMMS::Seek(__int64 iFilePosition, int iWhence)
{
  return -1;
}

void CFileMMS::Close()
{
  closesocket(s);
  s = -1;
}

CStdString CFileMMS::GetContent()
{
  //return "audio/x-ms-wma";
  return "";
}

