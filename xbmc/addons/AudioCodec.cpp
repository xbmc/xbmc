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

#include "AudioCodec.h"

using namespace ADDON;

CAudioCodec::CAudioCodec(const cp_extension_t *ext) :
  CAddonDll<DllAudioCodec, AudioCodec, AC_PROPS>(ext)
{
  if (ext)
  {
    m_exts = CAddonMgr::Get().GetExtValue(ext->configuration, "@file_exts");
  }
}

CStdString CAudioCodec::GetExtensions()
{
  return m_exts;
}

bool CAudioCodec::Supports(const CStdString& ext)
{
  if (ext.IsEmpty())
    return false;

  return m_exts.Find(ext) > -1;
}

AC_INFO* CAudioCodec::Init(const char* strFile)
{
  if (CAddonDll<DllAudioCodec, AudioCodec, AC_PROPS>::Create())
  {
    try
    {
      return m_pStruct->Init(strFile);
    }
    catch (...)
    {
    }
  }
  return NULL;
}

void CAudioCodec::DeInit(AC_INFO* info)
{
  try
  {
    m_pStruct->DeInit(info);
  }
  catch (...)
  {
  }
}

int64_t CAudioCodec::Seek(AC_INFO* info, int64_t seektime)
{
  try
  {
    return m_pStruct->Seek(info,seektime);
  }
  catch (...)
  {
  }
  return -1;
}

int CAudioCodec::ReadPCM(AC_INFO* info, void* pBuffer, unsigned int size,
                         unsigned int* actualsize)
{
  try
  {
    return m_pStruct->ReadPCM(info,pBuffer,size,actualsize);
  }
  catch (...)
  {
  }
  return READ_ERROR;
}
