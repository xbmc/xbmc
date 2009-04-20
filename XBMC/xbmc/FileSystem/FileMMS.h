#ifndef FILEMMS_H_
#define FILEMMS_H_

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "IFile.h"
#ifndef _LINUX
#include "lib/libiconv/iconv.h"
#else
#include <iconv.h>
#endif
#include <inttypes.h>

#define MMS_BUF_SIZE 102400
#define MMS_MAX_STREAMS 20

namespace XFILE
{
class CFileMMS: public IFile
{
public:
  CFileMMS();
  virtual ~CFileMMS();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url) { return true; };
  virtual int Stat(const CURL& url, struct __stat64* buffer)  { errno = ENOENT; return -1; };
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual CStdString GetContent();

protected:

  typedef struct
  {
    uint8_t buf[MMS_BUF_SIZE];
    int num_bytes;
  } command_t;

  void put_32(command_t *cmd, uint32_t value);
  uint32_t get_32(unsigned char *cmd, int offset);
  void send_command(int s, int command, uint32_t switches, uint32_t extra,
                  int length, char *data);
  void string_utf16(char *dest, const char *src, int len);
  void get_answer(int s);
  int get_data(int s, char *buf, size_t count);
  int get_header(int s, uint8_t *header);
  int interp_header(uint8_t *header, int header_len);
  int get_media_packet(int s, int padding);
  int streaming_start(char* hostname, int port, char* path);

  CStdString m_contenttype;
  int seq_num;
  int num_stream_ids;
  int stream_ids[MMS_MAX_STREAMS];
  uint8_t* out_buf[MMS_BUF_SIZE];
  int out_buf_len;
  int s;
  int plen;
  iconv_t url_conv;
};
}
#endif
