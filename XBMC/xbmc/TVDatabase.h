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

#include "Database.h"
#include "DateTime.h"
#include "settings/VideoSettings.h"
#include "utils/TVEPGInfoTag.h"
#include "utils/EPG.h"
#include "FileItem.h"

class CGUIDialogProgress;

class CTVDatabase : public CDatabase
{
public:
  CTVDatabase(void);
  virtual ~CTVDatabase(void);

  virtual bool CommitTransaction();

  // Clients
  /*bool GetSavedClients(PVR)*/

  // epg
  bool FillEPG(const CStdString &client, const CStdString &bouquet, const CStdString &channame, const CStdString &callsign, const int &channum, const CStdString &progTitle, 
               const CStdString &progSubtitle, const CStdString &progDescription, const CStdString &episode, const CStdString &series, 
               const CDateTime &progStartTime, const CDateTime &progEndTime, const CStdString &category);

  void GetChannelList(DWORD clientID, EPGData &channels);
  int  GetNumChannels(DWORD clientID);

  bool HasChannel(DWORD clientID, const CStdString &name);

  void AddChannelData(CFileItemList &channel);



  bool GetProgrammesByChannelName(const CStdString &channel, CFileItemList &shows, const CDateTime &start, const CDateTime &end);
  bool GetProgrammesByEpisodeID(const CStdString& episodeID, CFileItemList* items, bool noHistory /* == true */);
  void GetProgrammesByName(const CStdString& progName, CFileItemList& items, bool noHistory /* == true */);
  bool GetProgrammesBySubtitle(const CStdString& subtitle, CFileItemList* items, bool noHistory /* == true */);

  // per-channel video settings
  bool GetChannelSettings(const CStdString &channel, CVideoSettings &settings);
  bool SetChannelSettings(const CStdString &channel, const CVideoSettings &settings);
  
  CDateTime GetDataEnd(DWORD clientID);

  void EraseChannelSettings();

  // helper to add new channels from pvrmanager
  void NewChannel(DWORD clientID, CStdString bouquet, CStdString chanName, 
                  CStdString callsign, int chanNum, CStdString iconPath);

protected:
  CTVEPGInfoTag GetUniqueBroadcast(std::auto_ptr<dbiplus::Dataset> &pDS);
  void FillProperties(CFileItem* programme);

  long AddClient(const CStdString &plugin, const CStdString &client);
  long AddBouquet(const long &clientId, const CStdString &bouquet);
  long AddChannel(const long &clientId, const long &idBouquet, const CStdString &Callsign, const CStdString &Name, const int &Number, const CStdString &iconPath);
  long AddProgramme(const CStdString &Title, const long &categoryId);
  long AddCategory(const CStdString &category);

  long GetClientId(const CStdString &client);
  long GetBouquetId(const CStdString &bouquet);
  long GetChannelId(const CStdString &channel);
  long GetCategoryId(const CStdString &category);
  long GetProgrammeId(const CStdString &programme);

  CStdString m_progInfoSelectStatement;

private:
  virtual bool CreateTables();
};
