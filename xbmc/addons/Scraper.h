#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "addons/Addon.h"
#include "utils/ScraperUrl.h"

namespace ADDON
{
  class CScraper;
  typedef boost::shared_ptr<CScraper> ScraperPtr;

  class CScraper : public CAddon
  {
  public:
    CScraper(const AddonProps &props) : CAddon(props) { }
    virtual ~CScraper() {}
    virtual AddonPtr Clone(const AddonPtr &self) const;

    // from CAddon
    virtual bool HasSettings();
    virtual bool LoadSettings();

    // scraper specialization
    bool LoadUserXML(const CStdString& strXML);
    bool LoadSettingsXML(const CStdString& strFunction="GetSettings", const CScraperUrl* url=NULL);
    bool Load(const CStdString& strSettings, const CStdString& strSaved);
    CStdString GetSettings() const;
    CStdString m_strLanguage;
    CONTENT_TYPE Content() const { return m_pathContent; }
    const CStdString Framework() const { return m_framework; }
    CONTENT_TYPE m_pathContent;

  private:
    CScraper(const CScraper&, const AddonPtr&);
    bool m_hasSettings;
    CStdString m_framework;
  };

}; /* namespace ADDON */

