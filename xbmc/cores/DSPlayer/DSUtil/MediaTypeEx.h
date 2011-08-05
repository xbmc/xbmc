#pragma once

#include "streams.h"
#include "utils/StdString.h"
#include <list>
#include <map>

class CMediaTypeEx : public CMediaType
{
public:
  CMediaTypeEx();
  CMediaTypeEx(const CMediaType& mt) {CMediaType::operator = (mt);}

  CStdStringW ToString(IPin* pPin = NULL);

  static CStdStringA GetVideoCodecName(const GUID& subtype, DWORD biCompression, DWORD *fourcc = NULL);
  static CStdStringA GetAudioCodecName(const GUID& subtype, WORD wFormatTag);
  static CStdStringA GetSubtitleCodecName(const GUID& subtype);

  //void Dump(std::list<CStdString>& sl);
};
