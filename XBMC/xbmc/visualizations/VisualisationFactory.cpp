/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "stdafx.h"
#include "VisualisationFactory.h"
#include "Util.h"
#include "FileSystem/File.h"

using namespace XFILE;

CVisualisationFactory::CVisualisationFactory()
{

}

CVisualisationFactory::~CVisualisationFactory()
{

}

CVisualisation* CVisualisationFactory::LoadVisualisation(const CStdString& strVisz) const
{
  CStdString nullModule = "";
  return LoadVisualisation( strVisz, nullModule );
}

CVisualisation* CVisualisationFactory::LoadVisualisation(const CStdString& strVisz,
                                                         const CStdString& strSubModule) const
{
  // strip of the path & extension to get the name of the visualisation
  // like goom or spectrum
  CStdString strFileName = strVisz;
  CStdString strName = CUtil::GetFileName(strVisz);

  // if it's a relative path or just a name, convert to absolute path
  if ( strFileName[1] != ':' && strFileName[0] != '/' )
  {
    // first check home
    strFileName.Format("special://home/visualisations/%s", strName.c_str() );

    // if not found, use system
    if ( ! CFile::Exists( strFileName ) )
      strFileName.Format("special://xbmc/visualisations/%s", strName.c_str() );
  }
  strName = strName.Left(strName.ReverseFind('.'));

#ifdef HAS_VISUALISATION
  // load visualisation
  DllVisualisation* pDll = new DllVisualisation;
  pDll->SetFile(strFileName);
  //  FIXME: Some Visualisations do not work 
  //  when their dll is not unloaded immediatly
  pDll->EnableDelayedUnload(false);
  if (!pDll->Load())
  {
    delete pDll;
    return NULL;
  }

  struct Visualisation* pVisz = (struct Visualisation*)malloc(sizeof(struct Visualisation));
  ZeroMemory(pVisz, sizeof(struct Visualisation));
  pDll->GetModule(pVisz);

  // and pass it to a new instance of CVisualisation() which will hanle the visualisation
  return new CVisualisation(pVisz, pDll, strName, strSubModule);
#else
  return NULL;
#endif
}
