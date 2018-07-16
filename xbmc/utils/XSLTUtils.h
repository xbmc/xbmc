/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  xmlDocPtr m_xmlInput = nullptr;
  xmlDocPtr m_xmlOutput = nullptr;
  xmlDocPtr m_xmlStylesheet = nullptr;
  xsltStylesheetPtr m_xsltStylesheet = nullptr;
};
