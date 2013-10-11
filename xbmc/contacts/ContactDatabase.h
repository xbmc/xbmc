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
 \file ContactDatabase.h
 \brief
 */
#pragma once
#include "dbwrappers/Database.h"
#include "addons/Scraper.h"
#include "utils/SortUtils.h"
#include "ContactDbUrl.h"
#include "Contact.h"

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
 \ingroup Contact
 \brief A set of CStdString objects, used for CContactDatabase
 \sa ISETPATHES, CContactDatabase
 */
typedef std::set<CStdString> SETPATHES;

/*!
 \ingroup Contact
 \brief The SETPATHES iterator
 \sa SETPATHES, CContactDatabase
 */
typedef std::set<CStdString>::iterator ISETPATHES;

class CGUIDialogProgress;
class CFileItemList;

/*!
 \ingroup Contact
 \brief Class to store and read tag information
 
 CContactDatabase can be used to read and store
 tag information for faster access. It is based on
 sqlite (http://www.sqlite.org).
 
 Here is the database layout:
 \image html Contactdatabase.png
 
 \sa CContactAlbum, CContact, VECPICTURES, CMapContact, VECFaceS, VECPICTUREALBUMS, VECGENRES
 */
class CContactDatabase : public CDatabase
{
    friend class DatabaseUtils;
    friend class TestDatabaseUtilsHelper;
    
public:
    CContactDatabase(void);
    virtual ~CContactDatabase(void);
    
    virtual bool Open();
    virtual bool CommitTransaction();
    void EmptyCache();
    void Clean();
    int  Cleanup(CGUIDialogProgress *pDlgProgress=NULL);
    void DeleteContactAlbumInfo();
    bool LookupCDDBInfo(bool bRequery=false);
    void DeleteCDDBInfo();
    
    /////////////////////////////////////////////////
    // Contact CRUD
    /////////////////////////////////////////////////
    /*! \brief Add a contact to the database
     \param idContactAlbum [in] the database ID of the ContactAlbum for the contact
     \param strTitle [in] the title of the contact (required to be non-empty)
     \param strContactBrainzTrackID [in] the ContactBrainz track ID of the contact
     \param strPathAndFileName [in] the path and filename to the contact
     \param strComment [in] the ids of the added contacts
     \param strThumb [in] the ids of the added contacts
     \param Faces [in] a vector of Face names (will only be used for the cache names in the ContactAlbum views)
     \param locations [in] a vector of locations to which this contact belongs
     \param iTrack [in] the track number and disc number of the contact
     \param iDuration [in] the duration of the contact
     \param iYear [in] the year of the contact
     \param iContactCount [in] the number of times the contact has been played
     \param iStartOffset [in] the start offset of the contact (when using a single audio file with a .cue)
     \param iEndOffset [in] the end offset of the contact (when using a single audio file with .cue)
     \param dtTaken [in] the time the contact was last played
     \param rating [in] a rating for the contact
     \param iKaraokeNumber [in] the karaoke id of the contact
     \return the id of the contact
     */
  int AddContact(const CStdString& strPathAndFileName, std::map<std::string, std::string>& name, std::map<std::string, std::string>& phones, std::map<std::string, std::string>& emails, std::vector<std::map<std::string, std::string> >& addresses, std::map<std::string, std::string>& company, std::map<std::string, std::string>& dates, std::map<std::string, std::string>& relations, std::map<std::string, std::string>& IMs, std::map<std::string, std::string>& URLSs, int hasProfilePic);
  int AddPath(const CStdString& strPath1);

  
    /*! \brief Update a contact in the database
     \param idContact [in] the database ID of the contact to update
     \param strTitle [in] the title of the contact (required to be non-empty)
     \param strContactBrainzTrackID [in] the ContactBrainz track ID of the contact
     \param strPathAndFileName [in] the path and filename to the contact
     \param strComment [in] the ids of the added contacts
     \param strThumb [in] the ids of the added contacts
     \param Faces [in] a vector of Face names (will only be used for the cache names in the ContactAlbum views)
     \param locations [in] a vector of locations to which this contact belongs
     \param iTrack [in] the track number and disc number of the contact
     \param iDuration [in] the duration of the contact
     \param iYear [in] the year of the contact
     \param iContactCount [in] the number of times the contact has been played
     \param iStartOffset [in] the start offset of the contact (when using a single audio file with a .cue)
     \param iEndOffset [in] the end offset of the contact (when using a single audio file with .cue)
     \param dtTaken [in] the time the contact was last played
     \param rating [in] a rating for the contact
     \param iKaraokeNumber [in] the karaoke id of the contact
     \return the id of the contact
     */    
    //// Misc Contact
    bool GetContactByFileName(const CStdString& strFileName, CContact& contact, int startOffset = 0);
    //bool GetContactsByPath(const CStdString& strPath, MAPPICTURES& contacts, bool bAppendToMap = false);
    bool GetContactFromDataSet(const dbiplus::sql_record* const record, int idContact, CContact& contact);
  bool GetContact( int idContact, CContact& contact);
  
  /////////////////////////////////////////////////
    // Recently added
    /////////////////////////////////////////////////
    //bool GetRecentlyAddedContact(VECPICTUREALBUMS& ContactAlbums, unsigned int limit=0);
  
    /////////////////////////////////////////////////
    // VIEWS
    /////////////////////////////////////////////////
    bool GetContactsNav(const CStdString& strBaseDir, CFileItemList& items, const SortDescription &sortDescription = SortDescription());
    bool GetContactsByWhere(const CStdString &baseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription());
    int GetContactsCount(const Filter &filter = Filter());
    unsigned int GetContactIDs(const Filter &filter, std::vector<std::pair<int,int> > &contactIDs);
    virtual bool GetFilter(CDbUrl &ContactUrl, Filter &filter, SortDescription &sorting);
    
  
    /////////////////////////////////////////////////
    // Filters
    /////////////////////////////////////////////////
    bool GetItems(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
    bool GetItems(const CStdString &strBaseDir, const CStdString &itemType, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
    CStdString GetItemById(const CStdString &itemType, int id);
  static void SetPropertiesFromContact(CFileItem& item, const CContact& contact);

    /////////////////////////////////////////////////
    // Art
    /////////////////////////////////////////////////
    bool SaveContactAlbumThumb(int idContactAlbum, const CStdString &thumb);
    /*! \brief Sets art for a database item.
     Sets a single piece of art for a database item.
     \param mediaId the id in the media (contact/Face/ContactAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (contact/Face/ContactAlbum).
     \param artType the type of art to set, e.g. "thumb", "fanart"
     \param url the url to the art (this is the original url, not a cached url).
     \sa GetArtForItem
     */
    void SetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType, const std::string &url);
    
    /*! \brief Sets art for a database item.
     Sets multiple pieces of art for a database item.
     \param mediaId the id in the media (contact/Face/ContactAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (contact/Face/ContactAlbum).
     \param art a map of <type, url> where type is "thumb", "fanart", etc. and url is the original url of the art.
     \sa GetArtForItem
     */
    void SetArtForItem(int mediaId, const std::string &mediaType, const std::map<std::string, std::string> &art);
    
    /*! \brief Fetch art for a database item.
     Fetches multiple pieces of art for a database item.
     \param mediaId the id in the media (contact/Face/ContactAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (contact/Face/ContactAlbum).
     \param art [out] a map of <type, url> where type is "thumb", "fanart", etc. and url is the original url of the art.
     \return true if art is retrieved, false if no art is found.
     \sa SetArtForItem
     */
    bool GetArtForItem(int mediaId, const std::string &mediaType, std::map<std::string, std::string> &art);
    
    /*! \brief Fetch art for a database item.
     Fetches a single piece of art for a database item.
     \param mediaId the id in the media (contact/Face/ContactAlbum) table.
     \param mediaType the type of media, which corresponds to the table the item resides in (contact/Face/ContactAlbum).
     \param artType the type of art to retrieve, eg "thumb", "fanart".
     \return the original URL to the piece of art, if available.
     \sa SetArtForItem
     */
    std::string GetArtForItem(int mediaId, const std::string &mediaType, const std::string &artType);
    
protected:
    std::map<CStdString, int> m_FaceCache;
    std::map<CStdString, int> m_locationCache;
    std::map<CStdString, int> m_pathCache;
    std::map<CStdString, int> m_thumbCache;
    
    virtual bool CreateTables();
    virtual int GetMinVersion() const;
    
  const char *GetBaseDBName() const;
  
    
private:
    /*! \brief (Re)Create the generic database views for contacts and ContactAlbums
     */
    virtual void CreateViews();
    
    void SplitString(const CStdString &multiString, std::vector<std::string> &vecStrings, CStdString &extraStrings);
    CContact GetContactFromDataset(bool bWithContactDbPath=false);
    void GetFileItemFromDataset(CFileItem* item, const CStdString& strContactDBbasePath);
  void GetFileItemFromContact(CContact& contact, CFileItem* item, const CStdString& strContactDBbasePath);
    void GetFileItemFromDataset(const dbiplus::sql_record* const record, CFileItem* item, const CStdString& strContactDBbasePath);
//  void GetFileItemFromDataset(const dbiplus::sql_record* const record, CFileItem* item, const std::vector<std::string>& phones, const std::vector<std::string>& emails,  const CStdString& strContactDBbasePath);

    bool SearchContacts(const CStdString& strSearch, CFileItemList &contacts);
    int GetContactIDFromPath(const CStdString &filePath);
  bool SetContactPhones(CFileItemList &items);
  bool SetContactEmails(CFileItemList &items);


  // Fields sh;ould be ordered as they
    // appear in the contactview
    enum _ContactFields
    {
        contact_idContact=0,
        contact_strFirst,
      contact_strMiddle,
        contact_strLast,
      contact_profilePic,
        contact_idThumb,
        contact_idPhone,
        contact_idEmail
    } ContactFields;
    
    // Fields should be ordered as they
    // appear in the ContactAlbumview

};
