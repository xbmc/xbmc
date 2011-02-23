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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
  #if (defined HAVE_LIBPYTHON2_6)
    #include <python2.6/Python.h>
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/Python.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/Python.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
  #include "python/Include/Python.h"
#endif
#include "../XBPythonDll.h"
#include "listitem.h"
#include "pyutil.h"
#include "video/VideoInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "FileItem.h"

using namespace std;

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* ListItem_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    ListItem *self;
    static const char *keywords[] = { "label", "label2",
      "iconImage", "thumbnailImage", "path", NULL };

    PyObject* label = NULL;
    PyObject* label2 = NULL;
    PyObject* iconImage = NULL;
    PyObject* thumbnailImage = NULL;
    PyObject* path = NULL;

    // allocate new object
    self = (ListItem*)type->tp_alloc(type, 0);
    if (!self)
      return NULL;

    self->item.reset();

    // parse user input
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"|OOOOO",
      (char**)keywords,
      &label,
      &label2,
      &iconImage,
      &thumbnailImage,
      &path))
    {
      Py_DECREF( self );
      return NULL;
    }

    // create CFileItem
    self->item.reset(new CFileItem());
    if (!self->item)
    {
      Py_DECREF( self );
      return NULL;
    }
    CStdString utf8String;
    if (label && PyXBMCGetUnicodeString(utf8String, label, 1))
    {
      self->item->SetLabel( utf8String );
    }
    if (label2 && PyXBMCGetUnicodeString(utf8String, label2, 1))
    {
      self->item->SetLabel2( utf8String );
    }
    if (iconImage && PyXBMCGetUnicodeString(utf8String, iconImage, 1))
    {
      self->item->SetIconImage( utf8String );
    }
    if (thumbnailImage && PyXBMCGetUnicodeString(utf8String, thumbnailImage, 1))
    {
      self->item->SetThumbnailImage( utf8String );
    }
    if (path && PyXBMCGetUnicodeString(utf8String, path, 1))
    {
      self->item->m_strPath = utf8String;
    }
    return (PyObject*)self;
  }

  /*
   * allocate a new listitem. Used for c++ and not the python user
   * returns a new reference
   */
  ListItem* ListItem_FromString(string strLabel)
  {
    ListItem* self = (ListItem*)ListItem_Type.tp_alloc(&ListItem_Type, 0);
    if (!self) return NULL;

    self->item.reset(new CFileItem(strLabel));
    if (!self->item)
    {
      Py_DECREF( self );
      return NULL;
    }

    return self;
  }

  void ListItem_Dealloc(ListItem* self)
  {
    self->ob_type->tp_free((PyObject*)self);
  }

  PyDoc_STRVAR(getLabel__doc__,
    "getLabel() -- Returns the listitem label.\n"
    "\n"
    "example:\n"
    "  - label = self.list.getSelectedItem().getLabel()\n");

  PyObject* ListItem_GetLabel(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    PyXBMCGUILock();
    const char *cLabel = self->item->GetLabel().c_str();
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"s", cLabel);
  }

  PyDoc_STRVAR(getLabel2__doc__,
    "getLabel2() -- Returns the listitem's second label.\n"
    "\n"
    "example:\n"
    "  - label2 = self.list.getSelectedItem().getLabel2()\n");

  PyObject* ListItem_GetLabel2(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    PyXBMCGUILock();
    const char *cLabel = self->item->GetLabel2().c_str();
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"s", cLabel);
  }

  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label) -- Sets the listitem's label.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setLabel('Casino Royale')\n");

  PyObject* ListItem_SetLabel(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;
    PyObject* unicodeLine = NULL;

    if (!PyArg_ParseTuple(args, (char*)"O", &unicodeLine)) return NULL;

    string utf8Line;
    if (unicodeLine && !PyXBMCGetUnicodeString(utf8Line, unicodeLine, 1))
      return NULL;
    // set label
    PyXBMCGUILock();
    self->item->SetLabel(utf8Line);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setLabel2__doc__,
    "setLabel2(label2) -- Sets the listitem's second label.\n"
    "\n"
    "label2         : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setLabel2('[pg-13]')\n");

  PyObject* ListItem_SetLabel2(ListItem *self, PyObject *args)
  {
    PyObject* unicodeLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, (char*)"O", &unicodeLine)) return NULL;

    string utf8Line;
    if (unicodeLine && !PyXBMCGetUnicodeString(utf8Line, unicodeLine, 1))
      return NULL;
    // set label
    PyXBMCGUILock();
    self->item->SetLabel2(utf8Line);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setIconImage__doc__,
    "setIconImage(icon) -- Sets the listitem's icon image.\n"
    "\n"
    "icon            : string or unicode - image filename.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setIconImage('emailread.png')\n");

  PyObject* ListItem_SetIconImage(ListItem *self, PyObject *args)
  {
    PyObject* unicodeLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, (char*)"O", &unicodeLine)) return NULL;

    string utf8Line;
    if (unicodeLine && !PyXBMCGetUnicodeString(utf8Line, unicodeLine, 1))
      return NULL;

    // set label
    PyXBMCGUILock();
    self->item->SetIconImage(utf8Line);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setThumbnailImage__doc__,
    "setThumbnailImage(thumb) -- Sets the listitem's thumbnail image.\n"
    "\n"
    "thumb           : string or unicode - image filename.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setThumbnailImage('emailread.png')\n");

  PyObject* ListItem_SetThumbnailImage(ListItem *self, PyObject *args)
  {
    PyObject* unicodeLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, (char*)"O", &unicodeLine)) return NULL;

    string utf8Line;
    if (unicodeLine && !PyXBMCGetUnicodeString(utf8Line, unicodeLine, 1))
      return NULL;

    // set label
    PyXBMCGUILock();
    self->item->SetThumbnailImage(utf8Line);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(select__doc__,
    "select(selected) -- Sets the listitem's selected status.\n"
    "\n"
    "selected        : bool - True=selected/False=not selected\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().select(True)\n");

  PyObject* ListItem_Select(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    char bOnOff = false;
    if (!PyArg_ParseTuple(args, (char*)"b", &bOnOff)) return NULL;

    PyXBMCGUILock();
    self->item->Select(0 != bOnOff);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(isSelected__doc__,
    "isSelected() -- Returns the listitem's selected status.\n"
    "\n"
    "example:\n"
    "  - is = self.list.getSelectedItem().isSelected()\n");

  PyObject* ListItem_IsSelected(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    PyXBMCGUILock();
    bool bOnOff = self->item->IsSelected();
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"b", bOnOff);
  }

  PyDoc_STRVAR(setInfo__doc__,
    "setInfo(type, infoLabels) -- Sets the listitem's infoLabels.\n"
    "\n"
    "type              : string - type of media(video/music/pictures).\n"
    "infoLabels        : dictionary - pairs of { label: value }.\n"
    "\n"
    "*Note, To set pictures exif info, prepend 'exif:' to the label. Exif values must be passed\n"
    "       as strings, separate value pairs with a comma. (eg. {'exif:resolution': '720,480'}\n"
    "       See CPictureInfoTag::TranslateString in PictureInfoTag.cpp for valid strings.\n"
    "\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "General Values that apply to all types:\n"
    "    count         : integer (12) - can be used to store an id for later, or for sorting purposes\n"
    "    size          : long (1024) - size in bytes\n"
    "    date          : string (%d.%m.%Y / 01.01.2009) - file date\n"
    "\n"
    "Video Values:\n"
    "    genre         : string (Comedy)\n"
    "    year          : integer (2009)\n"
    "    episode       : integer (4)\n"
    "    season        : integer (1)\n"
    "    top250        : integer (192)\n"
    "    tracknumber   : integer (3)\n"
    "    rating        : float (6.4) - range is 0..10\n"
    "    watched       : depreciated - use playcount instead\n"
    "    playcount     : integer (2) - number of times this item has been played\n"
    "    overlay       : integer (2) - range is 0..8.  See GUIListItem.h for values\n"
    "    cast          : list (Michal C. Hall)\n"
    "    castandrole   : list (Michael C. Hall|Dexter)\n"
    "    director      : string (Dagur Kari)\n"
    "    mpaa          : string (PG-13)\n"
    "    plot          : string (Long Description)\n"
    "    plotoutline   : string (Short Description)\n"
    "    title         : string (Big Fan)\n"
    "    originaltitle : string (Big Fan)\n"
    "    duration      : string (3:18)\n"
    "    studio        : string (Warner Bros.)\n"
    "    tagline       : string (An awesome movie) - short description of movie\n"
    "    writer        : string (Robert D. Siegel)\n"
    "    tvshowtitle   : string (Heroes)\n"
    "    premiered     : string (2005-03-04)\n"
    "    status        : string (Continuing) - status of a TVshow\n"
    "    code          : string (tt0110293) - IMDb code\n"
    "    aired         : string (2008-12-07)\n"
    "    credits       : string (Andy Kaufman) - writing credits\n"
    "    lastplayed    : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)\n"
    "    album         : string (The Joshua Tree)\n"
    "    votes         : string (12345 votes)\n"
    "    trailer       : string (/home/user/trailer.avi)\n"
    "\n"
    "Music Values:\n"
    "    tracknumber   : integer (8)\n"
    "    duration      : integer (245) - duration in seconds\n"
    "    year          : integer (1998)\n"
    "    genre         : string (Rock)\n"
    "    album         : string (Pulse)\n"
    "    artist        : string (Muse)\n"
    "    title         : string (American Pie)\n"
    "    rating        : string (3) - single character between 0 and 5\n"
    "    lyrics        : string (On a dark desert highway...)\n"
    "    playcount     : integer (2) - number of times this item has been played\n"
    "    lastplayed    : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)\n"
    "\n"
    "Picture Values:\n"
    "    title         : string (In the last summer-1)\n"
    "    picturepath   : string (/home/username/pictures/img001.jpg)\n"
    "    exif*         : string (See CPictureInfoTag::TranslateString in PictureInfoTag.cpp for valid strings)\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setInfo('video', { 'Genre': 'Comedy' })\n");

  PyObject* ListItem_SetInfo(ListItem *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "type", "infoLabels", NULL };
    char *cType = NULL;
    PyObject *pInfoLabels = NULL;
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"sO",
      (char**)keywords,
      &cType,
      &pInfoLabels))
    {
      return NULL;
    }
    if (!PyObject_TypeCheck(pInfoLabels, &PyDict_Type))
    {
      PyErr_SetString(PyExc_TypeError, "infoLabels object should be of type Dict");
      return NULL;
    }
    if (PyDict_Size(pInfoLabels) == 0)
    {
      PyErr_SetString(PyExc_ValueError, "Empty InfoLabel dictionary");
      return NULL;
    }

    PyObject *key, *value;
    int pos = 0;

    PyXBMCGUILock();

    CStdString tmp;
    while (PyDict_Next(pInfoLabels, (Py_ssize_t*)&pos, &key, &value)) {
      if (strcmpi(cType, "video") == 0)
      {
        if (strcmpi(PyString_AsString(key), "year") == 0)
          self->item->GetVideoInfoTag()->m_iYear = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "episode") == 0)
          self->item->GetVideoInfoTag()->m_iEpisode = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "season") == 0)
          self->item->GetVideoInfoTag()->m_iSeason = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "top250") == 0)
          self->item->GetVideoInfoTag()->m_iTop250 = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "tracknumber") == 0)
          self->item->GetVideoInfoTag()->m_iTrack = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "count") == 0)
          self->item->m_iprogramCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "rating") == 0)
          self->item->GetVideoInfoTag()->m_fRating = (float)PyFloat_AsDouble(value);
        else if (strcmpi(PyString_AsString(key), "size") == 0)
          self->item->m_dwSize = (int64_t)PyLong_AsLongLong(value);
        else if (strcmpi(PyString_AsString(key), "watched") == 0) // backward compat - do we need it?
          self->item->GetVideoInfoTag()->m_playCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "playcount") == 0)
          self->item->GetVideoInfoTag()->m_playCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "overlay") == 0)
        {
          long overlay = PyInt_AsLong(value);
          if (overlay >= 0 && overlay <= 8)
            self->item->SetOverlayImage((CGUIListItem::GUIIconOverlay)overlay);
        }
        else if (strcmpi(PyString_AsString(key), "cast") == 0 || strcmpi(PyString_AsString(key), "castandrole") == 0)
        {
          if (!PyObject_TypeCheck(value, &PyList_Type)) continue;
          self->item->GetVideoInfoTag()->m_cast.clear();
          for (int i = 0; i < PyList_Size(value); i++)
          {
            PyObject *pTuple = NULL;
            pTuple = PyList_GetItem(value, i);
            if (pTuple == NULL) continue;
            PyObject *pActor = NULL;
            PyObject *pRole = NULL;
            if (PyObject_TypeCheck(pTuple, &PyTuple_Type))
            {
              if (!PyArg_ParseTuple(pTuple, (char*)"O|O", &pActor, &pRole)) continue;
            }
            else
              pActor = pTuple;
            SActorInfo info;
            if (!PyXBMCGetUnicodeString(info.strName, pActor, 1)) continue;
            if (pRole != NULL)
              PyXBMCGetUnicodeString(info.strRole, pRole, 1);
            self->item->GetVideoInfoTag()->m_cast.push_back(info);
          }
        }
        else
        {
          if (!PyXBMCGetUnicodeString(tmp, value, 1)) continue;
          if (strcmpi(PyString_AsString(key), "genre") == 0)
            self->item->GetVideoInfoTag()->m_strGenre = tmp;
          else if (strcmpi(PyString_AsString(key), "director") == 0)
            self->item->GetVideoInfoTag()->m_strDirector = tmp;
          else if (strcmpi(PyString_AsString(key), "mpaa") == 0)
            self->item->GetVideoInfoTag()->m_strMPAARating = tmp;
          else if (strcmpi(PyString_AsString(key), "plot") == 0)
            self->item->GetVideoInfoTag()->m_strPlot = tmp;
          else if (strcmpi(PyString_AsString(key), "plotoutline") == 0)
            self->item->GetVideoInfoTag()->m_strPlotOutline = tmp;
          else if (strcmpi(PyString_AsString(key), "title") == 0)
            self->item->GetVideoInfoTag()->m_strTitle = tmp;
          else if (strcmpi(PyString_AsString(key), "originaltitle") == 0)
            self->item->GetVideoInfoTag()->m_strOriginalTitle = tmp;
          else if (strcmpi(PyString_AsString(key), "duration") == 0)
            self->item->GetVideoInfoTag()->m_strRuntime = tmp;
          else if (strcmpi(PyString_AsString(key), "studio") == 0)
            self->item->GetVideoInfoTag()->m_strStudio = tmp;
          else if (strcmpi(PyString_AsString(key), "tagline") == 0)
            self->item->GetVideoInfoTag()->m_strTagLine = tmp;
          else if (strcmpi(PyString_AsString(key), "writer") == 0)
            self->item->GetVideoInfoTag()->m_strWritingCredits = tmp;
          else if (strcmpi(PyString_AsString(key), "tvshowtitle") == 0)
            self->item->GetVideoInfoTag()->m_strShowTitle = tmp;
          else if (strcmpi(PyString_AsString(key), "premiered") == 0)
            self->item->GetVideoInfoTag()->m_strPremiered = tmp;
          else if (strcmpi(PyString_AsString(key), "status") == 0)
            self->item->GetVideoInfoTag()->m_strStatus = tmp;
          else if (strcmpi(PyString_AsString(key), "code") == 0)
            self->item->GetVideoInfoTag()->m_strProductionCode = tmp;
          else if (strcmpi(PyString_AsString(key), "aired") == 0)
            self->item->GetVideoInfoTag()->m_strFirstAired = tmp;
          else if (strcmpi(PyString_AsString(key), "credits") == 0)
            self->item->GetVideoInfoTag()->m_strWritingCredits = tmp;
          else if (strcmpi(PyString_AsString(key), "lastplayed") == 0)
            self->item->GetVideoInfoTag()->m_lastPlayed = tmp;
          else if (strcmpi(PyString_AsString(key), "album") == 0)
            self->item->GetVideoInfoTag()->m_strAlbum = tmp;
          else if (strcmpi(PyString_AsString(key), "votes") == 0)
            self->item->GetVideoInfoTag()->m_strVotes = tmp;
          else if (strcmpi(PyString_AsString(key), "trailer") == 0)
            self->item->GetVideoInfoTag()->m_strTrailer = tmp;
          else if (strcmpi(PyString_AsString(key), "date") == 0)
          {
            if (strlen(tmp) == 10)
              self->item->m_dateTime.SetDate(atoi(tmp.Right(4).c_str()), atoi(tmp.Mid(3,4).c_str()), atoi(tmp.Left(2).c_str()));
          }
        }
      }
      else if (strcmpi(cType, "music") == 0)
      {
        if (strcmpi(PyString_AsString(key), "tracknumber") == 0)
          self->item->GetMusicInfoTag()->SetTrackNumber(PyInt_AsLong(value));
        else if (strcmpi(PyString_AsString(key), "count") == 0)
          self->item->m_iprogramCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "size") == 0)
          self->item->m_dwSize = (int64_t)PyLong_AsLongLong(value);
        else if (strcmpi(PyString_AsString(key), "duration") == 0)
          self->item->GetMusicInfoTag()->SetDuration(PyInt_AsLong(value));
        else if (strcmpi(PyString_AsString(key), "year") == 0)
          self->item->GetMusicInfoTag()->SetYear(PyInt_AsLong(value));
        else if (strcmpi(PyString_AsString(key), "listeners") == 0)
          self->item->GetMusicInfoTag()->SetListeners(PyInt_AsLong(value));
        else if (strcmpi(PyString_AsString(key), "playcount") == 0)
          self->item->GetMusicInfoTag()->SetPlayCount(PyInt_AsLong(value));
        else
        {
          if (!PyXBMCGetUnicodeString(tmp, value, 1)) continue;
          if (strcmpi(PyString_AsString(key), "genre") == 0)
            self->item->GetMusicInfoTag()->SetGenre(tmp);
          else if (strcmpi(PyString_AsString(key), "album") == 0)
            self->item->GetMusicInfoTag()->SetAlbum(tmp);
          else if (strcmpi(PyString_AsString(key), "artist") == 0)
            self->item->GetMusicInfoTag()->SetArtist(tmp);
          else if (strcmpi(PyString_AsString(key), "title") == 0)
            self->item->GetMusicInfoTag()->SetTitle(tmp);
          else if (strcmpi(PyString_AsString(key), "rating") == 0)
            self->item->GetMusicInfoTag()->SetRating(*tmp);
          else if (strcmpi(PyString_AsString(key), "lyrics") == 0)
            self->item->GetMusicInfoTag()->SetLyrics(tmp);
          else if (strcmpi(PyString_AsString(key), "lastplayed") == 0)
            self->item->GetMusicInfoTag()->SetLastPlayed(tmp);
          else if (strcmpi(PyString_AsString(key), "musicbrainztrackid") == 0)
            self->item->GetMusicInfoTag()->SetMusicBrainzTrackID(tmp);
          else if (strcmpi(PyString_AsString(key), "musicbrainzartistid") == 0)
            self->item->GetMusicInfoTag()->SetMusicBrainzArtistID(tmp);
          else if (strcmpi(PyString_AsString(key), "musicbrainzalbumid") == 0)
            self->item->GetMusicInfoTag()->SetMusicBrainzAlbumID(tmp);
          else if (strcmpi(PyString_AsString(key), "musicbrainzalbumartistid") == 0)
            self->item->GetMusicInfoTag()->SetMusicBrainzAlbumArtistID(tmp);
          else if (strcmpi(PyString_AsString(key), "musicbrainztrmid") == 0)
            self->item->GetMusicInfoTag()->SetMusicBrainzTRMID(tmp);
          else if (strcmpi(PyString_AsString(key), "comment") == 0)
            self->item->GetMusicInfoTag()->SetComment(tmp);
          else if (strcmpi(PyString_AsString(key), "date") == 0)
          {
            if (strlen(tmp) == 10)
              self->item->m_dateTime.SetDate(atoi(tmp.Right(4)), atoi(tmp.Mid(3,4)), atoi(tmp.Left(2)));
          }
        }
        self->item->GetMusicInfoTag()->SetLoaded(true);
      }
      else if (strcmpi(cType, "pictures") == 0)
      {
        if (strcmpi(PyString_AsString(key), "count") == 0)
          self->item->m_iprogramCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "size") == 0)
          self->item->m_dwSize = (int64_t)PyLong_AsLongLong(value);
        else
        {
          if (!PyXBMCGetUnicodeString(tmp, value, 1)) continue;
          if (strcmpi(PyString_AsString(key), "title") == 0)
            self->item->m_strTitle = tmp;
          else if (strcmpi(PyString_AsString(key), "picturepath") == 0)
            self->item->m_strPath = tmp;
          else if (strcmpi(PyString_AsString(key), "date") == 0)
          {
            if (strlen(tmp) == 10)
              self->item->m_dateTime.SetDate(atoi(tmp.Right(4)), atoi(tmp.Mid(3,4)), atoi(tmp.Left(2)));
          }
          else
          {
            CStdString exifkey = PyString_AsString(key);
            if (!exifkey.Left(5).Equals("exif:") || exifkey.length() < 6) continue;
            int info = CPictureInfoTag::TranslateString(exifkey.Mid(5));
            self->item->GetPictureInfoTag()->SetInfo(info, tmp);
          }
        }
        self->item->GetPictureInfoTag()->SetLoaded(true);
      }
    }
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setProperty__doc__,
    "setProperty(key, value) -- Sets a listitem property, similar to an infolabel.\n"
    "\n"
    "key            : string - property name.\n"
    "value          : string or unicode - value of property.\n"
    "\n"
    "*Note, Key is NOT case sensitive.\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    " Some of these are treated internally by XBMC, such as the 'StartOffset' property, which is\n"
    " the offset in seconds at which to start playback of an item.  Others may be used in the skin\n"
    " to add extra information, such as 'WatchedCount' for tvshow items\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setProperty('AspectRatio', '1.85 : 1')\n"
    "  - self.list.getSelectedItem().setProperty('StartOffset', '256.4')\n");

  PyObject* ListItem_SetProperty(ListItem *self, PyObject *args, PyObject *kwds)
  {
    if (!self->item) return NULL;

    static const char *keywords[] = { "key", "value", NULL };
    char *key = NULL;
    PyObject *value = NULL;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"sO",
      (char**)keywords,
      &key,
      &value))
    {
      return NULL;
    }
    if (!key || !value) return NULL;

    string uText;
    if (!PyXBMCGetUnicodeString(uText, value, 1))
      return NULL;

    PyXBMCGUILock();
    CStdString lowerKey = key;
    if (lowerKey.CompareNoCase("startoffset") == 0)
    { // special case for start offset - don't actually store in a property,
      // we store it in item.m_lStartOffset instead
      self->item->m_lStartOffset = (int)(atof(uText.c_str()) * 75.0); // we store the offset in frames, or 1/75th of a second
    }
    else if (lowerKey.CompareNoCase("mimetype") == 0)
    { // special case for mime type - don't actually stored in a property,
      self->item->SetMimeType(uText);
    }
    else if (lowerKey.CompareNoCase("specialsort") == 0)
    {
      if (uText == "bottom")
        self->item->SetSpecialSort(SORT_ON_BOTTOM);
      else if (uText == "top")
        self->item->SetSpecialSort(SORT_ON_TOP);
    }
    else
      self->item->SetProperty(lowerKey.ToLower(), uText.c_str());
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getProperty__doc__,
    "getProperty(key) -- Returns a listitem property as a string, similar to an infolabel.\n"
    "\n"
    "key            : string - property name.\n"
    "\n"
    "*Note, Key is NOT case sensitive.\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - AspectRatio = self.list.getSelectedItem().getProperty('AspectRatio')\n");

  PyObject* ListItem_GetProperty(ListItem *self, PyObject *args, PyObject *kwds)
  {
    if (!self->item) return NULL;

    static const char *keywords[] = { "key", NULL };
    char *key = NULL;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"s",
      (char**)keywords,
      &key))
    {
      return NULL;
    }
    if (!key) return NULL;

    PyXBMCGUILock();
    CStdString lowerKey = key;
    CStdString value;
    if (lowerKey.CompareNoCase("startoffset") == 0)
    { // special case for start offset - don't actually store in a property,
      // we store it in item.m_lStartOffset instead
      value.Format("%f", self->item->m_lStartOffset / 75.0);
    }
    else
      value = self->item->GetProperty(lowerKey.ToLower());
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"s", value.c_str());
  }

  // addContextMenuItems() method
  PyDoc_STRVAR(addContextMenuItems__doc__,
  "addContextMenuItems([(label, action,)*], replaceItems) -- Adds item(s) to the context menu for media lists.\n"
    "\n"
    "items               : list - [(label, action,)*] A list of tuples consisting of label and action pairs.\n"
    "  - label           : string or unicode - item's label.\n"
    "  - action          : string or unicode - any built-in function to perform.\n"
    "replaceItems        : [opt] bool - True=only your items will show/False=your items will be added to context menu(Default).\n"
    "\n"
    "List of functions - http://wiki.xbmc.org/?title=List_of_Built_In_Functions \n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - listitem.addContextMenuItems([('Theater Showtimes', 'XBMC.RunScript(special://home/scripts/showtimes/default.py,Iron Man)',)])\n");

  PyObject* ListItem_AddContextMenuItems(ListItem *self, PyObject *args, PyObject *kwds)
  {
    if (!self->item) return NULL;

    PyObject *pList = NULL;
    char bReplaceItems = false;
    static const char *keywords[] = { "items", "replaceItems", NULL };

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"O|b",
      (char**)keywords,
      &pList,
      &bReplaceItems) || pList == NULL || !PyObject_TypeCheck(pList, &PyList_Type))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type List");
      return NULL;
    }

    for (int item = 0; item < PyList_Size(pList); item++)
    {
      PyObject *pTuple = NULL;
      pTuple = PyList_GetItem(pList, item);
      if (!pTuple || !PyObject_TypeCheck(pTuple, &PyTuple_Type))
      {
        PyErr_SetString(PyExc_TypeError, "List must only contain tuples");
        return NULL;
      }
      PyObject *label = NULL;
      PyObject *action = NULL;
      if (!PyArg_ParseTuple(pTuple, (char*)"OO", &label, &action))
      {
        PyErr_SetString(PyExc_TypeError, "Error unpacking tuple found in list");
        return NULL;
      }
      if (!label || !action) return NULL;

      string uText;
      if (!PyXBMCGetUnicodeString(uText, label, 1))
        return NULL;
      string uAction;
      if (!PyXBMCGetUnicodeString(uAction, action, 1))
        return NULL;
      PyXBMCGUILock();

      CStdString property;
      property.Format("contextmenulabel(%i)", item);
      self->item->SetProperty(property, uText);

      property.Format("contextmenuaction(%i)", item);
      self->item->SetProperty(property, uAction);

      PyXBMCGUIUnlock();
    }

    // set our replaceItems status
    if (bReplaceItems)
      self->item->SetProperty("pluginreplacecontextitems", 0 != bReplaceItems);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setPath__doc__,
    "setPath(path) -- Sets the listitem's path.\n"
    "\n"
    "path           : string or unicode - path, activated when item is clicked.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setPath(path='ActivateWindow(Weather)')\n");

  PyObject* ListItem_SetPath(ListItem *self, PyObject *args, PyObject *kwds)
  {
    if (!self->item) return NULL;
    PyObject* pPath = NULL;

    if (!PyArg_ParseTuple(args, (char*)"O", &pPath)) return NULL;
    static const char *keywords[] = { "path", NULL };

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"O",
      (char**)keywords,
      &pPath
      ))
    {
      return NULL;
    }

    string path;
    if (pPath && !PyXBMCGetUnicodeString(path, pPath, 1))
      return NULL;
    // set path
    PyXBMCGUILock();
    self->item->m_strPath = path;
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ListItem_methods[] = {
    {(char*)"getLabel" , (PyCFunction)ListItem_GetLabel, METH_VARARGS, getLabel__doc__},
    {(char*)"setLabel" , (PyCFunction)ListItem_SetLabel, METH_VARARGS, setLabel__doc__},
    {(char*)"getLabel2", (PyCFunction)ListItem_GetLabel2, METH_VARARGS, getLabel2__doc__},
    {(char*)"setLabel2", (PyCFunction)ListItem_SetLabel2, METH_VARARGS, setLabel2__doc__},
    {(char*)"setIconImage", (PyCFunction)ListItem_SetIconImage, METH_VARARGS, setIconImage__doc__},
    {(char*)"setThumbnailImage", (PyCFunction)ListItem_SetThumbnailImage, METH_VARARGS, setThumbnailImage__doc__},
    {(char*)"select", (PyCFunction)ListItem_Select, METH_VARARGS, select__doc__},
    {(char*)"isSelected", (PyCFunction)ListItem_IsSelected, METH_VARARGS, isSelected__doc__},
    {(char*)"setInfo", (PyCFunction)ListItem_SetInfo, METH_VARARGS|METH_KEYWORDS, setInfo__doc__},
    {(char*)"setProperty", (PyCFunction)ListItem_SetProperty, METH_VARARGS|METH_KEYWORDS, setProperty__doc__},
    {(char*)"getProperty", (PyCFunction)ListItem_GetProperty, METH_VARARGS|METH_KEYWORDS, getProperty__doc__},
    {(char*)"addContextMenuItems", (PyCFunction)ListItem_AddContextMenuItems, METH_VARARGS|METH_KEYWORDS, addContextMenuItems__doc__},
    {(char*)"setPath" , (PyCFunction)ListItem_SetPath, METH_VARARGS|METH_KEYWORDS, setPath__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(listItem__doc__,
    "ListItem class.\n"
    "\n"
    "ListItem([label, label2, iconImage, thumbnailImage, path]) -- Creates a new ListItem.\n"
    "\n"
    "label          : [opt] string or unicode - label1 text.\n"
    "label2         : [opt] string or unicode - label2 text.\n"
    "iconImage      : [opt] string - icon filename.\n"
    "thumbnailImage : [opt] string - thumbnail filename.\n"
    "path           : [opt] string or unicode - listitem's path.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - listitem = xbmcgui.ListItem('Casino Royale', '[PG-13]', 'blank-poster.tbn', 'poster.tbn', path='f:\\\\movies\\\\casino_royale.mov')\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ListItem_Type;

  void initListItem_Type()
  {
    PyXBMCInitializeTypeObject(&ListItem_Type);

    ListItem_Type.tp_name = (char*)"xbmcgui.ListItem";
    ListItem_Type.tp_basicsize = sizeof(ListItem);
    ListItem_Type.tp_dealloc = (destructor)ListItem_Dealloc;
    ListItem_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ListItem_Type.tp_doc = listItem__doc__;
    ListItem_Type.tp_methods = ListItem_methods;
    ListItem_Type.tp_base = 0;
    ListItem_Type.tp_new = ListItem_New;
  }
}

#ifdef __cplusplus
}
#endif
