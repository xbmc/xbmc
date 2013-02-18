/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "PictureThumbLoader.h"
#include "Picture.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "TextureCache.h"
#include "filesystem/Directory.h"
#include "filesystem/MultiPathDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "settings/GUISettings.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "video/VideoThumbLoader.h"

using namespace XFILE;
using namespace std;

CPictureThumbLoader::CPictureThumbLoader() : CThumbLoader(1), CJobQueue(true)
{
}

CPictureThumbLoader::~CPictureThumbLoader()
{
  StopThread();
}

bool CPictureThumbLoader::LoadItem(CFileItem* pItem)
{
  return true;
}


void CPictureThumbLoader::OnLoaderFinish()
{
}

void CPictureThumbLoader::ProcessFoldersAndArchives(CFileItem *pItem)
{
}
