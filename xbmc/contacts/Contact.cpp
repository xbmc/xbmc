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

#include "Contact.h"
#include "contacts/tags/ContactInfoTag.h"
#include "utils/Variant.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"

#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#include "Contact.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "DllSwScale.h"
#include "guilib/Texture.h"
#include "guilib/imagefactory.h"
#if defined(HAS_OMXPLAYER)
#include "cores/omxplayer/OMXImage.h"
#endif

using namespace XFILE;
using namespace std;
using namespace CONTACT_INFO;


CContact::CContact(CFileItem& item)
{
  CContactInfoTag& tag = *item.GetContactInfoTag();
  strTitle = tag.GetTitle();
  phone = tag.GetPhone();
  strComment = tag.GetComment();
//  strFileName = tag.GetURL().IsEmpty() ? item.GetPath() : tag.GetURL();
  strThumb = item.GetUserContactThumb(true);
}

CContact::CContact()
{
  Clear();
}

void CContact::Serialize(CVariant& value) const
{
//  value["filename"] = strFileName;
  value["title"] = strTitle;
  value["phone"] = phone;
  value["profilepic"] = profilePic;
  value["comment"] = strComment;
  value["contactid"] = idContact;
}

void CContact::Clear()
{
//  strFileName.Empty();
  strTitle.Empty();
  phone.clear();
  phone.clear();
  strThumb.Empty();
  strComment.Empty();
}

bool CContact::HasArt() const
{
  if (!strThumb.empty()) return true;
  return false;
}

