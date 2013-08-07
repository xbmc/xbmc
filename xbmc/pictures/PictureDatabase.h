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
/*!
 \file PictureDatabase.h
 \brief
 */
#pragma once
#include "dbwrappers/Database.h"
#include "PictureAlbum.h"
#include "addons/Scraper.h"
#include "utils/SortUtils.h"
#include "PictureDbUrl.h"

class CFace;
class CFileItem;

namespace dbiplus
{
    class field_value;
    typedef std::vector<field_value> sql_record;
}

#include <set>

// return codes of Cleaning up the Database
// numbers are strings from strings.xml
#define ERROR_OK     317
#define ERROR_CANCEL    0
#define ERROR_DATABASE    315
#define ERROR_REORG_PICTURES   319
#define ERROR_REORG_FACE   321
#define ERROR_REORG_LOCATION   323
#define ERROR_REORG_PATH   325
#define ERROR_REORG_PICTURE_ALBUM   327
#define ERROR_WRITING_CHANGES  329
#define ERROR_COMPRESSING   332

#define NUM_PICTURES_BEFORE_COMMIT 500

/*!
 \ingroup Picture
 \brief A set of CStdString objects, used for CPictureDatabase
 \sa ISETPATHES, CPictureDatabase
 */
typedef std::set<CStdString> SETPATHES;

/*!
 \ingroup Picture
 \brief The SETPATHES iterator
 \sa SETPATHES, CPictureDatabase
 */
typedef std::set<CStdString>::iterator ISETPATHES;

class CGUIDialogProgress;
class CFileItemList;

/*!
 \ingroup Picture
 \brief Class to store and read tag information
 
 CPictureDatabase can be used to read and store
 tag information for faster access. It is based on
 sqlite (http://www.sqlite.org).
 
 Here is the database layout:
 \image html Picturedatabase.png
 
 \sa CPictureAlbum, CPicture, VECPICTURES, CMapPicture, VECFaceS, VECPICTUREALBUMS, VECGENRES
 */
class CPictureDatabase : public CDatabase
{
    friend class DatabaseUtils;
    friend class TestDatabaseUtilsHelper;
    
public:
    CPictureDatabase(void);
    virtual ~CPictureDatabase(void);
    
    virtual bool Open();
    virtual bool CommitTransaction();
    void EmptyCache();
    void Clean();
    int  Cleanup(CGUIDialogProgress *pDlgProgress=NULL);
    void DeletePictureAlbumInfo();
    bool LookupCDDBInfo(bool bRequery=false);
    void DeleteCDDBInfo();
    
    /////////////////////////////////////////////////
    // Picture CRUD
    /////////////////////////////////////////////////
    /*! \brief Add a picture to the database
     \param idPictureAlbum [in] the database ID of the PictureAlbum for the picture
     \param strTitle [in] the title of the picture (required to be non-empty)
     \param strPictureBrainzTrackID [in] the PictureBrainz track ID of the picture
     \param strPathAndFileName [in] the path and filename to the picture
     \param strComment [in] the ids of the added pictures
     \param strThumb [in] the ids of the added pictures
     \param Faces [in] a vector of Face names (will only be used for the cache names in the PictureAlbum views)
     \param locations [in] a vector of locations to which this picture belongs
     \param iTrack [in] the track number and disc number of the picture
     \param iDuration [in] the duration of the picture
     \param iYear [in] the year of the picture
     \param iTimesPlayed [in] the number of times the picture has been played
     \param iStartOffset [in] the start offset of the picture (when using a single audio file with a .cue)
     \param iEndOffset [in] the end offset of the picture (when using a single audio file with .cue)
     \param dtLastPlayed [in] the time the picture was last played
     \param rating [in] a rating for the picture
     \param iKaraokeNumber [in] the karaoke id of the picture
     \return the id of the picture
     */
    int AddPicture(const int idPictureAlbum, const CStdString& strTitle, const CStdString& strPictureBrainzTrackID, const CStdString& strPathAndFileName, const CStdString& strComment, const CStdString& strThumb, const std::vector<std::string>& Faces, const std::vector<std::string>& locations, int iTrack, int iDuration, int iYear, const int iTimesPlayed, int iStartOffset, int iEndOffset, const CDateTime& dtLastPlayed, char rating, int iKaraokeNumber);
    bool GetPicture(int idPicture, CPicture& picture);
    
    /*! \brief Update a picture in the database
     \param idPicture [in] the database ID of the picture to update
     \param strTitle [in] the title of the picture (required to be non-empty)
     \param strPictureBrainzTrackID [in] the PictureBrainz track ID of the picture
     \param strPathAndFileName [in] the path and filename to the picture
     \param strComment [in] the ids of the added pictures
     \param strThumb [in] the ids of the added pictures
     \param Faces [in] a vector of Face names (will only be used for the cache names in the PictureAlbum views)
     \param locations [in] a vector of locations to which this picture belongs
     \param iTrack [in] the track number and disc number of the picture
     \param iDuration [in] the duration of the picture
     \param iYear [in] the year of the picture
     \param iTimesPlayed [in] the number of times the picture has been played
     \param iStartOffset [in] the start offset of the picture (when using a single audio file with a .cue)
     \param iEndOffset [in] the end offset of the picture (when using a single audio file with .cue)
     \param dtLastPlayed [in] the time the picture was last played
     \param rating [in] a rating for the picture
     \param iKaraokeNumber [in] the karaoke id of the picture
     \return the id of the picture
     */
    int UpdatePicture(int idPicture, const CStdString& strTitle, const CStdString& strPictureBrainzTrackID, const CStdString& strPathAndFileName, const CStdString& strComment, const CStdString& strThumb, const std::vector<std::string>& Faces, const std::vector<std::string>& locations, int iTrack, int iDuration, int iYear, int iTimesPlayed, int iStartOffset, int iEndOffset, const CDateTime& dtLastPlayed, char rating, int iKaraokeNumber);
    // bool DeletePicture(int idPicture);
    
    //// Misc Picture
    bool GetPictureByFileName(const CStdString& strFileName, CPicture& picture, int startOffset = 0);
    bool GetPicturesByPath(const CStdString& strPath, MAPPICTURES& pictures, bool bAppendToMap = false);
    bool Search(const CStdString& search, CFileItemList &items);
    bool RemovePicturesFromPath(const CStdString &path, MAPPICTURES& pictures, bool exact=true);
    bool SetPictureRating(const CStdString &filePath, char rating);
    int  GetPictureByFaceAndPictureAlbumAndTitle(const CStdString& strFace, const CStdString& strPictureAlbum, const CStdString& strTitle);
    
    /////////////////////////////////////////////////
    // PictureAlbum
    /////////////////////////////////////////////////
    /*! \brief Add an PictureAlbum and all its pictures to the database
     \param PictureAlbum the PictureAlbum to add
     \param pictureIDs [out] the ids of the added pictures
     \return the id of the PictureAlbum
     */
    int  AddPictureAlbum(const CStdString& strPictureAlbum, const CStdString& strPictureBrainzPictureAlbumID, const CStdString& strFace, const CStdString& strLocation, int year, bool bCompilation);
    bool GetPictureAlbum(int idPictureAlbum, CPictureAlbum& PictureAlbum);
    int  UpdatePictureAlbum(int idPictureAlbum, const CPictureAlbum &PictureAlbum);
    bool DeletePictureAlbum(int idPictureAlbum);
    /*! \brief Checks if the given path is inside a folder that has already been scanned into the library
     \param path the path we want to check
     */
    bool InsideScannedPath(const CStdString& path);
    
    //// Misc PictureAlbum
    int  GetPictureAlbumIdByPath(const CStdString& path);
    bool GetPictureAlbumFromPicture(int idPicture, CPictureAlbum &PictureAlbum);
    int  GetPictureAlbumByName(const CStdString& strPictureAlbum, const CStdString& strFace="");
    int  GetPictureAlbumByName(const CStdString& strPictureAlbum, const std::vector<std::string>& Face);
    CStdString GetPictureAlbumById(int id);
    
    /////////////////////////////////////////////////
    // Face CRUD
    /////////////////////////////////////////////////
    int  AddFace(const CStdString& strFace, const CStdString& strPictureBrainzFaceID);
    bool GetFace(int idFace, CFace& Face);
    int  UpdateFace(int idFace, const CFace& Face);
    bool DeleteFace(int idFace);
    
    CStdString GetFaceById(int id);
    int GetFaceByName(const CStdString& strFace);
    
    /////////////////////////////////////////////////
    // Paths
    /////////////////////////////////////////////////
    int AddPath(const CStdString& strPath);
    
    bool GetPaths(std::set<std::string> &paths);
    bool SetPathHash(const CStdString &path, const CStdString &hash);
    bool GetPathHash(const CStdString &path, CStdString &hash);
    bool GetPictureAlbumPath(int idPictureAlbum, CStdString &path);
    bool GetFacePath(int idFace, CStdString &path);
    
    /////////////////////////////////////////////////
    // Locations
    /////////////////////////////////////////////////
    int AddLocation(const CStdString& strLocation);
    CStdString GetLocationById(int id);
    int GetLocationByName(const CStdString& strLocation);
    
    /////////////////////////////////////////////////
    // PictureAlbumInfo
    /////////////////////////////////////////////////
    bool HasPictureAlbumInfo(int idPictureAlbum);
    int SetPictureAlbumInfo(int idPictureAlbum, const CPictureAlbum& PictureAlbum, const VECPICTURES& pictures, bool bTransaction=true);
    bool GetPictureAlbumInfo(int idPictureAlbum, CPictureAlbum &info, VECPICTURES* pictures, bool scrapedInfo = false);
    bool DeletePictureAlbumInfo(int idFace);
    bool SetPictureAlbumInfoPictures(int idPictureAlbumInfo, const VECPICTURES& pictures);
    bool GetPictureAlbumInfoPictures(int idPictureAlbumInfo, VECPICTURES& pictures);
    
    /////////////////////////////////////////////////
    // FaceInfo
    /////////////////////////////////////////////////
    /*! \brief Check if an Face entity has additional metadata (scraped)
     \param idFace the id of the Face to check
     \return true or false - whether the Face has metadata
     */
    bool HasFaceInfo(int idFace);
    int SetFaceInfo(int idFace, const CFace& Face);
    bool GetFaceInfo(int idFace, CFace &info, bool needAll=true);
    bool DeleteFaceInfo(int idFace);
    
    /////////////////////////////////////////////////
    // Link tables
    /////////////////////////////////////////////////
    bool AddPictureAlbumFace(int idFace, int idPictureAlbum, std::string joinPhrase, bool featured, int iOrder);
    bool GetPictureAlbumsByFace(int idFace, bool includeFeatured, std::vector<int>& PictureAlbums);
    bool GetFacesByPictureAlbum(int idPictureAlbum, bool includeFeatured, std::vector<int>& Faces);
    
    bool AddPictureFace(int idFace, int idPicture, std::string joinPhrase, bool featured, int iOrder);
    bool GetPicturesByFace(int idFace, bool includeFeatured, std::vector<int>& pictures);
    bool GetFacesByPicture(int idPicture, bool includeFeatured, std::vector<int>& Faces);
    
    bool AddPictureLocation(int idLocation, int idPicture, int iOrder);
    bool GetLocationsByPicture(int idPicture, std::vector<int>& locations);
    
    bool AddPictureAlbumLocation(int idLocation, int idPictureAlbum, int iOrder);
    bool GetLocationsByPictureAlbum(int idPictureAlbum, std::vector<int>& locations);
    
    /////////////////////////////////////////////////
    // Top 100
    /////////////////////////////////////////////////
    bool GetTop100(const CStdString& strBaseDir, CFileItemList& items);
    bool GetTop100PictureAlbums(VECPICTUREALBUMS& PictureAlbums);
    bool GetTop100PictureAlbumPictures(const CStdString& strBaseDir, CFileItemList& item);
    
    /////////////////////////////////////////////////
    // Recently added
    /////////////////////////////////////////////////
    bool GetRecentlyAddedPictureAlbums(VECPICTUREALBUMS& PictureAlbums, unsigned int limit=0);
    bool GetRecentlyAddedPictureAlbumPictures(const CStdString& strBaseDir, CFileItemList& item, unsigned int limit=0);
    bool GetRecentlyPlayedPictureAlbums(VECPICTUREALBUMS& PictureAlbums);
    bool GetRecentlyPlayedPictureAlbumPictures(const CStdString& strBaseDir, CFileItemList& item);
    
    /////////////////////////////////////////////////
    // Compilations
    /////////////////////////////////////////////////
    bool GetCompilationPictureAlbums(const CStdString& strBaseDir, CFileItemList& items);
    bool GetCompilationPictures(const CStdString& strBaseDir, CFileItemList& items);
    int  GetCompilationPictureAlbumsCount();
    bool GetVariousFacesPictureAlbums(const CStdString& strBaseDir, CFileItemList& items);
    bool GetVariousFacesPictureAlbumsPictures(const CStdString& strBaseDir, CFileItemList& items);
    int GetVariousFacesPictureAlbumsCount();
    
    /*! \brief Increment the playcount of an item
     Increments the playcount and updates the last played date
     \param item CFileItem to increment the playcount for
     */
    void IncrementPlayCount(const CFileItem &item);
    bool CleanupOrphanedItems();
    
    /////////////////////////////////////////////////
    // VIEWS
    /////////////////////////////////////////////////
    bool GetLocationsNav(const CStdString& strBaseDir, CFileItemList& items, const Filter &filter = Filter(), bool countOnly = false);
    bool GetYearsNav(const CStdString& strBaseDir, CFileItemList& items, const Filter &filter = Filter());
    bool GetFacesNav(const CStdString& strBaseDir, CFileItemList& items, bool PictureAlbumFacesOnly = false, int idLocation = -1, int idPictureAlbum = -1, int idPicture = -1, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
    bool GetCommonNav(const CStdString &strBaseDir, const CStdString &table, const CStdString &labelField, CFileItemList &items, const Filter &filter /* = Filter() */, bool countOnly /* = false */);
    bool GetPictureAlbumTypesNav(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), bool countOnly = false);
    bool GetPictureLabelsNav(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), bool countOnly = false);
    bool GetPictureAlbumsNav(const CStdString& strBaseDir, CFileItemList& items, int idLocation = -1, int idFace = -1, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
    bool GetPictureAlbumsByYear(const CStdString &strBaseDir, CFileItemList& items, int year);
    bool GetPicturesNav(const CStdString& strBaseDir, CFileItemList& items, int idLocation, int idFace,int idPictureAlbum, const SortDescription &sortDescription = SortDescription());
    bool GetPicturesByYear(const CStdString& baseDir, CFileItemList& items, int year);
    bool GetPicturesByWhere(const CStdString &baseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription());
    bool GetPictureAlbumsByWhere(const CStdString &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
    bool GetFacesByWhere(const CStdString& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), bool countOnly = false);
    bool GetRandomPicture(CFileItem* item, int& idPicture, const Filter &filter);
    int GetPicturesCount(const Filter &filter = Filter());
    unsigned int GetPictureIDs(const Filter &filter, std::vector<std::pair<int,int> > &pictureIDs);
    virtual bool GetFilter(CDbUrl &PictureUrl, Filter &filter, SortDescription &sorting);
    
    /////////////////////////////////////////////////
    // Scraper
    /////////////////////////////////////////////////
    bool SetScraperForPath(const CStdString& strPath, const ADDON::ScraperPtr& info);
    bool GetScraperForPath(const CStdString& strPath, ADDON::ScraperPtr& info, const ADDON::TYPE &type);
    
    /*! \brief Check whether a given scraper is in use.
     \param scraperID the scraper to check for.
     \return true if the scraper is in use, false otherwise.
     */
    bool ScraperInUse(const CStdString &scraperID) const;
    
    /////////////////////////////////////////////////
    // Karaoke
    /////////////////////////////////////////////////
    void AddKaraokeData(int idPicture, int iKaraokeNumber, DWORD crc);
    bool GetPictureByKaraokeNumber( int number, CPicture& picture );
    bool SetKaraokePictureDelay( int idPicture, int delay );
    int GetKaraokePicturesCount();
    void ExportKaraokeInfo(const CStdString &outFile, bool asHTML );
    void ImportKaraokeInfo(const CStdString &inputFile );
    
    /////////////////////////////////////////////////
    // Filters
    /////////////////////////////////////////////////
    bool GetItems(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
    bool GetItems(const CStdString &strBaseDir, const CStdString &itemType, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
    CStdString GetItemById(const CStdString &itemType, int id);
    
    /////////////////////////////////////////////////
    // XML
    /////////////////////////////////////////////////
    void ExportToXML(const CStdString &xmlFile, bool singleFiles = false, bool images=false, bool overwrite=false);
    void ImportFromXML(const CStdString &xmlFile);
    
    /////////////////////////////////////////////////
    // Properties
    /////////////////////////////////////////////////
    void SetPropertiesForFileItem(CFileItem& item);
    static void SetPropertiesFromFace(CFileItem& item, const CFace& Face);
    static void SetPropertiesFromPictureAlbum(CFileItem& item, const CPictureAlbum& PictureAlbum);
    
    /////////////////////////////////////////////////
    // Art
    /////////////////////////////////////////////////
    bool SavePictureAlbumThumb(int idPictureAlbum, const CStdString &thumb);
    /*! \brief Sets art for a database item.
     Sets a single piece of art for a database item.
     \param mediaId the id in the media (picture/Face/PictureAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (picture/Face/PictureAlbum).
     \param artType the type of art to set, e.g. "thumb", "fanart"
     \param url the url to the art (this is the original url, not a cached url).
     \sa GetArtForItem
     */
    void SetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType, const std::string &url);
    
    /*! \brief Sets art for a database item.
     Sets multiple pieces of art for a database item.
     \param mediaId the id in the media (picture/Face/PictureAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (picture/Face/PictureAlbum).
     \param art a map of <type, url> where type is "thumb", "fanart", etc. and url is the original url of the art.
     \sa GetArtForItem
     */
    void SetArtForItem(int mediaId, const std::string &mediaType, const std::map<std::string, std::string> &art);
    
    /*! \brief Fetch art for a database item.
     Fetches multiple pieces of art for a database item.
     \param mediaId the id in the media (picture/Face/PictureAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (picture/Face/PictureAlbum).
     \param art [out] a map of <type, url> where type is "thumb", "fanart", etc. and url is the original url of the art.
     \return true if art is retrieved, false if no art is found.
     \sa SetArtForItem
     */
    bool GetArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art);
    
    /*! \brief Fetch art for a database item.
     Fetches a single piece of art for a database item.
     \param mediaId the id in the media (picture/Face/PictureAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (picture/Face/PictureAlbum).
     \param artType the type of art to retrieve, eg "thumb", "fanart".
     \return the original URL to the piece of art, if available.
     \sa SetArtForItem
     */
    std::string GetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType);
    
    /*! \brief Fetch Face art for a picture or PictureAlbum item.
     Fetches the art associated with the primary Face for the picture or PictureAlbum.
     \param mediaId the id in the media (picture/PictureAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (picture/PictureAlbum).
     \param art [out] the art map <type, url> of Face art.
     \return true if Face art is found, false otherwise.
     \sa GetArtForItem
     */
    bool GetFaceArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art);
    
    /*! \brief Fetch Face art for a picture or PictureAlbum item.
     Fetches a single piece of art associated with the primary Face for the picture or PictureAlbum.
     \param mediaId the id in the media (picture/PictureAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (picture/PictureAlbum).
     \param artType the type of art to retrieve, eg "thumb", "fanart".
     \return the original URL to the piece of art, if available.
     \sa GetArtForItem
     */
    std::string GetFaceArtForItem(int mediaId, const std::string &mediaType, const std::string &artType);
    
protected:
    std::map<CStdString, int> m_FaceCache;
    std::map<CStdString, int> m_locationCache;
    std::map<CStdString, int> m_pathCache;
    std::map<CStdString, int> m_thumbCache;
    std::map<CStdString, CPictureAlbum> m_albumCache;
    
    virtual bool CreateTables();
    virtual int GetMinVersion() const;
    
    const char *GetBaseDBName() const { return "MyPicture"; };
    
    
private:
    /*! \brief (Re)Create the generic database views for pictures and PictureAlbums
     */
    virtual void CreateViews();
    
    void SplitString(const CStdString &multiString, std::vector<std::string> &vecStrings, CStdString &extraStrings);
    CPicture GetPictureFromDataset(bool bWithPictureDbPath=false);
    CFace GetFaceFromDataset(dbiplus::Dataset* pDS, bool needThumb = true);
    CFace GetFaceFromDataset(const dbiplus::sql_record* const record, bool needThumb = true);
    CPictureAlbum GetPictureAlbumFromDataset(dbiplus::Dataset* pDS, bool imageURL=false);
    CPictureAlbum GetPictureAlbumFromDataset(const dbiplus::sql_record* const record, bool imageURL=false);
    CFaceCredit GetPictureAlbumFaceCreditFromDataset(const dbiplus::sql_record* const record);
    void GetFileItemFromDataset(CFileItem* item, const CStdString& strPictureDBbasePath);
    void GetFileItemFromDataset(const dbiplus::sql_record* const record, CFileItem* item, const CStdString& strPictureDBbasePath);
    bool CleanupPictures();
    bool CleanupPicturesByIds(const CStdString &strPictureIds);
    bool CleanupPaths();
    bool CleanupPictureAlbums();
    bool CleanupFaces();
    bool CleanupLocations();
    virtual bool UpdateOldVersion(int version);
    bool SearchFaces(const CStdString& search, CFileItemList &Faces);
    bool SearchPictureAlbums(const CStdString& search, CFileItemList &PictureAlbums);
    bool SearchPictures(const CStdString& strSearch, CFileItemList &pictures);
    int GetPictureIDFromPath(const CStdString &filePath);
    
    // Fields should be ordered as they
    // appear in the pictureview
    enum _PictureFields
    {
        picture_idPicture=0,
        picture_strFaces,
        picture_strLocations,
        picture_strTitle,
        picture_iTrack,
        picture_iDuration,
        picture_iYear,
        picture_dwFileNameCRC,
        picture_strFileName,
        picture_strPictureBrainzTrackID,
        picture_iTimesPlayed,
        picture_iStartOffset,
        picture_iEndOffset,
        picture_lastplayed,
        picture_rating,
        picture_comment,
        picture_idAlbum,
        picture_strAlbum,
        picture_strPath,
        picture_iKarNumber,
        picture_iKarDelay,
        picture_strKarEncoding,
        picture_bCompilation,
        picture_strPictureFaces
    } PictureFields;
    
    // Fields should be ordered as they
    // appear in the PictureAlbumview
    enum _PictureAlbumFields
    {
        Album_idAlbum=0,
        Album_strAlbum,
        Album_strPictureBrainzPictureAlbumID,
        Album_strFaces,
        Album_strLocations,
        Album_iYear,
        Album_idAlbumInfo,
        Album_strMoods,
        Album_strStyles,
        Album_strThemes,
        Album_strReview,
        Album_strLabel,
        Album_strType,
        Album_strThumbURL,
        Album_iRating,
        Album_bCompilation,
        Album_iTimesPlayed,
        
        // used for GetPictureAlbumInfo to get the cascaded Face credits
        Album_idFace,
        Album_strFace,
        Album_strPictureBrainzFaceID,
        Album_bFeatured,
        Album_strJoinPhrase
    } PictureAlbumFields;
    
    enum _FaceFields
    {
        Face_idFace=0,
        Face_strFace,
        Face_strPictureBrainzFaceID,
        Face_strBorn,
        Face_strFormed,
        Face_strLocations,
        Face_strMoods,
        Face_strStyles,
        Face_strInstruments,
        Face_strBiography,
        Face_strDied,
        Face_strDisbanded,
        Face_strYearsActive,
        Face_strImage,
        Face_strFanart
    } FaceFields;
};
