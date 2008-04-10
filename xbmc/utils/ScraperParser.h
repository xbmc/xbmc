#ifndef SCRAPER_PARSER_H
#define SCRAPER_PARSER_H

#include "tinyXML/tinyxml.h"
#include "StdString.h"

#include <vector>

#define MAX_SCRAPER_BUFFERS 20

class CHTTP;
class CScraperSettings;

class CScraperParser
{
public:
  CScraperParser();
  ~CScraperParser();

  bool Load(const CStdString& strXMLFile);
  const CStdString GetName() { return m_name; }
  const CStdString GetContent() { return m_content; }
  const CStdString Parse(const CStdString& strTag, CScraperSettings* pSettings=NULL);
  bool HasFunction(const CStdString& strTag);

  CStdString m_param[MAX_SCRAPER_BUFFERS];
  static void ClearCache();

private:
  void ReplaceBuffers(CStdString& strDest);
  void ParseExpression(const CStdString& input, CStdString& dest, TiXmlElement* element, bool bAppend);
  void ParseNext(TiXmlElement* element);
  void Clean(CStdString& strDirty);
  char* RemoveWhiteSpace(const char *string);
  void ClearBuffers();

  TiXmlDocument* m_document;
  TiXmlElement* m_pRootElement;

  const char* m_name;
  const char* m_content;
  CScraperSettings* m_settings;
};

#endif


