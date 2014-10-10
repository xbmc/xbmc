#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <string>
#include <libxslt/xslt.h>
#include <libxslt/xsltutils.h>

class XSLTUtils
{
public:
  XSLTUtils();
  ~XSLTUtils();

  /*! \brief Set the input XML for an XSLT transform from a string.
   This sets up the XSLT transformer with some input XML from a string in memory.
   The input XML should be well formed.
   \param input    the XML document to be transformed.
   */
  bool  SetInput(const std::string& input);

  /*! \brief Set the stylesheet (XSL) for an XSLT transform from a string.
   This sets up the XSLT transformer with some stylesheet XML from a string in memory.
   The input XSL should be well formed.
   \param input    the XSL document to be transformed.
   */
  bool  SetStylesheet(const std::string& stylesheet);

  /*! \brief Perform an XSLT transform on an inbound XML document.
   This will apply an XSLT transformation on an input XML document,
   giving an output XML document, using the specified XSLT document
   as the transformer.
   \param input    the parent containing the <tag>'s.
   \param filename         the <tag> in question.
   */
  bool  XSLTTransform(std::string& output);


private:
  xmlDocPtr m_xmlInput;
  xmlDocPtr m_xmlOutput;
  xmlDocPtr m_xmlStylesheet;
  xsltStylesheetPtr m_xsltStylesheet;
};
