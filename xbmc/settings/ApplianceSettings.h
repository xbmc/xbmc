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

#include "utils/StdString.h"
#include "utils/GlobalsHandling.h"

class TiXmlElement;

class CApplianceSettings
{
  public:
    static CApplianceSettings* getInstance();
    bool Load(CStdString profileName);
    void Clear();

    CApplianceSettings();
    void Initialize();

    bool CanQuit() { return m_canQuit; };
    bool CanWindowed() { return m_canWindowed; };
  private:
    const char *GetProfileRestrictions(const TiXmlElement *pElement);
    bool ProfileMatch(const TiXmlElement *pElement, const CStdString profileName);
    bool m_canQuit;
    bool m_canWindowed;
};

XBMC_GLOBAL(CApplianceSettings,g_applianceSettings);