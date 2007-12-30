#ifndef SCRAPER_PARSER_H
#define SCRAPER_PARSER_H

#include "../../guilib/tinyxml/tinyxml.h"
#include "../../guilib/stdstring.h"

#include <vector>

class CHTTP;

class CScraperUrl
{
public:
  CScraperUrl(const CStdString&);
  CScraperUrl(const TiXmlElement*);
  CScraperUrl();
  ~CScraperUrl();

  enum URLTYPES
  {
    URL_TYPE_GENERAL = 1,
    URL_TYPE_SEASON = 2
  };

  struct SUrlEntry 
  {
    CStdString m_spoof;
    CStdString m_url;
    URLTYPES m_type;
    bool m_post;
    int m_season;
  };

  bool Parse();
  bool ParseString(CStdString); // copies by intention
  bool ParseElement(const TiXmlElement*);
  bool ParseEpisodeGuide(CStdString strUrls); // copies by intention

  const SUrlEntry GetFirstThumb() const;
  const SUrlEntry GetSeasonThumb(int) const;
  void Clear();
  static bool Get(const SUrlEntry&, string&, CHTTP& http);
  static bool DownloadThumbnail(const CStdString &thumb, const SUrlEntry& entry);

  CStdString m_xml;
  CStdString m_spoof; // for backwards compatibility only!
  CStdString strTitle;
  CStdString strId;
  std::vector<SUrlEntry> m_url;
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
  bool HasFunction(const CStdString& strTag);

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
