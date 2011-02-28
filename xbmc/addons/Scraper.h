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
#include "XBDateTime.h"
#include "utils/ScraperParser.h"

typedef enum
{
  CONTENT_MOVIES,
  CONTENT_TVSHOWS,
  CONTENT_MUSICVIDEOS,
  CONTENT_ALBUMS,
  CONTENT_ARTISTS,
  CONTENT_NONE,
} CONTENT_TYPE;

namespace ADDON
{
  class CScraper;
  typedef boost::shared_ptr<CScraper> ScraperPtr;

  const CStdString   TranslateContent(const CONTENT_TYPE &content, bool pretty=false);
        CONTENT_TYPE TranslateContent(const CStdString &string);
        TYPE         ScraperTypeFromContent(const CONTENT_TYPE &content);

  class CScraper : public CAddon
  {
  public:
    CScraper(const AddonProps &props) : CAddon(props) { }
    CScraper(const cp_extension_t *ext);
    virtual ~CScraper() {}
    virtual AddonPtr Clone(const AddonPtr &self) const;

    /*! \brief Set the scraper settings for a particular path from an XML string
     Loads the default and user settings (if not already loaded) and, if the given XML string is non-empty,
     overrides the user settings with the XML.
     \param content Content type of the path
     \param xml string of XML with the settings.  If non-empty this overrides any saved user settings.
     \return true if settings are available, false otherwise
     \sa GetPathSettings
     */
    bool SetPathSettings(CONTENT_TYPE content, const CStdString& xml);

    /*! \brief Get the scraper settings for a particular path in the form of an XML string
     Loads the default and user settings (if not already loaded) and returns the user settings in the
     form or an XML string
     \return a string containing the XML settings
     \sa SetPathSettings
     */
    CStdString GetPathSettings();

    /*! \brief Clear any previously cached results for this scraper
     Any previously cached files are cleared if they have been cached for longer than the specified
     cachepersistence.
     */
    void ClearCache();
    bool Load();

    CONTENT_TYPE Content() const { return m_pathContent; }
    const CStdString& Framework() const { return m_framework; }
    const CStdString& Language() const { return m_language; }
    bool RequiresSettings() const { return m_requiressettings; }
    bool Supports(const CONTENT_TYPE &content) const;

    std::vector<CStdString> Run(const CStdString& function,
                                const CScraperUrl& url,
                                XFILE::CFileCurl& http,
                                const std::vector<CStdString>* extras=NULL);
    CScraperParser& GetParser() { return m_parser; }

    bool IsInUse() const;

  private:
    CScraper(const CScraper&, const AddonPtr&);

    CStdString InternalRun(const CStdString& function,
                           const CScraperUrl& url,
                           XFILE::CFileCurl& http,
                           const std::vector<CStdString>* extras);

    CStdString m_framework;
    CStdString m_language;
    bool m_requiressettings;
    CDateTimeSpan m_persistence;
    CONTENT_TYPE m_pathContent;
    CScraperParser m_parser;
  };

}; /* namespace ADDON */

