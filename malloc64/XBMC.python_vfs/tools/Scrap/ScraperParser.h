#ifndef SCRAPER_PARSER_H
#define SCRAPER_PARSER_H

#include "tinyxml/tinyxml.h"
#include "stdstring.h"

class CScraperUrl
{
public:
  CScraperUrl(const CStdString&);
  CScraperUrl(const TiXmlElement*);
  CScraperUrl();
	~CScraperUrl();
  void ParseString(CStdString);
  void ParseElement(const TiXmlElement*);
  CStdString m_spoof;
  CStdString m_url;
  bool m_post;
};
class CScraperParser
{
public:
	CScraperParser();
	~CScraperParser();

	bool Load(const CStdString& strXMLFile);
  const CStdString GetName() { return m_name; }
	const CStdString GetContent() { return m_content; }
	const CStdString Parse(const CStdString& strTag);

	CStdString m_param[9];
  static char* ConvertHTMLToAnsi(const char *szHTML);

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
};

#endif