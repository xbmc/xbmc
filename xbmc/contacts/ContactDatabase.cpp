

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
#include "ContactDatabase.h"
#include "network/cddb.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/ContactDatabaseDirectory/DirectoryNode.h"
#include "filesystem/ContactDatabaseDirectory/QueryParams.h"
#include "filesystem/ContactDatabaseDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "GUIInfoManager.h"
#include "contacts/tags/ContactInfoTag.h"
#include "addons/AddonManager.h"
#include "addons/Scraper.h"
#include "addons/Addon.h"
#include "utils/URIUtils.h"
#include "Contact.h"
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
#include <map>

using namespace std;
using namespace AUTOPTR;
using namespace XFILE;
using namespace CONTACTDATABASEDIRECTORY;
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

CContactDatabase::CContactDatabase(void)
{
}

CContactDatabase::~CContactDatabase(void)
{
}

const char *CContactDatabase::GetBaseDBName() const { return "MyContact"; };

bool CContactDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseContact);
}

bool CContactDatabase::CreateTables()
{
  BeginTransaction();
  try
  {
    CDatabase::CreateTables();
    
    /*
     // Property keys
     AB_EXTERN const ABPropertyID kABPersonFirstNameProperty;          // First name - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonLastNameProperty;           // Last name - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonMiddleNameProperty;         // Middle name - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonPrefixProperty;             // Prefix ("Sir" "Duke" "General") - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonSuffixProperty;             // Suffix ("Jr." "Sr." "III") - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonNicknameProperty;           // Nickname - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonFirstNamePhoneticProperty;  // First name Phonetic - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonLastNamePhoneticProperty;   // Last name Phonetic - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonMiddleNamePhoneticProperty; // Middle name Phonetic - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonOrganizationProperty;       // Company name - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonJobTitleProperty;           // Job Title - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonDepartmentProperty;         // Department name - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonEmailProperty;              // Email(s) - kABMultiStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonBirthdayProperty;           // Birthday associated with this person - kABDateTimePropertyType
     AB_EXTERN const ABPropertyID kABPersonNoteProperty;               // Note - kABStringPropertyType
     AB_EXTERN const ABPropertyID kABPersonCreationDateProperty;       // Creation Date (when first saved)
     AB_EXTERN const ABPropertyID kABPersonModificationDateProperty;   // Last saved date
     */
    
    /*
     // Addresses
     AB_EXTERN const ABPropertyID kABPersonAddressProperty;            // Street address - kABMultiDictionaryPropertyType
     AB_EXTERN const CFStringRef kABPersonAddressStreetKey;
     AB_EXTERN const CFStringRef kABPersonAddressCityKey;
     AB_EXTERN const CFStringRef kABPersonAddressStateKey;
     AB_EXTERN const CFStringRef kABPersonAddressZIPKey;
     AB_EXTERN const CFStringRef kABPersonAddressCountryKey;
     AB_EXTERN const CFStringRef kABPersonAddressCountryCodeKey;
     */
    
    CLog::Log(LOGINFO, "create contact table");
    m_pDS->exec("CREATE TABLE contact ( idContact integer primary key, strFirst text, strMiddle text, strLast text, strPrefix text, strSuffix text, strNick text, strNote text, idThumb integer)\n");
    
    CLog::Log(LOGINFO, "create office table");
    m_pDS->exec("CREATE TABLE office ( idOffice integer primary key, idContact integer, strOffice text, strJobTitle text, strDepartment text)\n");
    
    CLog::Log(LOGINFO, "create dates table");
    m_pDS->exec("CREATE TABLE dates ( idDate integer primary key, idContact integer, strName text, strValue text)\n");
    
    CLog::Log(LOGINFO, "create emails table");
    m_pDS->exec("CREATE TABLE emails ( idEmail integer primary key, idContact integer, strName text, strValue text)\n");
    
    CLog::Log(LOGINFO, "create addresses table");
    m_pDS->exec("CREATE TABLE addresses ( idAddress integer primary key, idContact integer, strName text, strHouseNo text, strFloorOrArea text, strStreet text, strCity text, strState text, strZip text, strCountry text, strCountryCode text)\n");
    
    CLog::Log(LOGINFO, "create phones table");
    m_pDS->exec("CREATE TABLE phones ( idPhone integer primary key, idContact integer, strName text, strValue text)\n");
    
    CLog::Log(LOGINFO, "create IMs table");
    m_pDS->exec("CREATE TABLE IMs ( idIM integer primary key, idContact integer, strName text, strValue text)\n");
    
    CLog::Log(LOGINFO, "create URLs table");
    m_pDS->exec("CREATE TABLE URLs ( idURL integer primary key, idContact integer, strName text, strValue text)\n");
    
    CLog::Log(LOGINFO, "create relations table");
    m_pDS->exec("CREATE TABLE relations ( idRelation integer primary key, idContact integer, strName text, strValue text, idRelated integer)\n");
    
    
    CLog::Log(LOGINFO, "create contact index");
    m_pDS->exec("CREATE INDEX idxContact ON contact(strFirst)");
    
    CLog::Log(LOGINFO, "create contact index1");
    m_pDS->exec("CREATE INDEX idxContact1 ON contact(strLast)");
    
    CLog::Log(LOGINFO, "create art table, index and triggers");
    m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");
    m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");
    m_pDS->exec("CREATE TRIGGER delete_contact AFTER DELETE ON contact FOR EACH ROW BEGIN DELETE FROM art WHERE media_id=old.idcontact AND media_type='contact'; END");
    
    /*
     CLog::Log(LOGINFO, "create contact_phone indexes");
     m_pDS->exec("CREATE UNIQUE INDEX idxContactPhone_1 ON contact_phone ( idContact, idPhone )\n");
     m_pDS->exec("CREATE UNIQUE INDEX idxContactPhone_2 ON contact_phone ( idPhone, idContact )\n");
     */
    // we create views last to ensure all indexes are rolled in
    CreateViews();
    
    //AddContact("Untitled", "Face", "Location", 2012, false);
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

void CContactDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create contact view");
  m_pDS->exec("DROP VIEW IF EXISTS contactview");
  m_pDS->exec("CREATE VIEW contactview AS SELECT "
              "  contact.idContact AS idContact, "
              "  contact.strFirst AS strFirst,"
              "  contact.strMiddle AS strMiddle,"
              "  contact.strLast AS strLast,"
              "  contact.idThumb AS idThumb"
              "  FROM contact");
  
  CLog::Log(LOGINFO, "create contact view");
}

int CContactDatabase::AddPath(const CStdString& strPath1)
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

int CContactDatabase::AddContact(const CStdString& strPathAndFileName, std::map<std::string, std::string>& name, std::map<std::string, std::string>& phones, std::map<std::string, std::string>& emails, std::vector<std::map<std::string, std::string> >& addresses, std::map<std::string, std::string>& company, std::map<std::string, std::string>& dates, std::map<std::string, std::string>& relations, std::map<std::string, std::string>& IMs, std::map<std::string, std::string>& URLSs)
{
  
  int idContact = -1;
  CStdString strSQL;
  
  try
  {
    
    CLog::Log(LOGERROR, "CContactDatabase::AddContact ");
    // We need at least the title
    
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    CStdString strPath, strFileName;
    URIUtils::Split(strPathAndFileName, strPath, strFileName);
    int idPath = AddPath(strPath);
    DWORD crc = ComputeCRC(strFileName);
    
    strSQL=PrepareSQL("SELECT * FROM contact WHERE strFirst = '%s' AND strLast='%s'",name["first"].c_str(), name["last"].c_str());
    CLog::Log(LOGERROR, "PrepareSQL %s ",strSQL.c_str());
    if (!m_pDS->query(strSQL.c_str()))
      return -1;
    
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      strSQL=PrepareSQL("INSERT INTO contact (idContact,strFirst, strMiddle, strLast, strPrefix, strSuffix, strNick, strNote) values (NULL,'%s', '%s', '%s', '%s', '%s', '%s', '%s')",name["first"].c_str(), name["middle"].c_str(),name["last"].c_str(), name["prefix"].c_str(),name["suffix"].c_str(), name["nick"].c_str(),name["note"].c_str());
      CLog::Log(LOGINFO, strSQL.c_str());
      m_pDS->exec(strSQL.c_str());
      idContact = (int)m_pDS->lastinsertid();
    }
    else
    {
      idContact = m_pDS->fv("idContact").get_asInt();
      m_pDS->close();
      CLog::Log(LOGERROR, "UpdateContact ");
      //UpdateContact(idContact, strTitle, strPathAndFileName, strComment, strThumb, Faces, locations, dtTaken);
      CLog::Log(LOGERROR, "Cazzo cazzo ");
      return idContact;
    }
    
    for(std::map<std::string, std::string>::iterator iter = phones.begin(); iter != phones.end(); ++iter)
    {
      if(iter->second.length())
      {
        strSQL = PrepareSQL("INSERT INTO phones (idPhone, idContact, strName, strValue) VALUES (NULL, %i, '%s', '%s')", idContact, iter->first.c_str(), iter->second.c_str());
        CLog::Log(LOGINFO, strSQL.c_str());
        m_pDS->exec(strSQL.c_str());
        int idPhone = (int)m_pDS->lastinsertid();
      }
    }
    for(std::map<std::string, std::string>::iterator iter = emails.begin(); iter != emails.end(); ++iter)
    {
      if(iter->second.length())
      {
        strSQL = PrepareSQL("INSERT INTO emails (idEmail, idContact, strName, strValue) VALUES (NULL, %i, '%s', '%s')", idContact, iter->first.c_str(), iter->second.c_str());
        CLog::Log(LOGINFO, strSQL.c_str());
        m_pDS->exec(strSQL.c_str());
        int idEmail = (int)m_pDS->lastinsertid();
      }
    }
    for(std::vector<std::map<std::string, std::string> >::iterator iter = addresses.begin(); iter != addresses.end(); ++iter)
    {
      std::map<std::string, std::string> add = *iter;

      strSQL = PrepareSQL("INSERT INTO addresses ( idAddress, idContact, strName, strHouseNo, strFloorOrArea, strStreet, strCity, strState, strZip, strCountry, strCountryCode) VALUES (NULL, %i, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')", idContact, add["type"].c_str(), add["houseno"].c_str(), add["floor"].c_str(), add["street"].c_str(), add["city"].c_str(), add["state"].c_str(), add["zip"].c_str(), add["country"].c_str(), add["countrycode"].c_str());
        CLog::Log(LOGINFO, strSQL.c_str());
        m_pDS->exec(strSQL.c_str());
        int idPhone = (int)m_pDS->lastinsertid();
    }
    for(std::map<std::string, std::string>::iterator iter = dates.begin(); iter != dates.end(); ++iter)
    {
      if(iter->second.length())
      {
        strSQL = PrepareSQL("INSERT INTO dates (idDate, idContact, strName, strValue) VALUES (NULL, %i, '%s', '%s')", idContact, iter->first.c_str(), iter->second.c_str());
        CLog::Log(LOGINFO, strSQL.c_str());
        m_pDS->exec(strSQL.c_str());
        int idEmail = (int)m_pDS->lastinsertid();
      }
    
    }
    
    AnnounceUpdate("contact", idContact);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Contactdatabase:unable to addcontact (%s)", strSQL.c_str());
  }
  return idContact;
}

CContact CContactDatabase::GetContactFromDataset(bool bWithContactDbPath/*=false*/)
{
  
  CContact contact;
  /*
   contact.idContact = m_pDS->fv(contact_idContact).get_asInt();
   // get the full Face string
   contact.face = StringUtils::Split(m_pDS->fv(contact_strFaces).get_asString(), g_advancedSettings.m_contactItemSeparator);
   // and the full location string
   contact.location = StringUtils::Split(m_pDS->fv(contact_strLocations).get_asString(), g_advancedSettings.m_contactItemSeparator);
   // and the rest...
   contact.strContact = m_pDS->fv(contact_strContact).get_asString();
   contact.strContactType = m_pDS->fv(contact_contacttype).get_asString();
   contact.strOrientation = m_pDS->fv(contact_orientation).get_asString();
   contact.idContact = m_pDS->fv(contact_idContact).get_asInt();
   contact.strTitle = m_pDS->fv(contact_strTitle).get_asString();
   contact.takenOn.SetFromDBDateTime(m_pDS->fv(contact_takenOn).get_asString());
   
   // Get filename with full path
   
   if (!bWithContactDbPath)
   contact.strFileName = URIUtils::AddFileToFolder(m_pDS->fv(contact_strPath).get_asString(), m_pDS->fv(contact_strFileName).get_asString());
   else
   {
   CStdString strFileName = m_pDS->fv(contact_strFileName).get_asString();
   CStdString strExt = URIUtils::GetExtension(strFileName);
   contact.strFileName.Format("Contactdb://contacts/%ld/%ld%s", m_pDS->fv(contact_idContact).get_asInt(), m_pDS->fv(contact_idContact).get_asInt(), strExt.c_str());
   }
   */
  return contact;
}

void CContactDatabase::GetFileItemFromDataset(CFileItem* item, const CStdString& strContactDBbasePath)
{
  return GetFileItemFromDataset(m_pDS->get_sql_record(), item, strContactDBbasePath);
}

void CContactDatabase::GetFileItemFromDataset(const dbiplus::sql_record* const record, CFileItem* item, const CStdString& strContactDBbasePath)
{
  CLog::Log(LOGDEBUG, " Aki si=>>>" + strContactDBbasePath);
/*
  contact_idContact=0,
  contact_strFirst,
  contact_strLast,
  contact_strMiddle,
  contact_idThumb,
  contact_idPhone,
  contact_idEmail
*/
  // get the full artist string
  // and the full genre string
//  item->GetContactInfoTag()->SetFirstName(record->at(contact_strFirst).get_asString());
//  item->GetContactInfoTag()->SetMiddleName(record->at(contact_strMiddle).get_asString());
//  item->GetContactInfoTag()->SetLastName(record->at(contact_strLast).get_asString());
  // and the rest...
  item->GetContactInfoTag()->SetContactId(record->at(contact_idContact).get_asInt());
  item->GetContactInfoTag()->SetFirstName(record->at(contact_strFirst).get_asString());
  item->GetContactInfoTag()->SetMiddleName(record->at(contact_strMiddle).get_asString());
  item->GetContactInfoTag()->SetLastName(record->at(contact_strLast).get_asString());
  CStdString title = record->at(contact_strFirst).get_asString() +" "+record->at(contact_strMiddle).get_asString() +" "+record->at(contact_strLast).get_asString();
  item->GetContactInfoTag()->SetTitle(title);
  item->GetContactInfoTag()->SetLabel(title);
  //  item->GetContactInfoTag()->SetPhoneId(record->at(contact_idPhone).get_asInt());
//  item->GetContactInfoTag()->SetEmailId(record->at(contact_idEmail).get_asInt());
  item->GetContactInfoTag()->SetDatabaseId(record->at(contact_idContact).get_asInt(), "contact");
  // Get filename with full path
  
}

bool CContactDatabase::GetContact(int idContact, CContact& contact)
{
  /*
   try
   {
   contact.Clear();
   
   if (NULL == m_pDB.get()) return false;
   if (NULL == m_pDS.get()) return false;
   
   CStdString strSQL=PrepareSQL("select * from contactview "
   "where idContact=%i"
   , idContact);
   
   if (!m_pDS->query(strSQL.c_str())) return false;
   int iRowsFound = m_pDS->num_rows();
   if (iRowsFound == 0)
   {
   m_pDS->close();
   return false;
   }
   contact = GetContactFromDataset();
   m_pDS->close(); // cleanup recordset data
   return true;
   }
   catch (...)
   {
   CLog::Log(LOGERROR, "%s(%i) failed", __FUNCTION__, idContact);
   }
   */
  return false;
}


bool CContactDatabase::SearchContacts(const CStdString& search, CFileItemList &items)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    CStdString strSQL;
    if (search.GetLength() >= MIN_FULL_SEARCH_LENGTH)
      strSQL=PrepareSQL("select * from contactview where strTitle like '%s%%' or strTitle like '%% %s%%' limit 1000", search.c_str(), search.c_str());
    else
      strSQL=PrepareSQL("select * from contactview where strTitle like '%s%%' limit 1000", search.c_str());
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0) return false;
    
    CStdString contactLabel = g_localizeStrings.Get(179); // Contact
    while (!m_pDS->eof())
    {
      CFileItemPtr item(new CFileItem);
      GetFileItemFromDataset(item.get(), "Contactdb://contacts/");
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

bool CContactDatabase::GetContactsByWhere(const CStdString &baseDir, const Filter &filter, CFileItemList &items, const SortDescription &sortDescription /* = SortDescription() */)
{
  if (m_pDB.get() == NULL || m_pDS.get() == NULL)
    return false;
  
  try
  {
    unsigned int time = XbmcThreads::SystemClockMillis();
    int total = -1;
    
    CStdString strSQL = "SELECT %s FROM contactview ";
    
    Filter extFilter = filter;
    CContactDbUrl ContactUrl;
    SortDescription sorting = sortDescription;
    if (!ContactUrl.FromString(baseDir) || !GetFilter(ContactUrl, extFilter, sorting))
      return false;
    
    // if there are extra WHERE conditions we might need access
    // to contactview for these conditions
    if (extFilter.where.find("contactview") != string::npos)
    {
     // extFilter.AppendJoin("JOIN contactview ON contactview.idContact = contactview.idContact");
      //extFilter.AppendGroup("contactview.idContact");
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
    
    strSQL = PrepareSQL(strSQL, !filter.fields.empty() && filter.fields.compare("*") != 0 ? filter.fields.c_str() : "contactview.*") + strSQLExtra;
    
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
    
    if (!SortUtils::SortFromDataset(sortDescription, MediaTypeContact, m_pDS, results))
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
        GetFileItemFromDataset(record, item.get(), ContactUrl.ToString());
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

bool CContactDatabase::GetContactsNav(const CStdString& strBaseDir, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */)
{
  CContactDbUrl ContactUrl;
  if (!ContactUrl.FromString(strBaseDir))
    return false;
  
  Filter filter;
  return GetContactsByWhere(ContactUrl.ToString(), filter, items, sortDescription);
}

int CContactDatabase::GetMinVersion() const
{
  return 1;
}


int CContactDatabase::GetContactsCount(const Filter &filter)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;
    
    CStdString strSQL = "select count(idContact) as NumContacts from contactview ";
    if (!CDatabase::BuildSQL(strSQL, filter, strSQL))
      return false;
    
    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }
    
    int iNumContacts = m_pDS->fv("NumContacts").get_asInt();
    // cleanup
    m_pDS->close();
    return iNumContacts;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, filter.where.c_str());
  }
  return 0;
}

void CContactDatabase::SplitString(const CStdString &multiString, vector<string> &vecStrings, CStdString &extraStrings)
{
  /*
   vecStrings = StringUtils::Split(multiString, g_advancedSettings.m_contactItemSeparator);
   for (unsigned int i = 1; i < vecStrings.size(); i++)
   extraStrings += g_advancedSettings.m_contactItemSeparator + CStdString(vecStrings[i]);
   */
}

bool CContactDatabase::GetItems(const CStdString &strBaseDir, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  CContactDbUrl ContactUrl;
  if (!ContactUrl.FromString(strBaseDir))
    return false;
  
  return GetItems(strBaseDir, ContactUrl.GetType(), items, filter, sortDescription);
}

bool CContactDatabase::GetItems(const CStdString &strBaseDir, const CStdString &itemType, CFileItemList &items, const Filter &filter /* = Filter() */, const SortDescription &sortDescription /* = SortDescription() */)
{
  /*
   if (itemType.Equals("locations"))
   return GetLocationsNav(strBaseDir, items, filter);
   else if (itemType.Equals("years"))
   return GetYearsNav(strBaseDir, items, filter);
   else if (itemType.Equals("Faces"))
   return GetFacesNav(strBaseDir, items, !CSettings::Get().GetBool("Contactlibrary.showcompilationFaces"), -1, -1, -1, filter, sortDescription);
   else if (itemType.Equals("contacts"))
   return GetContactsByWhere(strBaseDir, filter, items, sortDescription);
   else if (itemType.Equals("contacts"))
   return GetContactsByWhere(strBaseDir, filter, items, sortDescription);
   */
  return false;
}

CStdString CContactDatabase::GetItemById(const CStdString &itemType, int id)
{
  /*
   if (itemType.Equals("locations"))
   return GetLocationById(id);
   else if (itemType.Equals("years"))
   {
   CStdString tmp; tmp.Format("%d", id);
   return tmp;
   }
   else if (itemType.Equals("Faces"))
   return GetFaceById(id);
   else if (itemType.Equals("contacts"))
   return GetContactById(id);
   */
  return "";
}

void CContactDatabase::SetPropertiesFromContact(CFileItem& item, const CContact& contact)
{
  /*
  item.SetProperty("contact_description", contact.strReview);
  item.SetProperty("contact_theme", StringUtils::Join(contact.themes, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("contact_theme_array", contact.themes);
  item.SetProperty("contact_mood", StringUtils::Join(contact.moods, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("contact_mood_array", contact.moods);
  item.SetProperty("contact_style", StringUtils::Join(contact.styles, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("contact_style_array", contact.styles);
  item.SetProperty("contact_type", contact.strPictureType);
  item.SetProperty("contact_label", contact.strLabel);
  item.SetProperty("contact_Face", StringUtils::Join(contact.face, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("contact_face_array", contact.face);
  item.SetProperty("contact_location", StringUtils::Join(contact.location, g_advancedSettings.m_pictureItemSeparator));
  item.SetProperty("contact_location_array", contact.location);
  item.SetProperty("contact_title", contact.strContact);
   */
}
void CContactDatabase::SetArtForItem(int mediaId, const string &mediaType, const map<string, string> &art)
{
  for (map<string, string>::const_iterator i = art.begin(); i != art.end(); ++i)
    SetArtForItem(mediaId, mediaType, i->first, i->second);
}

void CContactDatabase::SetArtForItem(int mediaId, const string &mediaType, const string &artType, const string &url)
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

bool CContactDatabase::GetArtForItem(int mediaId, const string &mediaType, map<string, string> &art)
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

string CContactDatabase::GetArtForItem(int mediaId, const string &mediaType, const string &artType)
{
  std::string query = PrepareSQL("SELECT url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
  return GetSingleValue(query, m_pDS2);
}


bool CContactDatabase::GetFilter(CDbUrl &ContactUrl, Filter &filter, SortDescription &sorting)
{
  if (!ContactUrl.IsValid())
    return false;
  
  std::string type = ContactUrl.GetType();
  const CUrlOptions::UrlOptions& options = ContactUrl.GetOptions();
  CUrlOptions::UrlOptions::const_iterator option;
  
  if (type == "Faces")
  {
    int idFace = -1, idLocation = -1,  idContact = -1;
    bool contactFacesOnly = false;
    
    option = options.find("contactid");
    if (option != options.end())
      idContact = (int)option->second.asInteger();
    
    option = options.find("contactFacesonly");
    if (option != options.end())
      contactFacesOnly = option->second.asBoolean();
    
    CStdString strSQL = "(Faceview.idFace IN ";
    if (idFace > 0)
      strSQL += PrepareSQL("(%d)", idFace);
    else if (idContact > 0)
      strSQL += PrepareSQL("(SELECT contact_Face.idFace FROM contact_Face WHERE contact_Face.idContact = %i)", idContact);
    else if (idContact > 0)
      strSQL += PrepareSQL("(SELECT contact_Face.idFace FROM contact_Face WHERE contact_Face.idContact = %i)", idContact);
    else if (idLocation > 0)
    { // same statements as below, but limit to the specified location
      // in this case we show the whole lot always - there is no limitation to just contact Faces
      if (!contactFacesOnly)  // show all Faces in this case (ie those linked to a contact)
        strSQL+=PrepareSQL("(SELECT contact_Face.idFace FROM contact_Face" // All Faces linked to extra locations
                           " JOIN contact_location ON contact_Face.idContact = contact_location.idContact"
                           " WHERE contact_location.idLocation = %i)"
                           " OR idFace IN ", idLocation);
      // and add any Faces linked to an contact (may be different from above due to contact Face tag)
      strSQL += PrepareSQL("(SELECT contact_Face.idFace FROM contact_Face" // All contact Faces linked to extra locations
                           " JOIN contact_location ON contact_Face.idContact = contact_location.idContact"
                           " WHERE contact_location.idLocation = %i)", idLocation);
    }
    else
    {
      if (!contactFacesOnly)  // show all Faces in this case (ie those linked to a contact)
        strSQL += "(SELECT contact_Face.idFace FROM contact_Face)"
        " OR Faceview.idFace IN ";
      
      // and always show any Faces linked to an contact (may be different from above due to contact Face tag)
      strSQL +=   "(SELECT contact_Face.idFace FROM contact_Face"; // All Faces linked to an contact
      if (contactFacesOnly)
        strSQL += " JOIN contact ON contact.idContact = contact_Face.idContact WHERE contact.bCompilation = 0 ";            // then exclude those that have no extra Faces
      strSQL +=   ")";
    }
    
    // remove the null string
    strSQL += ") and Faceview.strFace != ''";
    
    // and the various Face entry if applicable
    if (!contactFacesOnly)
    {
      CStdString strVariousFaces = g_localizeStrings.Get(340);
      strSQL += PrepareSQL(" and Faceview.strFace <> '%s'", strVariousFaces.c_str());
    }
    
    filter.AppendWhere(strSQL);
  }
  else if (type == "contacts")
  {
    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.iYear = %i", (int)option->second.asInteger()));
    
    option = options.find("contactid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.idContact = %i", (int)option->second.asInteger()));
    
    option = options.find("compilation");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.bCompilation = %i", option->second.asBoolean() ? 1 : 0));
    
    option = options.find("contacttype");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.contacttype like '%s'", option->second.asString().c_str()));
    
    option = options.find("locationid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.idContact IN (SELECT contact.idContact FROM contact JOIN contact_location ON contact.idContact = contact_location.idContact WHERE contact_location.idLocation = %i)", (int)option->second.asInteger()));
    
    option = options.find("location");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.idContact IN (SELECT contact.idContact FROM contact JOIN contact_location ON contact.idContact = contact_location.idContact JOIN location ON location.idLocation = contact_location.idLocation WHERE location.strLocation like '%s')", option->second.asString().c_str()));
    
    
    option = options.find("Faceid");
    if (option != options.end())
    {
      filter.AppendJoin("JOIN contact ON contact.idContact = contactview.idContact "
                        "JOIN contact_Face ON contact.idContact = contact_Face.idContact "
                        "JOIN contact_Face ON contactview.idContact = contact_Face.idContact");
      filter.AppendWhere(PrepareSQL("      contact_Face.idFace = %i" // All contacts linked to this Face via contacts
                                    " OR  contact_Face.idFace = %i", // All contacts where contact Faces fit
                                    (int)option->second.asInteger(), (int)option->second.asInteger()));
      filter.AppendGroup("contactview.idContact");
    }
    else
    {
      option = options.find("Face");
      if (option != options.end())
        filter.AppendWhere(PrepareSQL("contactview.idContact IN (SELECT contact.idContact FROM contact JOIN contact_Face ON contact.idContact = contact_Face.idContact JOIN Face ON Face.idFace = contact_Face.idFace WHERE Face.strFace like '%s')" // All contacts linked to this Face via contacts
                                      " OR contactview.idContact IN (SELECT contact_Face.idContact FROM contact_Face JOIN Face ON Face.idFace = contact_Face.idFace WHERE Face.strFace like '%s')", // All contacts where contact Faces fit
                                      option->second.asString().c_str(), option->second.asString().c_str()));
      // no Face given, so exclude any single contacts (aka empty tagged contacts)
      else
        filter.AppendWhere("contactview.strFirst <> ''");
    }
  }
  else if (type == "contacts" || type == "singles")
  {
    option = options.find("singles");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.idContact %sIN (SELECT idContact FROM contact WHERE strContact = '')", option->second.asBoolean() ? "" : "NOT "));
    
    option = options.find("year");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.iYear = %i", (int)option->second.asInteger()));
    
    option = options.find("compilation");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.bCompilation = %i", option->second.asBoolean() ? 1 : 0));
    
    option = options.find("contactid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.idContact = %i", (int)option->second.asInteger()));
    
    option = options.find("contact");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.strContact like '%s'", option->second.asString().c_str()));
    
    option = options.find("locationid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.idContact IN (SELECT contact_location.idContact FROM contact_location WHERE contact_location.idLocation = %i)", (int)option->second.asInteger()));
    
    option = options.find("location");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.idContact IN (SELECT contact_location.idContact FROM contact_location JOIN location ON location.idLocation = contact_location.idLocation WHERE location.strLocation like '%s')", option->second.asString().c_str()));
    
    option = options.find("Faceid");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.idContact IN (SELECT contact_Face.idContact FROM contact_Face WHERE contact_Face.idFace = %i)" // contact Faces
                                    " OR contactview.idContact IN (SELECT contact.idContact FROM contact JOIN contact_Face ON contact.idContact=contact_Face.idContact WHERE contact_Face.idFace = %i)", // contact Faces
                                    (int)option->second.asInteger(), (int)option->second.asInteger()));
    
    option = options.find("Face");
    if (option != options.end())
      filter.AppendWhere(PrepareSQL("contactview.idContact IN (SELECT contact_Face.idContact FROM contact_Face JOIN Face ON Face.idFace = contact_Face.idFace WHERE Face.strFace like '%s')" // contact Faces
                                    " OR contactview.idContact IN (SELECT contact.idContact FROM contact JOIN contact_Face ON contact.idContact=contact_Face.idContact JOIN Face ON Face.idFace = contact_Face.idFace WHERE Face.strFace like '%s')", // contact Faces
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
      ContactUrl.RemoveOption("filter");
  }
  
  return true;
}
bool CContactDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so reset the infomanager cache
    //    g_infoManager.SetLibraryBool(LIBRARY_HAS_MUSIC, GetSongsCount() > 0);
    return true;
  }
  return false;
}
