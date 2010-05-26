#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "streams.h"
#include "StdString.h"
#include <list>

class CMediaTypeEx : public CMediaType
{
public:
  CMediaTypeEx();
  CMediaTypeEx(const CMediaType& mt) {CMediaType::operator = (mt);}

  CStdString ToString(IPin* pPin = NULL);

  static CStdString GetVideoCodecName(const GUID& subtype, DWORD biCompression, DWORD *fourcc = NULL);
  static CStdString GetAudioCodecName(const GUID& subtype, WORD wFormatTag);
  static CStdString GetSubtitleCodecName(const GUID& subtype);

  //void Dump(std::list<CStdString>& sl);
};
