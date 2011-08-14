#pragma once
/*
 *      Copyright (C) 2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IDirectory.h"
#include "lib/tinyXML/tinyxml.h"

namespace XFILE
{
  class CLibraryDirectory : public IDirectory
  {
  public:
    CLibraryDirectory();
    virtual ~CLibraryDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
    virtual bool IsAllowed(const CStdString& strFile) const { return true; };
  private:
    /*! \brief parse the given path, load our xml file and return the node corresponding to this path
     \param the library:// path to parse
     \return a pointer to the corresponding <node> TiXmlElement, NULL if not found.
     */
    TiXmlElement *GetNode(const CStdString &path);
    TiXmlDocument m_doc;
  };
}
