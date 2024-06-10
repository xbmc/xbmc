/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

#include <memory>

class CXBMCTinyXML2;

namespace tinyxml2
{
class XMLElement;
}

namespace XFILE
{
  class CLibraryDirectory : public IDirectory
  {
  public:
    CLibraryDirectory();
    ~CLibraryDirectory() override;
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool Exists(const CURL& url) override;
    bool AllowAll() const override { return true; }
  private:
    /*! \brief parse the given path and return the node corresponding to this path
     \param path the library:// path to parse
     \return path to the XML file or directory corresponding to this path
     */
    std::string GetNode(const CURL& path);

    /*! \brief load the XML file and return a pointer to the <node> root element.
     Checks visible attribute and only returns non-NULL for valid nodes that should be visible.
     \param xmlFile the XML file to load and parse
     \return the XMLElement pointer to the node, if it should be visible.
     */
    tinyxml2::XMLElement* LoadXML(const std::string& xmlFile);

    std::unique_ptr<CXBMCTinyXML2> m_doc;
  };
}
