/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <winioctl.h>
#include "TextFile.h"
#include "unrar.h"
#include "VobSubFile.h"

#include "../ILog.h"
#include <fstream>
//

struct lang_type {unsigned short id; LPCSTR lang_long;} lang_tbl[] =
{
  {'--', "(Not detected)"},
  {'cc', "Closed Caption"},
  {'aa', "Afar"},
  {'ab', "Abkhazian"},
  {'af', "Afrikaans"},
  {'am', "Amharic"},
  {'ar', "Arabic"},
  {'as', "Assamese"},
  {'ay', "Aymara"},
  {'az', "Azerbaijani"},
  {'ba', "Bashkir"},
  {'be', "Byelorussian"},
  {'bg', "Bulgarian"},
  {'bh', "Bihari"},
  {'bi', "Bislama"},
  {'bn', "Bengali; Bangla"},
  {'bo', "Tibetan"},
  {'br', "Breton"},
  {'ca', "Catalan"},
  {'co', "Corsican"},
  {'cs', "Czech"},
  {'cy', "Welsh"},
  {'da', "Dansk"},
  {'de', "Deutsch"},
  {'dz', "Bhutani"},
  {'el', "Greek"},
  {'en', "English"},
  {'eo', "Esperanto"},
  {'es', "Espanol"},
  {'et', "Estonian"},
  {'eu', "Basque"},
  {'fa', "Persian"},
  {'fi', "Finnish"},
  {'fj', "Fiji"},
  {'fo', "Faroese"},
  {'fr', "Francais"},
  {'fy', "Frisian"},
  {'ga', "Irish"},
  {'gd', "Scots Gaelic"},
  {'gl', "Galician"},
  {'gn', "Guarani"},
  {'gu', "Gujarati"},
  {'ha', "Hausa"},
  {'he', "Hebrew"},
  {'hi', "Hindi"},
  {'hr', "Hrvatski"},
  {'hu', "Hungarian"},
  {'hy', "Armenian"},
  {'ia', "Interlingua"},
  {'id', "Indonesian"},
  {'ie', "Interlingue"},
  {'ik', "Inupiak"},
  {'in', "Indonesian"},
  {'is', "Islenska"},
  {'it', "Italiano"},
  {'iu', "Inuktitut"},
  {'iw', "Hebrew"},
  {'ja', "Japanese"},
  {'ji', "Yiddish"},
  {'jw', "Javanese"},
  {'ka', "Georgian"},
  {'kk', "Kazakh"},
  {'kl', "Greenlandic"},
  {'km', "Cambodian"},
  {'kn', "Kannada"},
  {'ko', "Korean"},
  {'ks', "Kashmiri"},
  {'ku', "Kurdish"},
  {'ky', "Kirghiz"},
  {'la', "Latin"},
  {'ln', "Lingala"},
  {'lo', "Laothian"},
  {'lt', "Lithuanian"},
  {'lv', "Latvian, Lettish"},
  {'mg', "Malagasy"},
  {'mi', "Maori"},
  {'mk', "Macedonian"},
  {'ml', "Malayalam"},
  {'mn', "Mongolian"},
  {'mo', "Moldavian"},
  {'mr', "Marathi"},
  {'ms', "Malay"},
  {'mt', "Maltese"},
  {'my', "Burmese"},
  {'na', "Nauru"},
  {'ne', "Nepali"},
  {'nl', "Nederlands"},
  {'no', "Norsk"},
  {'oc', "Occitan"},
  {'om', "(Afan) Oromo"},
  {'or', "Oriya"},
  {'pa', "Punjabi"},
  {'pl', "Polish"},
  {'ps', "Pashto, Pushto"},
  {'pt', "Portugues"},
  {'qu', "Quechua"},
  {'rm', "Rhaeto-Romance"},
  {'rn', "Kirundi"},
  {'ro', "Romanian"},
  {'ru', "Russian"},
  {'rw', "Kinyarwanda"},
  {'sa', "Sanskrit"},
  {'sd', "Sindhi"},
  {'sg', "Sangho"},
  {'sh', "Serbo-Croatian"},
  {'si', "Sinhalese"},
  {'sk', "Slovak"},
  {'sl', "Slovenian"},
  {'sm', "Samoan"},
  {'sn', "Shona"},
  {'so', "Somali"},
  {'sq', "Albanian"},
  {'sr', "Serbian"},
  {'ss', "Siswati"},
  {'st', "Sesotho"},
  {'su', "Sundanese"},
  {'sv', "Svenska"},
  {'sw', "Swahili"},
  {'ta', "Tamil"},
  {'te', "Telugu"},
  {'tg', "Tajik"},
  {'th', "Thai"},
  {'ti', "Tigrinya"},
  {'tk', "Turkmen"},
  {'tl', "Tagalog"},
  {'tn', "Setswana"},
  {'to', "Tonga"},
  {'tr', "Turkish"},
  {'ts', "Tsonga"},
  {'tt', "Tatar"},
  {'tw', "Twi"},
  {'ug', "Uighur"},
  {'uk', "Ukrainian"},
  {'ur', "Urdu"},
  {'uz', "Uzbek"},
  {'vi', "Vietnamese"},
  {'vo', "Volapuk"},
  {'wo', "Wolof"},
  {'xh', "Xhosa"},
  {'yi', "Yiddish"},        // formerly ji
  {'yo', "Yoruba"},
  {'za', "Zhuang"},
  {'zh', "Chinese"},
  {'zu', "Zulu"},
};

int find_lang(unsigned short id)
{
  int mid, lo = 0, hi = countof(lang_tbl) - 1;

  while(lo < hi)
  {
    mid = (lo + hi) >> 1;
    if(id < lang_tbl[mid].id) hi = mid;
    else if(id > lang_tbl[mid].id) lo = mid + 1;
    else return(mid);
  }

  return(id == lang_tbl[lo].id ? lo : 0);
}

CStdString FindLangFromId(WORD id)
{
  return CStdString(lang_tbl[find_lang(id)].lang_long);
}

//
// CVobSubFile
//

CVobSubFile::CVobSubFile(CCritSec* pLock)
  : ISubPicProviderImpl(pLock)
{
}

CVobSubFile::~CVobSubFile()
{
}

//

bool CVobSubFile::Copy(CVobSubFile& vsf)
{
  Close();

  *(CVobSubSettings*)this = *(CVobSubSettings*)&vsf;
  m_title = vsf.m_title;
  m_iLang = vsf.m_iLang;

  //m_sub.SetLength(vsf.m_sub.GetLength());
  m_sub.seekg(0); m_sub.seekp(0);

  for(int i = 0; i < 32; i++)
  {
    SubLang& src = vsf.m_langs[i];
    SubLang& dst = m_langs[i];

    dst.id = src.id;
    dst.name = src.name;
    dst.alt = src.alt;

    for(int j = 0; j < src.subpos.size(); j++)
    {
      SubPos& sp = src.subpos[j];
      if(!sp.fValid) continue;

      vsf.m_sub.seekg(sp.filepos, std::ios_base::beg);
      if(sp.filepos != vsf.m_sub.tellg())
        continue;

      sp.filepos = m_sub.tellp();

      BYTE buff[2048];
      vsf.m_sub.read((char *)buff, 2048);
      m_sub.write((const char*)buff, 2048);

      WORD packetsize = (buff[buff[0x16]+0x18]<<8) | buff[buff[0x16]+0x19];

      for(int k = 0, size, sizeleft = packetsize - 4; 
        k < packetsize - 4; 
        k += size, sizeleft -= size)
      {
        int hsize = buff[0x16]+0x18 + ((buff[0x15]&0x80) ? 4 : 0);
        size = min(sizeleft, 2048 - hsize);
  
        if(size != sizeleft) 
        {
          while(vsf.m_sub.read((char *)buff, 2048))
          {
            if(!(buff[0x15]&0x80) && buff[buff[0x16]+0x17] == (i|0x20)) break;
          }

          m_sub.write((const char*)buff, 2048);
        }
      }

      dst.subpos.push_back(sp);
    }
  }

  //m_sub.SetLength(m_sub.GetPosition());

  return(true);
}

//

void CVobSubFile::TrimExtension(CStdString& fn)
{
  int i = fn.ReverseFind('.');
  if(i > 0)
  {
    CStdString ext = fn.Mid(i).MakeLower();
    if(ext == _T(".ifo") || ext == _T(".idx") || ext == _T(".sub")
    || ext == _T(".sst") || ext == _T(".son") || ext == _T(".rar"))
      fn = fn.Left(i);
  }
}

bool CVobSubFile::Open(CStdString fn)
{
  TrimExtension(fn);

  do
  {
    Close();

    int ver;
    if(!ReadIdx(fn + _T(".idx"), ver))
      break;

    if(ver < 6 && !ReadIfo(fn + _T(".ifo")))
      break;

    if(!ReadSub(fn + _T(".sub")) && !ReadRar(fn + _T(".rar")))
      break;

    m_title = fn;

    for(int i = 0; i < 32; i++)
    {
      std::vector<SubPos>& sp = m_langs[i].subpos;

      for(int j = 0; j < sp.size(); j++)
      {
        sp[j].stop = sp[j].start;
        sp[j].fForced = false;

        int packetsize = 0, datasize = 0;
        BYTE* buff = GetPacket(j, packetsize, datasize, i);
        if(!buff) continue;

        m_img.delay = j < (sp.size()-1) ? sp[j+1].start - sp[j].start : 3000;
        m_img.GetPacketInfo(buff, packetsize, datasize);
        if(j < (sp.size()-1)) m_img.delay = min(m_img.delay, sp[j+1].start - sp[j].start);

        sp[j].stop = sp[j].start + m_img.delay;
        sp[j].fForced = m_img.fForced;

        if(j > 0 && sp[j-1].stop > sp[j].start)
          sp[j-1].stop = sp[j].start;

        delete [] buff;
      }
    }

    return(true);
  }
  while(false);

  Close();

  return(false);
}

bool CVobSubFile::Save(CStdString fn, SubFormat sf)
{
  TrimExtension(fn);

  CVobSubFile vsf(NULL);
  if(!vsf.Copy(*this))
    return(false);

  switch(sf)
  {
  case VobSub: return vsf.SaveVobSub(fn); break;
  case WinSubMux: return vsf.SaveWinSubMux(fn); break;
  case Scenarist: return vsf.SaveScenarist(fn); break;
  case Maestro: return vsf.SaveMaestro(fn); break;
  default: break;
  }

  return(false);
}

void CVobSubFile::Close()
{
  InitSettings();
  m_title.Empty();
  m_sub.clear();
  m_img.Invalidate();
  m_iLang = -1;
  for(int i = 0; i < 32; i++)
  {
    m_langs[i].id = 0;
    m_langs[i].name.Empty();
    m_langs[i].alt.Empty();
    m_langs[i].subpos.clear();
  }
}

//

bool CVobSubFile::ReadIdx(CStdString fn, int& ver)
{
  CWebTextFile f;
  if(!f.Open(fn))
    return(false);

  bool fError = false;

  int id = -1, delay = 0, vobid = -1, cellid = -1;
  __int64 celltimestamp = 0;

  CStdString str;
  for(int line = 0; !fError && f.ReadString(str); line++)
  {
    str.Trim();

    if(line == 0)
    {
      TCHAR buff[] = _T("VobSub index file, v");

      const TCHAR* s = str;

      int i = str.Find(buff);
      if(i < 0 || _stscanf(&s[i+_tcslen(buff)], _T("%d"), &ver) != 1
      || ver > VOBSUBIDXVER)
      {
        g_log->Log(LOGERROR, "%s Wrong file version!", __FUNCTION__);
        fError = true;
        continue;
      }
    }
    else if(!str.GetLength())
    {
      continue;
    }
    else if(str[0] == _T('#'))
    {
      TCHAR buff[] = _T("Vob/Cell ID:");

      const TCHAR* s = str;

      int i = str.Find(buff);
      if(i >= 0)
      {
        _stscanf(&s[i+_tcslen(buff)], _T("%d, %d (PTS: %d)"), &vobid, &cellid, &celltimestamp);
      }

      continue;
    }

    int i = str.Find(':');
    if(i <= 0) continue;

    CStdString entry = str.Left(i).MakeLower();

    str = str.Mid(i+1);
    str.Trim();
    if(str.IsEmpty()) continue;

    if(entry == _T("size"))
    {
      int x, y;
      if(_stscanf(str, _T("%dx%d"), &x, &y) != 2) fError = true;
      m_size.cx = x;
      m_size.cy = y;
    }
    else if(entry == _T("org"))
    {
      if(_stscanf(str, _T("%d,%d"), &m_x, &m_y) != 2) fError = true;
      else m_org = Com::SmartPoint(m_x, m_y);
    }
    else if(entry == _T("scale"))
    {
      if(ver < 5) 
      {
        int scale = 100;
        if(_stscanf(str, _T("%d%%"), &scale) != 1) fError = true;
        m_scale_x = m_scale_y = scale;
      }
      else 
      {
        if(_stscanf(str, _T("%d%%,%d%%"), &m_scale_x, &m_scale_y) != 2) fError = true;
      }
    }
    else if(entry == _T("alpha"))
    {
      if(_stscanf(str, _T("%d"), &m_alpha) != 1) fError = true;
    }
    else if(entry == _T("smooth"))
    {
      str.MakeLower();

      if(str.Find(_T("old")) >= 0 || str.Find(_T("2")) >= 0) m_fSmooth = 2;
      else if(str.Find(_T("on")) >= 0 || str.Find(_T("1")) >= 0) m_fSmooth = 1;
      else if(str.Find(_T("off")) >= 0 || str.Find(_T("0")) >= 0) m_fSmooth = 0;
      else fError = true;
    }
    else if(entry == _T("fadein/out"))
    {
      if(_stscanf(str, _T("%d,%d"), &m_fadein, &m_fadeout) != 2) fError = true;
    }
    else if(entry == _T("align"))
    {
      str.MakeLower();

      int j = 0;
      std::vector<CStdString> tokens;
      str.Tokenize(_T(" "), tokens);
      CStdString token = tokens[0];
      for(int i = 1; j < 3 && !fError && i < tokens.size(); token = tokens[i++], j++)
      {
        if(j == 0)
        {
          if(token == _T("on") || token == _T("1")) m_fAlign = true;
          else if(token == _T("off") || token == _T("0")) m_fAlign = false;
          else fError = true;
        }
        else if(j == 1)
        {
          if(token == _T("at")) {j--; continue;}

          if(token == _T("left")) m_alignhor = 0;
          else if(token == _T("center")) m_alignhor = 1;
          else if(token == _T("right")) m_alignhor = 2;
          else fError = true;
        }
        else if(j == 2)
        {
          if(token == _T("top")) m_alignver = 0;
          else if(token == _T("center")) m_alignver = 1;
          else if(token == _T("bottom")) m_alignver = 2;
          else fError = true;
        }
      }
    }
    else if(entry == _T("time offset"))
    {
      bool fNegative = false;
      if(str[0] == '-') fNegative = true;
      str.TrimLeft(_T("+-"));

      TCHAR c;
      int hh, mm, ss, ms;
      int n = _stscanf(str, _T("%d%c%d%c%d%c%d"), &hh, &c, &mm, &c, &ss, &c, &ms);
      
      m_toff = n == 1 
          ? hh * (fNegative ? -1 : 1)
          : n == 4+3
          ? (hh*60*60*1000 + mm*60*1000 + ss*1000 + ms) * (fNegative ? -1 : 1)
          : fError = true, 0;
    }
    else if(entry == _T("forced subs"))
    {
      str.MakeLower();

      if(str.Find(_T("on")) >= 0 || str.Find(_T("1")) >= 0) m_fOnlyShowForcedSubs = true;
      else if(str.Find(_T("off")) >= 0 || str.Find(_T("0")) >= 0) m_fOnlyShowForcedSubs = false;
      else fError = true;
    }
    else if(entry == _T("langidx"))
    {
      if(_stscanf(str, _T("%d"), &m_iLang) != 1) fError = true;
    }
    else if(entry == _T("palette"))
    {
      if(_stscanf(str, _T("%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x"), 
        &m_orgpal[0], &m_orgpal[1], &m_orgpal[2], &m_orgpal[3],
        &m_orgpal[4], &m_orgpal[5], &m_orgpal[6], &m_orgpal[7],
        &m_orgpal[8], &m_orgpal[9], &m_orgpal[10], &m_orgpal[11],
        &m_orgpal[12], &m_orgpal[13], &m_orgpal[14], &m_orgpal[15]
        ) != 16) fError = true;
    }
    else if(entry == _T("custom colors"))
    {
      str.MakeLower();

      if(str.Find(_T("on")) == 0 || str.Find(_T("1")) == 0) m_fCustomPal = true;
      else if(str.Find(_T("off")) == 0 || str.Find(_T("0")) == 0) m_fCustomPal = false;
      else fError = true;

      i = str.Find(_T("tridx:"));
      if(i < 0) {fError = true; continue;}
      str = str.Mid(i + (int)_tcslen(_T("tridx:")));

      int tridx;
      if(_stscanf(str, _T("%x"), &tridx) != 1) {fError = true; continue;}
      tridx = ((tridx&0x1000)>>12) | ((tridx&0x100)>>7) | ((tridx&0x10)>>2) | ((tridx&1)<<3);

      i = str.Find(_T("colors:"));
      if(i < 0) {fError = true; continue;}
      str = str.Mid(i + (int)_tcslen(_T("colors:")));

      RGBQUAD pal[4];
      if(_stscanf(str, _T("%x,%x,%x,%x"), &pal[0], &pal[1], &pal[2], &pal[3]) != 4) {fError = true; continue;}

      SetCustomPal(pal, tridx);
    }
    else if(entry == _T("id"))
    {
      str.MakeLower();

      int langid = ((str[0]&0xff)<<8)|(str[1]&0xff);

      i = str.Find(_T("index:"));
      if(i < 0) {fError = true; continue;}
      str = str.Mid(i + (int)_tcslen(_T("index:")));

      if(_stscanf(str, _T("%d"), &id) != 1 || id < 0 || id >= 32) {fError = true; continue;}

      m_langs[id].id = langid;
      m_langs[id].name = lang_tbl[find_lang(langid)].lang_long;
      m_langs[id].alt = lang_tbl[find_lang(langid)].lang_long;

      delay = 0;
    }
    else if(id >= 0 && entry == _T("alt"))
    {
      m_langs[id].alt = str;
    }
    else if(id >= 0 && entry == _T("delay"))
    {
      bool fNegative = false;
      if(str[0] == '-') fNegative = true;
      str.TrimLeft(_T("+-"));

      TCHAR c;
      int hh, mm, ss, ms;
      if(_stscanf(str, _T("%d%c%d%c%d%c%d"), &hh, &c, &mm, &c, &ss, &c, &ms) != 4+3)  {fError = true; continue;}

      delay += (hh*60*60*1000 + mm*60*1000 + ss*1000 + ms) * (fNegative ? -1 : 1);
    }
    else if(id >= 0 && entry == _T("timestamp"))
    {
      SubPos sb;

      sb.vobid = vobid;
      sb.cellid = cellid;
      sb.celltimestamp = celltimestamp;
      sb.fValid = true;

      bool fNegative = false;
      if(str[0] == '-') fNegative = true;
      str.TrimLeft(_T("+-"));

      TCHAR c;
      int hh, mm, ss, ms;
      if(_stscanf(str, _T("%d%c%d%c%d%c%d"), &hh, &c, &mm, &c, &ss, &c, &ms) != 4+3)  {fError = true; continue;}

      sb.start = (hh*60*60*1000 + mm*60*1000 + ss*1000 + ms) * (fNegative ? -1 : 1) + delay;

      i = str.Find(_T("filepos:"));
      if(i < 0) {fError = true; continue;}
      str = str.Mid(i + (int)_tcslen(_T("filepos:")));

      if(_stscanf(str, _T("%I64x"), &sb.filepos) != 1) {fError = true; continue;}

      if(delay < 0 && m_langs[id].subpos.size() > 0)
      {
        __int64 ts = m_langs[id].subpos[m_langs[id].subpos.size()-1].start;
          
        if(sb.start < ts)
        {
          delay += (int)(ts - sb.start);
          sb.start = ts;
        }
      }

      m_langs[id].subpos.push_back(sb);
    }
    else fError = true;
  }

  return(!fError);
}

bool CVobSubFile::ReadSub(CStdString fn)
{
  ATL::CFile f;
  if(! f.Open(fn, ATL::CFile::modeRead|ATL::CFile::typeBinary|ATL::CFile::shareDenyNone))
    return false;

  //m_sub.SetLength(f.GetLength());
  m_sub.seekp(0); m_sub.seekg(0);
  m_sub.str("");
  m_sub.clear();

  int len = 0;
  BYTE buff[2048];
  while((len = f.Read(buff, sizeof(buff))) > 0 && *(DWORD*)buff == 0xba010000)
  {
    m_sub.write((const char*)buff, len);
    if (m_sub.bad())
      return false;
  }

  return(true);
}

static unsigned char* RARbuff = NULL;
static unsigned int RARpos = 0;

static int PASCAL MyProcessDataProc(unsigned char* Addr, int Size)
{
  ASSERT(RARbuff);

  memcpy(&RARbuff[RARpos], Addr, Size);
  RARpos += Size;

  return(1);
}

bool CVobSubFile::ReadRar(CStdString fn)
{
#ifdef _WIN64
	HMODULE h = LoadLibrary(_T("unrar64.dll"));
#else
  HMODULE h = LoadLibrary(_T("unrar.dll"));
#endif
  if(!h) return(false);

  RAROpenArchiveEx OpenArchiveEx = (RAROpenArchiveEx)GetProcAddress(h, "RAROpenArchiveEx");
  RARCloseArchive CloseArchive = (RARCloseArchive)GetProcAddress(h, "RARCloseArchive");
  RARReadHeaderEx ReadHeaderEx = (RARReadHeaderEx)GetProcAddress(h, "RARReadHeaderEx");
  RARProcessFile ProcessFile = (RARProcessFile)GetProcAddress(h, "RARProcessFile");
  RARSetChangeVolProc SetChangeVolProc = (RARSetChangeVolProc)GetProcAddress(h, "RARSetChangeVolProc");
  RARSetProcessDataProc SetProcessDataProc = (RARSetProcessDataProc)GetProcAddress(h, "RARSetProcessDataProc");
  RARSetPassword SetPassword = (RARSetPassword)GetProcAddress(h, "RARSetPassword");

  if(!(OpenArchiveEx && CloseArchive && ReadHeaderEx && ProcessFile 
  && SetChangeVolProc && SetProcessDataProc && SetPassword))
  {
    FreeLibrary(h);
    return(false);
  }

  struct RAROpenArchiveDataEx ArchiveDataEx;
  memset(&ArchiveDataEx, 0, sizeof(ArchiveDataEx));
#ifdef UNICODE
  ArchiveDataEx.ArcNameW = (LPTSTR)(LPCTSTR)fn;
  char fnA[MAX_PATH];
  if(wcstombs(fnA, fn, fn.GetLength()+1) == -1) fnA[0] = 0;
  ArchiveDataEx.ArcName = fnA;
#else
  ArchiveDataEx.ArcName = (LPTSTR)(LPCTSTR)fn;
#endif
  ArchiveDataEx.OpenMode = RAR_OM_EXTRACT;
  ArchiveDataEx.CmtBuf = 0;
  HANDLE hrar = OpenArchiveEx(&ArchiveDataEx);
  if(!hrar) 
  {
    FreeLibrary(h);
    return(false);
  }

  SetProcessDataProc(hrar, MyProcessDataProc);

  struct RARHeaderDataEx HeaderDataEx;
  HeaderDataEx.CmtBuf = NULL;
  
  while(ReadHeaderEx(hrar, &HeaderDataEx) == 0)
  {
#ifdef UNICODE
    CStdString subfn(HeaderDataEx.FileNameW);
#else
    CStdString subfn(HeaderDataEx.FileName);
#endif

    if(!subfn.Right(4).CompareNoCase(_T(".sub")))
    {
      std::vector<BYTE> buff(HeaderDataEx.UnpSize);
      if(! true)
      {
        CloseArchive(hrar);
        FreeLibrary(h);
        return(false);
      }

      RARbuff = (unsigned char*) &buff[0];
      RARpos = 0;

      if(ProcessFile(hrar, RAR_TEST, NULL, NULL))
      {
        CloseArchive(hrar);
        FreeLibrary(h);
        
        return(false);
      }

      //m_sub.SetLength(HeaderDataEx.UnpSize);
      m_sub.str("");
      m_sub.seekp(0);
      m_sub.write((char *)&buff[0], HeaderDataEx.UnpSize);
      m_sub.seekp(0);

      RARbuff = NULL;
      RARpos = 0;

      break;
    }

    ProcessFile(hrar, RAR_SKIP, NULL, NULL);
  }

  CloseArchive(hrar);
  FreeLibrary(h);

  return(true);
}

#define ReadBEdw(var) \
    f.read((char *)&((BYTE*)&var)[3], 1); \
  f.read((char *)&((BYTE*)&var)[2], 1); \
  f.read((char *)&((BYTE*)&var)[1], 1); \
  f.read((char *)&((BYTE*)&var)[0], 1); \

bool CVobSubFile::ReadIfo(CStdString fn)
{
  std::ifstream f;
  f.open(fn, std::ios_base::in | std::ios_base::binary);
  if(f.fail())
    return(false);

  /* PGC1 */

  f.seekg(0xc0+0x0c, std::ios_base::beg);

  DWORD pos;
  ReadBEdw(pos);

  f.seekg(pos*0x800 + 0x0c, std::ios_base::beg);

  DWORD offset;
  ReadBEdw(offset);
  
  /* Subpic palette */

  f.seekg(pos*0x800 + offset + 0xa4, std::ios_base::beg);

  for(int i = 0; i < 16; i++) 
  {
    BYTE y, u, v, tmp;

    f.read((char *)&tmp, 1);
    f.read((char *)&y, 1);
    f.read((char *)&u, 1);
    f.read((char *)&v, 1);

    y = (y-16)*255/219;

    m_orgpal[i].rgbRed = (BYTE)min(max(1.0*y + 1.4022*(u-128), 0), 255);
    m_orgpal[i].rgbGreen = (BYTE)min(max(1.0*y - 0.3456*(u-128) - 0.7145*(v-128), 0), 255);
    m_orgpal[i].rgbBlue = (BYTE)min(max(1.0*y + 1.7710*(v-128), 0) , 255);
  }

  return(true);
}

bool CVobSubFile::WriteIdx(CStdString fn)
{
  CTextFile f;
  if(!f.Save(fn, CTextFile::ASCII))
    return(false);

  CStdString str;
  str.Format(_T("# VobSub index file, v%d (do not modify this line!)\n"), VOBSUBIDXVER);

  f.WriteString(str);
  f.WriteString(_T("# \n"));
  f.WriteString(_T("# To repair desyncronization, you can insert gaps this way:\n"));
  f.WriteString(_T("# (it usually happens after vob id changes)\n"));
  f.WriteString(_T("# \n"));
  f.WriteString(_T("#\t delay: [sign]hh:mm:ss:ms\n"));
  f.WriteString(_T("# \n"));
  f.WriteString(_T("# Where:\n"));
  f.WriteString(_T("#\t [sign]: +, - (optional)\n"));
  f.WriteString(_T("#\t hh: hours (0 <= hh)\n"));
  f.WriteString(_T("#\t mm/ss: minutes/seconds (0 <= mm/ss <= 59)\n"));
  f.WriteString(_T("#\t ms: milliseconds (0 <= ms <= 999)\n"));
  f.WriteString(_T("# \n"));
  f.WriteString(_T("#\t Note: You can't position a sub before the previous with a negative value.\n"));
  f.WriteString(_T("# \n"));  
  f.WriteString(_T("# You can also modify timestamps or delete a few subs you don't like.\n"));
  f.WriteString(_T("# Just make sure they stay in increasing order.\n"));
  f.WriteString(_T("\n"));
  f.WriteString(_T("\n"));

  // Settings

  f.WriteString(_T("# Settings\n\n"));

  f.WriteString(_T("# Original frame size\n"));
  str.Format(_T("size: %dx%d\n\n"), m_size.cx, m_size.cy);
  f.WriteString(str);

  f.WriteString(_T("# Origin, relative to the upper-left corner, can be overloaded by aligment\n"));
  str.Format(_T("org: %d, %d\n\n"), m_x, m_y);
  f.WriteString(str);

  f.WriteString(_T("# Image scaling (hor,ver), origin is at the upper-left corner or at the alignment coord (x, y)\n"));
  str.Format(_T("scale: %d%%, %d%%\n\n"), m_scale_x, m_scale_y);
  f.WriteString(str);

  f.WriteString(_T("# Alpha blending\n"));
  str.Format(_T("alpha: %d%%\n\n"), m_alpha);
  f.WriteString(str);

  f.WriteString(_T("# Smoothing for very blocky images (use OLD for no filtering)\n"));
  str.Format(_T("smooth: %s\n\n"), m_fSmooth == 0 ? _T("OFF") : m_fSmooth == 1 ? _T("ON") : _T("OLD"));
  f.WriteString(str);

  f.WriteString(_T("# In millisecs\n"));
  str.Format(_T("fadein/out: %d, %d\n\n"), m_fadein, m_fadeout);
  f.WriteString(str);

  f.WriteString(_T("# Force subtitle placement relative to (org.x, org.y)\n"));
  str.Format(_T("align: %s %s %s\n\n"), 
    m_fAlign ? _T("ON at") : _T("OFF at"), 
    m_alignhor == 0 ? _T("LEFT") : m_alignhor == 1 ? _T("CENTER") : m_alignhor == 2 ? _T("RIGHT") : _T(""), 
    m_alignver == 0 ? _T("TOP") : m_alignver == 1 ? _T("CENTER") : m_alignver == 2 ? _T("BOTTOM") : _T(""));
  f.WriteString(str);

  f.WriteString(_T("# For correcting non-progressive desync. (in millisecs or hh:mm:ss:ms)\n"));
  f.WriteString(_T("# Note: Not effective in DirectVobSub, use \"delay: ... \" instead.\n"));
  str.Format(_T("time offset: %d\n\n"), m_toff);
  f.WriteString(str);

  f.WriteString(_T("# ON: displays only forced subtitles, OFF: shows everything\n"));
  str.Format(_T("forced subs: %s\n\n"), m_fOnlyShowForcedSubs ? _T("ON") : _T("OFF"));
  f.WriteString(str);

  f.WriteString(_T("# The original palette of the DVD\n"));
  str.Format(_T("palette: %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x\n\n"), 
    *((unsigned int*)&m_orgpal[0])&0xffffff,
    *((unsigned int*)&m_orgpal[1])&0xffffff,
    *((unsigned int*)&m_orgpal[2])&0xffffff,
    *((unsigned int*)&m_orgpal[3])&0xffffff,
    *((unsigned int*)&m_orgpal[4])&0xffffff,
    *((unsigned int*)&m_orgpal[5])&0xffffff,
    *((unsigned int*)&m_orgpal[6])&0xffffff,
    *((unsigned int*)&m_orgpal[7])&0xffffff,
    *((unsigned int*)&m_orgpal[8])&0xffffff,
    *((unsigned int*)&m_orgpal[9])&0xffffff,
    *((unsigned int*)&m_orgpal[10])&0xffffff,
    *((unsigned int*)&m_orgpal[11])&0xffffff,
    *((unsigned int*)&m_orgpal[12])&0xffffff,
    *((unsigned int*)&m_orgpal[13])&0xffffff,
    *((unsigned int*)&m_orgpal[14])&0xffffff,
    *((unsigned int*)&m_orgpal[15])&0xffffff);
  f.WriteString(str);

  int tridx = (!!(m_tridx&1))*0x1000 + (!!(m_tridx&2))*0x100 + (!!(m_tridx&4))*0x10 + (!!(m_tridx&8));

  f.WriteString(_T("# Custom colors (transp idxs and the four colors)\n"));
  str.Format(_T("custom colors: %s, tridx: %04x, colors: %06x, %06x, %06x, %06x\n\n"), 
    m_fCustomPal ? _T("ON") : _T("OFF"),
    tridx, 
    *((unsigned int*)&m_cuspal[0])&0xffffff,
    *((unsigned int*)&m_cuspal[1])&0xffffff,
    *((unsigned int*)&m_cuspal[2])&0xffffff,
    *((unsigned int*)&m_cuspal[3])&0xffffff);
  f.WriteString(str);

  f.WriteString(_T("# Language index in use\n"));
  str.Format(_T("langidx: %d\n\n"), m_iLang);
  f.WriteString(str);

  // Subs

  for(int i = 0; i < 32; i++)
  {
    SubLang& sl = m_langs[i];

    std::vector<SubPos>& sp = sl.subpos;
    if(sp.empty() && !sl.id) continue;

    str.Format(_T("# %s\n"), sl.name);
    f.WriteString(str);

    ASSERT(sl.id);
    if(!sl.id) sl.id = '--';
    str.Format(_T("id: %c%c, index: %d\n"), sl.id>>8, sl.id&0xff, i);
    f.WriteString(str);

    str.Format(_T("# Decomment next line to activate alternative name in DirectVobSub / Windows Media Player 6.x\n"));
    f.WriteString(str);
    str.Format(_T("alt: %s\n"), sl.alt);
    if(sl.name == sl.alt) str = _T("# ") + str;
    f.WriteString(str);

    char vobid = -1, cellid = -1;

    for(int j = 0; j < sp.size(); j++) 
    {
      if(!sp[j].fValid) continue;

      if(sp[j].vobid != vobid || sp[j].cellid != cellid)
      {
        str.Format(_T("# Vob/Cell ID: %d, %d (PTS: %d)\n"), sp[j].vobid, sp[j].cellid, sp[j].celltimestamp);
        f.WriteString(str);
        vobid = sp[j].vobid;
        cellid = sp[j].cellid;
      }
      
      str.Format(_T("timestamp: %s%02d:%02d:%02d:%03d, filepos: %09I64x\n"), 
        sp[j].start < 0 ? _T("-") : _T(""),
        abs(int((sp[j].start/1000/60/60)%60)), 
        abs(int((sp[j].start/1000/60)%60)), 
        abs(int((sp[j].start/1000)%60)), 
        abs(int((sp[j].start)%1000)), 
        sp[j].filepos);
      f.WriteString(str);
    }

    f.WriteString(_T("\n"));
  }

  return(true);
}

bool CVobSubFile::WriteSub(CStdString fn)
{
  ATL::CFile f;
  if(!f.Open(fn, ATL::CFile::modeCreate | ATL::CFile::modeWrite | ATL::CFile::typeBinary | ATL::CFile::shareDenyWrite))
    return(false);

  m_sub.seekg(0, std::ios_base::end);
  if((int) m_sub.tellg() == 0)
    return(true); // nothing to do...

  m_sub.seekg(0);

  int len;
  BYTE buff[2048];
  m_sub.read((char *)buff, sizeof(buff));
  while((len = m_sub.gcount()) > 0 && *(DWORD*)buff == 0xba010000)
  {
    f.Write(buff, len);
    m_sub.read((char *)buff, sizeof(buff));
  }

  return(true);
}

//

BYTE* CVobSubFile::GetPacket(int idx, int& packetsize, int& datasize, int iLang)
{
  BYTE* ret = NULL;

  if(iLang < 0 || iLang >= 32) iLang = m_iLang;
  std::vector<SubPos>& sp = m_langs[iLang].subpos;

  do
  {
    if(idx < 0 || idx >= sp.size())
      break;

    m_sub.seekg(sp[idx].filepos, std::ios::beg);
    if(m_sub.fail()) 
      break;

    BYTE buff[0x800];
    if(m_sub.rdbuf()->sgetn((char *)buff, sizeof(buff)) != sizeof(buff))
      break;

    BYTE offset = buff[0x16];

    // let's check a few things to make sure...
    if(*(DWORD*)&buff[0x00] != 0xba010000
    || *(DWORD*)&buff[0x0e] != 0xbd010000
    || !(buff[0x15] & 0x80)  
    || (buff[0x17] & 0xf0) != 0x20
    || (buff[buff[0x16] + 0x17] & 0xe0) != 0x20
    || (buff[buff[0x16] + 0x17] & 0x1f) != iLang)
      break;

    packetsize = (buff[buff[0x16] + 0x18] << 8) + buff[buff[0x16] + 0x19];
    datasize = (buff[buff[0x16] + 0x1a] << 8) + buff[buff[0x16] + 0x1b];

    ret = DNew BYTE[packetsize];
    if(!ret) break;

    int i = 0, sizeleft = packetsize;
    for(int size; 
      i < packetsize; 
      i += size, sizeleft -= size)
    {
      int hsize = 0x18 + buff[0x16];
      size = min(sizeleft, 0x800 - hsize);
      memcpy(&ret[i], &buff[hsize], size);

      if(size != sizeleft) 
      {
        m_sub.read((char *)buff, sizeof(buff));
        while(!m_sub.fail() || !m_sub.eof())
        {
          if(/*!(buff[0x15] & 0x80) &&*/ buff[buff[0x16] + 0x17] == (iLang|0x20)) 
            break;
          m_sub.read((char *)buff, sizeof(buff));
        }
      }
    }

    if(i != packetsize || sizeleft > 0)
      delete [] ret, ret = NULL;
  }
  while(false);

  return(ret);
}

bool CVobSubFile::GetFrame(int idx, int iLang)
{
  if(iLang < 0 || iLang >= 32) iLang = m_iLang;
  std::vector<SubPos>& sp = m_langs[iLang].subpos;

  if(idx < 0 || idx >= sp.size())
    return(false);

  if(m_img.iLang != iLang || m_img.iIdx != idx) 
  {
    int packetsize = 0, datasize = 0;
    BYTE* buff = GetPacket(idx, packetsize, datasize, iLang);

    if(!buff || packetsize <= 0 || datasize <= 0) return(false);

    m_img.start = sp[idx].start;
    m_img.delay = idx < (sp.size()-1)
      ? sp[idx+1].start - sp[idx].start
      : 3000;

    bool ret = m_img.Decode(buff, packetsize, datasize, m_fCustomPal, m_tridx, m_orgpal, m_cuspal, true);

    delete[] buff;
    
    if(idx < (sp.size()-1))
      m_img.delay = min(m_img.delay, sp[idx+1].start - m_img.start);

    if(!ret) return(false);
    
    m_img.iIdx = idx;
    m_img.iLang = iLang;
  }

  return(m_fOnlyShowForcedSubs ? m_img.fForced : true);
}

bool CVobSubFile::GetFrameByTimeStamp(__int64 time)
{
  return(GetFrame(GetFrameIdxByTimeStamp(time)));
}

int CVobSubFile::GetFrameIdxByTimeStamp(__int64 time)
{
  if(m_iLang < 0 || m_iLang >= 32)
    return(-1);

  std::vector<SubPos>& sp = m_langs[m_iLang].subpos;

  int i = 0, j = (int)sp.size() - 1, ret = -1;

  if(j >= 0 && time >= sp[j].start)
    return(j);

  while(i < j)
  {
    int mid = (i + j) >> 1;
    int midstart = (int)sp[mid].start;

    if(time == midstart)
    {
      ret = mid;
      break;
    }
    else if(time < midstart)
    {
      ret = -1;
      if(j == mid) mid--;
      j = mid;
    }
    else if(time > midstart)
    {
      ret = mid;
      if(i == mid) mid++;
      i = mid;
    }
  }

  return(ret);
}

//

STDMETHODIMP CVobSubFile::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);
    *ppv = NULL;

    return 
    QI(IPersist)
    QI(ISubStream)
    QI(ISubPicProvider)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

// TODO: return segments for the fade-in/out time (with animated set to "true" of course)

STDMETHODIMP_(__w64 int) CVobSubFile::GetStartPosition(REFERENCE_TIME rt, double fps)
{
  rt /= 10000;

  int i = GetFrameIdxByTimeStamp(rt);

  if(!GetFrame(i))
    return(NULL);

  if(rt >= (m_img.start + m_img.delay))
  {
    if(!GetFrame(++i))
      return(NULL);
  }

  return(i+1);
}

STDMETHODIMP_(REFERENCE_TIME) CVobSubFile::GetStart(int pos, double fps)
{
  int i = (int)pos-1;
  return(GetFrame(i) ? 10000i64*m_img.start : 0);
}

STDMETHODIMP_(REFERENCE_TIME) CVobSubFile::GetStop(int pos, double fps)
{
  int i = (int)pos-1;
  return(GetFrame(i) ? 10000i64*(m_img.start + m_img.delay) : 0);
}

STDMETHODIMP_(bool) CVobSubFile::IsAnimated(int pos)
{
  return(false);
}

STDMETHODIMP CVobSubFile::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
  if(spd.bpp != 32) return E_INVALIDARG;

  rt /= 10000;

  if(!GetFrame(GetFrameIdxByTimeStamp(rt)))
    return E_FAIL;

  if(rt >= (m_img.start + m_img.delay))
    return E_FAIL;

  return __super::Render(spd, bbox);
}

// IPersist

STDMETHODIMP CVobSubFile::GetClassID(CLSID* pClassID)
{
  return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CVobSubFile::GetStreamCount()
{
  int iStreamCount = 0;
  for(int i = 0; i < 32; i++)
    if(m_langs[i].subpos.size()) iStreamCount++;
  return(iStreamCount);
}

STDMETHODIMP CVobSubFile::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
  for(int i = 0; i < 32; i++)
  {
    SubLang& sl = m_langs[i];
    
    if(sl.subpos.empty() || iStream-- > 0)
      continue;

    if(ppName)
    {
      if(!(*ppName = (WCHAR*)CoTaskMemAlloc((sl.alt.GetLength()+1)*sizeof(WCHAR))))
        return E_OUTOFMEMORY;

      wcscpy(*ppName, CStdStringW(sl.alt));
    }

    if(pLCID)
    {
      *pLCID = 0; // TODO: make lcid out of "sl.id"
    }

    return S_OK;
  }

  return E_FAIL;
}

STDMETHODIMP_(int) CVobSubFile::GetStream()
{
  int iStream = 0;

  for(int i = 0; i < m_iLang; i++)
    if(!m_langs[i].subpos.empty()) iStream++;

  return(iStream);
}

STDMETHODIMP CVobSubFile::SetStream(int iStream)
{
  for(int i = 0; i < 32; i++)
  {
    std::vector<SubPos>& sp = m_langs[i].subpos;

    if(sp.empty() || iStream-- > 0)
      continue;

    m_iLang = i;

    m_img.Invalidate();

    break;
  }

  return iStream < 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CVobSubFile::Reload()
{
  
  if(!ATL::CFile::FileExists(m_title + _T(".idx"))) return E_FAIL;
  return !m_title.IsEmpty() && Open(m_title) ? S_OK : E_FAIL;
}

// StretchBlt

static void PixelAtBiLinear(RGBQUAD& c, int x, int y, CVobSubImage& src)
{
  int w = src.rect.Width(),
    h = src.rect.Height();

  int x1 = (x >> 16), y1 = (y >> 16) * w,
    x2 = min(x1 + 1, w-1), y2 = min(y1 + w, (h-1)*w);

  RGBQUAD* ptr = src.lpPixels;

  RGBQUAD c11 = ptr[y1 + x1],  c12 = ptr[y1 + x2],
      c21 = ptr[y2 + x1],  c22 = ptr[y2 + x2];

  __int64 u2 = x & 0xffff,
      v2 = y & 0xffff,
      u1 = 0x10000 - u2,
      v1 = 0x10000 - v2;

  int v1u1 = int(v1*u1 >> 16) * c11.rgbReserved, 
    v1u2 = int(v1*u2 >> 16) * c12.rgbReserved,
    v2u1 = int(v2*u1 >> 16) * c21.rgbReserved, 
    v2u2 = int(v2*u2 >> 16) * c22.rgbReserved;

  c.rgbRed = (c11.rgbRed * v1u1 + c12.rgbRed * v1u2
        + c21.rgbRed * v2u1 + c22.rgbRed * v2u2) >> 24;
  c.rgbGreen = (c11.rgbGreen * v1u1 + c12.rgbGreen * v1u2
        + c21.rgbGreen * v2u1 + c22.rgbGreen * v2u2) >> 24;
  c.rgbBlue = (c11.rgbBlue * v1u1 + c12.rgbBlue * v1u2
        + c21.rgbBlue * v2u1 + c22.rgbBlue * v2u2) >> 24;
  c.rgbReserved = (v1u1 + v1u2 
          + v2u1 + v2u2) >> 16;
}

static void StretchBlt(SubPicDesc& spd, Com::SmartRect dstrect, CVobSubImage& src)
{
  if(dstrect.IsRectEmpty()) return;

  if((dstrect & Com::SmartRect(0, 0, spd.w, spd.h)).IsRectEmpty()) return;

  int sw = src.rect.Width(),
    sh = src.rect.Height(),
    dw = dstrect.Width(),
    dh = dstrect.Height();

  int srcx = 0, 
    srcy = 0,
    srcdx = (sw << 16) / dw >> 1, 
    srcdy = (sh << 16) / dh >> 1;

  if(dstrect.left < 0)
  {
    srcx = -dstrect.left * (srcdx << 1);
    dstrect.left = 0;
  }
  if(dstrect.top < 0)
  {
    srcy = -dstrect.top * (srcdy << 1);
    dstrect.top = 0;
  }
  if(dstrect.right > spd.w)
  {
    dstrect.right = spd.w;
  }
  if(dstrect.bottom > spd.h)
  {
    dstrect.bottom = spd.h;
  }

  if((dstrect & Com::SmartRect(0, 0, spd.w, spd.h)).IsRectEmpty()) return;

  dw = dstrect.Width();
  dh = dstrect.Height();

  for(int y = dstrect.top; y < dstrect.bottom; y++, srcy += (srcdy<<1))
  {
    RGBQUAD* ptr = (RGBQUAD*)&((BYTE*)spd.bits)[y*spd.pitch] + dstrect.left;
    RGBQUAD* endptr = ptr + dw;
    
    for(int sx = srcx; ptr < endptr; sx += (srcdx<<1), ptr++)
    {
//      PixelAtBiLinear(*ptr,  sx,      srcy,    src);
////
      RGBQUAD cc[4];

      PixelAtBiLinear(cc[0],  sx,      srcy,    src);
      PixelAtBiLinear(cc[1],  sx+srcdx,  srcy,    src);
      PixelAtBiLinear(cc[2],  sx,      srcy+srcdy,  src);
      PixelAtBiLinear(cc[3],  sx+srcdx,  srcy+srcdy,  src);
      
      ptr->rgbRed = (cc[0].rgbRed + cc[1].rgbRed + cc[2].rgbRed + cc[3].rgbRed) >> 2;
      ptr->rgbGreen = (cc[0].rgbGreen + cc[1].rgbGreen + cc[2].rgbGreen + cc[3].rgbGreen) >> 2;
      ptr->rgbBlue = (cc[0].rgbBlue + cc[1].rgbBlue + cc[2].rgbBlue + cc[3].rgbBlue) >> 2;
      ptr->rgbReserved = (cc[0].rgbReserved + cc[1].rgbReserved + cc[2].rgbReserved + cc[3].rgbReserved) >> 2;
////
      ptr->rgbRed = ptr->rgbRed * ptr->rgbReserved >> 8;
      ptr->rgbGreen = ptr->rgbGreen * ptr->rgbReserved >> 8;
      ptr->rgbBlue = ptr->rgbBlue * ptr->rgbReserved >> 8;
      ptr->rgbReserved = ~ptr->rgbReserved;

    }
  }
}

//
// CVobSubSettings
//

void CVobSubSettings::InitSettings()
{
  m_size.SetSize(720, 480);
  m_toff = m_x = m_y = 0;
  m_org.SetPoint(0, 0);
  m_scale_x = m_scale_y = m_alpha = 100;
  m_fadein = m_fadeout = 50;
  m_fSmooth = 0;
  m_fAlign = false;
  m_alignhor = m_alignver = 0;
  m_fOnlyShowForcedSubs = false;
  m_fCustomPal = false;
  m_tridx = 0;
  memset(m_orgpal, 0, sizeof(m_orgpal));
  memset(m_cuspal, 0, sizeof(m_cuspal));
}

bool CVobSubSettings::GetCustomPal(RGBQUAD* cuspal, int& tridx)
{
  memcpy(cuspal, m_cuspal, sizeof(RGBQUAD)*4); 
  tridx = m_tridx;
  return(m_fCustomPal);
}

void CVobSubSettings::SetCustomPal(RGBQUAD* cuspal, int tridx) 
{
  memcpy(m_cuspal, cuspal, sizeof(RGBQUAD)*4);
  m_tridx = tridx & 0xf;
  for(int i = 0; i < 4; i++) m_cuspal[i].rgbReserved = (tridx&(1<<i)) ? 0 : 0xff;
  m_img.Invalidate();
}

void CVobSubSettings::GetDestrect(Com::SmartRect& r)
{
  int w = MulDiv(m_img.rect.Width(), m_scale_x, 100);
  int h = MulDiv(m_img.rect.Height(), m_scale_y, 100);

  if(!m_fAlign)
  {
    r.left = MulDiv(m_img.rect.left, m_scale_x, 100);
    r.right = MulDiv(m_img.rect.right, m_scale_x, 100);
    r.top = MulDiv(m_img.rect.top, m_scale_y, 100);
    r.bottom = MulDiv(m_img.rect.bottom, m_scale_y, 100);
  }
  else
  {
    switch(m_alignhor)
    {
    case 0:
      r.left = 0;
      r.right = w;
      break; // left
    case 1:
      r.left = -(w >> 1);
      r.right = -(w >> 1) + w;
      break; // center
    case 2:
      r.left = -w;
      r.right = 0;
      break; // right
    default:
      r.left = MulDiv(m_img.rect.left, m_scale_x, 100);
      r.right = MulDiv(m_img.rect.right, m_scale_x, 100);
      break;
    }
    
    switch(m_alignver)
    {
    case 0:
      r.top = 0;
      r.bottom = h;
      break; // top
    case 1:
      r.top = -(h >> 1);
      r.bottom = -(h >> 1) + h;
      break; // center
    case 2:
      r.top = -h;
      r.bottom = 0;
      break; // bottom
    default:
      r.top = MulDiv(m_img.rect.top, m_scale_y, 100);
      r.bottom = MulDiv(m_img.rect.bottom, m_scale_y, 100);
      break;
    }
  }

  r += m_org;
}

void CVobSubSettings::GetDestrect(Com::SmartRect& r, int w, int h)
{
  GetDestrect(r);

  r.left = MulDiv(r.left, w, m_size.cx);
  r.right = MulDiv(r.right, w, m_size.cx);
  r.top = MulDiv(r.top, h, m_size.cy);
  r.bottom = MulDiv(r.bottom, h, m_size.cy);
}

void CVobSubSettings::SetAlignment(bool fAlign, int x, int y, int hor, int ver)
{
  if(m_fAlign = fAlign)
  {
    m_org.x = MulDiv(m_size.cx, x, 100);
    m_org.y = MulDiv(m_size.cy, y, 100);
    m_alignhor = min(max(hor, 0), 2);
    m_alignver = min(max(ver, 0), 2);
  }
  else
  {
    m_org.x = m_x;
    m_org.y = m_y;
  }
}

#include "RTS.h"

HRESULT CVobSubSettings::Render(SubPicDesc& spd, RECT& bbox)
{
  Com::SmartRect r;
  GetDestrect(r, spd.w, spd.h);
  StretchBlt(spd, r, m_img);
/*
CRenderedTextSubtitle rts(NULL);
rts.CreateDefaultStyle(DEFAULT_CHARSET);
rts.m_dstScreenSize.SetSize(m_size.cx, m_size.cy);
CStdStringW assstr;
m_img.Polygonize(assstr, false);
REFERENCE_TIME rtStart = 10000i64*m_img.start, rtStop = 10000i64*(m_img.start+m_img.delay);
rts.Add(assstr, true, rtStart, rtStop);
rts.Render(spd, (rtStart+rtStop)/2, 25, r);
*/
  r &= Com::SmartRect(Com::SmartPoint(0, 0), Com::SmartSize(spd.w, spd.h));
  bbox = r;
  return !r.IsRectEmpty() ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////

static bool CompressFile(CStdString fn)
{
  if(GetVersion() < 0)
    return(false);

  BOOL b = FALSE;

  HANDLE h = CreateFile(fn, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
  if(h != INVALID_HANDLE_VALUE)
  {
    USHORT us = COMPRESSION_FORMAT_DEFAULT;
    DWORD nBytesReturned;
    b = DeviceIoControl(h, FSCTL_SET_COMPRESSION, (LPVOID)&us, 2, NULL, 0, (LPDWORD)&nBytesReturned, NULL);
    CloseHandle(h);
  }

  return(!!b);
}

bool CVobSubFile::SaveVobSub(CStdString fn)
{
  return WriteIdx(fn + _T(".idx")) && WriteSub(fn + _T(".sub"));
}

bool CVobSubFile::SaveWinSubMux(CStdString fn)
{
  TrimExtension(fn);

  ATL::CFile f;
  if(!f.Open(fn + _T(".sub"), ATL::CFile::modeCreate|ATL::CFile::modeWrite|ATL::CFile::typeText|ATL::CFile::shareDenyWrite))
    return(false);

  m_img.Invalidate();

  std::vector<BYTE> p4bpp(720*576/2);

  std::vector<SubPos>& sp = m_langs[m_iLang].subpos;
  for(int i = 0; i < sp.size(); i++)
  {
    if(!GetFrame(i)) continue;

    int pal[4] = {0, 1, 2, 3};

    for(int j = 0; j < 5; j++)
    {
      if(j == 4 || !m_img.pal[j].tr)
      {
        j &= 3;
        memset(&p4bpp[0], (j<<4)|j, 720*576/2);
        pal[j] ^= pal[0], pal[0] ^= pal[j], pal[j] ^= pal[0];
        break;
      }
    }

    int tr[4] = {m_img.pal[pal[0]].tr, m_img.pal[pal[1]].tr, m_img.pal[pal[2]].tr, m_img.pal[pal[3]].tr};

    DWORD uipal[4+12];

    if(!m_fCustomPal)
    {
      uipal[0] = *((DWORD*)&m_img.orgpal[m_img.pal[pal[0]].pal]);
      uipal[1] = *((DWORD*)&m_img.orgpal[m_img.pal[pal[1]].pal]);
      uipal[2] = *((DWORD*)&m_img.orgpal[m_img.pal[pal[2]].pal]);
      uipal[3] = *((DWORD*)&m_img.orgpal[m_img.pal[pal[3]].pal]);
    }
    else
    {
      uipal[0] = *((DWORD*)&m_img.cuspal[pal[0]]) & 0xffffff;
      uipal[1] = *((DWORD*)&m_img.cuspal[pal[1]]) & 0xffffff;
      uipal[2] = *((DWORD*)&m_img.cuspal[pal[2]]) & 0xffffff;
      uipal[3] = *((DWORD*)&m_img.cuspal[pal[3]]) & 0xffffff;
    }

    std::map<DWORD,BYTE> palmap;
    palmap[uipal[0]] = 0;
    palmap[uipal[1]] = 1;
    palmap[uipal[2]] = 2;
    palmap[uipal[3]] = 3;

    uipal[0] = 0xff; // blue background

    int w = m_img.rect.Width()-2;
    int h = m_img.rect.Height()-2;
    int pitch = (((w+1)>>1) + 3) & ~3;

    for(int y = 0; y < h; y++)
    {
      DWORD* p = (DWORD*)&m_img.lpPixels[(y+1)*(w+2)+1];

      for(int x = 0; x < w; x++, p++)
      {
        BYTE c = 0;

        if(*p & 0xff000000)
        {
          DWORD uic = *p & 0xffffff;
          c = palmap.find(uic)->second;
        }

        BYTE& c4bpp = p4bpp[(h-y-1)*pitch+(x>>1)];
        c4bpp = (x&1) ? ((c4bpp&0xf0)|c) : ((c4bpp&0x0f)|(c<<4));
      }
    }

    int t1 = m_img.start, t2 = t1 + m_img.delay /*+ (m_size.cy==480?(1000/29.97+1):(1000/25))*/;

    ASSERT(t2>t1);

    if(t2 <= 0) continue;
    if(t1 < 0) t1 = 0;

    CStdString bmpfn;
    bmpfn.Format(_T("%s_%06d.bmp"), fn, i+1);

    CStdString str;
    str.Format(_T("%s\t%02d:%02d:%02d:%02d %02d:%02d:%02d:%02d\t%03d %03d %03d %03d %d %d %d %d\n"), 
      bmpfn,
      t1/1000/60/60, (t1/1000/60)%60, (t1/1000)%60, (t1%1000)/10,
      t2/1000/60/60, (t2/1000/60)%60, (t2/1000)%60, (t2%1000)/10,
      m_img.rect.Width(), m_img.rect.Height(), m_img.rect.left, m_img.rect.top,
      (tr[0]<<4)|tr[0], (tr[1]<<4)|tr[1], (tr[2]<<4)|tr[2], (tr[3]<<4)|tr[3]);
    f.Write(str.c_str(), sizeof(str) * str.length());

    BITMAPFILEHEADER fhdr = 
    {
      0x4d42,
      sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD) + pitch*h,
      0, 0,
      sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD)
    };

    BITMAPINFOHEADER ihdr = 
    {
      sizeof(BITMAPINFOHEADER),
      w, h, 1, 4, 0,
      0,
      pitch*h, 0,
      16, 4
    };

    ATL::CFile bmp;
    if(bmp.Open(bmpfn, ATL::CFile::modeCreate|ATL::CFile::modeWrite|ATL::CFile::typeBinary|ATL::CFile::shareDenyWrite))
    {
      bmp.Write(&fhdr, sizeof(fhdr));
      bmp.Write(&ihdr, sizeof(ihdr));
      bmp.Write(uipal, sizeof(RGBQUAD)*16);
      bmp.Write(&p4bpp[0], pitch*h);
      bmp.Close();

      CompressFile(bmpfn);
    }
  }

  return(true);
}

bool CVobSubFile::SaveScenarist(CStdString fn)
{
  TrimExtension(fn);

  ATL::CFile f;
  if(!f.Open(fn + _T(".sst"), ATL::CFile::modeCreate|ATL::CFile::modeWrite|ATL::CFile::typeText|ATL::CFile::shareDenyWrite))
    return(false);

  m_img.Invalidate();

  fn.Replace('\\', '/');
  CStdString title = fn.Mid(fn.ReverseFind('/')+1);

  TCHAR buff[MAX_PATH], * pFilePart = buff;
  if(GetFullPathName(fn, MAX_PATH, buff, &pFilePart) == 0)
    return(false);

  CStdString fullpath = CStdString(buff).Left(pFilePart - buff);
  fullpath.TrimRight(_T("\\/"));
  if(fullpath.IsEmpty())
    return(false);

  CStdString str, str2;
  str += _T("st_format\t2\n");
  str += _T("Display_Start\t%s\n");
  str += _T("TV_Type\t\t%s\n");
  str += _T("Tape_Type\tNON_DROP\n");
  str += _T("Pixel_Area\t(0 %d)\n");
  str += _T("Directory\t%s\n");
  str += _T("Subtitle\t%s\n");
  str += _T("Display_Area\t(0 2 719 %d)\n");
  str += _T("Contrast\t(15 15 15 0)\n");
  str += _T("\n");
  str += _T("PA\t(0 0 255 - - - )\n");
  str += _T("E1\t(255 0 0 - - - )\n");
  str += _T("E2\t(0 0 0 - - - )\n");
  str += _T("BG\t(255 255 255 - - - )\n");
  str += _T("\n");
  str += _T("SP_NUMBER\tSTART\tEND\tFILE_NAME\n");
  str2.Format(str, 
    !m_fOnlyShowForcedSubs ? _T("non_forced") : _T("forced"),
    m_size.cy == 480 ? _T("NTSC") : _T("PAL"), 
    m_size.cy-3,
    fullpath,
    title, 
    m_size.cy == 480 ? 479 : 574);

  f.Write(str2.c_str(), sizeof(str2) * str2.length());

  f.Flush();

  RGBQUAD pal[16] = 
  {
    {255, 0, 0, 0},
    {0, 0, 255, 0},
    {0, 0, 0, 0},
    {255, 255, 255, 0},
    {0, 255, 0, 0},
    {255, 0, 255, 0},
    {0, 255, 255, 0},
    {125, 125, 0, 0},
    {125, 125, 125, 0},
    {225, 225, 225, 0},
    {0, 0, 125, 0},
    {0, 125, 0, 0},
    {125, 0, 0, 0},
    {255, 0, 222, 0},
    {0, 125, 222, 0},
    {125, 0, 125, 0},
  };

  BITMAPFILEHEADER fhdr = 
  {
    0x4d42,
    sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD) + 360*(m_size.cy-2),
    0, 0,
    sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD)
  };

  BITMAPINFOHEADER ihdr = 
  {
        sizeof(BITMAPINFOHEADER),
    720, m_size.cy-2, 1, 4, 0,
    360*(m_size.cy-2),
    0, 0,
    16, 4
  };

  bool fCustomPal = m_fCustomPal;
  m_fCustomPal = true;
  RGBQUAD tempCusPal[4], newCusPal[4+12] = {{255, 0, 0, 0}, {0, 0, 255, 0}, {0, 0, 0, 0}, {255, 255, 255, 0}};
  memcpy(tempCusPal, m_cuspal, sizeof(tempCusPal));
  memcpy(m_cuspal, newCusPal, sizeof(m_cuspal));

  std::vector<BYTE> p4bpp((m_size.cy-2)*360);

  BYTE colormap[16];

  for(int i = 0; i < 16; i++) 
  {
    int idx = 0, maxdif = 255*255*3+1;

    for(int j = 0; j < 16 && maxdif; j++)
    {
      int rdif = pal[j].rgbRed - m_orgpal[i].rgbRed;
      int gdif = pal[j].rgbGreen - m_orgpal[i].rgbGreen;
      int bdif = pal[j].rgbBlue - m_orgpal[i].rgbBlue;

      int dif = rdif*rdif + gdif*gdif + bdif*bdif;
      if(dif < maxdif) {maxdif = dif; idx = j;}
    }

    colormap[i] = idx+1;
  }

  int pc[4] = {1, 1, 1, 1}, pa[4] = {15, 15, 15, 0};

  std::vector<SubPos>& sp = m_langs[m_iLang].subpos;
  for(int i = 0, k = 0; i < sp.size(); i++)
  {
    if(!GetFrame(i)) continue;

    for(int j = 0; j < 5; j++)
    {
      if(j == 4 || !m_img.pal[j].tr)
      {
        j &= 3;
        memset(&p4bpp[0], (j<<4)|j, (m_size.cy-2)*360);
        break;
      }
    }

    for(int y = max(m_img.rect.top+1, 2); y < m_img.rect.bottom-1; y++)
    {
      ASSERT(m_size.cy-y-1 >= 0);
      if(m_size.cy-y-1 < 0) break;

      DWORD* p = (DWORD*)&m_img.lpPixels[(y-m_img.rect.top)*m_img.rect.Width()+1];

      for(int x = m_img.rect.left+1; x < m_img.rect.right-1; x++, p++)
      {
        DWORD rgb = *p&0xffffff;
        BYTE c = rgb == 0x0000ff ? 0 : rgb == 0xff0000 ? 1 : rgb == 0x000000 ? 2 : 3;
        BYTE& c4bpp = p4bpp[(m_size.cy-y-1)*360+(x>>1)];
        c4bpp = (x&1) ? ((c4bpp&0xf0)|c) : ((c4bpp&0x0f)|(c<<4));
      }
    }

    CStdString bmpfn;
    bmpfn.Format(_T("%s_%04d.bmp"), fn, i+1);
    title = bmpfn.Mid(bmpfn.ReverseFind('/')+1);

    // E1, E2, P, Bg
    int c[4] = {colormap[m_img.pal[1].pal], colormap[m_img.pal[2].pal], colormap[m_img.pal[0].pal], colormap[m_img.pal[3].pal]};
    c[0]^=c[1], c[1]^=c[0], c[0]^=c[1];

    if(memcmp(pc, c, sizeof(c)))
    {
      memcpy(pc, c, sizeof(c));
      str.Format(_T("Color\t (%d %d %d %d)\n"), c[0], c[1], c[2], c[3]);
      f.Write(str.c_str(), sizeof(str) * str.length());
    }

    // E1, E2, P, Bg
    int a[4] = {m_img.pal[1].tr, m_img.pal[2].tr, m_img.pal[0].tr, m_img.pal[3].tr};
    a[0]^=a[1], a[1]^=a[0], a[0]^=a[1];

    if(memcmp(pa, a, sizeof(a)))
    {
      memcpy(pa, a, sizeof(a));
      str.Format(_T("Contrast (%d %d %d %d)\n"), a[0], a[1], a[2], a[3]);
      f.Write(str.c_str(), sizeof(str) * str.length());
    }

    int t1 = sp[i].start;
    int h1 = t1/1000/60/60, m1 = (t1/1000/60)%60, s1 = (t1/1000)%60;
    int f1 = (int)((m_size.cy==480?29.97:25)*(t1%1000)/1000);

    int t2 = sp[i].stop;
    int h2 = t2/1000/60/60, m2 = (t2/1000/60)%60, s2 = (t2/1000)%60;
    int f2 = (int)((m_size.cy==480?29.97:25)*(t2%1000)/1000);

    if(t2 <= 0) continue;
    if(t1 < 0) t1 = 0;

    if(h1 == h2 && m1 == m2 && s1 == s2 && f1 == f2)
    {
      f2++;
      if(f2 == (m_size.cy==480?30:25)) {f2 = 0; s2++; if(s2 == 60) {s2 = 0; m2++; if(m2 == 60) {m2 = 0; h2++;}}}
    }

    if(i < sp.size()-1)
    {
      int t3 = sp[i+1].start;
      int h3 = t3/1000/60/60, m3 = (t3/1000/60)%60, s3 = (t3/1000)%60;
      int f3 = (int)((m_size.cy==480?29.97:25)*(t3%1000)/1000);

      if(h3 == h2 && m3 == m2 && s3 == s2 && f3 == f2) 
      {
        f2--;
        if(f2 == -1) {f2 = (m_size.cy==480?29:24); s2--; if(s2 == -1) {s2 = 59; m2--; if(m2 == -1) {m2 = 59; if(h2 > 0) h2--;}}}
      }
    }

    if(h1 == h2 && m1 == m2 && s1 == s2 && f1 == f2)
      continue;

    str.Format(_T("%04d\t%02d:%02d:%02d:%02d\t%02d:%02d:%02d:%02d\t%s\n"),
      ++k,
      h1, m1, s1, f1,
      h2, m2, s2, f2,
      title);
    f.Write(str.c_str(), sizeof(str) * str.length());

    ATL::CFile bmp;
    if(bmp.Open(bmpfn, ATL::CFile::modeCreate|ATL::CFile::modeWrite|ATL::CFile::modeRead|ATL::CFile::typeBinary))
    {
      bmp.Write(&fhdr, sizeof(fhdr));
      bmp.Write(&ihdr, sizeof(ihdr));
      bmp.Write(newCusPal, sizeof(RGBQUAD)*16);
      bmp.Write(&p4bpp[0], 360*(m_size.cy-2));
      bmp.Close();

      CompressFile(bmpfn);
    }
  }

  m_fCustomPal = fCustomPal;
  memcpy(m_cuspal, tempCusPal, sizeof(m_cuspal));

  return(true);
}

bool CVobSubFile::SaveMaestro(CStdString fn)
{
  TrimExtension(fn);

  ATL::CFile f;
  if(!f.Open(fn + _T(".son"), ATL::CFile::modeCreate|ATL::CFile::modeWrite|ATL::CFile::typeText|ATL::CFile::shareDenyWrite)) 
    return(false);

  m_img.Invalidate();

  fn.Replace('\\', '/');
  CStdString title = fn.Mid(fn.ReverseFind('/')+1);

  TCHAR buff[MAX_PATH], * pFilePart = buff;
  if(GetFullPathName(fn, MAX_PATH, buff, &pFilePart) == 0)
    return(false);

  CStdString fullpath = CStdString(buff).Left(pFilePart - buff);
  fullpath.TrimRight(_T("\\/"));
  if(fullpath.IsEmpty())
    return(false);

  CStdString str, str2;
  str += _T("st_format\t2\n");
  str += _T("Display_Start\t%s\n");
  str += _T("TV_Type\t\t%s\n");
  str += _T("Tape_Type\tNON_DROP\n");
  str += _T("Pixel_Area\t(0 %d)\n");
  str += _T("Directory\t%s\n");
  str += _T("Subtitle\t%s\n");
  str += _T("Display_Area\t(0 2 719 %d)\n");
  str += _T("Contrast\t(15 15 15 0)\n");
  str += _T("\n");
  str += _T("SP_NUMBER\tSTART\tEND\tFILE_NAME\n");
  str2.Format(str, 
    !m_fOnlyShowForcedSubs ? _T("non_forced") : _T("forced"),
    m_size.cy == 480 ? _T("NTSC") : _T("PAL"), 
    m_size.cy-3, 
    fullpath,
    title, 
    m_size.cy == 480 ? 479 : 574);

  f.Write(str2.c_str(), sizeof(str2) * str2.length());

  f.Flush();

  RGBQUAD pal[16] = 
  {
    {255, 0, 0, 0},
    {0, 0, 255, 0},
    {0, 0, 0, 0},
    {255, 255, 255, 0},
    {0, 255, 0, 0},
    {255, 0, 255, 0},
    {0, 255, 255, 0},
    {125, 125, 0, 0},
    {125, 125, 125, 0},
    {225, 225, 225, 0},
    {0, 0, 125, 0},
    {0, 125, 0, 0},
    {125, 0, 0, 0},
    {255, 0, 222, 0},
    {0, 125, 222, 0},
    {125, 0, 125, 0},
  };

  BITMAPFILEHEADER fhdr = 
  {
    0x4d42,
    sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD) + 360*(m_size.cy-2),
    0, 0,
    sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 16*sizeof(RGBQUAD)
  };

  BITMAPINFOHEADER ihdr = 
  {
        sizeof(BITMAPINFOHEADER),
    720, m_size.cy-2, 1, 4, 0,
    360*(m_size.cy-2),
    0, 0,
    16, 4
  };

  bool fCustomPal = m_fCustomPal;
  m_fCustomPal = true;
  RGBQUAD tempCusPal[4], newCusPal[4+12] = {{255, 0, 0, 0}, {0, 0, 255, 0}, {0, 0, 0, 0}, {255, 255, 255, 0}};
  memcpy(tempCusPal, m_cuspal, sizeof(tempCusPal));
  memcpy(m_cuspal, newCusPal, sizeof(m_cuspal));

  std::vector<BYTE> p4bpp((m_size.cy-2)*360);
  
  BYTE colormap[16];
  for(int i = 0; i < 16; i++)
    colormap[i] = i;

  ATL::CFile spf;
  if(spf.Open(fn + _T(".spf"), ATL::CFile::modeCreate|ATL::CFile::modeWrite|ATL::CFile::typeBinary))
  {
    for(int i = 0; i < 16; i++) 
    {
      COLORREF c = (m_orgpal[i].rgbBlue<<16) | (m_orgpal[i].rgbGreen<<8) | m_orgpal[i].rgbRed;
      spf.Write(&c, sizeof(COLORREF));
    }

    spf.Close();
  }

  int pc[4] = {1,1,1,1}, pa[4] = {15,15,15,0};

  std::vector<SubPos>& sp = m_langs[m_iLang].subpos;
  for(int i = 0, k = 0; i < sp.size(); i++)
  {
    if(!GetFrame(i)) continue;

    for(int j = 0; j < 5; j++)
    {
      if(j == 4 || !m_img.pal[j].tr)
      {
        j &= 3;
        memset(&p4bpp[0], (j<<4)|j, (m_size.cy-2)*360);
        break;
      }
    }

    for(int y = max(m_img.rect.top+1, 2); y < m_img.rect.bottom-1; y++)
    {
      ASSERT(m_size.cy-y-1 >= 0);
      if(m_size.cy-y-1 < 0) break;

      DWORD* p = (DWORD*)&m_img.lpPixels[(y-m_img.rect.top)*m_img.rect.Width()+1];

      for(int x = m_img.rect.left+1; x < m_img.rect.right-1; x++, p++)
      {
        DWORD rgb = *p&0xffffff;
        BYTE c = rgb == 0x0000ff ? 0 : rgb == 0xff0000 ? 1 : rgb == 0x000000 ? 2 : 3;
        BYTE& c4bpp = p4bpp[(m_size.cy-y-1)*360+(x>>1)];
        c4bpp = (x&1) ? ((c4bpp&0xf0)|c) : ((c4bpp&0x0f)|(c<<4));
      }
    }

    CStdString bmpfn;
    bmpfn.Format(_T("%s_%04d.bmp"), fn, i+1);
    title = bmpfn.Mid(bmpfn.ReverseFind('/')+1);

    // E1, E2, P, Bg
    int c[4] = {colormap[m_img.pal[1].pal], colormap[m_img.pal[2].pal], colormap[m_img.pal[0].pal], colormap[m_img.pal[3].pal]};

    if(memcmp(pc, c, sizeof(c)))
    {
      memcpy(pc, c, sizeof(c));
      str.Format(_T("Color\t (%d %d %d %d)\n"), c[0], c[1], c[2], c[3]);
      f.Write(str.c_str(), sizeof(str) * str.length());
    }

    // E1, E2, P, Bg
    int a[4] = {m_img.pal[1].tr, m_img.pal[2].tr, m_img.pal[0].tr, m_img.pal[3].tr};

    if(memcmp(pa, a, sizeof(a)))
    {
      memcpy(pa, a, sizeof(a));
      str.Format(_T("Contrast (%d %d %d %d)\n"), a[0], a[1], a[2], a[3]);
      f.Write(str.c_str(), sizeof(str) * str.length());
    }

    int t1 = sp[i].start;
    int h1 = t1/1000/60/60, m1 = (t1/1000/60)%60, s1 = (t1/1000)%60;
    int f1 = (int)((m_size.cy==480?29.97:25)*(t1%1000)/1000);

    int t2 = sp[i].stop;
    int h2 = t2/1000/60/60, m2 = (t2/1000/60)%60, s2 = (t2/1000)%60;
    int f2 = (int)((m_size.cy==480?29.97:25)*(t2%1000)/1000);

    if(t2 <= 0) continue;
    if(t1 < 0) t1 = 0;

    if(h1 == h2 && m1 == m2 && s1 == s2 && f1 == f2)
    {
      f2++;
      if(f2 == (m_size.cy==480?30:25)) {f2 = 0; s2++; if(s2 == 60) {s2 = 0; m2++; if(m2 == 60) {m2 = 0; h2++;}}}
    }

    if(i < sp.size()-1)
    {
      int t3 = sp[i+1].start;
      int h3 = t3/1000/60/60, m3 = (t3/1000/60)%60, s3 = (t3/1000)%60;
      int f3 = (int)((m_size.cy==480?29.97:25)*(t3%1000)/1000);

      if(h3 == h2 && m3 == m2 && s3 == s2 && f3 == f2)
      {
        f2--;
        if(f2 == -1) {f2 = (m_size.cy==480?29:24); s2--; if(s2 == -1) {s2 = 59; m2--; if(m2 == -1) {m2 = 59; if(h2 > 0) h2--;}}}
      }
    }

    if(h1 == h2 && m1 == m2 && s1 == s2 && f1 == f2)
      continue;

    str.Format(_T("%04d\t%02d:%02d:%02d:%02d\t%02d:%02d:%02d:%02d\t%s\n"),
      ++k,
      h1, m1, s1, f1,
      h2, m2, s2, f2,
      title);
    f.Write(str.c_str(), sizeof(str) * str.length());

    ATL::CFile bmp;
    if(bmp.Open(bmpfn, ATL::CFile::modeCreate|ATL::CFile::modeWrite|ATL::CFile::typeBinary))
    {
      bmp.Write(&fhdr, sizeof(fhdr));
      bmp.Write(&ihdr, sizeof(ihdr));
      bmp.Write(newCusPal, sizeof(RGBQUAD)*16);
      bmp.Write(&p4bpp[0], 360*(m_size.cy-2));
      bmp.Close();

      CompressFile(bmpfn);
    }
  }

  m_fCustomPal = fCustomPal;
  memcpy(m_cuspal, tempCusPal, sizeof(m_cuspal));

  return(true);
}

int CVobSubFile::GetNext( int pos )
{
  int i = pos;
  return(GetFrame(i) ? (i+1) : NULL);

}
//
// CVobSubStream
//

CVobSubStream::CVobSubStream(CCritSec* pLock)
  : ISubPicProviderImpl(pLock)
{
}

CVobSubStream::~CVobSubStream()
{
}

void CVobSubStream::Open(CStdString name, BYTE* pData, int len)
{
  CAutoLock cAutoLock(&m_csSubPics);

  m_name = name;

  std::list<CStdString> lines;
  Explode(CStdString(CStdStringA((CHAR*)pData, len)), lines, '\n');
  while(lines.size())
  {
    std::list<CStdString> sl;
    Explode(lines.front(), sl, ':', 2); lines.pop_front();
    if(sl.size() != 2) continue;
    CStdString key = sl.front();
    CStdString value = sl.back();
    if(key == _T("size"))
      _stscanf(value, _T("%dx%d"), &m_size.cx, &m_size.cy);
    else if(key == _T("org"))
      _stscanf(value, _T("%d, %d"), &m_org.x, &m_org.y);
    else if(key == _T("scale"))
      _stscanf(value, _T("%d%%, %d%%"), &m_scale_x, &m_scale_y);
    else if(key == _T("alpha"))
      _stscanf(value, _T("%d%%"), &m_alpha);
    else if(key == _T("smooth"))
      m_fSmooth = 
        value == _T("0") || value == _T("OFF") ? 0 : 
        value == _T("1") || value == _T("ON") ? 1 : 
        value == _T("2") || value == _T("OLD") ? 2 : 
        0;
    else if(key == _T("align"))
    {
      Explode(value, sl, ' ');
      if(sl.size() == 4) sl.erase(++sl.begin());
      if(sl.size() == 3)
      {
        m_fAlign = sl.front() == _T("ON"); sl.pop_front();
        CStdString hor = sl.front(), ver = sl.back();
        m_alignhor = hor == _T("LEFT") ? 0 : hor == _T("CENTER") ? 1 : hor == _T("RIGHT") ? 2 : 1;
        m_alignver = ver == _T("TOP") ? 0 : ver == _T("CENTER") ? 1 : ver == _T("BOTTOM") ? 2 : 2;
      }
    }
    else if (key == _T("fade in/out") || key == _T("fadein/out"))
      _stscanf(value, _T("%d%, %d%"), &m_fadein, &m_fadeout);
    else if(key == _T("time offset"))
      m_toff = _tcstol(value, NULL, 10);
    else if(key == _T("forced subs"))
      m_fOnlyShowForcedSubs = value == _T("1") || value == _T("ON");
    else if(key == _T("palette"))
    {
      Explode(value, sl, ',', 16);
      for(int i = 0; i < 16 && sl.size(); i++)
      {
        *(DWORD*)&m_orgpal[i] = _tcstol(sl.front(), NULL, 16);
        sl.pop_front();
      }
    }
    else if(key == _T("custom colors"))
    {
      m_fCustomPal = Explode(value, sl, ',', 3) == _T("ON");
      if(sl.size() == 3)
      {
        sl.pop_front();
        std::list<CStdString> tridx, colors;
        Explode(sl.front(), tridx, ':', 2); sl.pop_front();
        if(tridx.front() == _T("tridx"))
        {
          tridx.pop_front();
          TCHAR tr[4];
          _stscanf(tridx.front(), _T("%c%c%c%c"), &tr[0], &tr[1], &tr[2], &tr[3]); tridx.pop_front();
          for(int i = 0; i < 4; i++)
            m_tridx |= ((tr[i]=='1')?1:0)<<i;
        }
        Explode(sl.front(), colors, ':', 2); sl.pop_front();
        if(colors.front() == _T("colors"))
        {
          colors.pop_front();
          Explode(colors.front(), colors, ',', 4); colors.pop_front();
          for(int i = 0; i < 4 && colors.size(); i++)
          {
            *(DWORD*)&m_cuspal[i] = _tcstol(colors.front(), NULL, 16);
            colors.pop_front();
          }
        }
      }
    }
  }
}

void CVobSubStream::Add(REFERENCE_TIME tStart, REFERENCE_TIME tStop, BYTE* pData, int len)
{
  if(len <= 4 || ((pData[0]<<8)|pData[1]) != len) return;

  CVobSubImage vsi;
  vsi.GetPacketInfo(pData, (pData[0]<<8)|pData[1], (pData[2]<<8)|pData[3]);

  boost::shared_ptr<SubPic> p(DNew SubPic());
  p->tStart = tStart;
  p->tStop = vsi.delay > 0 ? (tStart + 10000i64*vsi.delay) : tStop;
  p->pData.resize(len);
  memcpy(&p->pData[0], pData, p->pData.size());

  CAutoLock cAutoLock(&m_csSubPics);
  while(m_subpics.size() && m_subpics.back().get()->tStart >= tStart)
  {
    m_subpics.pop_back();
    m_img.iIdx = -1;
  }
  m_subpics.push_back(p);
}

void CVobSubStream::RemoveAll()
{
  CAutoLock cAutoLock(&m_csSubPics);
  m_subpics.clear();
  m_img.iIdx = -1;
}

STDMETHODIMP CVobSubStream::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);
    *ppv = NULL;

    return 
    QI(IPersist)
    QI(ISubStream)
    QI(ISubPicProvider)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider
  
STDMETHODIMP_(__w64 int) CVobSubStream::GetStartPosition(REFERENCE_TIME rt, double fps)
{
  CAutoLock cAutoLock(&m_csSubPics);
  int idx = m_subpics.size();
  std::vector<boost::shared_ptr<SubPic>>::reverse_iterator it = m_subpics.rbegin();
  for(; it != m_subpics.rend(); ++it, --idx)
  {
    SubPic* sp = it->get();
    if(sp->tStart <= rt)
    {
      if(sp->tStop <= rt)
        idx++;
      break;
    }
  }

  // Must return 0 if no match
  // idx starts at one
  if (idx > m_subpics.size())
    return 0;
  else
    return idx;
}

STDMETHODIMP_(int) CVobSubStream::GetNext(int pos)
{
  return ((pos >=  m_subpics.size()) ? NULL : ++pos);
}

STDMETHODIMP_(REFERENCE_TIME) CVobSubStream::GetStart(int pos, double fps)
{
  CAutoLock cAutoLock(&m_csSubPics);
  return m_subpics[pos - 1]->tStart;
}

STDMETHODIMP_(REFERENCE_TIME) CVobSubStream::GetStop(int pos, double fps)
{
  CAutoLock cAutoLock(&m_csSubPics);
  return m_subpics[pos - 1]->tStop;
}

STDMETHODIMP_(bool) CVobSubStream::IsAnimated(int pos)
{
  return(false);
}

STDMETHODIMP CVobSubStream::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
  if(spd.bpp != 32) return E_INVALIDARG;

  std::vector<boost::shared_ptr<SubPic>>::reverse_iterator it = m_subpics.rbegin();
  for(; it != m_subpics.rend(); ++it)
  {
    SubPic* sp = it->get();
    if(sp->tStart <= rt && rt < sp->tStop)
    {
      if(m_img.iIdx != std::distance(it, m_subpics.rend()))
      {
        BYTE* pData = &sp->pData[0];
        m_img.Decode(
          pData, (pData[0] << 8) | pData[1], (pData[2] << 8) | pData[3],
          m_fCustomPal, m_tridx, m_orgpal, m_cuspal, true);
        m_img.iIdx = std::distance(it, m_subpics.rend());
      }

      return __super::Render(spd, bbox);
    }
  }

  return E_FAIL;
}

// IPersist

STDMETHODIMP CVobSubStream::GetClassID(CLSID* pClassID)
{
  return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CVobSubStream::GetStreamCount()
{
  return 1;
}

STDMETHODIMP CVobSubStream::GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID)
{
  CAutoLock cAutoLock(&m_csSubPics);

  if(ppName)
  {
    if(!(*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength()+1)*sizeof(WCHAR))))
      return E_OUTOFMEMORY;
    wcscpy(*ppName, CStdStringW(m_name));
  }

  if(pLCID)
  {
    *pLCID = 0; // TODO
  }

  return S_OK;
}

STDMETHODIMP_(int) CVobSubStream::GetStream()
{
  return 0;
}

STDMETHODIMP CVobSubStream::SetStream(int iStream)
{
  return iStream == 0 ? S_OK : E_FAIL;
}
