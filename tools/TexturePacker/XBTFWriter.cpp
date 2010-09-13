/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "XBTFWriter.h"
#include "EndianSwap.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define TEMP_FILE "temp.xbt"
#define TEMP_SIZE (10*1024*1024)

#define WRITE_STR(str, size, file) fwrite(str, size, 1, file)
#define WRITE_U32(i, file) { uint32_t _n = Endian_SwapLE32(i); fwrite(&_n, 4, 1, file); }
#define WRITE_U64(i, file) { uint64_t _n = i; _n = Endian_SwapLE64(i); fwrite(&_n, 8, 1, file); }

CXBTFWriter::CXBTFWriter(CXBTF& xbtf, const std::string& outputFile) : m_xbtf(xbtf)
{
  m_outputFile = outputFile;
  m_file = m_tempFile = NULL;
}

bool CXBTFWriter::Create()
{
  m_file = fopen(m_outputFile.c_str(), "wb");
  if (m_file == NULL)
  {
    return false;
  }

  m_tempFile = fopen(TEMP_FILE, "wb");
  if (m_tempFile == NULL)
  {
    return false;
  }

  return true;
}

bool CXBTFWriter::Close()
{
  if (m_file == NULL || m_tempFile == NULL)
  {
    return false;
  }

  fclose(m_tempFile);
  m_tempFile = fopen(TEMP_FILE, "rb");
  if (m_tempFile == NULL)
  {
    return false;
  }

  unsigned char* tmp = new unsigned char[10*1024*1024];
  size_t bytesRead;
  while ((bytesRead = fread(tmp, 1, TEMP_SIZE, m_tempFile)) > 0)
  {
    fwrite(tmp, bytesRead, 1, m_file);
  }
  delete[] tmp;

  fclose(m_file);
  fclose(m_tempFile);
  unlink(TEMP_FILE);

  return true;
}

bool CXBTFWriter::AppendContent(unsigned char const* data, size_t length)
{
  if (m_tempFile == NULL)
  {
    return false;
  }

  fwrite(data, length, 1, m_tempFile);

  return true;
}

bool CXBTFWriter::UpdateHeader(const std::vector<unsigned int>& dupes)
{
  if (m_file == NULL)
  {
    return false;
  }

  uint64_t offset = m_xbtf.GetHeaderSize();

  WRITE_STR(XBTF_MAGIC, 4, m_file);
  WRITE_STR(XBTF_VERSION, 1, m_file);

  std::vector<CXBTFFile>& files = m_xbtf.GetFiles();
  WRITE_U32(files.size(), m_file);
  for (size_t i = 0; i < files.size(); i++)
  {
    CXBTFFile& file = files[i];

    // Convert path to lower case
    char* ch = file.GetPath();
    while (*ch)
    {
      *ch = tolower(*ch);
      ch++;
    }

    WRITE_STR(file.GetPath(), 256, m_file);
    WRITE_U32(file.GetLoop(), m_file);

    std::vector<CXBTFFrame>& frames = file.GetFrames();
    WRITE_U32(frames.size(), m_file);
    for (size_t j = 0; j < frames.size(); j++)
    {
      CXBTFFrame& frame = frames[j];
      if (dupes[i] != i)
        frame.SetOffset(files[dupes[i]].GetFrames()[j].GetOffset());
      else
      {
        frame.SetOffset(offset);
        offset += frame.GetPackedSize();
      }

      WRITE_U32(frame.GetWidth(), m_file);
      WRITE_U32(frame.GetHeight(), m_file);
      WRITE_U32(frame.GetFormat(), m_file);
      WRITE_U64(frame.GetPackedSize(), m_file);
      WRITE_U64(frame.GetUnpackedSize(), m_file);
      WRITE_U32(frame.GetDuration(), m_file);
      WRITE_U64(frame.GetOffset(), m_file);
    }
  }

  // Sanity check
  int64_t pos = ftell(m_file);
  if (pos != (int64_t)m_xbtf.GetHeaderSize())
  {
    printf("Expected header size (%" PRId64 ") != actual size (%" PRId64 ")\n", m_xbtf.GetHeaderSize(), pos);
    return false;
  }

  return true;
}
