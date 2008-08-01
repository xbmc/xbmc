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
#include "lib/libPython/python/Python.h"
#include "FileSystem/PluginDirectory.h"
#include "listitem.h"
#include "PluginSettings.h"
#include "FileItem.h"

// include for constants
#include "pyutil.h"

using namespace std;
using namespace XFILE;

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
/*****************************************************************
 * start of xbmc methods
 *****************************************************************/
  PyDoc_STRVAR(addDirectoryItem__doc__,
    "addDirectoryItem(handle, url, listitem [,isFolder, totalItems]) -- Callback function to pass directory contents back to XBMC.\n"
    " - Returns a bool for successful completion.\n"
    "\n"
    "handle      : integer - handle the plugin was started with.\n"
    "url         : string - url of the entry. would be plugin:// for another virtual directory\n"
    "listitem    : ListItem - item to add.\n"
    "isFolder    : [opt] bool - True=folder / False=not a folder(default).\n"
    "totalItems  : [opt] integer - total number of items that will be passed.(used for progressbar)\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - if not xbmcplugin.addDirectoryItem(int(sys.argv[1]), 'F:\\\\Trailers\\\\300.mov', listitem, totalItems=50): break\n");

  PyObject* XBMCPLUGIN_AddDirectoryItem(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "handle", "url", "listitem", "isFolder", "totalItems", NULL };
    int handle = -1;
    PyObject *pURL = NULL;
    PyObject *pItem = NULL;
    bool bIsFolder = false;
    int iTotalItems = 0;
    // parse arguments
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "iOO|bl",
      keywords,
      &handle,
      &pURL,
      &pItem,
      &bIsFolder,
      &iTotalItems
      ))
    {
      return NULL;
    };

    string url;
    if (!PyGetUnicodeString(url, pURL, 1) || !ListItem_CheckExact(pItem)) return NULL;
    
    ListItem *pListItem = (ListItem *)pItem;
    pListItem->item->m_strPath = url;
    pListItem->item->m_bIsFolder = bIsFolder;

    // call the directory class to add our item
    bool bOk = DIRECTORY::CPluginDirectory::AddItem(handle, pListItem->item.get(), iTotalItems);
    return Py_BuildValue("b", bOk);
  }

  PyDoc_STRVAR(endOfDirectory__doc__,
    "endOfDirectory(handle[, succeeded, updateListing]) -- Callback function to tell XBMC that the end of the directory listing in a virtualPythonFolder module is reached.\n"
    "\n"
    "handle           : integer - handle the plugin was started with.\n"
    "succeeded        : [opt] bool - True=script completed successfully(Default)/False=Script did not.\n"
    "updateListing    : [opt] bool - True=this folder should update the current listing/False=Folder is a subfolder(Default).\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.endOfDirectory(int(sys.argv[1]))\n");

  PyObject* XBMCPLUGIN_EndOfDirectory(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "handle", "succeeded", "updateListing", NULL };
    int handle = -1;
    bool bSucceeded = true;
    bool bUpdateListing = false;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "i|bb",
      keywords,
      &handle,
      &bSucceeded,
      &bUpdateListing
      ))
    {
      return NULL;
    };

    // tell the directory class that we're done
    DIRECTORY::CPluginDirectory::EndOfDirectory(handle, bSucceeded, bUpdateListing);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(addSortMethod__doc__,
    "addSortMethod(handle, sortMethod) -- Adds a sorting method for the media list.\n"
    "\n"
    "handle      : integer - handle the plugin was started with.\n"
    "sortMethod  : integer - number for sortmethod see FileItem.h.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.addSortMethod(int(sys.argv[1]), xbmcplugin.SORT_METHOD_TITLE)\n");

  PyObject* XBMCPLUGIN_AddSortMethod(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "handle", "sortMethod", NULL };
    int handle = -1;
    int sortMethod = -1;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "ii",
      keywords,
      &handle,
      &sortMethod
      ))
    {
      return NULL;
    };

    // call the directory class to add the sort method.
    if (sortMethod >= SORT_METHOD_NONE && sortMethod < SORT_METHOD_MAX)
      DIRECTORY::CPluginDirectory::AddSortMethod(handle, (SORT_METHOD)sortMethod);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getSetting__doc__,
    "getSetting(id) -- Returns the value of a setting as a string.\n"
    "\n"
    "id        : string - id of the setting that the module needs to access.\n"
    "\n"
    "*Note, You can use the above as a keyword.\n"
    "\n"
    "example:\n"
    "  - apikey = xbmcplugin.getSetting('apikey')\n");

  PyObject* XBMCPLUGIN_GetSetting(PyObject *self, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "id", NULL };
    char *id;
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "s",
      keywords,
      &id
      ))
    {
      return NULL;
    };

    return Py_BuildValue("s", g_currentPluginSettings.Get(id).c_str());
  }

  PyDoc_STRVAR(setContent__doc__,
    "setContent(handle, content) -- Sets the plugins content.\n"
    "\n"
    "handle      : integer - handle the plugin was started with.\n"
    "content     : string - content type (eg. movies)\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "       content: files, songs, artists, albums, movies, tvshows, episodes, musicvideos\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.setContent(int(sys.argv[1]), 'movies')\n");

  PyObject* XBMCPLUGIN_SetContent(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "handle", "content", NULL };
    int handle = -1;
    char *content;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "is",
      keywords,
      &handle,
      &content
      ))
    {
      return NULL;
    };

    DIRECTORY::CPluginDirectory::SetContent(handle, content);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setPluginCategory__doc__,
    "setPluginCategory(handle, category) -- Sets the plugins name for skins to display.\n"
    "\n"
    "handle      : integer - handle the plugin was started with.\n"
    "category    : string or unicode - plugins sub category.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.setPluginCategory(int(sys.argv[1]), 'Comedy')\n");

  PyObject* XBMCPLUGIN_SetPluginCategory(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "handle", "category", NULL };
    int handle = -1;
    PyObject *category = NULL;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "iO",
      keywords,
      &handle,
      &category
      ))
    {
      return NULL;
    };

    CStdString uCategory;
    if (!category || (category && !PyGetUnicodeString(uCategory, category, 1)))
      return NULL;

    DIRECTORY::CPluginDirectory::SetProperty(handle, "plugincategory", uCategory);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setPluginFanart__doc__,
    "setPluginFanart(handle, image, color1, color2, color3) -- Sets the plugins fanart and color for skins to display.\n"
    "\n"
    "handle      : integer - handle the plugin was started with.\n"
    "image       : [opt] string - path to fanart image.\n"
    "color1      : [opt] hexstring - color1. (e.g. '0xFFFFFFFF')\n"
    "color2      : [opt] hexstring - color2. (e.g. '0xFFFF3300')\n"
    "color3      : [opt] hexstring - color3. (e.g. '0xFF000000')\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.setPluginFanart(int(sys.argv[1]), 'q:\\\\plugins\\\\Apple movie trailers II\\\\fanart.png', color2='0xFFFF3300')\n");

  PyObject* XBMCPLUGIN_SetPluginFanart(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "handle", "image", "color1", "color2", "color3", NULL };
    int handle = -1;
    char *image = NULL;
    char *color1 = NULL;
    char *color2 = NULL;
    char *color3 = NULL;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "i|ssss",
      keywords,
      &handle,
      &image,
      &color1,
      &color2,
      &color3
      ))
    {
      return NULL;
    };

    if (image)
      DIRECTORY::CPluginDirectory::SetProperty(handle, "fanart_image", image);
    if (color1)
      DIRECTORY::CPluginDirectory::SetProperty(handle, "fanart_color1", color1);
    if (color2)
      DIRECTORY::CPluginDirectory::SetProperty(handle, "fanart_color2", color2);
    if (color3)
      DIRECTORY::CPluginDirectory::SetProperty(handle, "fanart_color3", color3);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(disableCache__doc__,
    "disableCache(handle) -- Disables the cache to disc in current directory for this plugin.\n"
    "\n"
    "handle      : integer - handle the plugin was started with.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "       By default caching is time based.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.disableCache(int(sys.argv[1]))\n");

  PyObject* XBMCPLUGIN_DisableCache(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "handle", NULL };
    int handle = -1;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "i",
      keywords,
      &handle
      ))
    {
      return NULL;
    };

    DIRECTORY::CPluginDirectory::SetProperty(handle, "donotcachetodisc", "1");

    Py_INCREF(Py_None);
    return Py_None;
  }

  // define c functions to be used in python here
  PyMethodDef pluginMethods[] = {
    {"addDirectoryItem", (PyCFunction)XBMCPLUGIN_AddDirectoryItem, METH_VARARGS|METH_KEYWORDS, addDirectoryItem__doc__},
    {"endOfDirectory", (PyCFunction)XBMCPLUGIN_EndOfDirectory, METH_VARARGS|METH_KEYWORDS, endOfDirectory__doc__},
    {"addSortMethod", (PyCFunction)XBMCPLUGIN_AddSortMethod, METH_VARARGS|METH_KEYWORDS, addSortMethod__doc__},
    {"getSetting", (PyCFunction)XBMCPLUGIN_GetSetting, METH_VARARGS|METH_KEYWORDS, getSetting__doc__},
    {"setContent", (PyCFunction)XBMCPLUGIN_SetContent, METH_VARARGS|METH_KEYWORDS, setContent__doc__},
    {"setPluginCategory", (PyCFunction)XBMCPLUGIN_SetPluginCategory, METH_VARARGS|METH_KEYWORDS, setPluginCategory__doc__},
    {"setPluginFanart", (PyCFunction)XBMCPLUGIN_SetPluginFanart, METH_VARARGS|METH_KEYWORDS, setPluginFanart__doc__},
    {"disableCache", (PyCFunction)XBMCPLUGIN_DisableCache, METH_VARARGS|METH_KEYWORDS, disableCache__doc__},
    {NULL, NULL, 0, NULL}
  };

/*****************************************************************
 * end of methods and python objects
 * initxbmcplugin(void);
 *****************************************************************/

  PyMODINIT_FUNC
  initxbmcplugin(void)
  {
    // init general xbmc modules
    PyObject* pXbmcPluginModule;

    pXbmcPluginModule = Py_InitModule("xbmcplugin", pluginMethods);
    if (pXbmcPluginModule == NULL) return;
	
    // constants
    PyModule_AddStringConstant(pXbmcPluginModule, "__author__", PY_XBMC_AUTHOR);
    PyModule_AddStringConstant(pXbmcPluginModule, "__date__", "20 August 2007");
    PyModule_AddStringConstant(pXbmcPluginModule, "__version__", "1.0");
    PyModule_AddStringConstant(pXbmcPluginModule, "__credits__", PY_XBMC_CREDITS);
    PyModule_AddStringConstant(pXbmcPluginModule, "__platform__", PY_XBMC_PLATFORM);

    // sort method constants
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_NONE", SORT_METHOD_NONE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_LABEL", SORT_METHOD_LABEL);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_LABEL_IGNORE_THE", SORT_METHOD_LABEL_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_DATE", SORT_METHOD_DATE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_SIZE", SORT_METHOD_SIZE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_FILE", SORT_METHOD_FILE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_DRIVE_TYPE", SORT_METHOD_DRIVE_TYPE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_TRACKNUM", SORT_METHOD_TRACKNUM);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_DURATION", SORT_METHOD_DURATION);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_TITLE", SORT_METHOD_TITLE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_TITLE_IGNORE_THE", SORT_METHOD_TITLE_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_ARTIST", SORT_METHOD_ARTIST);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_ARTIST_IGNORE_THE", SORT_METHOD_ARTIST_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_ALBUM", SORT_METHOD_ALBUM);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_ALBUM_IGNORE_THE", SORT_METHOD_ALBUM_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_GENRE", SORT_METHOD_GENRE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_VIDEO_YEAR", SORT_METHOD_VIDEO_YEAR);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_VIDEO_RATING", SORT_METHOD_VIDEO_RATING);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_PROGRAM_COUNT", SORT_METHOD_PROGRAM_COUNT);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_PLAYLIST_ORDER", SORT_METHOD_PLAYLIST_ORDER);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_EPISODE", SORT_METHOD_EPISODE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_VIDEO_TITLE", SORT_METHOD_VIDEO_TITLE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_PRODUCTIONCODE", SORT_METHOD_PRODUCTIONCODE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_SONG_RATING", SORT_METHOD_SONG_RATING);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_MPAA_RATING", SORT_METHOD_MPAA_RATING);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_VIDEO_RUNTIME", SORT_METHOD_VIDEO_RUNTIME);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_STUDIO", SORT_METHOD_STUDIO);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_STUDIO_IGNORE_THE", SORT_METHOD_STUDIO_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, "SORT_METHOD_UNSORTED", SORT_METHOD_UNSORTED);
  }
}

#ifdef __cplusplus
}
#endif
