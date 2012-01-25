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
#ifndef VOBSUBFILE_H
#define VOBSUBFILE_H
#pragma once

#include "VobSubImage.h"
#include "..\SubPic\ISubPic.h"
#include <sstream>

#define VOBSUBIDXVER 7

extern CStdString FindLangFromId(WORD id);

class CVobSubSettings
{
protected:
  HRESULT Render(SubPicDesc& spd, RECT& bbox);

public:
  Com::SmartSize m_size;
  int m_x, m_y;
  Com::SmartPoint m_org;
  int m_scale_x, m_scale_y;  // % (don't set it to unsigned because as a side effect it will mess up negative coordinates in GetDestrect())
  int m_alpha;        // %
  int m_fSmooth;        // 0: OFF, 1: ON, 2: OLD (means no filtering at all)
  int m_fadein, m_fadeout;  // ms
  bool m_fAlign;
  int m_alignhor, m_alignver; // 0: left/top, 1: center, 2: right/bottom
  unsigned int m_toff;        // ms
  bool m_fOnlyShowForcedSubs;
  bool m_fCustomPal;
  int m_tridx;
  RGBQUAD m_orgpal[16], m_cuspal[4];

  CVobSubImage m_img;

  CVobSubSettings() {InitSettings();}
  void InitSettings();

  bool GetCustomPal(RGBQUAD* cuspal, int& tridx);
    void SetCustomPal(RGBQUAD* cuspal, int tridx);

  void GetDestrect(Com::SmartRect& r); // destrect of m_img, considering the current alignment mode
  void GetDestrect(Com::SmartRect& r, int w, int h); // this will scale it to the frame size of (w, h)

  void SetAlignment(bool fAlign, int x, int y, int hor, int ver);
};

[uuid("998D4C9A-460F-4de6-BDCD-35AB24F94ADF")]
class CVobSubFile : public CVobSubSettings, public ISubStream, public ISubPicProviderImpl
{
protected:
  CStdString m_title;

  void TrimExtension(CStdString& fn);
  bool ReadIdx(CStdString fn, int& ver), ReadSub(CStdString fn), ReadRar(CStdString fn), ReadIfo(CStdString fn);
  bool WriteIdx(CStdString fn), WriteSub(CStdString fn);

  std::stringstream m_sub;

  BYTE* GetPacket(int idx, int& packetsize, int& datasize, int iLang = -1);
  bool GetFrame(int idx, int iLang = -1);
  bool GetFrameByTimeStamp(__int64 time);
  int GetFrameIdxByTimeStamp(__int64 time);

  bool SaveVobSub(CStdString fn);
  bool SaveWinSubMux(CStdString fn);
  bool SaveScenarist(CStdString fn);
  bool SaveMaestro(CStdString fn);

public:
  typedef struct
  {
    __int64 filepos;
    __int64 start, stop;
    bool fForced;
    char vobid, cellid;
    __int64 celltimestamp;
    bool fValid;
  } SubPos;

  typedef struct
  {
    int id;
    CStdString name, alt;
    std::vector<SubPos> subpos;
  } SubLang;

  int m_iLang;
  SubLang m_langs[32];

public:
  CVobSubFile(CCritSec* pLock);
  virtual ~CVobSubFile();

  bool Copy(CVobSubFile& vsf);

  typedef enum {None,VobSub,WinSubMux,Scenarist,Maestro} SubFormat;

  bool Open(CStdString fn);
  bool Save(CStdString fn, SubFormat sf = VobSub);
  void Close();

  CStdString GetTitle() {return(m_title);}
  CStdString GetLanguage() {
    int i = 0;
    if(m_iLang >= 0 && m_iLang < 32)
      i = m_iLang;

    return FindLangFromId(m_langs[i].id);
  }

  DECLARE_IUNKNOWN
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // ISubPicProvider
  STDMETHODIMP_(__w64 int) GetStartPosition(REFERENCE_TIME rt, double fps);
  STDMETHODIMP_(int) GetNext(int pos);
  STDMETHODIMP_(REFERENCE_TIME) GetStart(int pos, double fps);
  STDMETHODIMP_(REFERENCE_TIME) GetStop(int pos, double fps);
  STDMETHODIMP_(bool) IsAnimated(int pos);
  STDMETHODIMP Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox);

  // IPersist
  STDMETHODIMP GetClassID(CLSID* pClassID);

  // ISubStream
  STDMETHODIMP_(int) GetStreamCount();
  STDMETHODIMP GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID);
  STDMETHODIMP_(int) GetStream();
  STDMETHODIMP SetStream(int iStream);
  STDMETHODIMP Reload();
};

[uuid("D7FBFB45-2D13-494F-9B3D-FFC9557D5C45")]
class CVobSubStream : public CVobSubSettings, public ISubStream, public ISubPicProviderImpl
{
  CStdString m_name;

  CCritSec m_csSubPics;
  struct SubPic {REFERENCE_TIME tStart, tStop; std::vector<BYTE> pData;};
  std::vector<boost::shared_ptr<SubPic>> m_subpics;

public:
  CVobSubStream(CCritSec* pLock);
  virtual ~CVobSubStream();

  void Open(CStdString name, BYTE* pData, int len);

  void Add(REFERENCE_TIME tStart, REFERENCE_TIME tStop, BYTE* pData, int len);
  void RemoveAll();

  DECLARE_IUNKNOWN
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // ISubPicProvider
  STDMETHODIMP_(__w64 int) GetStartPosition(REFERENCE_TIME rt, double fps);
  STDMETHODIMP_(int) GetNext(int pos);
  STDMETHODIMP_(REFERENCE_TIME) GetStart(int pos, double fps);
  STDMETHODIMP_(REFERENCE_TIME) GetStop(int pos, double fps);
  STDMETHODIMP_(bool) IsAnimated(int pos);
  STDMETHODIMP Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox);

  // IPersist
  STDMETHODIMP GetClassID(CLSID* pClassID);

  // ISubStream
  STDMETHODIMP_(int) GetStreamCount();
  STDMETHODIMP GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID);
  STDMETHODIMP_(int) GetStream();
  STDMETHODIMP SetStream(int iStream);
  STDMETHODIMP Reload() {return E_NOTIMPL;}
};
#endif