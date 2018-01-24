/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define __STDC_FORMAT_MACROS
#include <cinttypes>
#if defined(TARGET_FREEBSD) || defined(TARGET_DARWIN)
#include <cstdlib>
#elif !defined(TARGET_DARWIN)
#include <malloc.h>
#endif
#include <memory.h>
#include <cstring>

#include "XBTFWriter.h"
#include "guilib/XBTFReader.h"
#include "utils/EndianSwap.h"


#define WRITE_STR(str, size, file) fwrite(str, size, 1, file)
#define WRITE_U32(i, file) { uint32_t _n = Endian_SwapLE32(i); fwrite(&_n, 4, 1, file); }
#define WRITE_U64(i, file) { uint64_t _n = i; _n = Endian_SwapLE64(i); fwrite(&_n, 8, 1, file); }

CXBTFWriter::CXBTFWriter(const std::string& outputFile)
  : m_outputFile(outputFile),
    m_file(nullptr),
    m_data(nullptr),
    m_size(0)
{ }

CXBTFWriter::~CXBTFWriter()
{
  Close();
}

bool CXBTFWriter::Create()
{
  m_file = fopen(m_outputFile.c_str(), "wb");
  if (m_file == nullptr)
    return false;

  return true;
}

bool CXBTFWriter::Close()
{
  if (m_file == nullptr || m_data == nullptr)
    return false;

  fwrite(m_data, 1, m_size, m_file);

  Cleanup();

  return true;
}

void CXBTFWriter::Cleanup()
{
  free(m_data);
  m_data = nullptr;
  m_size = 0;
  if (m_file)
  {
    fclose(m_file);
    m_file = nullptr;
  }
}

bool CXBTFWriter::AppendContent(unsigned char const* data, size_t length)
{
  unsigned char *new_data = (unsigned char *)realloc(m_data, m_size + length);

  if (new_data == nullptr)
  { // OOM - cleanup and fail
    Cleanup();
    return false;
  }

  m_data = new_data;

  memcpy(m_data + m_size, data, length);
  m_size += length;

  return true;
}

bool CXBTFWriter::UpdateHeader(const std::vector<unsigned int>& dupes)
{
  if (m_file == nullptr)
    return false;

  uint64_t headerSize = GetHeaderSize();
  uint64_t offset = headerSize;

  WRITE_STR(XBTF_MAGIC.c_str(), 4, m_file);
  WRITE_STR(XBTF_VERSION.c_str(), 1, m_file);

  auto files = GetFiles();
  WRITE_U32(files.size(), m_file);
  for (size_t i = 0; i < files.size(); i++)
  {
    CXBTFFile& file = files[i];

    // Convert path to lower case and store it into a fixed size array because
    // we need to store the path as a fixed length 256 byte character array.
    std::string path = file.GetPath();
    char pathMem[CXBTFFile::MaximumPathLength];
    memset(pathMem, 0, sizeof(pathMem));

    for (std::string::iterator ch = path.begin(); ch != path.end(); ++ch)
      pathMem[std::distance(path.begin(), ch)] = tolower(*ch);

    WRITE_STR(pathMem, CXBTFFile::MaximumPathLength, m_file);
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
      WRITE_U32(frame.GetFormat(true), m_file);
      WRITE_U64(frame.GetPackedSize(), m_file);
      WRITE_U64(frame.GetUnpackedSize(), m_file);
      WRITE_U32(frame.GetDuration(), m_file);
      WRITE_U64(frame.GetOffset(), m_file);
    }
  }

  // Sanity check
  int64_t pos = ftell(m_file);
  if (pos != static_cast<int64_t>(headerSize))
  {
    printf("Expected header size (%" PRIu64 ") != actual size (%" PRId64 ")\n", headerSize, pos);
    return false;
  }

  return true;
}
