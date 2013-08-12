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

#include "system.h"
#include "PictureInfoTagLoaderFactory.h"
#include "PictureInfoTagLoaderDatabase.h"
#include "PictureInfoTagLoaderJPG.h"

#include "utils/URIUtils.h"
#include "FileItem.h"

#ifdef HAS_ASAP_CODEC
#include "cores/paplayer/ASAPCodec.h"
#endif

using namespace PICTURE_INFO;

CPictureInfoTagLoaderFactory::CPictureInfoTagLoaderFactory()
{}

CPictureInfoTagLoaderFactory::~CPictureInfoTagLoaderFactory()
{}

IPictureInfoTagLoader* CPictureInfoTagLoaderFactory::CreateLoader(const CStdString& strFileName)
{
  // dont try to read the tags for streams & shoutcast
  CFileItem item(strFileName, false);
  if (item.IsInternetStream())
    return NULL;
  
  if (item.IsPictureDb())
    return new CPictureInfoTagLoaderDatabase();
  
  CStdString strExtension = URIUtils::GetExtension(strFileName);
  strExtension.ToLower();
  strExtension.TrimLeft('.');
  
  if (strExtension.IsEmpty())
    return NULL;
  
  if (strExtension == "jpg" )
  {
    CPictureInfoTagLoaderJPG *pTagLoader = new CPictureInfoTagLoaderJPG();
    
//    CTagLoaderTagLib *pTagLoader = new CTagLoaderTagLib();
    return (IPictureInfoTagLoader*)pTagLoader;
  }
  return NULL;
}
