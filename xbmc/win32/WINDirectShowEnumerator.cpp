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

#include "system.h"
#include "WINDirectShowEnumerator.h"
#include "SingleLock.h"
#include "log.h"
#include "CharsetConverter.h"
#include "streams.h"
#include "DShowUtil/regkey.h"
#include "DShowUtil/DShowUtil.h"

CDirectShowEnumerator::CDirectShowEnumerator(void)
{
}

CDirectShowEnumerator::~CDirectShowEnumerator(void)
{
  vDSFilterInfo.clear();
}


std::vector<DSFilterInfo> CDirectShowEnumerator::GetAudioRenderers()
{
  CSingleLock lock (m_critSection);
  vDSFilterInfo.clear();
  
  Com::SmartPtr<IPropertyBag> propBag;

  BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker)
  {
    if (SUCCEEDED(pMoniker->BindToStorage(NULL, NULL, IID_IPropertyBag, (void**) &propBag)))
    {
      _variant_t var;

      CStdStringW filterName;
      CStdStringW filterGuid;

      if (SUCCEEDED(propBag->Read(L"FriendlyName", &var, 0)))
        filterName = CStdStringW(var.bstrVal);

      var.Clear();

      if (SUCCEEDED(propBag->Read(L"CLSID", &var, 0)))
        filterGuid = CStdStringW(var.bstrVal);

      AddFilter(filterGuid, filterName);

      propBag = NULL;
    }
  }
  EndEnumSysDev;


  return vDSFilterInfo;
}

bool CDirectShowEnumerator::AddFilter( CStdStringW lpGuid, CStdStringW lpName )
{
  DSFilterInfo filterInfo;
  filterInfo.lpstrGuid = lpGuid;

  g_charsetConverter.wToUTF8(lpName, filterInfo.lpstrName);

  vDSFilterInfo.push_back(filterInfo);
  
  CLog::Log(LOGDEBUG, "Found DirectShow device \"%s\" (guid: %s)", filterInfo.lpstrName.c_str(), filterInfo.lpstrGuid.c_str());
  return true;
}


void CDirectShowEnumerator::ForceStableCodecs()
{
  //This is the list of codecs that are considered stable in ffdshow
  CStdString strRegKey;
  strRegKey.Format("Software\\Gnu\\ffdshow");
  RegKey ffReg(HKEY_CURRENT_USER ,strRegKey.c_str() ,false);
  ffReg.setValue("cscd",DWORD(1)); ffReg.setValue("div3",DWORD(1)); ffReg.setValue("duck",DWORD(1));
  ffReg.setValue("dx50",DWORD(1)); ffReg.setValue("ffv1",DWORD(1)); ffReg.setValue("flv1",DWORD(1));
  ffReg.setValue("fvfw",DWORD(1)); ffReg.setValue("h261",DWORD(1)); ffReg.setValue("h263",DWORD(1));
  ffReg.setValue("h264",DWORD(1)); ffReg.setValue("hfyu",DWORD(1)); ffReg.setValue("iv32",DWORD(1));
  ffReg.setValue("mjpg",DWORD(1)); ffReg.setValue("mp41",DWORD(1)); ffReg.setValue("mp42",DWORD(1));
  ffReg.setValue("mp43",DWORD(1)); ffReg.setValue("mp4v",DWORD(1)); ffReg.setValue("mszh",DWORD(1));
  ffReg.setValue("png1",DWORD(1)); ffReg.setValue("qtrle",DWORD(1)); ffReg.setValue("qtrpza",DWORD(1));
  ffReg.setValue("raw_rawv",DWORD(1)); ffReg.setValue("rt21",DWORD(1)); ffReg.setValue("svq1",DWORD(1));
  ffReg.setValue("svq3",DWORD(1)); ffReg.setValue("theo",DWORD(1)); ffReg.setValue("vp3",DWORD(1));
  ffReg.setValue("vp5",DWORD(1)); ffReg.setValue("vp6",DWORD(1)); ffReg.setValue("vp6f",DWORD(1));
  ffReg.setValue("xvid",DWORD(1)); ffReg.setValue("zlib",DWORD(1)); ffReg.setValue("wmv1",DWORD(1));
  ffReg.setValue("wmv2",DWORD(1)); ffReg.setValue("wmv3",DWORD(1)); ffReg.setValue("wvc1",DWORD(1));
  ffReg.setValue("cavs",DWORD(1)); ffReg.setValue("mpegAVI",DWORD(1)); ffReg.setValue("em2v",DWORD(1));
  ffReg.setValue("avrn",DWORD(1)); ffReg.setValue("dvsd",DWORD(1)); ffReg.setValue("cdvc",DWORD(1));
  ffReg.setValue("cyuv",DWORD(1)); ffReg.setValue("asv1",DWORD(1)); ffReg.setValue("vcr1",DWORD(1));
  ffReg.setValue("cram",DWORD(1)); ffReg.setValue("cvid",DWORD(1)); ffReg.setValue("rv10",DWORD(1));
  ffReg.setValue("rle",DWORD(1)); ffReg.setValue("8bps",DWORD(1)); ffReg.setValue("tscc",DWORD(1));
  ffReg.setValue("qpeg",DWORD(1)); ffReg.setValue("loco",DWORD(1)); ffReg.setValue("wnv1",DWORD(1));
  ffReg.setValue("zmbv",DWORD(1)); ffReg.setValue("ulti",DWORD(1)); ffReg.setValue("vixl",DWORD(1));
  ffReg.setValue("aasc",DWORD(1));
}