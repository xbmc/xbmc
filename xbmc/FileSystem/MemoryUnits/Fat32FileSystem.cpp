/*
* XBoxMediaPlayer
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"
#include "Fat32FileSystem.h"
#include "Fat32Device.h"
#include "utils/MemoryUnitManager.h"
#include "FileItem.h"

namespace XFILE
{

CFat32FileSystem::CFat32FileSystem(unsigned char unit) : IFileSystem(unit)
{
  m_opened = CLOSED;
}

bool CFat32FileSystem::Open(const CStdString &file)
{
  CFat32Device *device = (CFat32Device *)g_memoryUnitManager.GetDevice(m_unit);
  if (!device) return false;
  // convert long path to short path
  CStdString shortPath;
  if(!GetShortFilePath(file, shortPath))
    return false;

  BYTE sector[FAT_PAGE_SIZE];
	if (DFS_OK != DFS_OpenFile(device->GetVolume(), (uint8_t*)shortPath.c_str(), DFS_READ, sector, &m_file))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" Error opening file %s", file.c_str());
		return false;
	}
  m_opened = OPEN_FOR_READ;
  return true;
}

bool CFat32FileSystem::OpenForWrite(const CStdString &file, bool overWrite)
{
  CFat32Device *device = (CFat32Device *)g_memoryUnitManager.GetDevice(m_unit);
  if (!device) return false;

  if (overWrite)
    Delete(file);
  BYTE sector[FAT_PAGE_SIZE];
	if (DFS_OK != DFS_OpenFile(device->GetVolume(), (uint8_t*)file.c_str(), DFS_READ | DFS_WRITE, sector, &m_file))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" Error opening file %s", file.c_str());
		return false;
	}
  m_opened = OPEN_FOR_WRITE;
  return true;
}

void CFat32FileSystem::Close()
{
#ifdef FAT32_ALLOW_WRITING
  if (m_opened == OPEN_FOR_WRITE)
  { // flush our caches
    CFat32Device *device = (CFat32Device *)g_memoryUnitManager.GetDevice(m_unit);
    if (!device) return;
    device->FlushWriteCache();
  }
#endif
  m_opened = CLOSED;
}

unsigned int CFat32FileSystem::Read(void *buffer, __int64 size)
{
  if (m_opened == CLOSED) return 0;
  BYTE sector[FAT_PAGE_SIZE];
  unsigned int amountRead = 0;
	if (DFS_OK != DFS_ReadFile(&m_file, sector, (unsigned char *)buffer, &amountRead, (unsigned int)size))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" Error reading file");
		return 0;
	}
  return amountRead;
}

bool CFat32FileSystem::GetShortFilePath(const CStdString &path, CStdString &shortPath)
{  
  shortPath.Empty();
  if (path.IsEmpty())
    return true;  // nothing to do
  // split the path up
  CStdStringArray folders;
  StringUtils::SplitString(path, "/", folders);

  bool isfolder = true;
  for (unsigned int i = 0; i < folders.size(); ++i)
  {
    if(!isfolder)
      return false;

    if (folders[i].IsEmpty())
      continue; // ignore empty portions (eg at the start of the url)
    CFileItemList items;
    if (GetDirectoryWithShortPaths(shortPath, items))
    {
      bool found(false);
      for (int j = 0; j < items.Size(); ++j)
      {
        if (items[j]->GetLabel() == folders[i])
        { // found :)
          shortPath += "/" + items[j]->m_strPath;
          found = true;
          isfolder = items[j]->m_bIsFolder;
          break;
        }
      }
      if (!found)
        return false;
    }
    else
    { // unable to get directory - return false
      return false;
    }
  }
  return true;
}

unsigned int CFat32FileSystem::Write(const void *buffer, __int64 size)
{
  if (m_opened != OPEN_FOR_WRITE) return 0;
  BYTE sector[FAT_PAGE_SIZE];
  unsigned int amountWritten = 0;
	if (DFS_OK != DFS_WriteFile(&m_file, sector, (unsigned char *)buffer, &amountWritten, (unsigned int)size))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" Error writing file");
		return 0;
	}
  return amountWritten;
}

__int64 CFat32FileSystem::Seek(__int64 position)
{
  if (m_opened == CLOSED) return -1;
  BYTE sector[FAT_PAGE_SIZE];
  DFS_Seek(&m_file, (unsigned int)position, sector);
  return m_file.pointer;
}

__int64 CFat32FileSystem::GetLength()
{
  return m_file.filelen;
}

__int64 CFat32FileSystem::GetPosition()
{
  return m_file.pointer;
}

bool CFat32FileSystem::Delete(const CStdString &file)
{
  return false;

/*  CFat32Device *device = (CFat32Device *)g_memoryUnitManager.GetDevice(m_unit);
  if (!device) return false;

  BYTE sector[FAT_PAGE_SIZE];
	if (DFS_OK != DFS_UnlinkFile(device->GetVolume(), (uint8_t*)file.c_str(), sector))
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" Error deleting file %s", file.c_str());
		return false;
	}
  return true;*/
}

bool CFat32FileSystem::Rename(const CStdString &oldFile, const CStdString &newFile)
{
  return false;
}

bool CFat32FileSystem::MakeDir(const CStdString &path)
{
  return false;
}

bool CFat32FileSystem::RemoveDir(const CStdString &path)
{
  return Delete(path);
}

bool CFat32FileSystem::GetDirectory(const CStdString &directory, CFileItemList &items)
{
  // first grab the shortened version of this directory
  CStdString shortDirectory;
  if (GetShortFilePath(directory, shortDirectory) && GetDirectoryWithShortPaths(shortDirectory, items))
  { // success - update our paths
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItem *item = items[i];
      if (directory.IsEmpty())
        item->m_strPath.Format("mem%d://%s", m_unit, item->GetLabel().c_str());
      else
        item->m_strPath.Format("mem%d://%s/%s", m_unit, directory.c_str(), item->GetLabel().c_str());
    }
    return true;
  }
  return false;
}

bool CFat32FileSystem::GetDirectoryWithShortPaths(const CStdString &directory, CFileItemList &items)
{
  CFat32Device *device = (CFat32Device *)g_memoryUnitManager.GetDevice(m_unit);
  if (!device) return false;

  DIRINFO di;
  BYTE buffer[FAT_PAGE_SIZE];
  di.scratch = (uint8_t*)buffer;
	if (DFS_OpenDir(device->GetVolume(), (uint8_t*)directory.c_str(), &di)) {
    CLog::Log(LOGDEBUG, __FUNCTION__" Error opening directory %s", directory.c_str());
		return false;
	}
  // vfat naming
  CStdStringW vfatName;
  unsigned short vfatSequence = 0;
  unsigned char vfatChecksum = 0;
  DIRENT de;
	while (!DFS_GetNext(device->GetVolume(), &di, &de))
  {
		if (de.name[0])
    {

      if ((de.attr & ATTR_LONG_NAME) == ATTR_LONG_NAME)
      { // long filename
        VFAT_DIR_ENTRY *vfat = (VFAT_DIR_ENTRY*)&de;
        // check it's a vfat entry
        if (vfat->attr_0f != 0x0f || vfat->cluster_0000 != 0x0000 || vfat->type_00 != 0x00)
        { // invalid entry
          continue;
        }
        // not sure why but very long filename's have 0x40 set on both their 5 and 6th part. 
        // let's only check if sequence mismatches
        if ((vfat->sequence & 0x40) == 0x40)
        { // last entry
          if((vfat->sequence & 0x1f) == vfatSequence && vfat->checksum == vfatChecksum)
            CLog::Log(LOGWARNING, __FUNCTION__" Last entry signaled, but sequence and checksum still match, ignoring");
          else
          {
            vfatName.Empty();
            vfatSequence = (vfat->sequence & 0x1f);
            vfatChecksum = vfat->checksum;
          }
        }
        if (vfat->checksum == vfatChecksum && (vfat->sequence & 0x1f) == vfatSequence && vfatSequence)
        {
          WCHAR unicode[14];
          memcpy(unicode, vfat->unicode1, 10);
          memcpy(unicode + 5, vfat->unicode2, 12);
          memcpy(unicode + 11, vfat->unicode3, 4);
          unicode[13] = 0;
          vfatName = unicode + vfatName;
          vfatSequence--;
        }
        continue;
      }
      // ignore the volume descripter
      if ((de.attr & ATTR_VOLUME_ID) == ATTR_VOLUME_ID)
        continue;
      CStdString shortPath;
      if ((de.attr & ATTR_DIRECTORY) == 0)
      { // filename
        shortPath.Format("%-8.8s", de.name);
        shortPath.TrimRight(' ');
        CStdString extension;
        extension.Format(".%-3.3s", de.name + 8);
        shortPath += extension;
      }
      else
      { // folders with a period in them must be catered for
        shortPath.Format("%-11.11s", de.name);
      }
      shortPath.TrimRight(' ');
      // we don't want require the parent and current directory items
      if (shortPath.Equals(".") || shortPath.Equals(".."))
        continue;
      CStdString longPath = shortPath;
      // do we have a vfatName here?
      if (!vfatSequence && vfatName.size())
      { // yes, check the checksum
        g_charsetConverter.wToUTF8(vfatName, longPath);
        vfatSequence = 0;
        vfatName.Empty();
      }
      CFileItem *item = new CFileItem(longPath);
      item->m_strPath = shortPath;
      item->m_bIsFolder = (de.attr & ATTR_DIRECTORY) == ATTR_DIRECTORY;
      // file size
      if ((de.attr & ATTR_DIRECTORY) == 0)
        item->m_dwSize = *((unsigned long *)(&de.filesize_0));
      // Currently using the last write time
      int day = (de.wrtdate_l & 0x1f);
      int month = ((de.wrtdate_l & 0xe0) >> 5) | ((de.wrtdate_h & 1) << 3);
      int year = 1980 + ((de.wrtdate_h & 0xfe) >> 1);
      int second = (de.wrttime_l & 0x1f) << 1;
      int minute = ((de.wrttime_l & 0xe0) >> 5) | ((de.wrttime_h & 0x07) << 3);
      int hour = (de.wrtdate_h & 0xf8) >> 3;
      item->m_dateTime.SetDateTime(year, month, day, hour, minute, second);

      items.Add(item);
    }
	}
  return true;
}
}
