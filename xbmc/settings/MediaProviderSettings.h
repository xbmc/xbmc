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

class CMediaProviderSettings
{
  public:
    CMediaProviderSettings();
    CMediaProviderSettings(TiXmlElement *pRootElement);
    int TuxBoxStreamtsPort();
    bool TuxBoxSubMenuSelection();
    int TuxBoxDefaultSubMenu();
    void SetTuxBoxDefaultRootMenu(int value);
    int TuxBoxDefaultRootMenu();
    bool TuxBoxAudioChannelSelection();
    bool TuxBoxPictureIcon();
    int TuxBoxEpgRequestTime();
    int TuxBoxZapWaitTime();
    bool TuxBoxSendAllAPids();
    bool TuxBoxZapstream();
    int TuxBoxZapstreamPort();
    int MythMovieLength();
    bool EdlMergeShortCommBreaks();
    int EdlMaxCommBreakLength();
    int EdlMinCommBreakLength();
    int EdlMaxCommBreakGap();
    int EdlMaxStartGap();
    int EdlCommBreakAutowait();
    int EdlCommBreakAutowind();
  private:
    void Initialise();
    int m_iTuxBoxStreamtsPort;
    bool m_bTuxBoxSubMenuSelection;
    int m_iTuxBoxDefaultSubMenu;
    int m_iTuxBoxDefaultRootMenu;
    bool m_bTuxBoxAudioChannelSelection;
    bool m_bTuxBoxPictureIcon;
    int m_iTuxBoxEpgRequestTime;
    int m_iTuxBoxZapWaitTime;
    bool m_bTuxBoxSendAllAPids;
    bool m_bTuxBoxZapstream;
    int m_iTuxBoxZapstreamPort;
    int m_iMythMovieLength;         // minutes
    bool m_bEdlMergeShortCommBreaks;
    int m_iEdlMaxCommBreakLength;   // seconds
    int m_iEdlMinCommBreakLength;   // seconds
    int m_iEdlMaxCommBreakGap;      // seconds
    int m_iEdlMaxStartGap;          // seconds
    int m_iEdlCommBreakAutowait;    // seconds
    int m_iEdlCommBreakAutowind;    // seconds
};