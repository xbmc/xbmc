/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <algorithm>
#include <functional>

#include "Fanart.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
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
  m_xml.clear();
  TiXmlElement fanart("fanart");
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
  doc.Parse(m_xml);

  m_fanart.clear();

  TiXmlElement *fanart = doc.FirstChildElement("fanart");
  while (fanart)
  {
    std::string url = XMLUtils::GetAttribute(fanart, "url");
    TiXmlElement *fanartThumb = fanart->FirstChildElement("thumb");
    while (fanartThumb)
    {
      if (!fanartThumb->NoChildren())
      {
        SFanartData data;
        if (url.empty())
        {
          data.strImage = fanartThumb->FirstChild()->ValueStr();
          data.strPreview = XMLUtils::GetAttribute(fanartThumb, "preview");
        }
        else
        {
          data.strImage = URIUtils::AddFileToFolder(url, fanartThumb->FirstChild()->ValueStr());
          if (fanartThumb->Attribute("preview"))
            data.strPreview = URIUtils::AddFileToFolder(url, fanartThumb->Attribute("preview"));
        }
        data.strResolution = XMLUtils::GetAttribute(fanartThumb, "dim");
        ParseColors(XMLUtils::GetAttribute(fanartThumb, "colors"), data.strColors);
        m_fanart.push_back(data);
      }
      fanartThumb = fanartThumb->NextSiblingElement("thumb");
    }
    fanart = fanart->NextSiblingElement("fanart");
  }
  return true;
}

std::string CFanart::GetImageURL(unsigned int index) const
{
  if (index >= m_fanart.size())
    return "";

  return m_fanart[index].strImage;
}

std::string CFanart::GetPreviewURL(unsigned int index) const
{
  if (index >= m_fanart.size())
    return "";

  return m_fanart[index].strPreview.empty() ? m_fanart[index].strImage : m_fanart[index].strPreview;
}

const std::string CFanart::GetColor(unsigned int index) const
{
  if (index >= max_fanart_colors || m_fanart.size() == 0 ||
      m_fanart[0].strColors.size() < index*9+8)
    return "FFFFFFFF";

  // format is AARRGGBB,AARRGGBB etc.
  return m_fanart[0].strColors.substr(index*9, 8);
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

unsigned int CFanart::GetNumFanarts() const
{
  return m_fanart.size();
}

bool CFanart::ParseColors(const std::string &colorsIn, std::string &colorsOut)
{
  // Formats:
  // 0: XBMC ARGB Hexadecimal string comma seperated "FFFFFFFF,DDDDDDDD,AAAAAAAA"
  // 1: The TVDB RGB Int Triplets, pipe seperate with leading/trailing pipes "|68,69,59|69,70,58|78,78,68|"

  // Essentially we read the colors in using the proper format, and store them in our own fixed temporary format (3 DWORDS), and then
  // write them back in in the specified format.

  if (colorsIn.empty())
    return false;

  // check for the TVDB RGB triplets "|68,69,59|69,70,58|78,78,68|"
  if (colorsIn[0] == '|')
  { // need conversion
    colorsOut.clear();
    std::vector<std::string> strColors = StringUtils::Split(colorsIn, "|");
    for (int i = 0; i < std::min((int)strColors.size()-1, (int)max_fanart_colors); i++)
    { // split up each color
      std::vector<std::string> strTriplets = StringUtils::Split(strColors[i+1], ",");
      if (strTriplets.size() == 3)
      { // convert
        if (colorsOut.size())
          colorsOut += ",";
        colorsOut += StringUtils::Format("FF%2lx%2lx%2lx", atol(strTriplets[0].c_str()), atol(strTriplets[1].c_str()), atol(strTriplets[2].c_str()));
      }
    }
  }
  else
  { // assume is our format
    colorsOut = colorsIn;
  }
  return true;
}
