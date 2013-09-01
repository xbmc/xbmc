/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "network/Network.h"
#include "threads/SystemClock.h"
#include "system.h"
#include "PictureDatabase.h"
#include "network/cddb.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/PictureDatabaseDirectory/DirectoryNode.h"
#include "filesystem/PictureDatabaseDirectory/QueryParams.h"
#include "filesystem/PictureDatabaseDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "GUIInfoManager.h"
#include "pictures/tags/PictureInfoTag.h"
#include "addons/AddonManager.h"
#include "addons/Scraper.h"
#include "addons/Addon.h"
#include "utils/URIUtils.h"
#include "Face.h"
#include "PictureAlbum.h"
#include "Picture.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "Application.h"
#include "storage/MediaManager.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "TextureCache.h"
#include "addons/AddonInstaller.h"
#include "utils/AutoPtrHandle.h"
#include "interfaces/AnnouncementManager.h"
#include "dbwrappers/dataset.h"
#include "utils/XMLUtils.h"
#include "URL.h"
#include "playlists/SmartPlayList.h"

using namespace std;
using namespace AUTOPTR;
using namespace XFILE;
using namespace PICTUREDATABASEDIRECTORY;
using ADDON::AddonPtr;

#define RECENTLY_PLAYED_LIMIT 25
#define MIN_FULL_SEARCH_LENGTH 3

#ifdef HAS_DVD_DRIVE
using namespace CDDB;
#endif

static void AnnounceRemove(const std::string& content, int id)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnRemove", data);
}

static void AnnounceUpdate(const std::string& content, int id)
{
  CVariant data;
  data["type"] = content;
  data["id"] = id;
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnUpdate", data);
}

CPictureDatabase::CPictureDatabase(void)
{
}

CPictureDatabase::~CPictureDatabase(void)
{
  EmptyCache();
}

const char *CPictureDatabase::GetBaseDBName() const { return "MyPicture"; };

bool CPictureDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databasePicture);
}

bool CPictureDatabase::CreateTables()
{
  BeginTransaction();
  try
  {
    CDatabase::CreateTables();
    
    CLog::Log(LOGINFO, "create face table");
    m_pDS->exec("CREATE TABLE face ( idFace integer primary key, strFace varchar(256))\n");
    CLog::Log(LOGINFO, "create album table");
    m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, strAlbum varchar(256), strFaces text, strLocations text, iYear integer, idThumb integer, bCompilation integer not null default '0')\n");
    CLog::Log(LOGINFO, "create album_face table");
    m_pDS->exec("CREATE TABLE album_face ( idFace integer, idAlbum integer, strJoinPhrase text, boolFeatured integer, iOrder integer )\n");
    CLog::Log(LOGINFO, "create album_location table");
    m_pDS->exec("CREATE TABLE album_location ( idLocation integer, idAlbum integer, iOrder integer )\n");
    
    CLog::Log(LOGINFO, "create location table");
    m_pDS->exec("CREATE TABLE location ( idLocation integer primary key, strLocation varchar(256))\n");
    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath varchar(512), strHash text)\n");
    CLog::Log(LOGINFO, "create picture table");
    m_pDS->exec("CREATE TABLE picture ( idPicture integer primary key, idAlbum integer, idPath integer, strFaces text, strLocations text, strTitle varchar(512), dwFileNameCRC text, strFileName text, idThumb integer, takenon varchar(20) default NULL, comment text )\n");
    CLog::Log(LOGINFO, "create picture_face table");
    m_pDS->exec("CREATE TABLE picture_face ( idFace integer, idPicture integer, strJoinPhrase text, boolFeatured integer, iOrder integer )\n");
    CLog::Log(LOGINFO, "create picture_location table");
    m_pDS->exec("CREATE TABLE picture_location ( idLocation integer, idPicture integer, iOrder integer )\n");
    
    CLog::Log(LOGINFO, "create albuminfo table");
    m_pDS->exec("CREATE TABLE albuminfo ( idAlbumInfo integer primary key, idAlbum integer, strImage text, strLabel text, strType text)\n");
    CLog::Log(LOGINFO, "create albuminfopicture table");
    m_pDS->exec("CREATE TABLE albuminfopicture ( idAlbumInfoPicture integer primary key, idAlbumInfo integer, strTitle text)\n");
    CLog::Log(LOGINFO, "create facenfo table");
    m_pDS->exec("CREATE TABLE faceinfo ( idFaceInfo integer primary key, idFace integer, strBorn text, strFormed text, strLocations text, strMoods text, strStyles text, strInstruments text, strBiography text, strDied text, strDisbanded text, strYearsActive text, strImage text, strFanart text)\n");
    CLog::Log(LOGINFO, "create content table");
    m_pDS->exec("CREATE TABLE content (strPath text, strScraperPath text, strContent text, strSettings text)\n");
    CLog::Log(LOGINFO, "create discography table");
    m_pDS->exec("CREATE TABLE discography (idFace integer, strAlbum text, strYear text)\n");
    
    CLog::Log(LOGINFO, "create karaokedata table");
    m_pDS->exec("CREATE TABLE karaokedata ( iKaraNumber integer, idPicture integer, iKaraDelay integer, strKaraEncoding text, "
                "strKaralyrics text, strKaraLyrFileCRC text )\n");
    
    CLog::Log(LOGINFO, "create album index");
    m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum)");
    CLog::Log(LOGINFO, "create album compilation index");
    m_pDS->exec("CREATE INDEX idxAlbum_1 ON album(bCompilation)");
    
    CLog::Log(LOGINFO, "create album_face indexes");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumface_1 ON album_face ( idAlbum, idFace )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumface_2 ON album_face ( idFace, idAlbum )\n");
    m_pDS->exec("CREATE INDEX idxAlbumface_3 ON album_face ( boolFeatured )\n");
    
    CLog::Log(LOGINFO, "create album_location indexes");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumLocation_1 ON album_location ( idAlbum, idLocation )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxAlbumLocation_2 ON album_location ( idLocation, idAlbum )\n");
    
    CLog::Log(LOGINFO, "create location index");
    m_pDS->exec("CREATE INDEX idxLocation ON location(strLocation)");
    
    CLog::Log(LOGINFO, "create face indexes");
    m_pDS->exec("CREATE INDEX idxFace ON face(strFace)");
    
    CLog::Log(LOGINFO, "create path index");
    m_pDS->exec("CREATE INDEX idxPath ON path(strPath)");
    
    CLog::Log(LOGINFO, "create picture index");
    m_pDS->exec("CREATE INDEX idxPicture ON picture(strTitle)");
//    CLog::Log(LOGINFO, "create picture index1");
//    m_pDS->exec("CREATE INDEX idxPicture1 ON picture(iPictureCount)");
    CLog::Log(LOGINFO, "create picture index2");
    m_pDS->exec("CREATE INDEX idxPicture2 ON picture(takenon)");
    CLog::Log(LOGINFO, "create picture index3");
    m_pDS->exec("CREATE INDEX idxPicture3 ON picture(idAlbum)");
    CLog::Log(LOGINFO, "create picture index6");
    m_pDS->exec("CREATE INDEX idxPicture6 ON picture( idPath, strFileName(255) )");
    
    CLog::Log(LOGINFO, "create picture_face indexes");
    m_pDS->exec("CREATE UNIQUE INDEX idxPictureface_1 ON picture_face ( idPicture, idFace )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxPictureface_2 ON picture_face ( idFace, idPicture )\n");
    m_pDS->exec("CREATE INDEX idxPictureface_3 ON picture_face ( boolFeatured )\n");
    
    CLog::Log(LOGINFO, "create picture_location indexes");
    m_pDS->exec("CREATE UNIQUE INDEX idxPictureLocation_1 ON picture_location ( idPicture, idLocation )\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxPictureLocation_2 ON picture_location ( idLocation, idPicture )\n");
    
    //m_pDS->exec("CREATE INDEX idxPicture ON picture(dwFileNameCRC)");
    CLog::Log(LOGINFO, "create faceinfo index");
    m_pDS->exec("CREATE INDEX idxFaceInfo on faceinfo(idFace)");
    CLog::Log(LOGINFO, "create albuminfo index");
    m_pDS->exec("CREATE INDEX idxAlbumInfo on albuminfo(idAlbum)");
    
    CLog::Log(LOGINFO, "create karaokedata index");
    m_pDS->exec("CREATE INDEX idxKaraNumber on karaokedata(iKaraNumber)");
    m_pDS->exec("CREATE INDEX idxKarPicture on karaokedata(idPicture)");
    
    // Trigger
    CLog::Log(LOGINFO, "create albuminfo trigger");
    m_pDS->exec("CREATE TRIGGER tgrAlbumInfo AFTER delete ON albuminfo FOR EACH ROW BEGIN delete from albuminfopicture where albuminfopicture.idAlbumInfo=old.idAlbumInfo; END");
    
    CLog::Log(LOGINFO, "create art table, index and triggers");
    m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");
    m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");
    m_pDS->exec("CREATE TRIGGER delete_picture AFTER DELETE ON picture FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idPicture AND media_type='picture'; END");
    m_pDS->exec("CREATE TRIGGER delete_album AFTER DELETE ON album FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idAlbum AND media_type='album'; END");
    m_pDS->exec("CREATE TRIGGER delete_face AFTER DELETE ON face FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idFace AND media_type='face'; END");
    
    // we create views last to ensure all indexes are rolled in
    CreateViews();
    
    // Add 'Karaoke' location
    //AddPictureAlbum("Untitled", "Face", "Location", 2012, false);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create tables:%i", __FUNCTION__, (int)GetLastError());
    RollbackTransaction();
    return false;
  }
  CommitTransaction();
  return true;
}

void CPictureDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create picture view");
  m_pDS->exec("DROP VIEW IF EXISTS pictureview");
  m_pDS->exec("CREATE VIEW pictureview AS SELECT "
              "  picture.idPicture AS idPicture, "
              "  picture.strFaces AS strFaces,"
              "  picture.takenon AS takenOn,"
              "  picture.strLocations AS strLocations,"
              "  picture.idPath AS idPath,"
              "  picture.idAlbum as idAlbum,"
              "  album.strAlbum as strAlbum,"
              "  picture.strTitle AS strTitle,"
              "  picture.strFileName AS strFileName,"
              "  picture.idThumb AS idThumb,"
              "  path.strPath as strPath"
              "  FROM picture"
              "  JOIN album ON"
              "  picture.idAlbum=album.idAlbum  "
              "  JOIN path ON"
              "  picture.idPath=path.idPath");
  
  CLog::Log(LOGINFO, "create album view");
  m_pDS->exec("DROP VIEW IF EXISTS albumview");
  if (m_sqlite)
  {    
    m_pDS->exec("CREATE VIEW albumview AS SELECT "
                "        album.idAlbum AS idAlbum, "
                "        strAlbum, "
                "        GROUP_CONCAT(strFace || strJoinPhrase, '') as strFaces, "
                "        album.strLocations AS strLocations, "
                "        albuminfo.idAlbumInfo AS albumInfo, "
                "        albuminfo.strImage AS strImage "
                "   FROM album  "
                "   LEFT OUTER JOIN "
                "       albuminfo ON album.idAlbum = albuminfo.idAlbum "
                "   LEFT OUTER JOIN album_face ON "
                "       album.idAlbum = album_face.idAlbum "
                "   LEFT OUTER JOIN face ON "
                "       album_face.idFace = face.idFace "
                "   GROUP BY album.idAlbum");
  }
  else
  {
    m_pDS->exec("CREATE VIEW albumview AS SELECT "
                "        album.idAlbum AS idAlbum, "
                "        strAlbum, "
                "        GROUP_CONCAT(strFace, strJoinPhrase ORDER BY iOrder SEPARATOR '') as strFaces, "
                "        album.strLocations AS strLocations, "
                "        album.iYear AS iYear, "
                "        albuminfo.idAlbumInfo AS albumInfo, "
                "        albuminfo.strImage AS strImage "
                "   FROM album  "
                "   LEFT OUTER JOIN "
                "       albuminfo ON album.idAlbum = albuminfo.idAlbum "
                "   LEFT OUTER JOIN album_face ON "
                "       album.idAlbum = album_face.idAlbum "
                "   LEFT OUTER JOIN face ON "
                "       album_face.idFace = face.idFace "
                "   GROUP BY album.idAlbum");
  }
  
  CLog::Log(LOGINFO, "create face view");
  m_pDS->exec("DROP VIEW IF EXISTS faceview");
  m_pDS->exec("CREATE VIEW faceview AS SELECT"
              "  face.idFace AS idFace, strFace, "
              "  strBorn, strFormed, strLocations,"
              "  strMoods, strStyles, strInstruments, "
              "  strBiography, strDied, strDisbanded, "
              "  strYearsActive, strImage, strFanart "
              "FROM face "
              "  LEFT OUTER JOIN faceinfo ON"
              "    face.idFace = faceinfo.idFace");
}

int CPictureDatabase::AddPicture(const int idAlbum, const CStdString& strTitle, const CStdString& strPathAndFileName, const CStdString& strComment, const CStdString& strThumb, const std::vector<std::string>& Faces, const std::vector<std::string>& locations,const CStdString& dtTaken)
{
  int idPicture = -1;
  CStdString strSQL;
  try
  {
    // We need at least the title
    if (strTitle.IsEmpty())
      return -1;
    
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    CStdString strPath, strFileName;
    URIUtils::Split(strPathAndFileName, strPath, strFileName);
    int idPath = AddPath(strPath);
    DWORD crc = ComputeCRC(strFileName);
    
    strSQL=PrepareSQL("SELECT * FROM picture WHERE idAlbum = %i AND strTitle='%s'",
                      idAlbum,
                      strTitle.c_str());
    
    if (!m_pDS->query(strSQL.c_str()))
      return -1;
    
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      strSQL=PrepareSQL("INSERT INTO picture (idPicture,idAlbum,idPath,strFaces,strLocations,strTitle,dwFileNameCRC,strFileName,comment,takenon) values (NULL, %i, %i, '%s', '%s', '%s', '%ul', '%s' ",
                        idAlbum,
                        idPath,
                        StringUtils::Join(Faces, g_advancedSettings.m_pictureItemSeparator).c_str(),
                        StringUtils::Join(locations, g_advancedSettings.m_pictureItemSeparator).c_str(),
                        strTitle.c_str(),
                        crc, strFileName.c_str());
      
      if (dtTaken.length())
        strSQL += PrepareSQL(",'%s','%s')",dtTaken.c_str(), strComment.c_str());
      else
        strSQL += PrepareSQL(",NULL,'%s')",strComment.c_str());
      CLog::Log(LOGINFO, strSQL.c_str());
      m_pDS->exec(strSQL.c_str());
      idPicture = (int)m_pDS->lastinsertid();
    }
    else
    {
      idPicture = m_pDS->fv("idPicture").get_asInt();
      m_pDS->close();
      UpdatePicture(idPicture, strTitle, strPathAndFileName, strComment, strThumb, Faces, locations, dtTaken);
    }
    
    if (!strThumb.empty())
      SetArtForItem(idPicture, "picture", "thumb", strThumb);
    
    unsigned int index = 0;
    for (vector<string>::const_iterator i = locations.begin(); i != locations.end(); ++i)
    {
      // index will be wrong for albums, but ordering is not all that relevant
      // for locations anyway
      
      int idLocation = AddLocation(*i);
      AddPictureLocation(idLocation, idPicture, index);
      AddPictureAlbumLocation(idLocation, idAlbum, index++);
      
    }
    
    AnnounceUpdate("picture", idPicture);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Picturedatabase:unable to addpicture (%s)", strSQL.c_str());
  }
  return idPicture;
}

int CPictureDatabase::UpdatePicture(int idPicture, const CStdString& strTitle, const CStdString& strPathAndFileName, const CStdString& strComment, const CStdString& strThumb, const std::vector<std::string>& Faces, const std::vector<std::string>& locations, const CStdString& dtTaken)
{
  CStdString sql;
  if (idPicture < 0)
    return -1;
  
  CStdString strSQL;
  try
  {
    CStdString strPath, strFileName;
    URIUtils::Split(strPathAndFileName, strPath, strFileName);
    int idPath = AddPath(strPath);
    DWORD crc = ComputeCRC(strFileName);
    
    strSQL = PrepareSQL("UPDATE picture SET idPath = %i, strFaces = '%s', strLocations = '%s', strTitle = '%s', dwFileNameCRC = '%ul', strFileName = '%s'",
                        idPath,
                        StringUtils::Join(Faces, g_advancedSettings.m_pictureItemSeparator).c_str(),
                        StringUtils::Join(locations, g_advancedSettings.m_pictureItemSeparator).c_str(),
                        strTitle.c_str(),
                        crc, strFileName.c_str());
    
    if (dtTaken.length())
      strSQL += PrepareSQL(",  takenon = '%s',  comment = '%s'", dtTaken.c_str(), strComment.c_str());
    else
      strSQL += PrepareSQL(", takenon = NULL, comment = '%s'", strComment.c_str());
    strSQL += PrepareSQL(" WHERE idPicture = %i", idPicture);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Picturedatabase:unable to addpicture (%s)", strSQL.c_str());
  }
  return idPicture;
}

int CPictureDatabase::AddPictureAlbum(const CStdString& strAlbum, const CStdString& strFace, const CStdString& strLocation)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    if(strAlbum.length() <= 0) return -1;
    
    strSQL=PrepareSQL("SELECT * FROM album WHERE strAlbum like '%s'",strAlbum.c_str());
    CLog::Log(LOGINFO, strSQL.c_str());
    m_pDS->query(strSQL.c_str());
    
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      
      strSQL=PrepareSQL("insert into album (strAlbum, strFaces, strLocations) values ( '%s', '%s', '%s')",
                        strAlbum.c_str(),
                        strFace.c_str(),
                        strLocation.c_str());
      
      CLog::Log(LOGINFO, strSQL.c_str());
      m_pDS->exec(strSQL.c_str());
      
      return (int)m_pDS->lastinsertid();
    }
    else
    {
      // exists in our database and not scanned during this scan, so we should update it as the details
      // may have changed (there's a reason we're rescanning, afterall!)
      int idAlbum = m_pDS->fv("idAlbum").get_asInt();
      m_pDS->close();
      strSQL=PrepareSQL("update album set strLocations='%s'", strLocation.c_str());
      m_pDS->exec(strSQL.c_str());
      // and clear the link tables - these are updated in AddPicture()
      strSQL=PrepareSQL("delete from album_Face where idAlbum=%i", idAlbum);
      m_pDS->exec(strSQL.c_str());
      strSQL=PrepareSQL("delete from album_location where idAlbum=%i", idAlbum);
      m_pDS->exec(strSQL.c_str());
      return idAlbum;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
  }
  
  return -1;
}

int CPictureDatabase::AddLocation(const CStdString& strLocation1)
{
  CStdString strSQL;
  try
  {
    CStdString strLocation = strLocation1;
    strLocation.Trim();
    
    if (strLocation.IsEmpty())
      strLocation=g_localizeStrings.Get(13205); // Unknown
    
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    map <CStdString, int>::const_iterator it;
    
    it = m_locationCache.find(strLocation);
    if (it != m_locationCache.end())
      return it->second;
    
    
    strSQL=PrepareSQL("select * from location where strLocation like '%s'", strLocation.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into location (idLocation, strLocation) values( NULL, '%s' )", strLocation.c_str());
      m_pDS->exec(strSQL.c_str());
      
      int idLocation = (int)m_pDS->lastinsertid();
      m_locationCache.insert(pair<CStdString, int>(strLocation1, idLocation));
      return idLocation;
    }
    else
    {
      int idLocation = m_pDS->fv("idLocation").get_asInt();
      m_locationCache.insert(pair<CStdString, int>(strLocation1, idLocation));
      m_pDS->close();
      return idLocation;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Picturedatabase:unable to addlocation (%s)", strSQL.c_str());
  }
  
  return -1;
}

int CPictureDatabase::AddFace(const CStdString& strFace)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    strSQL = PrepareSQL("SELECT * FROM Face WHERE strFace = '%s'",
                        strFace.c_str());
    m_pDS->query(strSQL.c_str());
    
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL = PrepareSQL("INSERT INTO Face (idFace, strFace) VALUES( NULL, '%s' )",
                          strFace.c_str());
      m_pDS->exec(strSQL.c_str());
      int idFace = (int)m_pDS->lastinsertid();
      return idFace;
    }
    else
    {
      int idFace = (int)m_pDS->fv("idFace").get_asInt();
      m_pDS->close();
      return idFace;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Picturedatabase:unable to addFace (%s)", strSQL.c_str());
  }
  
  return -1;
}

bool CPictureDatabase::AddPictureFace(int idFace, int idPicture, std::string joinPhrase, bool featured, int iOrder)
{
  CStdString strSQL;
  strSQL=PrepareSQL("replace into picture_Face (idFace, idPicture, strJoinPhrase, boolFeatured, iOrder) values(%i,%i,'%s',%i,%i)",
                    idFace, idPicture, joinPhrase.c_str(), featured == true ? 1 : 0, iOrder);
  return ExecuteQuery(strSQL);
};

bool CPictureDatabase::AddPictureAlbumFace(int idFace, int idAlbum, std::string joinPhrase, bool featured, int iOrder)
{
  CStdString strSQL;
  strSQL=PrepareSQL("replace into album_Face (idFace, idAlbum, strJoinPhrase, boolFeatured, iOrder) values(%i,%i,'%s',%i,%i)",
                    idFace, idAlbum, joinPhrase.c_str(), featured == true ? 1 : 0, iOrder);
  return ExecuteQuery(strSQL);
};

bool CPictureDatabase::AddPictureLocation(int idLocation, int idPicture, int iOrder)
{
  if (idLocation == -1 || idPicture == -1)
    return true;
  
  CStdString strSQL;
  strSQL=PrepareSQL("replace into picture_location (idLocation, idPicture, iOrder) values(%i,%i,%i)",
                    idLocation, idPicture, iOrder);
  return ExecuteQuery(strSQL);};

bool CPictureDatabase::AddPictureAlbumLocation(int idLocation, int idAlbum, int iOrder)
{
  if (idLocation == -1 || idAlbum == -1)
    return true;
  
  CStdString strSQL;
  strSQL=PrepareSQL("replace into album_location (idLocation, idAlbum, iOrder) values(%i,%i,%i)",
                    idLocation, idAlbum, iOrder);
  return ExecuteQuery(strSQL);
};

bool CPictureDatabase::GetPictureAlbumsByFace(int idFace, bool includeFeatured, std::vector<int> &albums)
{
  try
  {
    CStdString strSQL, strPrepSQL;
    
    strPrepSQL = "select idAlbum from album_Face where idFace=%i";
    if (includeFeatured == false)
      strPrepSQL += " AND boolFeatured = 0";
    
    strSQL=PrepareSQL(strPrepSQL, idFace);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    
    while (!m_pDS->eof())
    {
      albums.push_back(m_pDS->fv("idAlbum").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idFace);
  }
  return false;
}

bool CPictureDatabase::GetFacesByPictureAlbum(int idAlbum, bool includeFeatured, std::vector<int> &Faces)
{
  try
  {
    CStdString strSQL, strPrepSQL;
    
    strPrepSQL = "select idFace from album_Face where idAlbum=%i";
    if (includeFeatured == false)
      strPrepSQL += " AND boolFeatured = 0";
    
    strSQL=PrepareSQL(strPrepSQL, idAlbum);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    
    while (!m_pDS->eof())
    {
      Faces.push_back(m_pDS->fv("idFace").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  return false;
}

bool CPictureDatabase::GetPicturesByFace(int idFace, bool includeFeatured, std::vector<int> &pictures)
{
  try
  {
    CStdString strSQL, strPrepSQL;
    
    strPrepSQL = "select idPicture from picture_Face where idFace=%i";
    if (includeFeatured == false)
      strPrepSQL += " AND boolFeatured = 0";
    
    strSQL=PrepareSQL(strPrepSQL, idFace);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    
    while (!m_pDS->eof())
    {
      pictures.push_back(m_pDS->fv("idPicture").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idFace);
  }
  return false;
};

bool CPictureDatabase::GetFacesByPicture(int idPicture, bool includeFeatured, std::vector<int> &Faces)
{
  try
  {
    CStdString strSQL, strPrepSQL;
    
    strPrepSQL = "select idFace from picture_Face where idPicture=%i";
    if (includeFeatured == false)
      strPrepSQL += " AND boolFeatured = 0";
    
    strSQL=PrepareSQL(strPrepSQL, idPicture);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    
    while (!m_pDS->eof())
    {
      Faces.push_back(m_pDS->fv("idFace").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idPicture);
  }
  return false;
}

bool CPictureDatabase::GetLocationsByPictureAlbum(int idAlbum, std::vector<int>& locations)
{
  try
  {
    CStdString strSQL = PrepareSQL("select idLocation from album_location where idAlbum = %i ORDER BY iOrder ASC", idAlbum);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return true;
    }
    
    while (!m_pDS->eof())
    {
      locations.push_back(m_pDS->fv("idLocation").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  return false;
}

bool CPictureDatabase::GetLocationsByPicture(int idPicture, std::vector<int>& locations)
{
  try
  {
    CStdString strSQL = PrepareSQL("select idLocation from picture_location where idPicture = %i ORDER BY iOrder ASC", idPicture);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return true;
    }
    
    while (!m_pDS->eof())
    {
      locations.push_back(m_pDS->fv("idLocation").get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idPicture);
  }
  return false;
}

int CPictureDatabase::AddPath(const CStdString& strPath1)
{
  CStdString strSQL;
  try
  {
    CStdString strPath(strPath1);
    if (!URIUtils::HasSlashAtEnd(strPath))
      URIUtils::AddSlashAtEnd(strPath);
    
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    map <CStdString, int>::const_iterator it;
    
    it = m_pathCache.find(strPath);
    if (it != m_pathCache.end())
      return it->second;
    
    strSQL=PrepareSQL( "select * from path where strPath='%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=PrepareSQL("insert into path (idPath, strPath) values( NULL, '%s' )", strPath.c_str());
      m_pDS->exec(strSQL.c_str());
      
      int idPath = (int)m_pDS->lastinsertid();
      m_pathCache.insert(pair<CStdString, int>(strPath, idPath));
      return idPath;
    }
    else
    {
      int idPath = m_pDS->fv("idPath").get_asInt();
      m_pathCache.insert(pair<CStdString, int>(strPath, idPath));
      m_pDS->close();
      return idPath;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Picturedatabase:unable to addpath (%s)", strSQL.c_str());
  }
  
  return -1;
}

CPicture CPictureDatabase::GetPictureFromDataset(bool bWithPictureDbPath/*=false*/)
{
  CPicture picture;
  
  picture.idPicture = m_pDS->fv(picture_idPicture).get_asInt();
  // get the full Face string
  picture.face = StringUtils::Split(m_pDS->fv(picture_strFaces).get_asString(), g_advancedSettings.m_pictureItemSeparator);
  // and the full location string
  picture.location = StringUtils::Split(m_pDS->fv(picture_strLocations).get_asString(), g_advancedSettings.m_pictureItemSeparator);
  // and the rest...
  picture.strAlbum = m_pDS->fv(picture_strAlbum).get_asString();
  picture.idAlbum = m_pDS->fv(picture_idAlbum).get_asInt();
  picture.strTitle = m_pDS->fv(picture_strTitle).get_asString();
  picture.takenOn.SetFromDBDateTime(m_pDS->fv(picture_takenOn).get_asString());
  
  // Get filename with full path
  
  if (!bWithPictureDbPath)
    picture.strFileName = URIUtils::AddFileToFolder(m_pDS->fv(picture_strPath).get_asString(), m_pDS->fv(picture_strFileName).get_asString());
  else
  {
    CStdString strFileName = m_pDS->fv(picture_strFileName).get_asString();
    CStdString strExt = URIUtils::GetExtension(strFileName);
    picture.strFileName.Format("Picturedb://albums/%ld/%ld%s", m_pDS->fv(picture_idAlbum).get_asInt(), m_pDS->fv(picture_idPicture).get_asInt(), strExt.c_str());
  }
  
  return picture;
}

void CPictureDatabase::GetFileItemFromDataset(CFileItem* item, const CStdString& strPictureDBbasePath)
{
  return GetFileItemFromDataset(m_pDS->get_sql_record(), item, strPictureDBbasePath);
}

void CPictureDatabase::GetFileItemFromDataset(const dbiplus::sql_record* const record, CFileItem* item, const CStdString& strPictureDBbasePath)
{

  // get the full artist string
  item->GetPictureInfoTag()->SetFace(StringUtils::Split(record->at(picture_strFaces).get_asString(), g_advancedSettings.m_pictureItemSeparator));
  // and the full genre string
  item->GetPictureInfoTag()->SetLocation(record->at(picture_strLocations).get_asString());
  // and the rest...
  item->GetPictureInfoTag()->SetAlbum(record->at(picture_strAlbum).get_asString());
  item->GetPictureInfoTag()->SetAlbumId(record->at(picture_idAlbum).get_asInt());
  item->GetPictureInfoTag()->SetDatabaseId(record->at(picture_idPicture).get_asInt(), "picture");
  item->GetPictureInfoTag()->SetTitle(record->at(picture_strTitle).get_asString());
  item->SetLabel(record->at(picture_strTitle).get_asString());
  CStdString strRealPath = URIUtils::AddFileToFolder(record->at(picture_strPath).get_asString(), record->at(picture_strFileName).get_asString());
  item->GetPictureInfoTag()->SetURL(strRealPath);
//  item->GetPictureInfoTag()->SetAlbumFace(record->at(picture_strAlbumFaces).get_asString());
  item->GetPictureInfoTag()->SetLoaded(true);
  // Get filename with full path
  if (strPictureDBbasePath.IsEmpty())
    item->SetPath(strRealPath);
  else
  {
    CPictureDbUrl itemUrl;
    if (!itemUrl.FromString(strPictureDBbasePath))
      return;
    
    CStdString strFileName = record->at(picture_strFileName).get_asString();
    CStdString strExt = URIUtils::GetExtension(strFileName);
    CStdString path; path.Format("%ld%s", record->at(picture_idPicture).get_asInt(), strExt.c_str());
    itemUrl.AppendPath(path);
    item->SetPath(itemUrl.ToString());
  }
}

CPictureAlbum CPictureDatabase::GetPictureAlbumFromDataset(dbiplus::Dataset* pDS, bool imageURL /* = false*/)
{
  return GetPictureAlbumFromDataset(pDS->get_sql_record(), imageURL);
}

CPictureAlbum CPictureDatabase::GetPictureAlbumFromDataset(const dbiplus::sql_record* const record, bool imageURL /* = false*/)
{
  CPictureAlbum album;
  
  album.idAlbum = record->at(album_idAlbum).get_asInt();
  album.strAlbum = record->at(album_strAlbum).get_asString();
  if (album.strAlbum.IsEmpty())
    album.strAlbum = g_localizeStrings.Get(1050);
  album.face = StringUtils::Split(record->at(album_strFaces).get_asString(), g_advancedSettings.m_pictureItemSeparator);
  album.location = StringUtils::Split(record->at(album_strLocations).get_asString(), g_advancedSettings.m_pictureItemSeparator);
  if (imageURL)
    album.thumbURL.ParseString(record->at(album_strThumbURL).get_asString());
//  album.iPictureCount = record->at(album_iPictureCount).get_asInt();
  
  return album;
}

CFaceCredit CPictureDatabase::GetPictureAlbumFaceCreditFromDataset(const dbiplus::sql_record* const record)
{
  CFaceCredit FaceCredit;
  /*
   FaceCredit.idFace = record->at(album_idFace).get_asInt();
   FaceCredit.m_strFace = record->at(album_strFace).get_asString();
   FaceCredit.m_strPictureBrainzFaceID = record->at(album_strPictureBrainzFaceID).get_asString();
   FaceCredit.m_boolFeatured = record->at(album_bFeatured).get_asBool();
   FaceCredit.m_strJoinPhrase = record->at(album_strJoinPhrase).get_asString();
   */
  return FaceCredit;
}

CFace CPictureDatabase::GetFaceFromDataset(dbiplus::Dataset* pDS, bool needThumb)
{
  return GetFaceFromDataset(pDS->get_sql_record(), needThumb);
}

CFace CPictureDatabase::GetFaceFromDataset(const dbiplus::sql_record* const record, bool needThumb /* = true */)
{
  CFace Face;
  Face.idFace = record->at(face_idFace).get_asInt();
  Face.strFace = record->at(face_strFace).get_asString();
  
  Face.location = StringUtils::Split(record->at(face_strLocations).get_asString(), g_advancedSettings.m_pictureItemSeparator);
  Face.strBiography = record->at(face_strBiography).get_asString();
  Face.styles = StringUtils::Split(record->at(face_strStyles).get_asString(), g_advancedSettings.m_pictureItemSeparator);
  Face.moods = StringUtils::Split(record->at(face_strMoods).get_asString(), g_advancedSettings.m_pictureItemSeparator);
  Face.strBorn = record->at(face_strBorn).get_asString();
  Face.strFormed = record->at(face_strFormed).get_asString();
  Face.strDied = record->at(face_strDied).get_asString();
  Face.strDisbanded = record->at(face_strDisbanded).get_asString();
  Face.yearsActive = StringUtils::Split(record->at(face_strYearsActive).get_asString(), g_advancedSettings.m_pictureItemSeparator);
  Face.instruments = StringUtils::Split(record->at(face_strInstruments).get_asString(), g_advancedSettings.m_pictureItemSeparator);
  
  if (needThumb)
  {
    Face.fanart.m_xml = record->at(face_strFanart).get_asString();
    Face.fanart.Unpack();
    Face.thumbURL.ParseString(record->at(face_strImage).get_asString());
  }
  
  return Face;
}

bool CPictureDatabase::GetPictureByFileName(const CStdString& strFileName, CPicture& picture, int startOffset)
{
  try
  {
    picture.Clear();
    CURL url(strFileName);
    
    if (url.GetProtocol()=="Picturedb")
    {
      CStdString strFile = URIUtils::GetFileName(strFileName);
      URIUtils::RemoveExtension(strFile);
      return GetPicture(atol(strFile.c_str()), picture);
    }
    
    CStdString strPath;
    URIUtils::GetDirectory(strFileName, strPath);
    URIUtils::AddSlashAtEnd(strPath);
    
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    DWORD crc = ComputeCRC(strFileName);
    
    CStdString strSQL=PrepareSQL("select * from pictureview "
                                 "where dwFileNameCRC='%ul' and strPath='%s'"
                                 , crc,
                                 strPath.c_str());
    if (startOffset)
      strSQL += PrepareSQL(" AND iStartOffset=%i", startOffset);
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    picture = GetPictureFromDataset();
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strFileName.c_str());
  }
  
  return false;
}

int CPictureDatabase::GetPictureAlbumIdByPath(const CStdString& strPath)
{
  try
  {
    CStdString strSQL=PrepareSQL("select distinct idAlbum from picture join path on picture.idPath = path.idPath where path.strPath='%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->eof())
      return -1;
    
    int idAlbum = m_pDS->fv(0).get_asInt();
    m_pDS->close();
    
    return idAlbum;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strPath.c_str());
  }
  
  return false;
}

int CPictureDatabase::GetPictureByFaceAndPictureAlbumAndTitle(const CStdString& strFace, const CStdString& strAlbum, const CStdString& strTitle)
{
  try
  {
    CStdString strSQL=PrepareSQL("select idPicture from pictureview "
                                 "where strFace like '%s' and strAlbum like '%s' and "
                                 "strTitle like '%s'",strFace.c_str(),strAlbum.c_str(),strTitle.c_str());
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return -1;
    }
    int lResult = m_pDS->fv(0).get_asInt();
    m_pDS->close(); // cleanup recordset data
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%s,%s) failed", __FUNCTION__, strFace.c_str(),strAlbum.c_str(),strTitle.c_str());
  }
  
  return -1;
}

bool CPictureDatabase::GetPicture(int idPicture, CPicture& picture)
{
  try
  {
    picture.Clear();
    
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL=PrepareSQL("select * from pictureview "
                                 "where idPicture=%i"
                                 , idPicture);
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    picture = GetPictureFromDataset();
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idPicture);
  }
  
  return false;
}

bool CPictureDatabase::SearchFaces(const CStdString& search, CFileItemList &Faces)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strVariousFaces = g_localizeStrings.Get(340).c_str();
    CStdString strSQL;
    if (search.GetLength() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from Face "
                        "where (strFace like '%s%%' or strFace like '%% %s%%') and strFace <> '%s' "
                        , search.c_str(), search.c_str(), strVariousFaces.c_str() );
    else
      strSQL=PrepareSQL("select * from Face "
                        "where strFace like '%s%%' and strFace <> '%s' "
                        , search.c_str(), strVariousFaces.c_str() );
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    
    CStdString FaceLabel(g_localizeStrings.Get(557)); // Face
    while (!m_pDS->eof())
    {
      CStdString path;
      path.Format("Picturedb://Faces/%ld/", m_pDS->fv(0).get_asInt());
      CFileItemPtr pItem(new CFileItem(path, true));
      CStdString label;
      label.Format("[%s] %s", FaceLabel.c_str(), m_pDS->fv(1).get_asString());
      pItem->SetLabel(label);
      label.Format("A %s", m_pDS->fv(1).get_asString()); // sort label is stored in the title tag
      //            pItem->GetPictureInfoTag()->SetTitle(label);
      //            pItem->GetPictureInfoTag()->SetDatabaseId(m_pDS->fv(0).get_asInt(), "Face");
      Faces.Add(pItem);
      m_pDS->next();
    }
    
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CPictureDatabase::GetPictureAlbumInfo(int idAlbum, CPictureAlbum &info, VECPICTURES* pictures, bool scrapedInfo /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;
    
    if (idAlbum == -1)
      return false; // not in the database
    
    CStdString strSQL=PrepareSQL("SELECT albumview.*, album_Face.idFace, Face.strFace, album_Face.boolFeatured, album_Face.strJoinPhrase FROM albumview JOIN album_Face ON albumview.idAlbum = album_Face.idAlbum JOIN Face ON album_Face.idFace = Face.idFace WHERE albumview.idAlbum = %ld", idAlbum);
    if (scrapedInfo) // require additional information
      strSQL += " and idAlbumInfo > 0";
    
    if (!m_pDS2->query(strSQL.c_str())) return false;
    if (m_pDS2->num_rows() == 0)
    {
      m_pDS2->close();
      return false;
    }
    
    info = GetPictureAlbumFromDataset(m_pDS2.get()->get_sql_record(), true); // true to grab the thumburl rather than the thumb
    /*
     int idAlbumInfo = m_pDS2->fv(album_idAlbumInfo).get_asInt();
     while (!m_pDS2->eof())
     {
     if (!info.FaceCredits.empty() && (m_pDS2->fv(album_idFace).get_asInt() != info.FaceCredits.back().idFace))
     {
     info.FaceCredits.push_back(GetPictureAlbumFaceCreditFromDataset(m_pDS2.get()->get_sql_record()));
     }
     m_pDS2->next();
     }
     
     if (pictures)
     GetPictureAlbumInfoPictures(idAlbumInfo, *pictures);
     */
    m_pDS2->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  
  return false;
}

bool CPictureDatabase::HasPictureAlbumInfo(int idAlbum)
{
  try
  {
    if (idAlbum == -1)
      return false; // not in the database
    
    CStdString strSQL=PrepareSQL("select * from albuminfo where idAlbum = %ld", idAlbum);
    
    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    m_pDS2->close();
    return iRowsFound > 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  
  return false;
}

bool CPictureDatabase::DeletePictureAlbumInfo(int idAlbum)
{
  if (idAlbum == -1)
    return false; // not in the database
  return ExecuteQuery(PrepareSQL("delete from albuminfo where idAlbum=%i",idAlbum));
}

bool CPictureDatabase::GetFaceInfo(int idFace, CFace &info, bool needAll)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;
    
    if (idFace == -1)
      return false; // not in the database
    
    CStdString strSQL=PrepareSQL("SELECT * FROM Faceview WHERE idFace = %i", idFace);
    
    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound != 0)
    {
      info = GetFaceFromDataset(m_pDS2.get(),needAll);
      if (needAll)
      {
        strSQL=PrepareSQL("select * from discography where idFace=%i",idFace);
        m_pDS2->query(strSQL.c_str());
        while (!m_pDS2->eof())
        {
          info.discography.push_back(make_pair(m_pDS2->fv("strAlbum").get_asString(),m_pDS2->fv("strYear").get_asString()));
          m_pDS2->next();
        }
      }
      m_pDS2->close(); // cleanup recordset data
      return true;
    }
    m_pDS2->close();
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - (%i) failed", __FUNCTION__, idFace);
  }
  
  return false;
}

bool CPictureDatabase::HasFaceInfo(int idFace)
{
  return strtol(GetSingleValue("Faceinfo", "count(idFace)", PrepareSQL("idFace = %ld", idFace)), NULL, 10) > 0;
}

bool CPictureDatabase::DeleteFaceInfo(int idFace)
{
  if (idFace == -1)
    return false; // not in the database
  
  return ExecuteQuery(PrepareSQL("delete from Faceinfo where idFace=%i",idFace));
}

bool CPictureDatabase::GetPictureAlbumInfoPictures(int idAlbumInfo, VECPICTURES& pictures)
{
  try
  {
    CStdString strSQL=PrepareSQL("select * from albuminfopicture "
                                 "where idAlbumInfo=%i "
                                 "order by iTrack", idAlbumInfo);
    
    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound == 0) return false;
    while (!m_pDS2->eof())
    {
      CPicture picture;
      
      pictures.push_back(picture);
      m_pDS2->next();
    }
    
    m_pDS2->close(); // cleanup recordset data
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbumInfo);
  }
  
  return false;
}

bool CPictureDatabase::GetTop100(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL="select * from pictureview "
    "where iPictureCount>0 "
    "order by iPictureCount desc "
    "limit 100";
    
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), strBaseDir);
      items.Add(item);
      m_pDS->next();
    }
    
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CPictureDatabase::GetTop100PictureAlbums(VECPICTUREALBUMS& albums)
{
  try
  {
    albums.erase(albums.begin(), albums.end());
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    // NOTE: The picture.idAlbum is needed for the group by, as for some reason group by albumview.idAlbum doesn't work
    //       consistently - possibly an SQLite bug, as it works fine in SQLiteSpy (v3.3.17)
    CStdString strSQL = "select albumview.* from albumview "
    "where albumview.iPictureCount>0 and albumview.strAlbum != '' "
    "order by albumview.iPictureCount desc "
    "limit 100 ";
    
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    while (!m_pDS->eof())
    {
      albums.push_back(GetPictureAlbumFromDataset(m_pDS.get()));
      m_pDS->next();
    }
    
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CPictureDatabase::GetTop100PictureAlbumPictures(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    strSQL.Format("select * from pictureview join albumview on (pictureview.idAlbum = albumview.idAlbum) where albumview.idAlbum in (select picture.idAlbum from picture where picture.iPictureCount>0 group by idAlbum order by sum(picture.iPictureCount) desc limit 100) order by albumview.idAlbum in (select picture.idAlbum from picture where picture.iPictureCount>0 group by idAlbum order by sum(picture.iPictureCount) desc limit 100)");
    CLog::Log(LOGDEBUG,"GetTop100PictureAlbumPictures() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    
    // get data from returned rows
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), strBaseDir);
      items.Add(item);
      m_pDS->next();
    }
    
    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CPictureDatabase::GetRecentlyPlayedPictureAlbums(VECPICTUREALBUMS& albums)
{
  try
  {
    albums.erase(albums.begin(), albums.end());
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    strSQL.Format("select distinct albumview.* from picture join albumview on albumview.idAlbum=picture.idAlbum where picture.takenon IS NOT NULL order by picture.takenon desc limit %i", RECENTLY_PLAYED_LIMIT);
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    while (!m_pDS->eof())
    {
      albums.push_back(GetPictureAlbumFromDataset(m_pDS.get()));
      m_pDS->next();
    }
    
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CPictureDatabase::GetRecentlyPlayedPictureAlbumPictures(const CStdString& strBaseDir, CFileItemList& items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    strSQL.Format("select * from pictureview join albumview on (pictureview.idAlbum = albumview.idAlbum) where albumview.idAlbum in (select distinct albumview.idAlbum from albumview join picture on albumview.idAlbum=picture.idAlbum where picture.takenon IS NOT NULL order by picture.takenon desc limit %i)", g_advancedSettings.m_iPictureLibraryRecentlyAddedItems);
    CLog::Log(LOGDEBUG,"GetRecentlyPlayedPictureAlbumPictures() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    
    // get data from returned rows
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), strBaseDir);
      items.Add(item);
      m_pDS->next();
    }
    
    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CPictureDatabase::GetRecentlyAddedPictureAlbums(VECPICTUREALBUMS& albums, unsigned int limit)
{
  try
  {
    albums.erase(albums.begin(), albums.end());
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    strSQL.Format("select * from albumview where strAlbum != '' order by idAlbum desc limit %u", limit ? limit : g_advancedSettings.m_iPictureLibraryRecentlyAddedItems);
    
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    
    while (!m_pDS->eof())
    {
      albums.push_back(GetPictureAlbumFromDataset(m_pDS.get()));
      m_pDS->next();
    }
    
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CPictureDatabase::GetRecentlyAddedPictureAlbumPictures(const CStdString& strBaseDir, CFileItemList& items, unsigned int limit)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    strSQL = PrepareSQL("SELECT pictureview.* FROM (SELECT idAlbum FROM albumview ORDER BY idAlbum DESC LIMIT %u) AS recentalbums JOIN pictureview ON pictureview.idAlbum=recentalbums.idAlbum", limit ? limit : g_advancedSettings.m_iPictureLibraryRecentlyAddedItems);
    CLog::Log(LOGDEBUG,"GetRecentlyAddedPictureAlbumPictures() query: %s", strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    
    // get data from returned rows
    items.Reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), strBaseDir);
      items.Add(item);
      m_pDS->next();
    }
    
    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

void CPictureDatabase::IncrementPlayCount(const CFileItem& item)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    
    int idPicture = GetPictureIDFromPath(item.GetPath());
    
    CStdString sql=PrepareSQL("UPDATE picture SET iPictureCount=iPictureCount+1, takenon=CURRENT_TIMESTAMP where idPicture=%i", idPicture);
    m_pDS->exec(sql.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, item.GetPath().c_str());
  }
}

bool CPictureDatabase::GetPicturesByPath(const CStdString& strPath1, MAPPICTURES& pictures, bool bAppendToMap)
{
  CStdString strPath(strPath1);
  try
  {
    if (!URIUtils::HasSlashAtEnd(strPath))
      URIUtils::AddSlashAtEnd(strPath);
    
    if (!bAppendToMap)
      pictures.clear();
    
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL=PrepareSQL("select * from pictureview where strPath='%s'", strPath.c_str() );
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    while (!m_pDS->eof())
    {
      CPicture picture = GetPictureFromDataset();
      pictures.insert(make_pair(picture.strFileName, picture));
      m_pDS->next();
    }
    
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strPath.c_str());
  }
  
  return false;
}

void CPictureDatabase::EmptyCache()
{
  m_FaceCache.erase(m_FaceCache.begin(), m_FaceCache.end());
  m_locationCache.erase(m_locationCache.begin(), m_locationCache.end());
  m_pathCache.erase(m_pathCache.begin(), m_pathCache.end());
  m_albumCache.erase(m_albumCache.begin(), m_albumCache.end());
  m_thumbCache.erase(m_thumbCache.begin(), m_thumbCache.end());
}

bool CPictureDatabase::Search(const CStdString& search, CFileItemList &items)
{
  unsigned int time = XbmcThreads::SystemClockMillis();
  // first grab all the Faces that match
  SearchFaces(search, items);
  CLog::Log(LOGDEBUG, "%s Face search in %i ms",
            __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();
  
  // then albums that match
  SearchPictureAlbums(search, items);
  CLog::Log(LOGDEBUG, "%s PictureAlbum search in %i ms",
            __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();
  
  // and finally pictures
  SearchPictures(search, items);
  CLog::Log(LOGDEBUG, "%s Pictures search in %i ms",
            __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();
  return true;
}

bool CPictureDatabase::SearchPictures(const CStdString& search, CFileItemList &items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    if (search.GetLength() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from pictureview where strTitle like '%s%%' or strTitle like '%% %s%%' limit 1000", search.c_str(), search.c_str());
    else
      strSQL=PrepareSQL("select * from pictureview where strTitle like '%s%%' limit 1000", search.c_str());
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0) return false;
    
    CStdString pictureLabel = g_localizeStrings.Get(179); // Picture
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), "Picturedb://pictures/");
      items.Add(item);
      m_pDS->next();
    }
    
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CPictureDatabase::SearchPictureAlbums(const CStdString& search, CFileItemList &albums)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    if (search.GetLength() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from albumview where strAlbum like '%s%%' or strAlbum like '%% %s%%'", search.c_str(), search.c_str());
    else
      strSQL=PrepareSQL("select * from albumview where strAlbum like '%s%%'", search.c_str());
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    
    CStdString albumLabel(g_localizeStrings.Get(558)); // PictureAlbum
    while (!m_pDS->eof())
    {
      CPictureAlbum album = GetPictureAlbumFromDataset(m_pDS.get());
      CStdString path;
      path.Format("Picturedb://albums/%ld/", album.idAlbum);
      /*
       CFileItemPtr pItem(new CFileItem(path, album));
       CStdString label;
       label.Format("[%s] %s", albumLabel.c_str(), album.strAlbum);
       pItem->SetLabel(label);
       label.Format("B %s", album.strAlbum); // sort label is stored in the title tag
       pItem->GetPictureInfoTag()->SetTitle(label);
       albums.Add(pItem);
       */
      m_pDS->next();
    }
    m_pDS->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

int CPictureDatabase::SetPictureAlbumInfo(int idAlbum, const CPictureAlbum& album, const VECPICTURES& pictures, bool bTransaction)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    if (bTransaction)
      BeginTransaction();
    
    // delete any album info we may have
    strSQL=PrepareSQL("delete from albuminfo where idAlbum=%i", idAlbum);
    m_pDS->exec(strSQL.c_str());
    
    // insert the albuminfo
    strSQL=PrepareSQL("insert into albuminfo (idAlbumInfo,idAlbum,strImage,strLabel) values(NULL,%i,'%s','%s')",
                      idAlbum,
                      album.thumbURL.m_xml.c_str(),
                      album.strLabel.c_str());
    m_pDS->exec(strSQL.c_str());
    int idAlbumInfo = (int)m_pDS->lastinsertid();
    
    if (SetPictureAlbumInfoPictures(idAlbumInfo, pictures))
    {
      if (bTransaction)
        CommitTransaction();
    }
    else
    {
      if (bTransaction) // icky
        RollbackTransaction();
      idAlbumInfo = -1;
    }
    
    return idAlbumInfo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
  }
  
  if (bTransaction)
    RollbackTransaction();
  
  return -1;
}

int CPictureDatabase::SetFaceInfo(int idFace, const CFace& Face)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    // delete any Face info we may have
    strSQL=PrepareSQL("delete from Faceinfo where idFace=%i", idFace);
    m_pDS->exec(strSQL.c_str());
    strSQL=PrepareSQL("delete from discography where idFace=%i", idFace);
    m_pDS->exec(strSQL.c_str());
    
    // insert the Faceinfo
    strSQL=PrepareSQL("insert into Faceinfo (idFaceInfo,idFace,strBorn,strFormed,strLocations,strMoods,strStyles,strInstruments,strBiography,strDied,strDisbanded,strYearsActive,strImage,strFanart) values(NULL,%i,'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')",
                      idFace, Face.strBorn.c_str(),
                      Face.strFormed.c_str(),
                      StringUtils::Join(Face.location, g_advancedSettings.m_pictureItemSeparator).c_str(),
                      StringUtils::Join(Face.moods, g_advancedSettings.m_pictureItemSeparator).c_str(),
                      StringUtils::Join(Face.styles, g_advancedSettings.m_pictureItemSeparator).c_str(),
                      StringUtils::Join(Face.instruments, g_advancedSettings.m_pictureItemSeparator).c_str(),
                      Face.strBiography.c_str(),
                      Face.strDied.c_str(),
                      Face.strDisbanded.c_str(),
                      StringUtils::Join(Face.yearsActive, g_advancedSettings.m_pictureItemSeparator).c_str(),
                      Face.thumbURL.m_xml.c_str(),
                      Face.fanart.m_xml.c_str());
    m_pDS->exec(strSQL.c_str());
    int idFaceInfo = (int)m_pDS->lastinsertid();
    for (unsigned int i=0;i<Face.discography.size();++i)
    {
      strSQL=PrepareSQL("insert into discography (idFace,strAlbum,strYear) values (%i,'%s','%s')",idFace,Face.discography[i].first.c_str(),Face.discography[i].second.c_str());
      m_pDS->exec(strSQL.c_str());
    }
    
    return idFaceInfo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s -  failed with query (%s)", __FUNCTION__, strSQL.c_str());
  }
  
  
  return -1;
}

bool CPictureDatabase::SetPictureAlbumInfoPictures(int idAlbumInfo, const VECPICTURES& pictures)
{
  CStdString strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    strSQL=PrepareSQL("delete from albuminfopicture where idAlbumInfo=%i", idAlbumInfo);
    m_pDS->exec(strSQL.c_str());
    
    for (int i = 0; i < (int)pictures.size(); i++)
    {
      CPicture picture = pictures[i];
      strSQL=PrepareSQL("insert into albuminfopicture (idAlbumInfoPicture,idAlbumInfo,strTitle,iDuration) values(NULL,%i,'%s')",
                        idAlbumInfo,
                        picture.strTitle.c_str());
      m_pDS->exec(strSQL.c_str());
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed with query (%s)", __FUNCTION__, strSQL.c_str());
  }
  
  return false;
}

bool CPictureDatabase::CleanupPicturesByIds(const CStdString &strPictureIds)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now find all idPicture's
    CStdString strSQL=PrepareSQL("select * from picture join path on picture.idPath = path.idPath where picture.idPicture in %s", strPictureIds.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    CStdString strPicturesToDelete = "";
    while (!m_pDS->eof())
    { // get the full picture path
      CStdString strFileName = URIUtils::AddFileToFolder(m_pDS->fv("path.strPath").get_asString(), m_pDS->fv("picture.strFileName").get_asString());
      
      //  Special case for streams inside an ogg file. (oggstream)
      //  The last dir in the path is the ogg file that
      //  contains the stream, so test if its there
      if (URIUtils::HasExtension(strFileName, ".oggstream|.nsfstream"))
      {
        CStdString strFileAndPath=strFileName;
        URIUtils::GetDirectory(strFileAndPath, strFileName);
        // we are dropping back to a file, so remove the slash at end
        URIUtils::RemoveSlashAtEnd(strFileName);
      }
      
      if (!CFile::Exists(strFileName))
      { // file no longer exists, so add to deletion list
        strPicturesToDelete += m_pDS->fv("picture.idPicture").get_asString() + ",";
      }
      m_pDS->next();
    }
    m_pDS->close();
    
    if ( ! strPicturesToDelete.IsEmpty() )
    {
      strPicturesToDelete = "(" + strPicturesToDelete.TrimRight(",") + ")";
      // ok, now delete these pictures + all references to them from the linked tables
      strSQL = "delete from picture where idPicture in " + strPicturesToDelete;
      m_pDS->exec(strSQL.c_str());
      strSQL = "delete from picture_Face where idPicture in " + strPicturesToDelete;
      m_pDS->exec(strSQL.c_str());
      strSQL = "delete from picture_location where idPicture in " + strPicturesToDelete;
      m_pDS->exec(strSQL.c_str());
      strSQL = "delete from karaokedata where idPicture in " + strPicturesToDelete;
      m_pDS->exec(strSQL.c_str());
      m_pDS->close();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CPictureDatabase::CleanupPicturesFromPaths()");
  }
  return false;
}

bool CPictureDatabase::CleanupPictures()
{
  try
  {
    // run through all pictures and get all unique path ids
    int iLIMIT = 1000;
    for (int i=0;;i+=iLIMIT)
    {
      CStdString strSQL=PrepareSQL("select picture.idPicture from picture order by picture.idPicture limit %i offset %i",iLIMIT,i);
      if (!m_pDS->query(strSQL.c_str())) return false;
      int iRowsFound = m_pDS->num_rows();
      // keep going until no rows are left!
      if (iRowsFound == 0)
      {
        m_pDS->close();
        return true;
      }
      CStdString strPictureIds = "(";
      while (!m_pDS->eof())
      {
        strPictureIds += m_pDS->fv("picture.idPicture").get_asString() + ",";
        m_pDS->next();
      }
      m_pDS->close();
      strPictureIds.TrimRight(",");
      strPictureIds += ")";
      CLog::Log(LOGDEBUG,"Checking pictures from picture ID list: %s",strPictureIds.c_str());
      if (!CleanupPicturesByIds(strPictureIds)) return false;
    }
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "Exception in CPictureDatabase::CleanupPictures()");
  }
  return false;
}

bool CPictureDatabase::CleanupPictureAlbums()
{
  try
  {
    // This must be run AFTER pictures have been cleaned up
    // delete albums with no reference to pictures
    CStdString strSQL = "select * from album where album.idAlbum not in (select idAlbum from picture)";
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    CStdString strAlbumIds = "(";
    while (!m_pDS->eof())
    {
      strAlbumIds += m_pDS->fv("album.idAlbum").get_asString() + ",";
      m_pDS->next();
    }
    m_pDS->close();
    
    strAlbumIds.TrimRight(",");
    strAlbumIds += ")";
    // ok, now we can delete them and the references in the linked tables
    strSQL = "delete from album where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from album_Face where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from album_location where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    strSQL = "delete from albuminfo where idAlbum in " + strAlbumIds;
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CPictureDatabase::CleanupPictureAlbums()");
  }
  return false;
}

bool CPictureDatabase::CleanupPaths()
{
  try
  {
    // needs to be done AFTER the pictures and albums have been cleaned up.
    // we can happily delete any path that has no reference to a picture
    // but we must keep all paths that have been scanned that may contain pictures in subpaths
    
    // first create a temporary table of picture paths
    m_pDS->exec("CREATE TEMPORARY TABLE picturepaths (idPath integer, strPath varchar(512))\n");
    m_pDS->exec("INSERT INTO picturepaths select idPath,strPath from path where idPath in (select idPath from picture)\n");
    
    // grab all paths that aren't immediately connected with a picture
    CStdString sql = "select * from path where idPath not in (select idPath from picture)";
    if (!m_pDS->query(sql.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    // and construct a list to delete
    CStdString deleteSQL;
    while (!m_pDS->eof())
    {
      // anything that isn't a parent path of a picture path is to be deleted
      CStdString path = m_pDS->fv("strPath").get_asString();
      CStdString sql = PrepareSQL("select count(idPath) from picturepaths where SUBSTR(strPath,1,%i)='%s'", StringUtils::utf8_strlen(path.c_str()), path.c_str());
      if (m_pDS2->query(sql.c_str()) && m_pDS2->num_rows() == 1 && m_pDS2->fv(0).get_asInt() == 0)
        deleteSQL += PrepareSQL("%i,", m_pDS->fv("idPath").get_asInt()); // nothing found, so delete
      m_pDS2->close();
      m_pDS->next();
    }
    m_pDS->close();
    
    if ( ! deleteSQL.IsEmpty() )
    {
      deleteSQL = "DELETE FROM path WHERE idPath IN (" + deleteSQL.TrimRight(',') + ")";
      // do the deletion, and drop our temp table
      m_pDS->exec(deleteSQL.c_str());
    }
    m_pDS->exec("drop table picturepaths");
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CPictureDatabase::CleanupPaths() or was aborted");
  }
  return false;
}

bool CPictureDatabase::InsideScannedPath(const CStdString& path)
{
  CStdString sql = PrepareSQL("select idPath from path where SUBSTR(strPath,1,%i)='%s' LIMIT 1", path.size(), path.c_str());
  return !GetSingleValue(sql).empty();
}

bool CPictureDatabase::CleanupFaces()
{
  try
  {
    // (nested queries by Bobbin007)
    // must be executed AFTER the picture, album and their Face link tables are cleaned.
    // don't delete the "Various Faces" string
    CStdString strSQL = "delete from Face where idFace not in (select idFace from picture_Face)";
    strSQL += " and idFace not in (select idFace from album_Face)";
    CStdString strSQL2;
    m_pDS->exec(strSQL.c_str());
    m_pDS->exec("delete from Faceinfo where idFace not in (select idFace from Face)");
    m_pDS->exec("delete from album_Face where idFace not in (select idFace from Face)");
    m_pDS->exec("delete from picture_Face where idFace not in (select idFace from Face)");
    m_pDS->exec("delete from discography where idFace not in (select idFace from Face)");
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CPictureDatabase::CleanupFaces() or was aborted");
  }
  return false;
}

bool CPictureDatabase::CleanupLocations()
{
  try
  {
    // Cleanup orphaned locations (ie those that don't belong to a picture or an albuminfo entry)
    // (nested queries by Bobbin007)
    // Must be executed AFTER the picture, picture_location, albuminfo and album_location tables have been cleaned.
    CStdString strSQL = "delete from location where idLocation not in (select idLocation from picture_location) and";
    strSQL += " idLocation not in (select idLocation from album_location)";
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CPictureDatabase::CleanupLocations() or was aborted");
  }
  return false;
}

bool CPictureDatabase::CleanupOrphanedItems()
{
  // paths aren't cleaned up here - they're cleaned up in RemovePicturesFromPath()
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (!CleanupPictureAlbums()) return false;
  if (!CleanupFaces()) return false;
  if (!CleanupLocations()) return false;
  return true;
}

int CPictureDatabase::Cleanup(CGUIDialogProgress *pDlgProgress)
{
  if (NULL == m_pDB.get()) return ERROR_DATABASE;
  if (NULL == m_pDS.get()) return ERROR_DATABASE;
  
  int ret = ERROR_OK;
  unsigned int time = XbmcThreads::SystemClockMillis();
  CLog::Log(LOGNOTICE, "%s: Starting Picturedatabase cleanup ..", __FUNCTION__);
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanStarted");
  
  // first cleanup any pictures with invalid paths
  if (pDlgProgress)
  {
    pDlgProgress->SetHeading(700);
    pDlgProgress->SetLine(0, "");
    pDlgProgress->SetLine(1, 318);
    pDlgProgress->SetLine(2, 330);
    pDlgProgress->SetPercentage(0);
    pDlgProgress->StartModal();
    pDlgProgress->ShowProgressBar(true);
  }
  if (!CleanupPictures())
  {
    ret = ERROR_REORG_PICTURES;
    goto error;
  }
  // then the albums that are not linked to a picture or to albuminfo, or whose path is removed
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 326);
    pDlgProgress->SetPercentage(20);
    pDlgProgress->Progress();
  }
  if (!CleanupPictureAlbums())
  {
    ret = ERROR_REORG_PICTURE_ALBUM;
    goto error;
  }
  // now the paths
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 324);
    pDlgProgress->SetPercentage(40);
    pDlgProgress->Progress();
  }
  if (!CleanupPaths())
  {
    ret = ERROR_REORG_PATH;
    goto error;
  }
  // and finally Faces + locations
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 320);
    pDlgProgress->SetPercentage(60);
    pDlgProgress->Progress();
  }
  if (!CleanupFaces())
  {
    ret = ERROR_REORG_FACE;
    goto error;
  }
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 322);
    pDlgProgress->SetPercentage(80);
    pDlgProgress->Progress();
  }
  if (!CleanupLocations())
  {
    ret = ERROR_REORG_LOCATION;
    goto error;
  }
  // commit transaction
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 328);
    pDlgProgress->SetPercentage(90);
    pDlgProgress->Progress();
  }
  if (!CommitTransaction())
  {
    ret = ERROR_WRITING_CHANGES;
    goto error;
  }
  // and compress the database
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(1, 331);
    pDlgProgress->SetPercentage(100);
    pDlgProgress->Progress();
  }
  time = XbmcThreads::SystemClockMillis() - time;
  CLog::Log(LOGNOTICE, "%s: Cleaning Picturedatabase done. Operation took %s", __FUNCTION__, StringUtils::SecondsToTimeString(time / 1000).c_str());
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");
  
  if (!Compress(false))
  {
    return ERROR_COMPRESSING;
  }
  return ERROR_OK;
  
error:
  RollbackTransaction();
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");
  return ret;
}

void CPictureDatabase::DeletePictureAlbumInfo()
{
  // open our database
  Open();
  if (NULL == m_pDB.get()) return ;
  if (NULL == m_pDS.get()) return ;
  
  // If we are scanning for Picture info in the background,
  // other writing access to the database is prohibited.
  if (g_application.IsPictureScanning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }
  
  CStdString strSQL="select * from albuminfo,album,Face where and albuminfo.idAlbum=album.idAlbum and album.idFace=Face.idFace order by album.strAlbum";
  if (!m_pDS->query(strSQL.c_str())) return ;
  int iRowsFound = m_pDS->num_rows();
  if (iRowsFound == 0)
  {
    m_pDS->close();
    CGUIDialogOK::ShowAndGetInput(313, 425, 0, 0);
  }
  vector<CPictureAlbum> vecPictureAlbums;
  while (!m_pDS->eof())
  {
    CPictureAlbum album;
    album.idAlbum = m_pDS->fv("album.idAlbum").get_asInt() ;
    album.strAlbum = m_pDS->fv("album.strAlbum").get_asString();
    album.face = StringUtils::Split(m_pDS->fv("album.strFaces").get_asString(), g_advancedSettings.m_pictureItemSeparator);
    vecPictureAlbums.push_back(album);
    m_pDS->next();
  }
  m_pDS->close();
  
  // Show a selectdialog that the user can select the albuminfo to delete
  CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
    pDlg->SetHeading(g_localizeStrings.Get(181).c_str());
    pDlg->Reset();
    for (int i = 0; i < (int)vecPictureAlbums.size(); ++i)
    {
      CPictureAlbum& album = vecPictureAlbums[i];
      pDlg->Add(album.strAlbum + " - " + StringUtils::Join(album.face, g_advancedSettings.m_pictureItemSeparator));
    }
    pDlg->DoModal();
    
    // and wait till user selects one
    int iSelectedPictureAlbum = pDlg->GetSelectedLabel();
    if (iSelectedPictureAlbum < 0)
    {
      vecPictureAlbums.erase(vecPictureAlbums.begin(), vecPictureAlbums.end());
      return ;
    }
    
    CPictureAlbum& album = vecPictureAlbums[iSelectedPictureAlbum];
    strSQL=PrepareSQL("delete from albuminfo where albuminfo.idAlbum=%i", album.idAlbum);
    if (!m_pDS->exec(strSQL.c_str())) return ;
    
    vecPictureAlbums.erase(vecPictureAlbums.begin(), vecPictureAlbums.end());
  }
}

bool CPictureDatabase::LookupCDDBInfo(bool bRequery/*=false*/)
{
#ifdef HAS_DVD_DRIVE
  if (!CSettings::Get().GetBool("audiocds.usecddb"))
    return false;
  
  // check network connectivity
  if (!g_application.getNetwork().IsAvailable())
    return false;
  
  // Get information for the inserted disc
  CCdInfo* pCdInfo = g_mediaManager.GetCdInfo();
  if (pCdInfo == NULL)
    return false;
  
  // If the disc has no tracks, we are finished here.
  int nTracks = pCdInfo->GetTrackCount();
  if (nTracks <= 0)
    return false;
  
  //  Delete old info if any
  if (bRequery)
  {
    CStdString strFile;
    strFile.Format("%x.cddb", pCdInfo->GetCddbDiscId());
    CFile::Delete(URIUtils::AddFileToFolder(CProfilesManager::Get().GetCDDBFolder(), strFile));
  }
  
  // Prepare cddb
  Xcddb cddb;
  cddb.setCacheDir(CProfilesManager::Get().GetCDDBFolder());
  
  // Do we have to look for cddb information
  if (pCdInfo->HasCDDBInfo() && !cddb.isCDCached(pCdInfo))
  {
    CGUIDialogProgress* pDialogProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    
    if (!pDialogProgress) return false;
    if (!pDlgSelect) return false;
    
    // Show progress dialog if we have to connect to freedb.org
    pDialogProgress->SetHeading(255); //CDDB
    pDialogProgress->SetLine(0, ""); // Querying freedb for CDDB info
    pDialogProgress->SetLine(1, 256);
    pDialogProgress->SetLine(2, "");
    pDialogProgress->ShowProgressBar(false);
    pDialogProgress->StartModal();
    
    // get cddb information
    if (!cddb.queryCDinfo(pCdInfo))
    {
      pDialogProgress->Close();
      int lasterror = cddb.getLastError();
      
      // Have we found more then on match in cddb for this disc,...
      if (lasterror == E_WAIT_FOR_INPUT)
      {
        // ...yes, show the matches found in a select dialog
        // and let the user choose an entry.
        pDlgSelect->Reset();
        pDlgSelect->SetHeading(255);
        int i = 1;
        while (1)
        {
          CStdString strTitle = cddb.getInexactTitle(i);
          if (strTitle == "") break;
          
          /*                    CStdString strFace = cddb.getInexactFace(i);
           if (!strFace.IsEmpty())
           strTitle += " - " + strFace;
           */
          pDlgSelect->Add(strTitle);
          i++;
        }
        pDlgSelect->DoModal();
        
        // Has the user selected a match...
        int iSelectedCD = pDlgSelect->GetSelectedLabel();
        if (iSelectedCD >= 0)
        {
          // ...query cddb for the inexact match
          if (!cddb.queryCDinfo(pCdInfo, 1 + iSelectedCD))
            pCdInfo->SetNoCDDBInfo();
        }
        else
          pCdInfo->SetNoCDDBInfo();
      }
      else if (lasterror == E_NO_MATCH_FOUND)
      {
        pCdInfo->SetNoCDDBInfo();
      }
      else
      {
        pCdInfo->SetNoCDDBInfo();
        // ..no, an error occured, display it to the user
        CStdString strErrorText;
        strErrorText.Format("[%d] %s", cddb.getLastError(), cddb.getLastErrorText());
        CGUIDialogOK::ShowAndGetInput(255, 257, strErrorText, 0);
      }
    } // if ( !cddb.queryCDinfo( pCdInfo ) )
    else
      pDialogProgress->Close();
  }
  
  // Filling the file items with cddb info happens in CPictureInfoTagLoaderCDDA
  
  return pCdInfo->HasCDDBInfo();
#else
  return false;
#endif
}

void CPictureDatabase::DeleteCDDBInfo()
{
#ifdef HAS_DVD_DRIVE
  CFileItemList items;
  if (!CDirectory::GetDirectory(CProfilesManager::Get().GetCDDBFolder(), items, ".cddb", DIR_FLAG_NO_FILE_DIRS))
  {
    CGUIDialogOK::ShowAndGetInput(313, 426, 0, 0);
    return ;
  }
  // Show a selectdialog that the user can select the albuminfo to delete
  CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
    pDlg->SetHeading(g_localizeStrings.Get(181).c_str());
    pDlg->Reset();
    
    map<ULONG, CStdString> mapCDDBIds;
    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder)
        continue;
      
      CStdString strFile = URIUtils::GetFileName(items[i]->GetPath());
      strFile.Delete(strFile.size() - 5, 5);
      ULONG lDiscId = strtoul(strFile.c_str(), NULL, 16);
      Xcddb cddb;
      cddb.setCacheDir(CProfilesManager::Get().GetCDDBFolder());
      
      if (!cddb.queryCache(lDiscId))
        continue;
      
      CStdString strDiskTitle, strDiskFace;
      cddb.getDiskTitle(strDiskTitle);
      //            cddb.getDiskFace(strDiskFace);
      
      CStdString str;
      if (strDiskFace.IsEmpty())
        str = strDiskTitle;
      else
        str = strDiskTitle + " - " + strDiskFace;
      
      pDlg->Add(str);
      mapCDDBIds.insert(pair<ULONG, CStdString>(lDiscId, str));
    }
    
    pDlg->Sort();
    pDlg->DoModal();
    
    // and wait till user selects one
    int iSelectedPictureAlbum = pDlg->GetSelectedLabel();
    if (iSelectedPictureAlbum < 0)
    {
      mapCDDBIds.erase(mapCDDBIds.begin(), mapCDDBIds.end());
      return ;
    }
    
    CStdString strSelectedPictureAlbum = pDlg->GetSelectedLabelText();
    map<ULONG, CStdString>::iterator it;
    for (it = mapCDDBIds.begin();it != mapCDDBIds.end();it++)
    {
      if (it->second == strSelectedPictureAlbum)
      {
        CStdString strFile;
        strFile.Format("%x.cddb", it->first);
        CFile::Delete(URIUtils::AddFileToFolder(CProfilesManager::Get().GetCDDBFolder(), strFile));
        break;
      }
    }
    mapCDDBIds.erase(mapCDDBIds.begin(), mapCDDBIds.end());
  }
#endif
}

void CPictureDatabase::Clean()
{
  // If we are scanning for Picture info in the background,
  // other writing access to the database is prohibited.
  if (g_application.IsPictureScanning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }
  
  if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
  {
    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (dlgProgress)
    {
      CPictureDatabase Picturedatabase;
      if (Picturedatabase.Open())
      {
        int iReturnString = Picturedatabase.Cleanup(dlgProgress);
        Picturedatabase.Close();
        
        if (iReturnString != ERROR_OK)
        {
          CGUIDialogOK::ShowAndGetInput(313, iReturnString, 0, 0);
        }
      }
      dlgProgress->Close();
    }
  }
}

bool CPictureDatabase::GetLocationsNav(const CStdString& strBaseDir, CFileItemList& items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    // get primary locations for pictures - could be simplified to just SELECT * FROM location?
    CStdString strSQL = "SELECT %s FROM location ";
    
    Filter extFilter = filter;
    CPictureDbUrl PictureUrl;
    SortDescription sorting;
    if (!PictureUrl.FromString(strBaseDir) || !GetFilter(PictureUrl, extFilter, sorting))
      return false;
    
    // if there are extra WHERE conditions we might need access
    // to pictureview or albumview for these conditions
    if (extFilter.where.size() > 0)
    {
      if (extFilter.where.find("Faceview") != string::npos)
        extFilter.AppendJoin("JOIN picture_location ON picture_location.idLocation = location.idLocation JOIN pictureview ON pictureview.idPicture = picture_location.idPicture "
                             "JOIN picture_Face ON picture_Face.idPicture = pictureview.idPicture JOIN Faceview ON Faceview.idFace = picture_Face.idFace");
      else if (extFilter.where.find("pictureview") != string::npos)
        extFilter.AppendJoin("JOIN picture_location ON picture_location.idLocation = location.idLocation JOIN pictureview ON pictureview.idPicture = picture_location.idPicture");
      else if (extFilter.where.find("albumview") != string::npos)
        extFilter.AppendJoin("JOIN album_location ON album_location.idLocation = location.idLocation JOIN albumview ON albumview.idAlbum = album_location.idAlbum");
      
      extFilter.AppendGroup("location.idLocation");
    }
    extFilter.AppendWhere("location.strLocation != ''");
    
    if (countOnly)
    {
      extFilter.fields = "COUNT(DISTINCT location.idLocation)";
      extFilter.group.clear();
      extFilter.order.clear();
    }
    
    CStdString strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;
    
    strSQL = PrepareSQL(strSQL.c_str(), !extFilter.fields.empty() && extFilter.fields.compare("*") != 0 ? extFilter.fields.c_str() : "location.*") + strSQLExtra;
    
    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    
    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);
      
      m_pDS->close();
      return true;
    }
    
    // get data from returned rows
    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("location.strLocation").get_asString()));
      //            pItem->GetPictureInfoTag()->SetLocation(m_pDS->fv("location.strLocation").get_asString());
      //            pItem->GetPictureInfoTag()->SetDatabaseId(m_pDS->fv("location.idLocation").get_asInt(), "location");
      
      CPictureDbUrl itemUrl = PictureUrl;
      CStdString strDir; strDir.Format("%ld/", m_pDS->fv("location.idLocation").get_asInt());
      itemUrl.AppendPath(strDir);
      pItem->SetPath(itemUrl.ToString());
      
      pItem->m_bIsFolder = true;
      items.Add(pItem);
      
      m_pDS->next();
    }
    
    // cleanup
    m_pDS->close();
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CPictureDatabase::GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, const Filter &filter /* = Filter() */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    Filter extFilter = filter;
    CPictureDbUrl PictureUrl;
    SortDescription sorting;
    if (!PictureUrl.FromString(strBaseDir) || !GetFilter(PictureUrl, extFilter, sorting))
      return false;
    
    // get years from album list
    CStdString strSQL = "SELECT DISTINCT albumview.iYear FROM albumview ";
    extFilter.AppendWhere("albumview.iYear <> 0");
    
    if (!BuildSQL(strSQL, extFilter, strSQL))
      return false;
    
    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    
    // get data from returned rows
    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("iYear").get_asString()));
      SYSTEMTIME stTime;
      stTime.wYear = (WORD)m_pDS->fv("iYear").get_asInt();
      //            pItem->GetPictureInfoTag()->SetReleaseDate(stTime);
      
      CPictureDbUrl itemUrl = PictureUrl;
      CStdString strDir; strDir.Format("%ld/", m_pDS->fv("iYear").get_asInt());
      itemUrl.AppendPath(strDir);
      pItem->SetPath(itemUrl.ToString());
      
      pItem->m_bIsFolder = true;
      items.Add(pItem);
      
      m_pDS->next();
    }
    
    // cleanup
    m_pDS->close();
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CPictureDatabase::GetPictureAlbumsByYear(const CStdString& strBaseDir, CFileItemList& items, int year)
{
  CPictureDbUrl PictureUrl;
  if (!PictureUrl.FromString(strBaseDir))
    return false;
  
  PictureUrl.AddOption("year", year);
  
  Filter filter;
  return GetPictureAlbumsByWhere(PictureUrl.ToString(), filter, items);
}

bool CPictureDatabase::GetCommonNav(const CStdString &strBaseDir, const CStdString &table, const CStdString &labelField, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  
  if (table.empty() || labelField.empty())
    return false;
  
  try
  {
    Filter extFilter = filter;
    CStdString strSQL = "SELECT %s FROM " + table + " ";
    extFilter.AppendGroup(labelField);
    extFilter.AppendWhere(labelField + " != ''");
    
    if (countOnly)
    {
      extFilter.fields = "COUNT(DISTINCT " + labelField + ")";
      extFilter.group.clear();
      extFilter.order.clear();
    }
    
    CPictureDbUrl PictureUrl;
    if (!BuildSQL(strBaseDir, strSQL, extFilter, strSQL, PictureUrl))
      return false;
    
    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : labelField.c_str());
    
    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound <= 0)
    {
      m_pDS->close();
      return false;
    }
    
    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);
      
      m_pDS->close();
      return true;
    }
    
    // get data from returned rows
    while (!m_pDS->eof())
    {
      string labelValue = m_pDS->fv(labelField).get_asString();
      CFileItemPtr pItem(new CFileItem(labelValue));
      
      CPictureDbUrl itemUrl = PictureUrl;
      CStdString strDir; strDir.Format("%s/", labelValue.c_str());
      itemUrl.AppendPath(strDir);
      pItem->SetPath(itemUrl.ToString());
      
      pItem->m_bIsFolder = true;
      items.Add(pItem);
      
      m_pDS->next();
    }
    
    // cleanup
    m_pDS->close();
    
    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

bool CPictureDatabase::GetPictureAlbumTypesNav(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetCommonNav(strBaseDir, "albumview", "albumview.strType", items, filter, countOnly);
}

bool CPictureDatabase::GetPictureLabelsNav(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */)
{
  return GetCommonNav(strBaseDir, "albumview", "albumview.strLabel", items, filter, countOnly);
}

bool CPictureDatabase::GetFacesNav(const CStdString& strBaseDir, CFileItemList& items, bool albumFacesOnly /* = false */, int idLocation /* = -1 */, int idAlbum /* = -1 */, int idPicture /* = -1 */, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  try
  {
    unsigned int time = XbmcThreads::SystemClockMillis();
    
    CPictureDbUrl PictureUrl;
    if (!PictureUrl.FromString(strBaseDir))
      return false;
    
    if (idLocation > 0)
      PictureUrl.AddOption("locationid", idLocation);
    else if (idAlbum > 0)
      PictureUrl.AddOption("albumid", idAlbum);
    else if (idPicture > 0)
      PictureUrl.AddOption("pictureid", idPicture);
    
    PictureUrl.AddOption("albumFacesonly", albumFacesOnly);
    
    bool result = GetFacesByWhere(PictureUrl.ToString(), filter, items, sortDescription, countOnly);
    CLog::Log(LOGDEBUG,"Time to retrieve Faces from dataset = %i", XbmcThreads::SystemClockMillis() - time);
    
    return result;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CPictureDatabase::GetFacesByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  
  try
  {
    int total = -1;
    
    CStdString strSQL = "SELECT %s FROM Faceview ";
    
    Filter extFilter = filter;
    CPictureDbUrl PictureUrl;
    SortDescription sorting = sortDescription;
    if (!PictureUrl.FromString(strBaseDir) || !GetFilter(PictureUrl, extFilter, sorting))
      return false;
    
    // if there are extra WHERE conditions we might need access
    // to pictureview or albumview for these conditions
    if (extFilter.where.size() > 0)
    {
      bool extended = false;
      if (extFilter.where.find("pictureview") != string::npos)
      {
        extended = true;
        extFilter.AppendJoin("JOIN picture_Face ON picture_Face.idFace = Faceview.idFace JOIN pictureview ON pictureview.idPicture = picture_Face.idPicture");
      }
      else if (extFilter.where.find("albumview") != string::npos)
      {
        extended = true;
        extFilter.AppendJoin("JOIN album_Face ON album_Face.idFace = Faceview.idFace JOIN albumview ON albumview.idAlbum = album_Face.idAlbum");
      }
      
      if (extended)
        extFilter.AppendGroup("Faceview.idFace");
    }
    
    if (countOnly)
    {
      extFilter.fields = "COUNT(DISTINCT Faceview.idFace)";
      extFilter.group.clear();
      extFilter.order.clear();
    }
    
    CStdString strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;
    
    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
        sortDescription.sortBy == SortByNone &&
        (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }
    
    strSQL = PrepareSQL(strSQL.c_str(), !extFilter.fields.empty() && extFilter.fields.compare("*") != 0 ? extFilter.fields.c_str() : "Faceview.*") + strSQLExtra;
    
    // run query
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    
    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", iRowsFound == 1 ? m_pDS->fv(0).get_asInt() : iRowsFound);
      items.Add(pItem);
      
      m_pDS->close();
      return true;
    }
    
    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);
    
    DatabaseResults results;
    results.reserve(iRowsFound);
    if (!SortUtils::SortFromDataset(sortDescription, MediaTypeFace, m_pDS, results))
      return false;
    
    // get data from returned rows
    items.Reserve(results.size());
    const dbiplus::query_data &data = m_pDS->get_result_set().records;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);
      
      try
      {
        CFace Face = GetFaceFromDataset(record, false);
/*
        CFileItemPtr pItem(new CFileItem(Face));
        
        CPictureDbUrl itemUrl = PictureUrl;
        CStdString path; path.Format("%ld/", Face.idFace);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());
        
        pItem->GetPictureInfoTag()->SetDatabaseId(Face.idFace, "Face");
        pItem->SetIconImage("DefaultFace.png");
        
        SetPropertiesFromFace(*pItem, Face);
        items.Add(pItem);
*/
      }
      catch (...)
      {
        m_pDS->close();
        CLog::Log(LOGERROR, "%s - out of memory getting listing (got %i)", __FUNCTION__, items.Size());
      }
    }
    
    // cleanup
    m_pDS->close();
    
    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CPictureDatabase::GetPictureAlbumFromPicture(int idPicture, CPictureAlbum &album)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL = PrepareSQL("select albumview.* from picture join albumview on picture.idAlbum = albumview.idAlbum where picture.idPicture='%i'", idPicture);
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }
    
    album = GetPictureAlbumFromDataset(m_pDS.get());
    
    m_pDS->close();
    return true;
    
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CPictureDatabase::GetPictureAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idLocation /* = -1 */, int idFace /* = -1 */, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  CPictureDbUrl PictureUrl;
  if (!PictureUrl.FromString(strBaseDir))
    return false;
  
  // where clause
  if (idLocation > 0)
    PictureUrl.AddOption("locationid", idLocation);
  
  if (idFace > 0)
    PictureUrl.AddOption("Faceid", idFace);
  
  return GetPictureAlbumsByWhere(PictureUrl.ToString(), filter, items, sortDescription, countOnly);
}

bool CPictureDatabase::GetPictureAlbumsByWhere(const CStdString &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */, bool countOnly /* = false */)
{
  if (m_pDB.get() == NULL || m_pDS.get() == NULL)
    return false;
  try
  {
    int total = -1;
    
    CStdString strSQL = "SELECT %s FROM albumview ";
    
    Filter extFilter = filter;
    CPictureDbUrl PictureUrl;
    SortDescription sorting = sortDescription;
    if (!PictureUrl.FromString(baseDir) || !GetFilter(PictureUrl, extFilter, sorting))
      return false;
    
    // if there are extra WHERE conditions we might need access
    // to pictureview for these conditions
    if (extFilter.where.find("pictureview") != string::npos)
    {
      extFilter.AppendJoin("JOIN pictureview ON pictureview.idAlbum = albumview.idAlbum");
      extFilter.AppendGroup("albumview.idAlbum");
    }
    
    CStdString strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;
    
    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
        sortDescription.sortBy == SortByNone &&
        (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }
    
    strSQL = PrepareSQL(strSQL, !filter.fields.empty() && filter.fields.compare("*") != 0 ? filter.fields.c_str() : "albumview.*") + strSQLExtra;
    
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());
    // run query
    unsigned int time = XbmcThreads::SystemClockMillis();
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    CLog::Log(LOGDEBUG, "%s - query took %i ms",
              __FUNCTION__, XbmcThreads::SystemClockMillis() - time); time = XbmcThreads::SystemClockMillis();
    
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound <= 0)
    {
      m_pDS->close();
      return true;
    }
    
    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);
    
    if (countOnly)
    {
      CFileItemPtr pItem(new CFileItem());
      pItem->SetProperty("total", total);
      items.Add(pItem);
      
      m_pDS->close();
      return true;
    }
    
    DatabaseResults results;
    results.reserve(iRowsFound);
    
    if (!SortUtils::SortFromDataset(sortDescription, MediaTypePictureAlbum, m_pDS, results))
      return false;
    
    // get data from returned rows
    items.Reserve(results.size());
    const dbiplus::query_data &data = m_pDS->get_result_set().records;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);
      
      try
      {
        CPictureDbUrl itemUrl = PictureUrl;
        CStdString path; path.Format("%ld/", record->at(album_idAlbum).get_asInt());
        itemUrl.AppendPath(path);
        
        CFileItemPtr pItem(new CFileItem(itemUrl.ToString(), GetPictureAlbumFromDataset(record)));
        pItem->SetIconImage("DefaultPictureAlbumCover.png");
        items.Add(pItem);
        
      }
      catch (...)
      {
        m_pDS->close();
        CLog::Log(LOGERROR, "%s - out of memory getting listing (got %i)", __FUNCTION__, items.Size());
      }
    }
    
    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    m_pDS->close();
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CPictureDatabase::GetPicturesByWhere(const CStdString &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (m_pDB.get() == NULL || m_pDS.get() == NULL)
    return false;
  
  try
  {
    unsigned int time = XbmcThreads::SystemClockMillis();
    int total = -1;
    
    CStdString strSQL = "SELECT %s FROM pictureview ";
    
    Filter extFilter = filter;
    CPictureDbUrl PictureUrl;
    SortDescription sorting = sortDescription;
    if (!PictureUrl.FromString(baseDir) || !GetFilter(PictureUrl, extFilter, sorting))
      return false;
    
    // if there are extra WHERE conditions we might need access
    // to pictureview for these conditions
    if (extFilter.where.find("albumview") != string::npos)
    {
      extFilter.AppendJoin("JOIN albumview ON albumview.idAlbum = pictureview.idAlbum");
      extFilter.AppendGroup("pictureview.idPicture");
    }
    
    CStdString strSQLExtra;
    if (!BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;
    
    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
        sortDescription.sortBy == SortByNone &&
        (sortDescription.limitStart > 0 || sortDescription.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sortDescription.limitEnd, sortDescription.limitStart);
    }
    
    strSQL = PrepareSQL(strSQL, !filter.fields.empty() && filter.fields.compare("*") != 0 ? filter.fields.c_str() : "pictureview.*") + strSQLExtra;
    
    CLog::Log(LOGDEBUG, "%s query = %s", __FUNCTION__, strSQL.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    
    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);
    
    DatabaseResults results;
    results.reserve(iRowsFound);
    
    if (!SortUtils::SortFromDataset(sortDescription, MediaTypePicture, m_pDS, results))
      return false;
    
    // get data from returned rows
    items.Reserve(results.size());
    const dbiplus::query_data &data = m_pDS->get_result_set().records;
    int count = 0;
    for (DatabaseResults::const_iterator it = results.begin(); it != results.end(); it++)
    {
      unsigned int targetRow = (unsigned int)it->at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);
      
      try
      {
        CFileItemPtr item(new CFileItem);
        GetFileItemFromDataset(record, item.get(), PictureUrl.ToString());
        // HACK for sorting by database returned order
        item->m_iprogramCount = ++count;
        items.Add(item);
      }
      catch (...)
      {
        m_pDS->close();
        CLog::Log(LOGERROR, "%s: out of memory loading query: %s", __FUNCTION__, filter.where.c_str());
        return (items.Size() > 0);
      }
    }
    
    // cleanup
    m_pDS->close();
    CLog::Log(LOGDEBUG, "%s(%s) - took %d ms", __FUNCTION__, filter.where.c_str(), XbmcThreads::SystemClockMillis() - time);
    return true;
  }
  catch (...)
  {
    // cleanup
    m_pDS->close();
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CPictureDatabase::GetPicturesByYear(const CStdString& baseDir, CFileItemList& items, int year)
{
  CPictureDbUrl PictureUrl;
  if (!PictureUrl.FromString(baseDir))
    return false;
  
  PictureUrl.AddOption("year", year);
  
  Filter filter;
  return GetPicturesByWhere(baseDir, filter, items);
}

bool CPictureDatabase::GetPicturesNav(const CStdString& strBaseDir, CFileItemList& items, int idLocation, int idFace, int idAlbum, const SortDescription &sortDescription /* = SortDescription() */)
{
  CPictureDbUrl PictureUrl;
  if (!PictureUrl.FromString(strBaseDir))
    return false;
  
  if (idAlbum > 0)
    PictureUrl.AddOption("albumid", idAlbum);
  
  if (idLocation > 0)
    PictureUrl.AddOption("locationid", idLocation);
  
  if (idFace > 0)
    PictureUrl.AddOption("Faceid", idFace);
  
  Filter filter;
  return GetPicturesByWhere(PictureUrl.ToString(), filter, items, sortDescription);
}

bool CPictureDatabase::UpdateOldVersion(int version)
{
  
  // always recreate the views after any table change
  CreateViews();
  
  return true;
}

int CPictureDatabase::GetMinVersion() const
{
  return 1;
}

unsigned int CPictureDatabase::GetPictureIDs(const Filter &filter, vector<pair<int,int> > &pictureIDs)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;
    
    CStdString strSQL = "select idPicture from pictureview ";
    if (!CDatabase::BuildSQL(strSQL, filter, strSQL))
      return false;
    
    if (!m_pDS->query(strSQL.c_str())) return 0;
    pictureIDs.clear();
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }
    pictureIDs.reserve(m_pDS->num_rows());
    while (!m_pDS->eof())
    {
      pictureIDs.push_back(make_pair<int,int>(1,m_pDS->fv(picture_idPicture).get_asInt()));
      m_pDS->next();
    }    // cleanup
    m_pDS->close();
    return pictureIDs.size();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return 0;
}

int CPictureDatabase::GetPicturesCount(const Filter &filter)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;
    
    CStdString strSQL = "select count(idPicture) as NumPictures from pictureview ";
    if (!CDatabase::BuildSQL(strSQL, filter, strSQL))
      return false;
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }
    
    int iNumPictures = m_pDS->fv("NumPictures").get_asInt();
    // cleanup
    m_pDS->close();
    return iNumPictures;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return 0;
}

bool CPictureDatabase::GetPictureAlbumPath(int idAlbum, CStdString& path)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;
    
    path.Empty();
    
    CStdString strSQL=PrepareSQL("select strPath from picture join path on picture.idPath = path.idPath where picture.idAlbum=%ld", idAlbum);
    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS2->close();
      return false;
    }
    
    // if this returns more than one path, we just grab the first one.  It's just for determining where to obtain + place
    // a local thumbnail
    path = m_pDS2->fv("strPath").get_asString();
    
    m_pDS2->close(); // cleanup recordset data
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idAlbum);
  }
  
  return false;
}

bool CPictureDatabase::SavePictureAlbumThumb(int idAlbum, const CStdString& strThumb)
{
  SetArtForItem(idAlbum, "album", "thumb", strThumb);
  // TODO: We should prompt the user to update the art for pictures
  CStdString sql = PrepareSQL("UPDATE art"
                              " SET url='-'"
                              " WHERE media_type='picture'"
                              " AND type='thumb'"
                              " AND media_id IN"
                              " (SELECT idPicture FROM picture WHERE idAlbum=%ld)", idAlbum);
  ExecuteQuery(sql);
  return true;
}

bool CPictureDatabase::GetFacePath(int idFace, CStdString &basePath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;
    
    // find all albums from this Face, and all the paths to the pictures from those albums
    CStdString strSQL=PrepareSQL("SELECT strPath"
                                 "  FROM album_Face"
                                 "  JOIN picture "
                                 "    ON album_Face.idAlbum = picture.idAlbum"
                                 "  JOIN path"
                                 "    ON picture.idPath = path.idPath"
                                 " WHERE album_Face.idFace = %i"
                                 " GROUP BY picture.idPath", idFace);
    
    // run query
    if (!m_pDS2->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS2->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS2->close();
      return false;
    }
    
    // special case for single path - assume that we're in an Face/album/pictures filesystem
    if (iRowsFound == 1)
    {
      URIUtils::GetParentPath(m_pDS2->fv("strPath").get_asString(), basePath);
      m_pDS2->close();
      return true;
    }
    
    // find the common path (if any) to these albums
    basePath.Empty();
    while (!m_pDS2->eof())
    {
      CStdString path = m_pDS2->fv("strPath").get_asString();
      if (basePath.IsEmpty())
        basePath = path;
      else
        URIUtils::GetCommonPath(basePath,path);
      
      m_pDS2->next();
    }
    
    // cleanup
    m_pDS2->close();
    return true;
    
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

int CPictureDatabase::GetFaceByName(const CStdString& strFace)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL=PrepareSQL("select idFace from Face where Face.strFace like '%s'", strFace.c_str());
    
    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    int lResult = m_pDS->fv("Face.idFace").get_asInt();
    m_pDS->close();
    return lResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

int CPictureDatabase::GetPictureAlbumByName(const CStdString& strAlbum, const CStdString& strFace)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    if (strFace.IsEmpty())
      strSQL=PrepareSQL("SELECT idAlbum FROM album WHERE album.strAlbum LIKE '%s'", strAlbum.c_str());
    else
      strSQL=PrepareSQL("SELECT album.idAlbum FROM album WHERE album.strAlbum LIKE '%s' AND album.strFaces LIKE '%s'", strAlbum.c_str(),strFace.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    return m_pDS->fv("album.idAlbum").get_asInt();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

int CPictureDatabase::GetPictureAlbumByName(const CStdString& strAlbum, const std::vector<std::string>& Face)
{
  return GetPictureAlbumByName(strAlbum, StringUtils::Join(Face, g_advancedSettings.m_pictureItemSeparator));
}

CStdString CPictureDatabase::GetLocationById(int id)
{
  return GetSingleValue("location", "strLocation", PrepareSQL("idLocation=%i", id));
}

CStdString CPictureDatabase::GetFaceById(int id)
{
  return GetSingleValue("Face", "strFace", PrepareSQL("idFace=%i", id));
}

CStdString CPictureDatabase::GetPictureAlbumById(int id)
{
  return GetSingleValue("album", "strAlbum", PrepareSQL("idAlbum=%i", id));
}

int CPictureDatabase::GetLocationByName(const CStdString& strLocation)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    strSQL=PrepareSQL("select idLocation from location where location.strLocation like '%s'", strLocation.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return -1;
    }
    return m_pDS->fv("location.idLocation").get_asInt();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

bool CPictureDatabase::GetRandomPicture(CFileItem* item, int& idPicture, const Filter &filter)
{
  try
  {
    idPicture = -1;
    
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    // We don't use PrepareSQL here, as the WHERE clause is already formatted
    CStdString strSQL = PrepareSQL("select %s from pictureview ", !filter.fields.empty() ? filter.fields.c_str() : "*");
    Filter extFilter = filter;
    extFilter.AppendOrder(PrepareSQL("RANDOM()"));
    extFilter.limit = "1";
    
    if (!CDatabase::BuildSQL(strSQL, extFilter, strSQL))
      return false;
    
    CLog::Log(LOGDEBUG, "%s query = %s", __FUNCTION__, strSQL.c_str());
    // run query
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound != 1)
    {
      m_pDS->close();
      return false;
    }
    GetFileItemFromDataset(item, "");
    idPicture = m_pDS->fv("pictureview.idPicture").get_asInt();
    m_pDS->close();
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return false;
}

bool CPictureDatabase::GetCompilationPictureAlbums(const CStdString& strBaseDir, CFileItemList& items)
{
  CPictureDbUrl PictureUrl;
  if (!PictureUrl.FromString(strBaseDir))
    return false;
  
  PictureUrl.AddOption("compilation", true);
  
  Filter filter;
  return GetPictureAlbumsByWhere(PictureUrl.ToString(), filter, items);
}

bool CPictureDatabase::GetCompilationPictures(const CStdString& strBaseDir, CFileItemList& items)
{
  CPictureDbUrl PictureUrl;
  if (!PictureUrl.FromString(strBaseDir))
    return false;
  
  PictureUrl.AddOption("compilation", true);
  
  Filter filter;
  return GetPicturesByWhere(PictureUrl.ToString(), filter, items);
}

int CPictureDatabase::GetCompilationPictureAlbumsCount()
{
  return strtol(GetSingleValue("album", "count(idAlbum)", "bCompilation = 1"), NULL, 10);
}

void CPictureDatabase::SplitString(const CStdString &multiString, vector<string> &vecStrings, CStdString &extraStrings)
{
  vecStrings = StringUtils::Split(multiString, g_advancedSettings.m_pictureItemSeparator);
  for (unsigned int i = 1; i < vecStrings.size(); i++)
    extraStrings += g_advancedSettings.m_pictureItemSeparator + CStdString(vecStrings[i]);
}

bool CPictureDatabase::SetPathHash(const CStdString &path, const CStdString &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    if (hash.IsEmpty())
    { // this is an empty folder - we need only add it to the path table
      // if the path actually exists
      if (!CDirectory::Exists(path))
        return false;
    }
    int idPath = AddPath(path);
    if (idPath < 0) return false;
    
    CStdString strSQL=PrepareSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), idPath);
    m_pDS->exec(strSQL.c_str());
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, path.c_str(), hash.c_str());
  }
  
  return false;
}

bool CPictureDatabase::GetPathHash(const CStdString &path, CStdString &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL=PrepareSQL("select strHash from path where strPath='%s'", path.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
      return false;
    hash = m_pDS->fv("strHash").get_asString();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, path.c_str());
  }
  
  return false;
}

bool CPictureDatabase::RemovePicturesFromPath(const CStdString &path1, MAPPICTURES& pictures, bool exact)
{
  // We need to remove all pictures from this path, as their tags are going
  // to be re-read.  We need to remove all pictures from the picture table + all links to them
  // from the picture link tables (as otherwise if a picture is added back
  // to the table with the same idPicture, these tables can't be cleaned up properly later)
  
  // TODO: SQLite probably doesn't allow this, but can we rely on that??
  
  // We don't need to remove orphaned albums at this point as in AddPictureAlbum() we check
  // first whether the album has already been read during this scan, and if it hasn't
  // we check whether it's in the table and update accordingly at that point, removing the entries from
  // the album link tables.  The only failure point for this is albums
  // that span multiple folders, where just the files in one folder have been changed.  In this case
  // any linked fields that are only in the files that haven't changed will be removed.  Clearly
  // the primary albumFace still matches (as that's what we looked up based on) so is this really
  // an issue?  I don't think it is, as those Faces will still have links to the album via the pictures
  // which is generally what we rely on, so the only failure point is albumFace lookup.  In this
  // case, it will return only things in the album_Face table from the newly updated pictures (and
  // only if they have additional Faces).  I think the effect of this is minimal at best, as ALL
  // pictures in the album should have the same albumFace!
  
  // we also remove the path at this point as it will be added later on if the
  // path still exists.
  // After scanning we then remove the orphaned Faces, locations and thumbs.
  
  // Note: when used to remove all pictures from a path and its subpath (exact=false), this
  // does miss archived pictures.
  CStdString path(path1);
  try
  {
    if (!URIUtils::HasSlashAtEnd(path))
      URIUtils::AddSlashAtEnd(path);
    
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString where;
    if (exact)
      where = PrepareSQL(" where strPath='%s'", path.c_str());
    else
      where = PrepareSQL(" where SUBSTR(strPath,1,%i)='%s'", StringUtils::utf8_strlen(path.c_str()), path.c_str());
    CStdString sql = "select * from pictureview" + where;
    if (!m_pDS->query(sql.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound > 0)
    {
      std::vector<int> ids;
      CStdString pictureIds = "(";
      while (!m_pDS->eof())
      {
        CPicture picture = GetPictureFromDataset();
        picture.strThumb = GetArtForItem(picture.idPicture, "picture", "thumb");
        pictures.insert(make_pair(picture.strFileName, picture));
        pictureIds += PrepareSQL("%i,", picture.idPicture);
        ids.push_back(picture.idPicture);
        m_pDS->next();
      }
      pictureIds.TrimRight(",");
      pictureIds += ")";
      
      m_pDS->close();
      
      //TODO: move this below the m_pDS->exec block, once UPnP doesn't rely on this anymore
      for (unsigned int i = 0; i < ids.size(); i++)
        AnnounceRemove("picture", ids[i]);
      
      // and delete all pictures, and anything linked to them
      sql = "delete from picture where idPicture in " + pictureIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from picture_Face where idPicture in " + pictureIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from picture_location where idPicture in " + pictureIds;
      m_pDS->exec(sql.c_str());
      sql = "delete from karaokedata where idPicture in " + pictureIds;
      m_pDS->exec(sql.c_str());
      
    }
    // and remove the path as well (it'll be re-added later on with the new hash if it's non-empty)
    sql = "delete from path" + where;
    m_pDS->exec(sql.c_str());
    return iRowsFound > 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, path.c_str());
  }
  return false;
}

bool CPictureDatabase::GetPaths(set<string> &paths)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    paths.clear();
    
    // find all paths
    if (!m_pDS->query("select strPath from path")) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return true;
    }
    while (!m_pDS->eof())
    {
      paths.insert(m_pDS->fv("strPath").get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CPictureDatabase::SetPictureRating(const CStdString &filePath, char rating)
{
  try
  {
    if (filePath.IsEmpty()) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    int pictureID = GetPictureIDFromPath(filePath);
    if (-1 == pictureID) return false;
    
    CStdString sql = PrepareSQL("update picture set rating='%c' where idPicture = %i", rating, pictureID);
    m_pDS->exec(sql.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s,%c) failed", __FUNCTION__, filePath.c_str(), rating);
  }
  return false;
}

int CPictureDatabase::GetPictureIDFromPath(const CStdString &filePath)
{
  // grab the where string to identify the picture id
  CURL url(filePath);
  if (url.GetProtocol()=="Picturedb")
  {
    CStdString strFile=URIUtils::GetFileName(filePath);
    URIUtils::RemoveExtension(strFile);
    return atol(strFile.c_str());
  }
  // hit the db
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath;
    URIUtils::GetDirectory(filePath, strPath);
    URIUtils::AddSlashAtEnd(strPath);
    
    DWORD crc = ComputeCRC(filePath);
    
    CStdString sql = PrepareSQL("select idPicture from picture join path on picture.idPath = path.idPath where picture.dwFileNameCRC='%ul'and path.strPath='%s'", crc, strPath.c_str());
    if (!m_pDS->query(sql.c_str())) return -1;
    
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return -1;
    }
    
    int pictureID = m_pDS->fv("idPicture").get_asInt();
    m_pDS->close();
    return pictureID;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
  return -1;
}

bool CPictureDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so reset the infomanager cache
    //        g_infoManager.SetLibraryBool(LIBRARY_HAS_PICTURE, GetPicturesCount() > 0);
    return true;
  }
  return false;
}

bool CPictureDatabase::SetScraperForPath(const CStdString& strPath, const ADDON::ScraperPtr& scraper)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    // wipe old settings
    CStdString strSQL = PrepareSQL("delete from content where strPath='%s'",strPath.c_str());
    m_pDS->exec(strSQL.c_str());
    
    // insert new settings
    strSQL = PrepareSQL("insert into content (strPath, strScraperPath, strContent, strSettings) values ('%s','%s','%s','%s')",
                        strPath.c_str(), scraper->ID().c_str(), ADDON::TranslateContent(scraper->Content()).c_str(), scraper->GetPathSettings().c_str());
    m_pDS->exec(strSQL.c_str());
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CPictureDatabase::GetScraperForPath(const CStdString& strPath, ADDON::ScraperPtr& info, const ADDON::TYPE &type)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL = PrepareSQL("select * from content where strPath='%s'",strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->eof()) // no info set for path - fallback logic commencing
    {
      CQueryParams params;
      CDirectoryNode::GetDatabaseInfo(strPath, params);
      if (params.GetLocationId() != -1) // check location
      {
        strSQL = PrepareSQL("select * from content where strPath='Picturedb://locations/%i/'",params.GetLocationId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof() && params.GetPictureAlbumId() != -1) // check album
      {
        strSQL = PrepareSQL("select * from content where strPath='Picturedb://albums/%i/'",params.GetPictureAlbumId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof() && params.GetFaceId() != -1) // check Face
      {
        strSQL = PrepareSQL("select * from content where strPath='Picturedb://Faces/%i/'",params.GetFaceId());
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof()) // general albums setting
      {
        strSQL = PrepareSQL("select * from content where strPath='Picturedb://albums/'");
        m_pDS->query(strSQL.c_str());
      }
      if (m_pDS->eof()) // general Face setting
      {
        strSQL = PrepareSQL("select * from content where strPath='Picturedb://Faces/'");
        m_pDS->query(strSQL.c_str());
      }
    }
    
    if (!m_pDS->eof())
    { // try and ascertain scraper for this path
      CONTENT_TYPE content = ADDON::TranslateContent(m_pDS->fv("content.strContent").get_asString());
      CStdString scraperUUID = m_pDS->fv("content.strScraperPath").get_asString();
      
      if (content != CONTENT_NONE)
      { // content set, use pre configured or default scraper
        ADDON::AddonPtr addon;
        if (!scraperUUID.empty() && ADDON::CAddonMgr::Get().GetAddon(scraperUUID, addon) && addon)
        {
          info = boost::dynamic_pointer_cast<ADDON::CScraper>(addon->Clone(addon));
          if (!info)
            return false;
          // store this path's settings
          info->SetPathSettings(content, m_pDS->fv("content.strSettings").get_asString());
        }
      }
      else
      { // use default scraper of the requested type
        ADDON::AddonPtr defaultScraper;
        if (ADDON::CAddonMgr::Get().GetDefault(type, defaultScraper))
        {
          info = boost::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper->Clone(defaultScraper));
        }
      }
    }
    m_pDS->close();
    
    if (!info)
    { // use default Picture scraper instead
      ADDON::AddonPtr addon;
      if(ADDON::CAddonMgr::Get().GetDefault(type, addon))
      {
        info = boost::dynamic_pointer_cast<ADDON::CScraper>(addon);
        return (info);
      }
      else
        return false;
    }
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s -(%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

bool CPictureDatabase::ScraperInUse(const CStdString &scraperID) const
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString sql = PrepareSQL("select count(1) from content where strScraperPath='%s'",scraperID.c_str());
    if (!m_pDS->query(sql.c_str()) || m_pDS->num_rows() == 0)
      return false;
    bool found = m_pDS->fv(0).get_asInt() > 0;
    m_pDS->close();
    return found;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, scraperID.c_str());
  }
  return false;
}

bool CPictureDatabase::GetItems(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  CPictureDbUrl PictureUrl;
  if (!PictureUrl.FromString(strBaseDir))
    return false;
  
  return GetItems(strBaseDir, PictureUrl.GetType(), items, filter, sortDescription);
}

bool CPictureDatabase::GetItems(const CStdString &strBaseDir, const CStdString &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (itemType.Equals("locations"))
    return GetLocationsNav(strBaseDir, items, filter);
  else if (itemType.Equals("years"))
    return GetYearsNav(strBaseDir, items, filter);
  else if (itemType.Equals("Faces"))
    return GetFacesNav(strBaseDir, items, !CSettings::Get().GetBool("Picturelibrary.showcompilationFaces"), -1, -1, -1, filter, sortDescription);
  else if (itemType.Equals("albums"))
    return GetPictureAlbumsByWhere(strBaseDir, filter, items, sortDescription);
  else if (itemType.Equals("pictures"))
    return GetPicturesByWhere(strBaseDir, filter, items, sortDescription);
  
  return false;
}

CStdString CPictureDatabase::GetItemById(const CStdString &itemType, int id)
{
  if (itemType.Equals("locations"))
    return GetLocationById(id);
  else if (itemType.Equals("years"))
  {
    CStdString tmp; tmp.Format("%d", id);
    return tmp;
  }
  else if (itemType.Equals("Faces"))
    return GetFaceById(id);
  else if (itemType.Equals("albums"))
    return GetPictureAlbumById(id);
  
  return "";
}

void CPictureDatabase::ExportToXML(const CStdString &xmlFile, bool singleFiles, bool images, bool overwrite)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    if (NULL == m_pDS2.get()) return;
    
    // find all albums
    CStdString sql = "select albumview.*,albuminfo.strImage,albuminfo.idAlbumInfo from albuminfo "
    "join albumview on albuminfo.idAlbum=albumview.idAlbum ";
    
    m_pDS->query(sql.c_str());
    
    CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(20196);
      progress->SetLine(0, 650);
      progress->SetLine(1, "");
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }
    
    int total = m_pDS->num_rows();
    int current = 0;
    
    // create our xml document
    CXBMCTinyXML xmlDoc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    xmlDoc.InsertEndChild(decl);
    TiXmlNode *pMain = NULL;
    if (singleFiles)
      pMain = &xmlDoc;
    else
    {
      TiXmlElement xmlMainElement("Picturedb");
      pMain = xmlDoc.InsertEndChild(xmlMainElement);
    }
    while (!m_pDS->eof())
    {
      CPictureAlbum album = GetPictureAlbumFromDataset(m_pDS.get());
      album.thumbURL.Clear();
      album.thumbURL.ParseString(m_pDS->fv("albuminfo.strImage").get_asString());
      int idAlbumInfo = m_pDS->fv("albuminfo.idAlbumInfo").get_asInt();
      GetPictureAlbumInfoPictures(idAlbumInfo,album.pictures);
      CStdString strPath;
      GetPictureAlbumPath(album.idAlbum,strPath);
      album.Save(pMain, "album", strPath);
      if (singleFiles)
      {
        if (!CDirectory::Exists(strPath))
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, strPath.c_str());
        else
        {
          CStdString nfoFile = URIUtils::AddFileToFolder(strPath, "album.nfo");
          if (overwrite || !CFile::Exists(nfoFile))
          {
            if (!xmlDoc.SaveFile(nfoFile))
              CLog::Log(LOGERROR, "%s: PictureAlbum nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
          }
          
          if (images)
          {
            string thumb = GetArtForItem(album.idAlbum, "album", "thumb");
            if (!thumb.empty() && (overwrite || !CFile::Exists(URIUtils::AddFileToFolder(strPath,"folder.jpg"))))
              CTextureCache::Get().Export(thumb, URIUtils::AddFileToFolder(strPath,"folder.jpg"));
          }
          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }
      }
      
      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, album.strAlbum);
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }
      m_pDS->next();
      current++;
    }
    m_pDS->close();
    
    // find all Faces
    sql = "SELECT Face.idFace AS idFace, strFace, "
    "  strBorn, strFormed, strLocations, "
    "  strMoods, strStyles, strInstruments, "
    "  strBiography, strDied, strDisbanded, "
    "  strYearsActive, strImage, strFanart "
    "  FROM Face "
    "  JOIN Faceinfo "
    "    ON Face.idFace=Faceinfo.idFace";
    
    // needed due to getFacepath
    auto_ptr<dbiplus::Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    pDS->query(sql.c_str());
    
    total = pDS->num_rows();
    current = 0;
    
    while (!pDS->eof())
    {
      CFace Face = GetFaceFromDataset(pDS.get());
      CStdString strSQL=PrepareSQL("select * from discography where idFace=%i",Face.idFace);
      m_pDS->query(strSQL.c_str());
      while (!m_pDS->eof())
      {
        Face.discography.push_back(make_pair(m_pDS->fv("strAlbum").get_asString(),m_pDS->fv("strYear").get_asString()));
        m_pDS->next();
      }
      m_pDS->close();
      CStdString strPath;
      GetFacePath(Face.idFace,strPath);
      Face.Save(pMain, "Face", strPath);
      
      map<string, string> artwork;
      if (GetArtForItem(Face.idFace, "Face", artwork) && !singleFiles)
      { // append to the XML
        TiXmlElement additionalNode("art");
        for (map<string, string>::const_iterator i = artwork.begin(); i != artwork.end(); ++i)
          XMLUtils::SetString(&additionalNode, i->first.c_str(), i->second);
        pMain->LastChild()->InsertEndChild(additionalNode);
      }
      if (singleFiles)
      {
        if (!CDirectory::Exists(strPath))
          CLog::Log(LOGDEBUG, "%s - Not exporting item %s as it does not exist", __FUNCTION__, strPath.c_str());
        else
        {
          CStdString nfoFile = URIUtils::AddFileToFolder(strPath, "Face.nfo");
          if (overwrite || !CFile::Exists(nfoFile))
          {
            if (!xmlDoc.SaveFile(nfoFile))
              CLog::Log(LOGERROR, "%s: Face nfo export failed! ('%s')", __FUNCTION__, nfoFile.c_str());
          }
          
          if (images && !artwork.empty())
          {
            CStdString savedThumb = URIUtils::AddFileToFolder(strPath,"folder.jpg");
            CStdString savedFanart = URIUtils::AddFileToFolder(strPath,"fanart.jpg");
            if (artwork.find("thumb") != artwork.end() && (overwrite || !CFile::Exists(savedThumb)))
              CTextureCache::Get().Export(artwork["thumb"], savedThumb);
            if (artwork.find("fanart") != artwork.end() && (overwrite || !CFile::Exists(savedFanart)))
              CTextureCache::Get().Export(artwork["fanart"], savedFanart);
          }
          xmlDoc.Clear();
          TiXmlDeclaration decl("1.0", "UTF-8", "yes");
          xmlDoc.InsertEndChild(decl);
        }
      }
      
      if ((current % 50) == 0 && progress)
      {
        progress->SetLine(1, Face.strFace);
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }
      pDS->next();
      current++;
    }
    pDS->close();
    
    if (progress)
      progress->Close();
    
    xmlDoc.SaveFile(xmlFile);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CPictureDatabase::ImportFromXML(const CStdString &xmlFile)
{
  CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    
    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(xmlFile))
      return;
    
    TiXmlElement *root = xmlDoc.RootElement();
    if (!root) return;
    
    if (progress)
    {
      progress->SetHeading(20197);
      progress->SetLine(0, 649);
      progress->SetLine(1, 330);
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }
    
    TiXmlElement *entry = root->FirstChildElement();
    int current = 0;
    int total = 0;
    // first count the number of items...
    while (entry)
    {
      if (strnicmp(entry->Value(), "Face", 6)==0 ||
          strnicmp(entry->Value(), "album", 5)==0)
        total++;
      entry = entry->NextSiblingElement();
    }
    
    BeginTransaction();
    entry = root->FirstChildElement();
    while (entry)
    {
      CStdString strTitle;
      if (strnicmp(entry->Value(), "Face", 6) == 0)
      {
        CFace Face;
        Face.Load(entry);
        strTitle = Face.strFace;
        int idFace = GetFaceByName(Face.strFace);
        if (idFace > -1)
          SetFaceInfo(idFace,Face);
        
        current++;
      }
      else if (strnicmp(entry->Value(), "album", 5) == 0)
      {
        CPictureAlbum album;
        album.Load(entry);
        strTitle = album.strAlbum;
        int idAlbum = GetPictureAlbumByName(album.strAlbum,album.face);
        if (idAlbum > -1)
          SetPictureAlbumInfo(idAlbum,album,album.pictures,false);
        
        current++;
      }
      entry = entry ->NextSiblingElement();
      if (progress && total)
      {
        progress->SetPercentage(current * 100 / total);
        progress->SetLine(2, strTitle);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          RollbackTransaction();
          return;
        }
      }
    }
    CommitTransaction();
    
    g_infoManager.ResetLibraryBools();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  if (progress)
    progress->Close();
}

void CPictureDatabase::AddKaraokeData(int idPicture, int iKaraokeNumber, DWORD crc)
{
  try
  {
    CStdString strSQL;
    
    // If picture.iKaraokeNumber is non-zero, we already have it in the database. Just replace the picture ID.
    if (iKaraokeNumber > 0)
    {
      CStdString strSQL = PrepareSQL("UPDATE karaokedata SET idPicture=%i WHERE iKaraNumber=%i", idPicture, iKaraokeNumber);
      m_pDS->exec(strSQL.c_str());
      return;
    }
    
    // Get the maximum number allocated
    strSQL=PrepareSQL( "SELECT MAX(iKaraNumber) FROM karaokedata" );
    if (!m_pDS->query(strSQL.c_str())) return;
    
    int iKaraokeNumber = g_advancedSettings.m_karaokeStartIndex;
    
    if ( m_pDS->num_rows() == 1 )
      iKaraokeNumber = m_pDS->fv("MAX(iKaraNumber)").get_asInt() + 1;
    
    // Add the data
    strSQL=PrepareSQL( "INSERT INTO karaokedata (iKaraNumber, idPicture, iKaraDelay, strKaraEncoding, strKaralyrics, strKaraLyrFileCRC) "
                      "VALUES( %i, %i, 0, NULL, NULL, '%ul' )", iKaraokeNumber, idPicture, crc );
    
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s -(%i, %i) failed", __FUNCTION__, idPicture, iKaraokeNumber);
  }
}

bool CPictureDatabase::GetPictureByKaraokeNumber(int number, CPicture & picture)
{
  try
  {
    // Get info from karaoke db
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL=PrepareSQL("SELECT * FROM karaokedata where iKaraNumber=%ld", number);
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return false;
    }
    
    int idPicture = m_pDS->fv("karaokedata.idPicture").get_asInt();
    m_pDS->close();
    
    return GetPicture( idPicture, picture );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, number);
  }
  
  return false;
}

void CPictureDatabase::ExportKaraokeInfo(const CStdString & outFile, bool asHTML)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    
    // find all karaoke pictures
    CStdString sql = "SELECT * FROM pictureview WHERE iKaraNumber > 0 ORDER BY strFileName";
    
    m_pDS->query(sql.c_str());
    
    int total = m_pDS->num_rows();
    int current = 0;
    
    if ( total == 0 )
    {
      m_pDS->close();
      return;
    }
    
    // Write the document
    XFILE::CFile file;
    
    if ( !file.OpenForWrite( outFile, true ) )
      return;
    
    CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(asHTML ? 22034 : 22035);
      progress->SetLine(0, 650);
      progress->SetLine(1, "");
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }
    
    CStdString outdoc;
    if ( asHTML )
    {
      outdoc = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></meta></head>\n"
      "<body>\n<table>\n";
      
      file.Write( outdoc, outdoc.size() );
    }
    
    while (!m_pDS->eof())
    {
      CPicture picture = GetPictureFromDataset( false );
      CStdString picturenum;
      
      if ( asHTML )
        outdoc = "<tr><td>" + picturenum + "</td><td>" + StringUtils::Join(picture.face, g_advancedSettings.m_pictureItemSeparator) + "</td><td>" + picture.strTitle + "</td></tr>\r\n";
      else
        outdoc = picturenum + "\t" + StringUtils::Join(picture.face, g_advancedSettings.m_pictureItemSeparator) + "\t" + picture.strTitle + "\t" + picture.strFileName + "\r\n";
      
      file.Write( outdoc, outdoc.size() );
      
      if ((current % 50) == 0 && progress)
      {
        progress->SetPercentage(current * 100 / total);
        progress->Progress();
        if (progress->IsCanceled())
        {
          progress->Close();
          m_pDS->close();
          return;
        }
      }
      m_pDS->next();
      current++;
    }
    
    m_pDS->close();
    
    if ( asHTML )
    {
      outdoc = "</table>\n</body>\n</html>\n";
      file.Write( outdoc, outdoc.size() );
    }
    
    file.Close();
    
    if (progress)
      progress->Close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CPictureDatabase::ImportKaraokeInfo(const CStdString & inputFile)
{
  CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  
  try
  {
    if (NULL == m_pDB.get()) return;
    
    XFILE::CFile file;
    
    if ( !file.Open( inputFile ) )
    {
      CLog::Log( LOGERROR, "Cannot open karaoke import file %s", inputFile.c_str() );
      return;
    }
    
    unsigned int size = (unsigned int) file.GetLength();
    
    if ( !size )
      return;
    
    // Read the file into memory array
    std::vector<char> data( size + 1 );
    
    file.Seek( 0, SEEK_SET );
    
    // Read the whole file
    if ( file.Read( &data[0], size) != size )
    {
      CLog::Log( LOGERROR, "Cannot read karaoke import file %s", inputFile.c_str() );
      return;
    }
    
    file.Close();
    data[ size ] = '\0';
    
    if (progress)
    {
      progress->SetHeading( 22036 );
      progress->SetLine(0, 649);
      progress->SetLine(1, "");
      progress->SetLine(2, "");
      progress->SetPercentage(0);
      progress->StartModal();
      progress->ShowProgressBar(true);
    }
    
    if (NULL == m_pDS.get()) return;
    BeginTransaction();
    
    //
    // A simple state machine to parse the file
    //
    char * linestart = &data[0];
    unsigned int offset = 0, lastpercentage = 0;
    
    for ( char * p = &data[0]; *p; p++, offset++ )
    {
      // Skip \r
      if ( *p == 0x0D )
      {
        *p = '\0';
        continue;
      }
      
      // Line number
      if ( *p == 0x0A )
      {
        *p = '\0';
        
        unsigned int tabs = 0;
        char * picturepath, *Face = 0, *title = 0;
        for ( picturepath = linestart; *picturepath; picturepath++ )
        {
          if ( *picturepath == '\t' )
          {
            tabs++;
            *picturepath = '\0';
            
            switch( tabs )
            {
              case 1: // the number end
                Face = picturepath + 1;
                break;
                
              case 2: // the Face end
                title = picturepath + 1;
                break;
                
              case 3: // the title end
                break;
            }
          }
        }
        
        int num = atoi( linestart );
        if ( num <= 0 || tabs < 3 || *Face == '\0' || *title == '\0' )
        {
          CLog::Log( LOGERROR, "Karaoke import: error in line %s", linestart );
          linestart = p + 1;
          continue;
        }
        
        linestart = p + 1;
        CStdString strSQL=PrepareSQL("select idPicture from pictureview "
                                     "where strFace like '%s' and strTitle like '%s'", Face, title );
        
        if ( !m_pDS->query(strSQL.c_str()) )
        {
          RollbackTransaction();
          if (progress)
            progress->Close();
          m_pDS->close();
          return;
        }
        
        int iRowsFound = m_pDS->num_rows();
        if (iRowsFound == 0)
        {
          CLog::Log( LOGERROR, "Karaoke import: picture %s by %s #%d is not found in the database, skipped",
                    title, Face, num );
          continue;
        }
        
        int lResult = m_pDS->fv(0).get_asInt();
        strSQL = PrepareSQL("UPDATE karaokedata SET iKaraNumber=%i WHERE idPicture=%i", num, lResult );
        m_pDS->exec(strSQL.c_str());
        
        if ( progress && (offset * 100 / size) != lastpercentage )
        {
          lastpercentage = offset * 100 / size;
          progress->SetPercentage( lastpercentage);
          progress->Progress();
          if ( progress->IsCanceled() )
          {
            RollbackTransaction();
            progress->Close();
            m_pDS->close();
            return;
          }
        }
      }
    }
    
    CommitTransaction();
    CLog::Log( LOGNOTICE, "Karaoke import: file '%s' was imported successfully", inputFile.c_str() );
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  if (progress)
    progress->Close();
}

bool CPictureDatabase::SetKaraokePictureDelay(int idPicture, int delay)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL = PrepareSQL("UPDATE karaokedata SET iKaraDelay=%i WHERE idPicture=%i", delay, idPicture);
    m_pDS->exec(strSQL.c_str());
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  
  return false;
}

int CPictureDatabase::GetKaraokePicturesCount()
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;
    
    if (!m_pDS->query( "select count(idPicture) as NumPictures from karaokedata")) return 0;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }
    
    int iNumPictures = m_pDS->fv("NumPictures").get_asInt();
    // cleanup
    m_pDS->close();
    return iNumPictures;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return 0;
}

void CPictureDatabase::SetPropertiesFromFace(CFileItem& item, const CFace& Face)
{
  item.SetProperty("face_instrument", StringUtils::Join(Face.instruments, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("face_instrument_array", Face.instruments);
  item.SetProperty("face_style", StringUtils::Join(Face.styles, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("face_style_array", Face.styles);
  item.SetProperty("face_mood", StringUtils::Join(Face.moods, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("face_mood_array", Face.moods);
  item.SetProperty("face_born", Face.strBorn);
  item.SetProperty("face_formed", Face.strFormed);
  item.SetProperty("face_description", Face.strBiography);
  item.SetProperty("face_location", StringUtils::Join(Face.location, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("face_location_array", Face.location);
  item.SetProperty("face_died", Face.strDied);
  item.SetProperty("face_disbanded", Face.strDisbanded);
  item.SetProperty("face_yearsactive", StringUtils::Join(Face.yearsActive, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("face_yearsactive_array", Face.yearsActive);
}

void CPictureDatabase::SetPropertiesFromPictureAlbum(CFileItem& item, const CPictureAlbum& album)
{
  item.SetProperty("album_description", album.strReview);
  item.SetProperty("album_theme", StringUtils::Join(album.themes, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("album_theme_array", album.themes);
  item.SetProperty("album_mood", StringUtils::Join(album.moods, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("album_mood_array", album.moods);
  item.SetProperty("album_style", StringUtils::Join(album.styles, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("album_style_array", album.styles);
  item.SetProperty("album_type", album.strType);
  item.SetProperty("album_label", album.strLabel);
  item.SetProperty("album_Face", StringUtils::Join(album.face, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("album_face_array", album.face);
  item.SetProperty("album_location", StringUtils::Join(album.location, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("album_location_array", album.location);
  item.SetProperty("album_title", album.strAlbum);
}

void CPictureDatabase::SetPropertiesForFileItem(CFileItem& item)
{
  if (!item.HasPictureInfoTag())
    return;
  /*
   int idFace = GetFaceByName(StringUtils::Join(item.GetPictureInfoTag()->GetFace(), g_advancedSettings.m_pictureItemSeparator));
   if (idFace > -1)
   {
   CFace Face;
   if (GetFaceInfo(idFace,Face))
   SetPropertiesFromFace(item,Face);
   }
   int idAlbum = item.GetPictureInfoTag()->GetPictureAlbumId();
   if (idAlbum <= 0)
   idAlbum = GetPictureAlbumByName(item.GetPictureInfoTag()->GetPictureAlbum(),
   item.GetPictureInfoTag()->GetFace());
   if (idAlbum > -1)
   {
   CPictureAlbum album;
   if (GetPictureAlbumInfo(idAlbum,album,NULL,true)) // true to force additional information
   SetPropertiesFromPictureAlbum(item,album);
   }
   */
}

void CPictureDatabase::SetArtForItem(int mediaId, const string &mediaType, const map<string, string> &art)
{
  for (map<string, string>::const_iterator i = art.begin(); i != art.end(); ++i)
    SetArtForItem(mediaId, mediaType, i->first, i->second);
}

void CPictureDatabase::SetArtForItem(int mediaId, const string &mediaType, const string &artType, const string &url)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    
    // don't set <foo>.<bar> art types - these are derivative types from parent items
    if (artType.find('.') != string::npos)
      return;
    
    CStdString sql = PrepareSQL("SELECT art_id FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
    { // update
      int artId = m_pDS->fv(0).get_asInt();
      m_pDS->close();
      sql = PrepareSQL("UPDATE art SET url='%s' where art_id=%d", url.c_str(), artId);
      m_pDS->exec(sql.c_str());
    }
    else
    { // insert
      m_pDS->close();
      sql = PrepareSQL("INSERT INTO art(media_id, media_type, type, url) VALUES (%d, '%s', '%s', '%s')", mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
      m_pDS->exec(sql.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d, '%s', '%s', '%s') failed", __FUNCTION__, mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
  }
}

//$$$$this is it

bool CPictureDatabase::GetArtForItem(int mediaId, const string &mediaType, map<string, string> &art)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1
    
    CStdString sql = PrepareSQL("SELECT type,url FROM art WHERE media_id=%i AND media_type='%s'", mediaId, mediaType.c_str());
    m_pDS2->query(sql.c_str());
    while (!m_pDS2->eof())
    {
      art.insert(make_pair(m_pDS2->fv(0).get_asString(), m_pDS2->fv(1).get_asString()));
      m_pDS2->next();
    }
    m_pDS2->close();
    return !art.empty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return false;
}

string CPictureDatabase::GetArtForItem(int mediaId, const string &mediaType, const string &artType)
{
  std::string query = PrepareSQL("SELECT url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
  return GetSingleValue(query, m_pDS2);
}

bool CPictureDatabase::GetFaceArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1
    
    CStdString sql = PrepareSQL("SELECT type,url FROM art WHERE media_id=(SELECT idFace from %s_Face WHERE id%s=%i AND iOrder=0) AND media_type='Face'", mediaType.c_str(), mediaType.c_str(), mediaId);
    m_pDS2->query(sql.c_str());
    while (!m_pDS2->eof())
    {
      art.insert(make_pair(m_pDS2->fv(0).get_asString(), m_pDS2->fv(1).get_asString()));
      m_pDS2->next();
    }
    m_pDS2->close();
    return !art.empty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return false;
}

string CPictureDatabase::GetFaceArtForItem(int mediaId, const string &mediaType, const string &artType)
{
  std::string query = PrepareSQL("SELECT url FROM art WHERE media_id=(SELECT idFace from %s_Face WHERE id%s=%i AND iOrder=0) AND media_type='Face' AND type='%s'", mediaType.c_str(), mediaType.c_str(), mediaId, artType.c_str());
  return GetSingleValue(query, m_pDS2);
}

bool CPictureDatabase::GetFilter(CDbUrl &PictureUrl, Filter &filter, SortDescription &sorting)
{
  if (!PictureUrl.IsValid())
    return false;
  
  std::string type = PictureUrl.GetType();
  const CUrlOptions::UrlOptions& options = PictureUrl.GetOptions();
  CUrlOptions::UrlOptions::const_iterator option;
  
  if (type == "Faces")
  {
    int idFace = -1, idLocation = -1, idAlbum = -1, idPicture = -1;
    bool albumFacesOnly = false;
    
    option = options.find("Faceid");
    if (option != options.end())
      idFace = (int)option->second.asInteger();
    
    option = options.find("locationid");
    if (option != options.end())
      idLocation = (int)option->second.asInteger();
    else
    {
      option = options.find("location");
      if (option != options.end())
        idLocation = GetLocationByName(option->second.asString());
    }
    
    option = options.find("albumid");
    if (option != options.end())
      idAlbum = (int)option->second.asInteger();
    else
    {
      option = options.find("album");
      if (option != options.end())
        idAlbum = GetPictureAlbumByName(option->second.asString());
    }
    
    option = options.find("pictureid");
    if (option != options.end())
      idPicture = (int)option->second.asInteger();
    
    option = options.find("albumFacesonly");
    if (option != options.end())
      albumFacesOnly = option->second.asBoolean();
    
    CStdString strSQL = "(Faceview.idFace IN ";
    if (idFace > 0)
      strSQL += PrepareSQL("(%d)", idFace);
    else if (idAlbum > 0)
      strSQL += PrepareSQL("(SELECT album_Face.idFace FROM album_Face WHERE album_Face.idAlbum = %i)", idAlbum);
    else if (idPicture > 0)
      strSQL += PrepareSQL("(SELECT picture_Face.idFace FROM picture_Face WHERE picture_Face.idPicture = %i)", idPicture);
    else if (idLocation > 0)
    { // same statements as below, but limit to the specified location
      // in this case we show the whole lot always - there is no limitation to just album Faces
      if (!albumFacesOnly)  // show all Faces in this case (ie those linked to a picture)
        strSQL+=PrepareSQL("(SELECT picture_Face.idFace FROM picture_Face" // All Faces linked to extra locations
                           " JOIN picture_location ON picture_Face.idPicture = picture_location.idPicture"
                           " WHERE picture_location.idLocation = %i)"
                           " OR idFace IN ", idLocation);
      // and add any Faces linked to an album (may be different from above due to album Face tag)
      strSQL += PrepareSQL("(SELECT album_Face.idFace FROM album_Face" // All album Faces linked to extra locations
                           " JOIN album_location ON album_Face.idAlbum = album_location.idAlbum"
                           " WHERE album_location.idLocation = %i)", idLocation);
    }
    else
    {
      if (!albumFacesOnly)  // show all Faces in this case (ie those linked to a picture)
        strSQL += "(SELECT picture_Face.idFace FROM picture_Face)"
        " OR Faceview.idFace IN ";
      
      // and always show any Faces linked to an album (may be different from above due to album Face tag)
      strSQL +=   "(SELECT album_Face.idFace FROM album_Face"; // All Faces linked to an album
      if (albumFacesOnly)
        strSQL += " JOIN album ON album.idAlbum = album_Face.idAlbum WHERE album.bCompilation = 0 ";            // then exclude those that have no extra Faces
      strSQL +=   ")";
    }
    
    // remove the null string
    strSQL += ") and Faceview.strFace != ''";
    
    // and the various Face entry if applicable
    if (!albumFacesOnly)
    {
      CStdString strVariousFaces = g_localizeStrings.Get(340);
      strSQL += PrepareSQL(" and Faceview.strFace <> '%s'", strVariousFaces.c_str());
    }
    
    filter.AppendWhere(strSQL);
  }
  else if (type == "albums")
  {
    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.iYear = %i", (int)option->second.asInteger()));

    option = options.find("albumid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.idAlbum = %i", (int)option->second.asInteger()));
    
    option = options.find("compilation");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.bCompilation = %i", option->second.asBoolean() ? 1 : 0));
    
    option = options.find("locationid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.idAlbum IN (SELECT picture.idAlbum FROM picture JOIN picture_location ON picture.idPicture = picture_location.idPicture WHERE picture_location.idLocation = %i)", (int)option->second.asInteger()));
    
    option = options.find("location");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("albumview.idAlbum IN (SELECT picture.idAlbum FROM picture JOIN picture_location ON picture.idPicture = picture_location.idPicture JOIN location ON location.idLocation = picture_location.idLocation WHERE location.strLocation like '%s')", option->second.asString().c_str()));
    
    option = options.find("Faceid");
    if (option != options.end())
    {
      filter.AppendJoin("JOIN picture ON picture.idAlbum = albumview.idAlbum "
                        "JOIN picture_Face ON picture.idPicture = picture_Face.idPicture "
                        "JOIN album_Face ON albumview.idAlbum = album_Face.idAlbum");
      filter.AppendWhere(PrepareSQL("      picture_Face.idFace = %i" // All albums linked to this Face via pictures
                                    " OR  album_Face.idFace = %i", // All albums where album Faces fit
                                    (int)option->second.asInteger(), (int)option->second.asInteger()));
      filter.AppendGroup("albumview.idAlbum");
    }
    else
    {
      option = options.find("Face");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("albumview.idAlbum IN (SELECT picture.idAlbum FROM picture JOIN picture_Face ON picture.idPicture = picture_Face.idPicture JOIN Face ON Face.idFace = picture_Face.idFace WHERE Face.strFace like '%s')" // All albums linked to this Face via pictures
                                      " OR albumview.idAlbum IN (SELECT album_Face.idAlbum FROM album_Face JOIN Face ON Face.idFace = album_Face.idFace WHERE Face.strFace like '%s')", // All albums where album Faces fit
                                      option->second.asString().c_str(), option->second.asString().c_str()));
      // no Face given, so exclude any single albums (aka empty tagged albums)
      else
        filter.AppendWhere("albumview.strAlbum <> ''");
    }
  }
  else if (type == "pictures" || type == "singles")
  {
    option = options.find("singles");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("pictureview.idAlbum %sIN (SELECT idAlbum FROM album WHERE strAlbum = '')", option->second.asBoolean() ? "" : "NOT "));
    
    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("pictureview.iYear = %i", (int)option->second.asInteger()));
    
    option = options.find("compilation");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("pictureview.bCompilation = %i", option->second.asBoolean() ? 1 : 0));
    
    option = options.find("albumid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("pictureview.idAlbum = %i", (int)option->second.asInteger()));
    
    option = options.find("album");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("pictureview.strAlbum like '%s'", option->second.asString().c_str()));
    
    option = options.find("locationid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("pictureview.idPicture IN (SELECT picture_location.idPicture FROM picture_location WHERE picture_location.idLocation = %i)", (int)option->second.asInteger()));
    
    option = options.find("location");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("pictureview.idPicture IN (SELECT picture_location.idPicture FROM picture_location JOIN location ON location.idLocation = picture_location.idLocation WHERE location.strLocation like '%s')", option->second.asString().c_str()));
    
    option = options.find("Faceid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("pictureview.idPicture IN (SELECT picture_Face.idPicture FROM picture_Face WHERE picture_Face.idFace = %i)" // picture Faces
                                    " OR pictureview.idPicture IN (SELECT picture.idPicture FROM picture JOIN album_Face ON picture.idAlbum=album_Face.idAlbum WHERE album_Face.idFace = %i)", // album Faces
                                    (int)option->second.asInteger(), (int)option->second.asInteger()));
    
    option = options.find("Face");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("pictureview.idPicture IN (SELECT picture_Face.idPicture FROM picture_Face JOIN Face ON Face.idFace = picture_Face.idFace WHERE Face.strFace like '%s')" // picture Faces
                                    " OR pictureview.idPicture IN (SELECT picture.idPicture FROM picture JOIN album_Face ON picture.idAlbum=album_Face.idAlbum JOIN Face ON Face.idFace = album_Face.idFace WHERE Face.strFace like '%s')", // album Faces
                                    option->second.asString().c_str(), option->second.asString().c_str()));
  }
  
  option = options.find("xsp");
  if (option != options.end())
  {
    CSmartPlaylist xsp;
    if (!xsp.LoadFromJson(option->second.asString()))
      return false;
    
    // check if the filter playlist matches the item type
    if (xsp.GetType()  == type ||
        (xsp.GetGroup() == type && !xsp.IsGroupMixed()))
    {
      std::set<CStdString> playlists;
      filter.AppendWhere(xsp.GetWhereClause(*this, playlists));
      
      if (xsp.GetLimit() > 0)
        sorting.limitEnd = xsp.GetLimit();
      if (xsp.GetOrder() != SortByNone)
        sorting.sortBy = xsp.GetOrder();
      sorting.sortOrder = xsp.GetOrderAscending() ? SortOrderAscending : SortOrderDescending;
      if (CSettings::Get().GetBool("filelists.ignorethewhensorting"))
        sorting.sortAttributes = SortAttributeIgnoreArticle;
    }
  }
  
  option = options.find("filter");
  if (option != options.end())
  {
    CSmartPlaylist xspFilter;
    if (!xspFilter.LoadFromJson(option->second.asString()))
      return false;
    
    // check if the filter playlist matches the item type
    if (xspFilter.GetType() == type)
    {
      std::set<CStdString> playlists;
      filter.AppendWhere(xspFilter.GetWhereClause(*this, playlists));
    }
    // remove the filter if it doesn't match the item type
    else
      PictureUrl.RemoveOption("filter");
  }
  
  return true;
}
