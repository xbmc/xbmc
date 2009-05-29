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

#include "stdafx.h"
#include "PVRFile.h"
#include "Util.h"

using namespace XFILE;
using namespace std;

CPVRFile::CPVRFile()
{
}

CPVRFile::~CPVRFile()
{
}

void CPVRFile::Close()
{
}

bool CPVRFile::Open(const CURL& url2)
{
  return false;
}

unsigned int CPVRFile::Read(void* buffer, __int64 size)
{
  return 0;
}

__int64 CPVRFile::Seek(__int64 pos, int whence)
{
  return -1;
}

bool CPVRFile::NextChannel()
{
  return false;
}

bool CPVRFile::PrevChannel()
{
  return false;
}

bool CPVRFile::SelectChannel(unsigned int channel)
{
  return false;
}

CStdString CPVRFile::TranslatePVRFilename(const CStdString& pathFile)
{
  CStdString ret = pathFile;
  return ret;
}

bool CPVRFile::CanRecord()
{
  return false;
}

bool CPVRFile::IsRecording()
{
  return false;
}

bool CPVRFile::Record(bool bOnOff)
{
  return false;
}
