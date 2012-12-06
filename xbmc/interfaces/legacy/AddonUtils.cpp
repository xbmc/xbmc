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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "AddonUtils.h"
#include "guilib/GraphicContext.h"
#include "utils/XBMCTinyXML.h"
#include "addons/Skin.h"
#include "utils/log.h"
#include "threads/ThreadLocal.h"

namespace XBMCAddonUtils
{
  //***********************************************************
  // Some simple helpers
  void guiLock()
  {
    g_graphicsContext.Lock();
  }

  void guiUnlock()
  {
    g_graphicsContext.Unlock();
  }
  //***********************************************************
  
  static char defaultImage[1024];

  const char *getDefaultImage(char* cControlType, char* cTextureType, char* cDefault)
  {
    // create an xml block so that we can resolve our defaults
    // <control type="type">
    //   <description />
    // </control>
    TiXmlElement control("control");
    control.SetAttribute("type", cControlType);
    TiXmlElement filler("description");
    control.InsertEndChild(filler);
    g_SkinInfo->ResolveIncludes(&control);

    // ok, now check for our texture type
    TiXmlElement *pTexture = control.FirstChildElement(cTextureType);
    if (pTexture)
    {
      // found our textureType
      TiXmlNode *pNode = pTexture->FirstChild();
      if (pNode && pNode->Value()[0] != '-')
      {
        strncpy(defaultImage, pNode->Value(), sizeof(defaultImage));
        defaultImage[sizeof(defaultImage) - 1] = '\0';
        return defaultImage;
      }
    }
    return cDefault;
  }

#ifdef ENABLE_TRACE_API
  static XbmcThreads::ThreadLocal<TraceGuard> tlParent;

  static char** getSpacesArray(int size)
  {
    char** ret = new char*[size];
    for (int i = 0; i < size; i++)
    {
      ret[i] = new char[i + 1];

      int j;
      for (j = 0; j < i; j++)
        ret[i][j] = ' ';
      ret[i][j] = 0;
    }
    return ret;
  }

  static char** spaces = getSpacesArray(256);

  const char* TraceGuard::getSpaces() { return spaces[depth]; }

  TraceGuard::TraceGuard(const char* _function) :function(_function) 
  {
    parent = tlParent.get();
    depth = parent == NULL ? 0 : parent->depth + 1;

    tlParent.set(this);

    CLog::Log(LOGDEBUG, "%sNEWADDON Entering %s", spaces[depth], function); 
  }

  TraceGuard::TraceGuard() :function(NULL) 
  {
    parent = tlParent.get();
    depth = parent == NULL ? 0 : parent->depth + 1;
    tlParent.set(this);
    // silent
  }

  TraceGuard::~TraceGuard() 
  {
    if (function)
      CLog::Log(LOGDEBUG, "%sNEWADDON Leaving %s", spaces[depth], function);

    // need to pop the stack
    tlParent.set(this->parent);
  }
#endif


}
