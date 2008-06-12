#pragma once
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

#include "IFileSystem.h"
#include "FileSystem/FileHD.h"

class CFatXFileSystem : public IFileSystem
{
public:
  CFatXFileSystem(unsigned char unit);

  virtual bool Open(const CStdString &file);
  virtual bool OpenForWrite(const CStdString &file, bool overWrite);
  unsigned int Read(void *buffer, __int64 size);
  unsigned int Write(const void *buffer, __int64 size);
  virtual __int64 Seek(__int64 iFilePosition);
  virtual void Close();
  virtual __int64 GetLength();
  virtual __int64 GetPosition();
  virtual bool GetDirectory(const CStdString &directory, CFileItemList &items);
  virtual bool Delete(const CStdString &file);
  virtual bool Rename(const CStdString &oldFile, const CStdString &newFile);
  virtual bool MakeDir(const CStdString &path);
  virtual bool RemoveDir(const CStdString &path);
protected:
  CStdString GetLocal(const CStdString &file);
  XFILE::CFileHD m_file;
};
