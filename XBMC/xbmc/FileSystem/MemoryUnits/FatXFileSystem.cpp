/*
* XBMC Media Center
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
#include "FatXFileSystem.h"
#include "FatXDevice.h"
#include "utils/MemoryUnitManager.h"
#include "FileSystem/HDDirectory.h"
#include "URL.h"
#include "FileItem.h"

using namespace XFILE;
using namespace DIRECTORY;

CFatXFileSystem::CFatXFileSystem(unsigned char unit) : IFileSystem(unit)
{
}

bool CFatXFileSystem::Open(const CStdString &file)
{
  CURL url(GetLocal(file));
  return m_file.Open(url);
}

bool CFatXFileSystem::OpenForWrite(const CStdString &file, bool overWrite)
{
  CURL url(GetLocal(file));
  return m_file.OpenForWrite(url, overWrite);
}

void CFatXFileSystem::Close()
{
  m_file.Close();
}

unsigned int CFatXFileSystem::Read(void *buffer, __int64 length)
{
  return m_file.Read(buffer, length);
}

unsigned int CFatXFileSystem::Write(const void *buffer, __int64 length)
{
  return m_file.Write(buffer, length);
}

__int64 CFatXFileSystem::Seek(__int64 position)
{
  return m_file.Seek(position, SEEK_SET);
}

__int64 CFatXFileSystem::GetLength()
{
  return m_file.GetLength();
}

__int64 CFatXFileSystem::GetPosition()
{
  return m_file.GetPosition();
}

bool CFatXFileSystem::Delete(const CStdString &file)
{
  CURL url(GetLocal(file));
  CFileHD hdFile;
  return hdFile.Delete(url);
}

bool CFatXFileSystem::Rename(const CStdString &oldFile, const CStdString &newFile)
{
  CURL urlOld(GetLocal(oldFile));
  CURL urlNew(GetLocal(newFile));
  CFileHD hdFile;
  return hdFile.Rename(urlOld, urlNew);
}

bool CFatXFileSystem::MakeDir(const CStdString &path)
{
  CHDDirectory hdDir;
  return hdDir.Create(GetLocal(path).c_str());
}

bool CFatXFileSystem::RemoveDir(const CStdString &path)
{
  CHDDirectory hdDir;
  return hdDir.Remove(GetLocal(path).c_str());
}

bool CFatXFileSystem::GetDirectory(const CStdString &directory, CFileItemList &items)
{
  CStdString dirMask(GetLocal(directory));
  CHDDirectory hd;
  if (hd.GetDirectory(dirMask, items))
  { // replace our items with our nicer URL
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      item->m_strPath.Format("mem%d://%s", m_unit, item->m_strPath.Mid(3).c_str());
      item->m_strPath.Replace("\\","/");
    }
    return true;
  }
  return false;
}

CStdString CFatXFileSystem::GetLocal(const CStdString &file)
{
  CStdString path;
  CFatXDevice *device = (CFatXDevice *)g_memoryUnitManager.GetDevice(m_unit);
  if (device)
  {
    path.Format("%c:\\%s", device->GetDrive(), file);
    path.Replace("/", "\\");
  }
  return path;
}
