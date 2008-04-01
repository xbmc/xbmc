#include "stdafx.h"
#include "Fanart.h"
#include "HTTP.h"

#ifdef RESAMPLE_CACHED_IMAGES
#include "Picture.h"
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CFanart Functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFanart::CFanart()
{
}

void CFanart::Pack()
{
  // Take our data and pack it into the m_xml string
  m_xml.Empty();
  TiXmlElement fanart("fanart");
  fanart.SetAttribute("url", m_url.c_str());
  for (std::vector<SFanartData>::const_iterator it = m_fanart.begin(); it != m_fanart.end(); ++it)
  {
    TiXmlElement thumb("thumb");
    thumb.SetAttribute("dim", it->strResolution.c_str());
    thumb.SetAttribute("colors", it->strColors.c_str());
    TiXmlText text(it->strImage);
    thumb.InsertEndChild(text);
    fanart.InsertEndChild(thumb);
  }
  m_xml << fanart;
}

bool CFanart::Unpack()
{
  TiXmlDocument doc;
  doc.Parse(m_xml.c_str());

  m_fanart.clear();
  m_url.Empty();

  TiXmlElement *fanart = doc.FirstChildElement("fanart");
  if (fanart)
  {
    m_url = fanart->Attribute("url");
    TiXmlElement *fanartThumb = fanart->FirstChildElement("thumb");
	  while (fanartThumb)
	  {
      SFanartData data;
      data.strImage = fanartThumb->GetText();
      data.strThumb.Format("_cache/%s", data.strImage.c_str());
      data.strResolution = fanartThumb->Attribute("dim");
      ParseColors(fanartThumb->Attribute("colors"), data.strColors);
      m_fanart.push_back(data);
      fanartThumb = fanartThumb->NextSiblingElement("thumb");
	  }
  }
  return true;
}

const CStdString CFanart::GetImageURL() const
{
  if (m_fanart.size() == 0)
    return "";

  CStdString result;
  result.Format("%s%s", m_url.c_str(), m_fanart[0].strImage.c_str());
  return result;
}

const CStdString CFanart::GetThumbURL() const
{
  if (m_fanart.size() == 0)
    return "";

  CStdString result;
  result.Format("%s%s", m_url.c_str(), m_fanart[0].strThumb.c_str());
  return result;
}

const CStdString CFanart::GetColor(unsigned int index) const
{
  if (index >= max_fanart_colors || m_fanart.size() == 0)
    return "FFFFFFFF";

  // format is AARRGGBB,AARRGGBB etc.
  return m_fanart[0].strColors.Mid(index*9, 8);
}

bool CFanart::SetPrimaryFanart(unsigned int index)
{
  if (index >= m_fanart.size())
    return false;

  std::iter_swap(m_fanart.begin()+index, m_fanart.begin());

  // repack our data
  Pack();

  return true;
}

bool CFanart::DownloadThumb(unsigned int index, const CStdString &strDestination) const
{
  if (index >= m_fanart.size())
    return false;

  CHTTP http;
  CStdString thumbURL;
  thumbURL.Format("%s%s", m_url.c_str(), m_fanart[index].strThumb.c_str());
  return (http.Download(thumbURL, strDestination));
}

bool CFanart::DownloadImage(const CStdString &strDestination) const
{
  if (m_fanart.size() == 0)
    return false;

  // Ideally we'd just call CPicture::CacheImage() directly, but for some
  // reason curl doesn't seem to like downloading these for us
  CHTTP http;
#ifdef RESAMPLE_CACHED_IMAGES
  CStdString tempFile = "Z:\\fanart_download.jpg";
  if (http.Download(GetImageURL(), tempFile))
  { 
    CPicture pic;
    pic.CacheImage(tempFile, strDestination);
    XFILE::CFile::Delete(tempFile);
    return true;
  }
  return false;
#else
  return http.Download(GetImageURL(), strDestination);
#endif
}

unsigned int CFanart::GetNumFanarts()
{
  return m_fanart.size();
}

bool CFanart::ParseColors(const CStdString &colorsIn, CStdString &colorsOut)
{
  // Formats: 
  // 0: XBMC ARGB Hexadecimal string comma seperated "FFFFFFFF,DDDDDDDD,AAAAAAAA"
  // 1: The TVDB RGB Int Triplets, pipe seperate with leading/trailing pipes "|68,69,59|69,70,58|78,78,68|"

  // Essentially we read the colors in using the proper format, and store them in our own fixed temporary format (3 DWORDS), and then 
  // write them back in in the specified format.

  if (colorsIn.IsEmpty())
    return false;

  // check for the TVDB RGB triplets "|68,69,59|69,70,58|78,78,68|"
  if (colorsIn[0] == '|')
  { // need conversion
    colorsOut.Empty();
    CStdStringArray strColors;
    StringUtils::SplitString(colorsIn, "|", strColors);
    for (unsigned int i = 0; i < min(strColors.size()-1, max_fanart_colors); i++)
    { // split up each color
      CStdStringArray strTriplets;
      StringUtils::SplitString(strColors[i+1], ",", strTriplets);
      if (strTriplets.size() == 3)
      { // convert
        if (colorsOut.size())
          colorsOut += ",";
        colorsOut.AppendFormat("FF%2x%2x%2x", atol(strTriplets[0].c_str()), atol(strTriplets[1].c_str()), atol(strTriplets[2].c_str()));
      }
    }
  }
  else
  { // assume is our format
    colorsOut = colorsIn;
  }
  return true;
}
