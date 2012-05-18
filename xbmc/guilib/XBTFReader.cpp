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

#include <sys/stat.h>
#include "XBTFReader.h"
#include "utils/EndianSwap.h"
#include "utils/CharsetConverter.h"
#ifdef _WIN32
#include "FileSystem/SpecialProtocol.h"
#endif

#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#include <string.h>
#include "PlatformDefs.h"

#define READ_STR(str, size, file)           \
  if (!fread(str, size, 1, file))           \
    return false;                           \

#define READ_U32(i, file)                   \
  if (!fread(&i, 4, 1, file))               \
    return false;                           \
  i = Endian_SwapLE32(i);

#define READ_U64(i, file)                   \
  if (!fread(&i, 8, 1, file))               \
    return false;                           \
  i = Endian_SwapLE64(i);

CXBTFReader::CXBTFReader()
{
  m_file = NULL;
}

bool CXBTFReader::IsOpen() const
{
  return m_file != NULL;
}

bool CXBTFReader::Open(const CStdString& fileName)
{
  unsigned int buffersize = 0;
  m_fileName = fileName;

#ifdef _WIN32
  CStdStringW strPathW;
  g_charsetConverter.utf8ToW(CSpecialProtocol::TranslatePath(m_fileName), strPathW, false);
  m_file = _wfopen(strPathW.c_str(), L"rb");
#else
  m_file = fopen(m_fileName.c_str(), "rb");
#endif
  if (m_file == NULL)
  {
    return false;
  }

  if(g_advancedSettings.m_bufferXBTinMemory)
  {
    if(g_advancedSettings.m_bufferXBTinMemorySize)
      buffersize = g_advancedSettings.m_bufferXBTinMemorySize;
    else
      buffersize = 10*1024*1024;
    CLog::Log(LOGNOTICE, "Buffer XBT Textures in memory : maximum %d bytes", buffersize);
  }

  char magic[4];
  READ_STR(magic, 4, m_file);

  if (strncmp(magic, XBTF_MAGIC, sizeof(magic)) != 0)
  {
    return false;
  }

  char version[1];
  READ_STR(version, 1, m_file);

  if (strncmp(version, XBTF_VERSION, sizeof(version)) != 0)
  {
    return false;
  }

  unsigned int nofFiles;

  READ_U32(nofFiles, m_file);
  for (unsigned int i = 0; i < nofFiles; i++)
  {
    CXBTFFile *file = new CXBTFFile();
    unsigned int u32;
    uint64_t u64;

    READ_STR(file->GetPath(), 256, m_file);
    READ_U32(u32, m_file);
    file->SetLoop(u32);

    unsigned int nofFrames;
    READ_U32(nofFrames, m_file);

    for (unsigned int j = 0; j < nofFrames; j++)
    {
      CXBTFFrame *frame = new CXBTFFrame();

      READ_U32(u32, m_file);
      frame->SetWidth(u32);
      READ_U32(u32, m_file);
      frame->SetHeight(u32);
      READ_U32(u32, m_file);
      frame->SetFormat(u32);
      READ_U64(u64, m_file);
      frame->SetPackedSize(u64);
      READ_U64(u64, m_file);
      frame->SetUnpackedSize(u64);
      READ_U32(u32, m_file);
      frame->SetDuration(u32);
      READ_U64(u64, m_file);
      frame->SetOffset(u64);

      /* save read position */
      if(g_advancedSettings.m_bufferXBTinMemory && frame->GetPackedSize() <= buffersize)
      {
        size_t pos = ftell(m_file);
        fseek(m_file, frame->GetOffset(), SEEK_SET);

        unsigned char *data = new unsigned char [frame->GetPackedSize()];
        READ_STR(data, frame->GetPackedSize(), m_file);

        frame->SetData(data);
        frame->SetCached(true);

        /* restore read position */
        fseek(m_file, pos, SEEK_SET);

        buffersize -= frame->GetPackedSize();
      }

      file->GetFrames().push_back(frame);
    }

    m_xbtf.GetFiles().push_back(file);

    m_filesMap[file->GetPath()] = file;
  }

  // Sanity check
  int64_t pos = ftell(m_file);
  if (pos != (int64_t)m_xbtf.GetHeaderSize())
  {
    printf("Expected header size (%"PRId64") != actual size (%"PRId64")\n", m_xbtf.GetHeaderSize(), pos);
    return false;
  }

  return true;
}

void CXBTFReader::Close()
{
  if (m_file)
  {
    fclose(m_file);
    m_file = NULL;
  }

  m_xbtf.Clear();
  m_filesMap.clear();
}

time_t CXBTFReader::GetLastModificationTimestamp()
{
  struct stat fileStat;
  if (fstat(fileno(m_file), &fileStat) == -1)
  {
    return 0;
  }

  return fileStat.st_mtime;
}

bool CXBTFReader::Exists(const CStdString& name)
{
  return Find(name) != NULL;
}

CXBTFFile* CXBTFReader::Find(const CStdString& name)
{
  std::map<CStdString, CXBTFFile *>::iterator iter = m_filesMap.find(name);
  if (iter == m_filesMap.end())
  {
    return NULL;
  }

  return iter->second;
}

bool CXBTFReader::Load(const CXBTFFrame *frame, unsigned char* buffer)
{
  if (!m_file)
  {
    return false;
  }
#if defined(__APPLE__) || defined(__FreeBSD__)
  if (fseeko(m_file, (off_t)frame->GetOffset(), SEEK_SET) == -1)
#else
  if (fseeko64(m_file, (off_t)frame->GetOffset(), SEEK_SET) == -1)
#endif
  {
    return false;
  }

  if (fread(buffer, 1, (size_t)frame->GetPackedSize(), m_file) != frame->GetPackedSize())
  {
    return false;
  }

  return true;
}

std::vector<CXBTFFile *>& CXBTFReader::GetFiles()
{
  return m_xbtf.GetFiles();
}
