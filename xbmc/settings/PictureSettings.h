/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/XMLUtils.h"

class CPictureSettings
{
  public:
    CPictureSettings();
    CPictureSettings(TiXmlElement *pRootElement);
    void Clear();
    float SlideshowBlackBarCompensation();
    float SlideshowZoomAmount();
    float SlideshowPanAmount();
    CStdStringArray ExcludeFromListingRegExps();
  private:
    void Initialise();
    void GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings);
    void GetCustomExtensions(TiXmlElement *pRootElement, CStdString& extensions);
    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;
    CStdStringArray m_excludeFromListingRegExps;
};