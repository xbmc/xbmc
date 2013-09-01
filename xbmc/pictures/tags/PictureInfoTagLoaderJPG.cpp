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

#include "PictureInfoTagLoaderJPG.h"
#include "PictureInfoTag.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace PICTURE_INFO;

CPictureInfoTagLoaderJPG::CPictureInfoTagLoaderJPG(void)
{}

CPictureInfoTagLoaderJPG::~CPictureInfoTagLoaderJPG()
{}

bool CPictureInfoTagLoaderJPG::Load(const CStdString& strFileName, CPictureInfoTag& tag, EmbeddedArt *art)
{
  try
  {
    CStdString strExtension = URIUtils::GetExtension(strFileName);
    strExtension.ToLower();
    strExtension.TrimLeft('.');
    
    if (strExtension.IsEmpty())
      return false;

    tag.Load(strFileName);
    tag.SetTitle(URIUtils::GetFileName(strFileName));
    if (!tag.GetTitle().IsEmpty() || !tag.GetPictureAlbum().IsEmpty())
      tag.SetLoaded();
    else
      tag.SetLoaded();
    
    tag.SetURL(strFileName);

        
    return true;
    
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader wav: exception in file %s", strFileName.c_str());
  }
  
  return false;
}

