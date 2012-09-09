/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Fanart.h"
#include "utils/XBMCTinyXML.h"
#include "URIUtils.h"
#include "StringUtils.h"

const unsigned int CFanart::max_fanart_colors=3;


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
    thumb.SetAttribute("preview", it->strPreview.c_str());
    TiXmlText text(it->strImage);
    thumb.InsertEndChild(text);
    fanart.InsertEndChild(thumb);
  }
  m_xml << fanart;
}

bool CFanart::Unpack()
{
  CXBMCTinyXML doc;
  doc.Parse(m_xml.c_str());

  m_fanart.clear();
  m_url.Empty();

  TiXmlElement *fanart = doc.FirstChildElement("fanart");
  while (fanart)
  {
    m_url = fanart->Attribute("url");
    TiXmlElement *fanartThumb = fanart->FirstChildElement("thumb");
    while (fanartThumb)
    {
      SFanartData data;
      data.strImage = fanartThumb->GetText();
      data.strResolution = fanartThumb->Attribute("dim");
      data.strPreview = fanartThumb->Attribute("preview");
      if (data.strPreview.IsEmpty())
      { // could be due to an old version in db - use old hardcoded method for now
        if (m_url.Equals("http://thetvdb.com/banners/"))
          data.strPreview = "_cache/" + data.strImage;
      }
      ParseColors(fanartThumb->Attribute("colors"), data.strColors);
      m_fanart.push_back(data);
      fanartThumb = fanartThumb->NextSiblingElement("thumb");
    }
    fanart = fanart->NextSiblingElement("fanart");
  }
  return true;
}

CStdString CFanart::GetImageURL(unsigned int index) const
{
  if (index >= m_fanart.size())
    return "";
  
  if (m_url.IsEmpty())
    return m_fanart[index].strImage;
  return URIUtils::AddFileToFolder(m_url, m_fanart[index].strImage);
}

CStdString CFanart::GetPreviewURL(unsigned int index) const
{
  if (index >= m_fanart.size())
    return "";
  
  CStdString thumb = !m_fanart[index].strPreview.IsEmpty() ? m_fanart[index].strPreview : m_fanart[index].strImage;
  if (m_url.IsEmpty())
    return thumb;
  return URIUtils::AddFileToFolder(m_url, thumb);
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
    for (int i = 0; i < std::min((int)strColors.size()-1, (int)max_fanart_colors); i++)
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
