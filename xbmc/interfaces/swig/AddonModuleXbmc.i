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

%module(directors="1") xbmc

%{
#include "interfaces/legacy/Player.h"
#include "interfaces/legacy/RenderCapture.h"
#include "interfaces/legacy/Keyboard.h"
#include "interfaces/legacy/ModuleXbmc.h"
#include "interfaces/legacy/Monitor.h"

using namespace XBMCAddon;
using namespace xbmc;

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
%}

// This is all about warning suppression. It's OK that these base classes are 
// not part of what swig parses.
%feature("knownbasetypes") XBMCAddon::xbmc "AddonClass,IPlayerCallback,AddonCallback"
%feature("knownapitypes") XBMCAddon::xbmc "XBMCAddon::xbmcgui::ListItem,XBMCAddon::xbmc::PlayListItem"

%include "interfaces/legacy/swighelper.h"

%feature("python:coerceToUnicode") XBMCAddon::xbmc::getLocalizedString "true"

%include "interfaces/legacy/ModuleXbmc.h"

%feature("director") Player;

%feature("python:method:play") Player
{
    PyObject *pObject = NULL;
    PyObject *pObjectListItem = NULL;
    char bWindowed = false;
    static const char *keywords[] = { "item", "listitem", "windowed", NULL };

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"|OOb",
      (char**)keywords,
      &pObject,
      &pObjectListItem,
      &bWindowed))
    {
      return NULL;
    }

    try
    {
      Player* player = ((Player*)retrieveApiInstance((PyObject*)self,&PyXBMCAddon_xbmc_Player_Type,"play","XBMCAddon::xbmc::Player"));

      // set fullscreen or windowed
      bool windowed = (0 != bWindowed);

      if (pObject == NULL)
        player->playCurrent(windowed);
      else if ((PyString_Check(pObject) || PyUnicode_Check(pObject)))
      {
        CStdString item;
        PyXBMCGetUnicodeString(item,pObject,"item","Player::play");
        XBMCAddon::xbmcgui::ListItem* pListItem = 
          (pObjectListItem ? 
           (XBMCAddon::xbmcgui::ListItem *)retrieveApiInstance(pObjectListItem,"p.XBMCAddon::xbmcgui::ListItem","XBMCAddon::xbmc::","play") :
           NULL);
        player->playStream(item,pListItem,windowed);
      }
      else // pObject must be a playlist
        player->playPlaylist((PlayList *)retrieveApiInstance(pObject,"p.XBMCAddon::xbmc::PlayList","XBMCAddon::xbmc::","play"), windowed);
    }
    catch (const XbmcCommons::Exception& e)
    { 
      CLog::Log(LOGERROR,"Leaving Python method 'XBMCAddon_xbmc_Player_play'. Exception from call to 'play' '%s' ... returning NULL", e.GetMessage());
      PyErr_SetString(PyExc_RuntimeError, e.GetMessage()); 
      return NULL; 
    }
    catch (...)
    {
      CLog::Log(LOGERROR,"Unknown exception thrown from the call 'play'");
      PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call 'play'"); 
      return NULL; 
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

%feature("python:nokwds") XBMCAddon::xbmc::Keyboard::Keyboard "true"
%feature("python:nokwds") XBMCAddon::xbmc::Player::Player "true"
%feature("python:nokwds") XBMCAddon::xbmc::PlayList::PlayList "true"

%include "interfaces/legacy/Player.h"

 // TODO: This needs to be done with a class that holds the Image
 // data. A memory buffer type. Then a typemap needs to be defined
 // for that type.
%feature("python:method:getImage") RenderCapture
{
  RenderCapture* rc = ((RenderCapture*)retrieveApiInstance((PyObject*)self,&PyXBMCAddon_xbmc_RenderCapture_Type,"getImage","XBMCAddon::xbmc::RenderCapture"));
  if (rc->GetUserState() != CAPTURESTATE_DONE)
  {
    PyErr_SetString(PyExc_SystemError, "illegal user state");
    return NULL;
  }
  
  Py_ssize_t size = rc->getWidth() * rc->getHeight() * 4;
  return PyByteArray_FromStringAndSize((const char *)rc->GetPixels(), size);
}

%include "interfaces/legacy/RenderCapture.h"

%include "interfaces/legacy/InfoTagMusic.h"
%include "interfaces/legacy/InfoTagVideo.h"
%include "interfaces/legacy/Keyboard.h"
%include "interfaces/legacy/PlayList.h"

%feature("director") Monitor;

%include "interfaces/legacy/Monitor.h"


