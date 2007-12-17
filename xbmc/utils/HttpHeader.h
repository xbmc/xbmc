#pragma once

#include <vector>
#include "StdString.h"

#define HTTPHEADER_CONTENT_TYPE "Content-Type"

typedef std::map<CStdString, CStdString> HeaderParams;
typedef std::map<CStdString, CStdString>::iterator HeaderParamsIter;

class CHttpHeader
{
public:
  CHttpHeader();
  ~CHttpHeader();

  void Parse(CStdString strData);
  CStdString GetValue(CStdString strParam);
  
  void GetHeader(CStdString& strHeader);
  
  CStdString GetContentType() { return GetValue(HTTPHEADER_CONTENT_TYPE); }
  CStdString GetProtoLine() { return m_protoLine; }

  void Clear();
  
protected:
  HeaderParams m_params;
  CStdString   m_protoLine;
};

