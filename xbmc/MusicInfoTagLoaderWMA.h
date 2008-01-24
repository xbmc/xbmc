#pragma once

#include "ImusicInfoTagLoader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderWMA: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderWMA(void);
  virtual ~CMusicInfoTagLoaderWMA();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);

protected:
  void SetTagValueString(const CStdString& strFrameName, const CStdString& strValue, CMusicInfoTag& tag);
  void SetTagValueDWORD(const CStdString& strFrameName, DWORD dwValue, CMusicInfoTag& tag);
  void SetTagValueBinary(const CStdString& strFrameName, const LPBYTE pValue, CMusicInfoTag& tag);
  void SetTagValueBool(const CStdString& strFrameName, BOOL bValue, CMusicInfoTag& tag);
};
}
