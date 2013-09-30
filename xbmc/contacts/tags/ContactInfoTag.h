#pragma once
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

#include "utils/ISerializable.h"
#include "utils/ISortable.h"
#include "utils/Archive.h"
#include "XBDateTime.h"



class CEmail;
class CPhone;
class CContact;

namespace CONTACT_INFO
{  
  class CContactInfoTag : public IArchivable, public ISerializable, public ISortable
  {
  public:
    CContactInfoTag(void);
    CContactInfoTag(const CContactInfoTag& tag);
    virtual ~CContactInfoTag();
    
    void Reset();
    virtual void Archive(CArchive& ar);
    virtual void Serialize(CVariant& value) const;
    virtual void ToSortable(SortItem& sortable);
    const CContactInfoTag& operator=(const CContactInfoTag& item);
    const CStdString GetInfo(int info) const;
    
    bool Load(const CStdString &path);
    
    static int TranslateString(const CStdString &info);
    
    void SetInfo(int info, const CStdString& value);
    
    /**
     * GetDateTimeTaken() -- Returns the EXIF DateTimeOriginal for current picture
     *
     * The exif library returns DateTimeOriginal if available else the other
     * DateTime tags. See libexif CExifParse::ProcessDir for details.
     */
    const CDateTime& GetDateTimeTaken() const;
  private:
    void GetStringFromArchive(CArchive &ar, char *string, size_t length);
    CDateTime  m_dateTimeTaken;
    void ConvertDateTime();
    
    //////////////////////////////
    //support for JSON response
  public:
    bool operator !=(const CContactInfoTag& tag) const;
    bool Loaded() const;
    const CStdString& GetTitle() const;
    const CStdString& GetFirstName() const;
    const CStdString& GetMiddleName() const;
    const CStdString& GetLastName() const;
    const CStdString& GetLabel() const;
    const CStdString& GetURL() const;
    const std::vector<std::string>& GetEmail() const;
    const CStdString& GetContact() const;
    int GetContactId() const;
    const std::vector<std::string>& GetContactEmail() const;
    const std::vector<std::string>& GetPhone() const;
    int GetDatabaseId() const;
    const std::string &GetType() const;
    
    void GetReleaseDate(SYSTEMTIME& dateTime) const;
    CStdString GetYearString() const;
    const CStdString& GetComment() const;
    const CStdString& GetLyrics() const;
    const CDateTime& GetTakenOn() const;
    const CStdString &GetContactType() const;
    const CStdString &GetOrientation() const;
    int  GetListeners() const;
    
    void SetURL(const CStdString& strURL);
    void SetTitle(const CStdString& strTitle);
    void SetFirstName(const CStdString& strTitle);
    void SetMiddleName(const CStdString& strTitle);
    void SetLastName(const CStdString& strTitle);
    void SetLabel(const CStdString& strTitle);
    void SetEmail(const CStdString& strEmail);
    void SetEmail(const std::vector<std::string>& emails);
    void SetContact(const CStdString& strContact);
    void SetContactId(const int iContactId);
    void SetContactEmail(const CStdString& strContactEmail);
    void SetContactEmail(const std::vector<std::string>& contactEmails);
    void SetPhone(const CStdString& strPhone);
    void SetPhone(const std::vector<std::string>& phones);
    void SetDatabaseId(long id, const std::string &type);
    void SetPartOfSet(int m_iPartOfSet);
    void SetLoaded(bool bOnOff = true);
    void SetEmail(const CEmail& email);
    void SetContact(const CContact& picture);
    void SetComment(const CStdString& comment);
    void SetContactType(const CStdString& type);
    void SetOrientation(const CStdString& orientation);
    void SetListeners(int listeners);
    void SetTakenOn(const CStdString& strTakenOn);
    void SetTakenOn(const CDateTime& strTakenOn);
    void SetCoverArtInfo(size_t size, const std::string &mimeType);
    
    
    /*! \brief Append a unique email to the email list
     Checks if we have this email already added, and if not adds it to the pictures email list.
     \param value email to add.
     */
    void AppendEmail(const CStdString &email);
    
    /*! \brief Append a unique contact email to the email list
     Checks if we have this contact email already added, and if not adds it to the pictures contact email list.
     \param contactEmail contact email to add.
     */
    void AppendContactEmail(const CStdString &contactEmail);
    
    /*! \brief Append a unique phone to the phone list
     Checks if we have this phone already added, and if not adds it to the pictures phone list.
     \param phone phone to add.
     */
    void AppendPhone(const CStdString &phone);
    
    
    void Clear();
  protected:
    /*! \brief Trim whitespace off the given string
     \param value string to trim
     \return trimmed value, with spaces removed from left and right, as well as carriage returns from the right.
     */
    CStdString Trim(const CStdString &value) const;
    
    CStdString m_strURL;
    CStdString m_strTitle;
    CStdString m_strFirstName;
    CStdString m_strMiddleName;
    CStdString m_strLastName;
    CStdString m_strLabel;
    std::vector<std::string> m_email;
    CStdString m_strContact;
    std::vector<std::string> m_contactEmail;
    std::vector<std::string> m_phone;
    CStdString m_strOrientation;
    CStdString m_strType;
    CStdString m_strComment;
    CStdString m_strLyrics;
    CDateTime m_takenOn;
    int m_iDbId;
    std::string m_type; ///< item type "picture", "contact", "email"
    bool m_bLoaded;
    int m_listeners;
    int m_iContactId;
    
  };
}
