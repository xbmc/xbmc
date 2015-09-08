#pragma once
/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
*
*      Copyright (C) 2015 Romank
*      https://github.com/Romank1
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


#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "DSPVRBackend.h"

class CDSMediaPortalCards;
class CDSMediaPortal : public CDSPVRBackend
{
public:
  CDSMediaPortal(const CStdString& strBackendBaseAddress, const CStdString& strBackendName);
  virtual ~CDSMediaPortal();
  virtual bool          ConvertStreamURLToTimeShiftFilePath(const CStdString& strUrl, CStdString& strTimeShiftFile);
  virtual bool          SupportsStreamConversion(const CStdString& strUrl) const { return true; };
  virtual bool          SupportsFastChannelSwitch() const { return true; };
  virtual bool          GetRecordingStreamURL(const CStdString& strRecordingId, CStdString& strRecordingUrl, bool bGetUNCPath = false);

private:
  bool                  SendCommandToMPTVServer(const CStdString& strCommand, CStdString & strResponse);
  bool                  ConnectToMPTVServer();
  bool                  ConvertRtspStreamUrlToTimeShiftFilePath(const CStdString& strUrl, CStdString& strTimeShiftFile);
  bool                  LoadCardSettings();
  bool                  TranslatePathToUNC(const CStdString& strFilePath, CStdString& strTranslatedFilePath);

  CDSMediaPortalCards  *m_pCardsSettings;
};



/* From Kodi's MediaPortal TVServer client addon: https://github.com/kodi-pvr/pvr.mediaportal.tvserver */

/**
* MediaPortal TVServer card settings ("card" table in the database)
*/
typedef struct MPCard
{
  int       IdCard;
  string    DevicePath;
  string    Name;
  int       Priority;
  bool      GrabEPG;
  CDateTime LastEpgGrab;
  string    RecordingFolder;
  string    RecordingFolderUNC;
  int       IdServer;
  bool      Enabled;
  int       CamType;
  string    TimeshiftFolder;
  string    TimeshiftFolderUNC;
  int       RecordingFormat;
  int       DecryptLimit;
  bool      Preload;
  bool      CAM;
  int       NetProvider;
  bool      StopGraph;
} MPCard;

class CDSMediaPortalCards : public vector<MPCard>
{
public:

  /**
  * \brief Parse the multi-line string response from the TVServerXBMC plugin command "GetCardSettings"
  * The data is stored in "struct MediaPortalCard" item.
  *
  * \param lines Vector with response lines
  * \return True on success, False on failure
  */
  bool ParseLines(vector<string>& lines);

  /**
  * \brief Return the data for the card with the given id
  * \param id The card id
  * \param card Return value: card data or NULL if not found.
  * \return True on success, False on failure
  */
  bool GetCard(int id, MPCard& card);
};
