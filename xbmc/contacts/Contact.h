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
 \file Contact.h
 \brief
 */
#pragma once

#include "utils/StdString.h"
#include "utils/ISerializable.h"
#include "XBDateTime.h"
//#include "pictures/tags/ContactInfoTag.h" // for EmbeddedArt
#include <map>
#include <vector>

#include "utils/StdString.h"
#include "utils/Job.h"


class CFileItem;

/*!
 \ingroup music
 \brief Class to store and read contact information from CContactDatabase
 \sa CContact, CContactDatabase
 */
class CContact: public ISerializable
{
public:
  CContact() ;
  CContact(CFileItem& item);
  virtual ~CContact(){};
  void Clear() ;
  virtual void Serialize(CVariant& value) const;

  /*
  bool operator<(const CContact &contact) const
  {
    if (strFileName < contact.strFileName) return true;
    if (strFileName > contact.strFileName) return false;
    return false;
  }
   */
  
  /*! \brief whether this contact has art associated with it
   Tests both the strThumb and embeddedArt members.
   */
  bool HasArt() const;
  
  
  int profilePic;
  int idContact;
  CStdString strFirst;
  CStdString strLast;
  CStdString strMiddle;
  CStdString strNick;
  CStdString strPrefix;
  CStdString strSuffix;
  CStdString strNote;
  CStdString strTitle;
  CStdString strLabel;
  CStdString strJobTitle;
  CStdString strOffice;
  CStdString strDepartment;
  
  std::vector<std::string> phone;
  std::vector<std::string> email;
  std::vector<std::string> address;
  std::vector<std::string> dates;
  std::vector<std::string> relation;
  CStdString strThumb;
  CStdString strComment;
};

/*!
 \ingroup music
 \brief A map of CContact objects, used for CContactDatabase
 */
typedef std::map<std::string, CContact> MAPCONTACTS;

