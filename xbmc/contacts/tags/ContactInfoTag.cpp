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

#include "ContactInfoTag.h"
#include "XBDateTime.h"
#include "Util.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"

#include "contacts/Contact.h"
#include "contacts/Email.h"
#include "contacts/Phone.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"


using namespace std;

using namespace CONTACT_INFO;

int CContactInfoTag::GetDatabaseId() const
{
  return m_iDbId;
}


void CContactInfoTag::Reset()
{
  m_bLoaded = false;
  m_dateTimeTaken.Reset();
}


bool CContactInfoTag::Load(const CStdString &path)
{
  m_bLoaded = false;
  
  
  ConvertDateTime();
  
  return m_bLoaded;
}

void CContactInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_bLoaded;
    ar << m_dateTimeTaken;
    
  }
  else
  {
    ar >> m_bLoaded;
    ar >> m_dateTimeTaken;
    
  }
  if (ar.IsStoring())
  {
    ar << m_strURL;
    ar << m_strTitle;
    ar << m_strLabel;
    ar << m_email;
    ar << m_strContact;
    ar << m_contactEmail;
    ar << m_phone;
    ar << m_bLoaded;
    ar << m_takenOn;
    ar << m_strOrientation;
    ar << m_strType;
    ar << m_strComment;
    ar << m_iContactId;
    ar << m_iDbId;
    ar << m_type;
    ar << m_strLyrics;
    ar << m_listeners;
  }
  else
  {
    ar >> m_strURL;
    ar >> m_strTitle;
    ar >> m_strLabel;
    ar >> m_email;
    ar >> m_strContact;
    ar >> m_contactEmail;
    ar >> m_phone;
    ar >> m_strOrientation;
    ar >> m_strType;
    ar >> m_bLoaded;
    ar >> m_takenOn;
    ar >> m_strComment;
    ar >> m_iContactId;
    ar >> m_iDbId;
    ar >> m_type;
    ar >> m_strLyrics;
    ar >> m_listeners;
  }
}


void CContactInfoTag::ToSortable(SortItem& sortable)
{
  sortable[FieldEmails] = m_email;
  sortable[FieldContact] = m_strContact;
  sortable[FieldPhones] = m_phone;
  sortable[FieldComment] = m_strComment;
  sortable[FieldId] = (int64_t)m_iDbId;
  
  if (m_dateTimeTaken.IsValid())
    sortable[FieldDateTaken] = m_dateTimeTaken.GetAsDBDateTime();
}

void CContactInfoTag::GetStringFromArchive(CArchive &ar, char *string, size_t length)
{
  CStdString temp;
  ar >> temp;
  length = min((size_t)temp.GetLength(), length - 1);
  if (!temp.IsEmpty())
    memcpy(string, temp.c_str(), length);
  string[length] = 0;
}

const CStdString CContactInfoTag::GetInfo(int info) const
{
  CStdString value;
 
  return value;
}

int CContactInfoTag::TranslateString(const CStdString &info)
{
 return 0;
}

void CContactInfoTag::SetInfo(int info, const CStdString& value)
{
}

const CDateTime& CContactInfoTag::GetDateTimeTaken() const
{
  return m_dateTimeTaken;
}

void CContactInfoTag::ConvertDateTime()
{

}


///////////
//json support

CContactInfoTag::CContactInfoTag(void)
{
  Clear();
  Reset();
}

CContactInfoTag::CContactInfoTag(const CContactInfoTag& tag)
{
  *this = tag;
}

CContactInfoTag::~CContactInfoTag()
{}

const CContactInfoTag& CContactInfoTag::operator =(const CContactInfoTag& tag)
{
  if (this == &tag) return * this;
  
  m_strURL = tag.m_strURL;
  m_email = tag.m_email;
  m_contactEmail = tag.m_contactEmail;
  m_strContact = tag.m_strContact;
  m_phone = tag.m_phone;
  m_strTitle = tag.m_strTitle;
  m_strLabel = tag.m_strLabel;
  m_strComment = tag.m_strComment;
  m_strType = tag.m_strType;
  m_strOrientation = tag.m_strOrientation;
  m_strLyrics = tag.m_strLyrics;
  m_takenOn = tag.m_takenOn;
  m_bLoaded = tag.m_bLoaded;
  m_listeners = tag.m_listeners;
  m_iDbId = tag.m_iDbId;
  m_type = tag.m_type;
  m_iContactId = tag.m_iContactId;
  
  if (this == &tag) return * this;
  m_bLoaded = tag.m_bLoaded;
  m_dateTimeTaken = tag.m_dateTimeTaken;
  return *this;
  
  return *this;
}

bool CContactInfoTag::operator !=(const CContactInfoTag& tag) const
{
  if (this == &tag) return false;
  if (m_strURL != tag.m_strURL) return true;
  if (m_strTitle != tag.m_strTitle) return true;
  if (m_strLabel != tag.m_strLabel) return true;
  if (m_email != tag.m_email) return true;
  if (m_contactEmail != tag.m_contactEmail) return true;
  if (m_strContact != tag.m_strContact) return true;
  return false;
}



const CStdString& CContactInfoTag::GetTitle() const
{
  return m_strTitle;
}
const CStdString& CContactInfoTag::GetLabel() const
{
  return m_strLabel;
}

const CStdString& CContactInfoTag::GetURL() const
{
  return m_strURL;
}

const std::vector<std::string>& CContactInfoTag::GetEmail() const
{
  return m_email;
}

const CStdString& CContactInfoTag::GetContact() const
{
  return m_strContact;
}

int CContactInfoTag::GetContactId() const
{
  return m_iContactId;
}

const std::vector<std::string>& CContactInfoTag::GetContactEmail() const
{
  return m_contactEmail;
}

const std::vector<std::string>& CContactInfoTag::GetPhone() const
{
  return m_phone;
}


const std::string &CContactInfoTag::GetType() const
{
  return m_type;
}


const CStdString &CContactInfoTag::GetComment() const
{
  return m_strComment;
}

const CStdString &CContactInfoTag::GetOrientation() const
{
  return m_strOrientation;
}
const CStdString &CContactInfoTag::GetContactType() const
{
  return m_strType;
}



int CContactInfoTag::GetListeners() const
{
  return m_listeners;
}


const CDateTime &CContactInfoTag::GetTakenOn() const
{
  return m_takenOn;
}




void CContactInfoTag::SetURL(const CStdString& strURL)
{
  m_strURL = strURL;
}

void CContactInfoTag::SetTitle(const CStdString& strTitle)
{
  m_strTitle = Trim(strTitle);
}

void CContactInfoTag::SetFirstName(const CStdString& strTitle)
{
  m_strFirstName = Trim(strTitle);
}

void CContactInfoTag::SetMiddleName(const CStdString &strTitle)
{
  m_strMiddleName = Trim(strTitle);
}

void CContactInfoTag::SetLastName(const CStdString &strTitle)
{
  m_strLastName = Trim(strTitle);
}

void CContactInfoTag::SetLabel(const CStdString& strLabel)
{
  m_strLabel = Trim(strLabel);
}

void CContactInfoTag::SetEmail(const CStdString& strEmail)
{
  if (!strEmail.empty())
    SetEmail(StringUtils::Split(strEmail, g_advancedSettings.m_contactItemSeparator));
  else
    m_email.clear();
}

void CContactInfoTag::SetEmail(const std::vector<std::string>& emails)
{
  m_email = emails;
}

void CContactInfoTag::SetContact(const CStdString& strContact)
{
  m_strContact = Trim(strContact);
}

void CContactInfoTag::SetContactId(const int iContactId)
{
  m_iContactId = iContactId;
}

void CContactInfoTag::SetContactEmail(const CStdString& strContactEmail)
{
  if (!strContactEmail.empty())
    SetContactEmail(StringUtils::Split(strContactEmail, g_advancedSettings.m_contactItemSeparator));
  else
    m_contactEmail.clear();
}

void CContactInfoTag::SetContactEmail(const std::vector<std::string>& contactEmails)
{
  m_contactEmail = contactEmails;
}

void CContactInfoTag::SetPhone(const CStdString& strPhone)
{
  if (!strPhone.empty())
    SetPhone(StringUtils::Split(strPhone, g_advancedSettings.m_contactItemSeparator));
  else
    m_phone.clear();
}

void CContactInfoTag::SetPhone(const std::vector<std::string>& phones)
{
  m_phone = phones;
}


void CContactInfoTag::SetDatabaseId(long id, const std::string &type)
{
  m_iDbId = id;
  m_type = type;
}


void CContactInfoTag::SetComment(const CStdString& comment)
{
  m_strComment = comment;
}

void CContactInfoTag::SetOrientation(const CStdString& orientation)
{
  m_strOrientation = orientation;
}

void CContactInfoTag::SetContactType(const CStdString& type)
{
  m_strType = type;
}



void CContactInfoTag::SetListeners(int listeners)
{
  m_listeners = listeners;
}


void CContactInfoTag::SetTakenOn(const CStdString& takenond)
{
  m_takenOn.SetFromDBDateTime(takenond);
}

void CContactInfoTag::SetTakenOn(const CDateTime& takenond)
{
  m_takenOn = takenond;
}


void CContactInfoTag::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CContactInfoTag::Loaded() const
{
  return m_bLoaded;
}




void CContactInfoTag::SetEmail(const CEmail& email)
{
  SetEmail(email.strEmail);
  SetContactEmail(email.strEmail);
//  SetPhone(phone.phone);
  m_iDbId = email.idEmail;
  m_type = "email";
  m_bLoaded = true;
}

void CContactInfoTag::SetContact(const CContact& contact)
{
  SetTitle(contact.strTitle);
  SetFirstName(contact.strFirst);
  SetMiddleName(contact.strMiddle);
    SetLastName(contact.strLast);
  SetLabel(contact.strLabel);
  SetPhone(contact.phone);
  SetEmail(contact.email);
  /*
  SetContact(contact.strContact);
  SetContactEmail(contact.contactEmail);
  SetComment(contact.strComment);
  SetTakenOn(contact.takenOn);
  SetOrientation(contact.strOrientation);
  SetContactType(contact.strContactType);
  m_strURL = contact.strFileName;
  */
  SYSTEMTIME stTime;
  m_iDbId = contact.idContact;
  m_type = "contact";
  m_bLoaded = true;
  m_iContactId = contact.idContact;
}

void CContactInfoTag::Serialize(CVariant& value) const
{
  
  
  value["url"] = m_strURL;
  value["title"] = m_strTitle;
  value["label"] = m_strLabel;
  if (m_type.compare("email") == 0 && m_email.size() == 1)
    value["email"] = m_email[0];
  else
    value["email"] = m_email;
  value["displayemail"] = StringUtils::Join(m_email, g_advancedSettings.m_contactItemSeparator);
  value["contact"] = m_strContact;
  value["contactemail"] = m_contactEmail;
  value["phone"] = m_phone;
  value["loaded"] = m_bLoaded;
  value["comment"] = m_strComment;
  value["takenon"] = m_takenOn.IsValid() ? m_takenOn.GetAsDBDateTime() : StringUtils::EmptyString;
  value["lyrics"] = m_strLyrics;
  value["contactid"] = m_iContactId;
}

void CContactInfoTag::Clear()
{
  m_strURL.Empty();
  m_email.clear();
  m_strContact.Empty();
  m_contactEmail.clear();
  m_phone.clear();
  m_strTitle.Empty();
  m_strLabel.Empty();
  m_bLoaded = false;
  m_takenOn.Reset();
  m_strComment.Empty();
  m_iDbId = -1;
  m_type.clear();
  m_iContactId = -1;
}

void CContactInfoTag::AppendEmail(const CStdString &email)
{
  for (unsigned int index = 0; index < m_email.size(); index++)
  {
    if (email.Equals(m_email.at(index).c_str()))
      return;
  }
  
  m_email.push_back(email);
}

void CContactInfoTag::AppendContactEmail(const CStdString &contactEmail)
{
  for (unsigned int index = 0; index < m_contactEmail.size(); index++)
  {
    if (contactEmail.Equals(m_contactEmail.at(index).c_str()))
      return;
  }
  
  m_contactEmail.push_back(contactEmail);
}

void CContactInfoTag::AppendPhone(const CStdString &phone)
{
  for (unsigned int index = 0; index < m_phone.size(); index++)
  {
    if (phone.Equals(m_phone.at(index).c_str()))
      return;
  }
  
  m_phone.push_back(phone);
}

CStdString CContactInfoTag::Trim(const CStdString &value) const
{
  CStdString trimmedValue(value);
  trimmedValue.TrimLeft(' ');
  trimmedValue.TrimRight(" \n\r");
  return trimmedValue;
}