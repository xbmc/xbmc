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
#include "VideoInfoTag.h"
#include "FileItem.h"

//struct TVProgramme
//{
//  CStdString ProgrammeName;
//  CStdString ShortDesc;
//  CStdString LongDesc;
//  CStdString Category;
//  bool       freeToView;
//
//  CStdString SourceName;
//  CStdString BouquetName;
//
//  CStdString ChannelCallsign
//  CStdString ChannelName;
//  int        ChannelNumber;
//  
//  CDateTime FirstBroadcast;
//  CDateTime StartTime;
//  int       Duration;
//};

typedef VECFILEITEMS  VECTVSHOWS;
typedef VECFILEITEMS  VECTVCHANNELS;

struct EPGRow
{
  VECTVSHOWS shows;
  CStdString channelName;
  int        channelNum;
  bool       freeToView;
};

typedef VECTVSHOWS::const_iterator itEPGShow;
typedef std::vector<struct EPGRow> EPGGrid;
typedef std::vector<struct EPGRow>::const_iterator itEPGRow;

//****************************************************************************/
class CTVDatabase : public CDatabase
{
public:
  CTVDatabase(void);
  virtual ~CTVDatabase(void);

  virtual bool CommitTransaction();

  // epg
  bool FillEPG(const CStdString &source, const CStdString &bouquet, const CStdString &channel, const int &channum, const CStdString &progTitle, 
               const CStdString &progShortDesc, const CStdString &progLongDesc, const CStdString &episode, const CStdString &series, 
               const CDateTime &progStartTime, const int &progDuration, const CDateTime &progAirDate, const CStdString &category);

  void GetAllChannels(bool freeToAirOnly, VECTVCHANNELS &channels);
  bool GetShowsByChannel(const CStdString &channel, VECTVSHOWS &shows, const CDateTime &start, const CDateTime &end);

  // per-channel video settings
  bool GetChannelSettings(const CStdString &channel, CVideoSettings &settings);
  bool SetChannelSettings(const CStdString &channel, const CVideoSettings &settings);
  // per-programme video settings
  bool GetProgrammeSettings(const CStdString &programme, CVideoSettings &settings);
  void SetProgrammeSettings(const CStdString &programme, const CVideoSettings &settings);
  
  CDateTime GetDataEnd();

  void EraseChannelSettings();

protected:
  long AddSource(const CStdString &source);
  long AddBouquet(const long &sourceId, const CStdString &bouquet);
  long AddChannel(const long &idSource, const long &idBouquet, const CStdString &Callsign, const CStdString &Name, const int &Number);
  long AddProgramme(const CStdString &Title, const long &categoryId);
  long AddCategory(const CStdString &category);

  long GetSourceId(const CStdString &source);
  long GetBouquetId(const CStdString &bouquet);
  long GetChannelId(const CStdString &channel);
  long GetCategoryId(const CStdString &category);
  long GetProgrammeId(const CStdString &programme);

  void AddToLinkTable(const char *table, const char *firstField, long firstID, const char *secondField, long secondID);

  CDateTime m_dataEnd; // schedule data exists up to this date

private:
  virtual bool CreateTables();
};
