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

#include "filesystem/PluginDirectory.h"
#include "listitem.h"
#include "FileItem.h"

// include for constants
#include "pyutil.h"

using namespace std;
using namespace XFILE;
using namespace ADDON;

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
    static const char *keywords[] = { "handle", "url", "listitem", "isFolder", "totalItems", NULL };
    int handle = -1;
    PyObject *pURL = NULL;
    PyObject *pItem = NULL;
    char bIsFolder = false;
    int iTotalItems = 0;
    // parse arguments
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"iOO|bl",
      (char**)keywords,
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
    if (!PyXBMCGetUnicodeString(url, pURL, 1) || !ListItem_CheckExact(pItem)) return NULL;

    ListItem *pListItem = (ListItem *)pItem;
    pListItem->item->m_strPath = url;
    pListItem->item->m_bIsFolder = (0 != bIsFolder);

    // call the directory class to add our item
    bool bOk = XFILE::CPluginDirectory::AddItem(handle, pListItem->item.get(), iTotalItems);
    return Py_BuildValue((char*)"b", bOk);
  }

  PyDoc_STRVAR(addDirectoryItems__doc__,
    "addDirectoryItems(handle, items [,totalItems]) -- Callback function to pass directory contents back to XBMC as a list.\n"
    " - Returns a bool for successful completion.\n"
    "\n"
    "handle      : integer - handle the plugin was started with.\n"
    "items       : List - list of (url, listitem[, isFolder]) as a tuple to add.\n"
    "totalItems  : [opt] integer - total number of items that will be passed.(used for progressbar)\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "       Large lists benefit over using the standard addDirectoryItem()\n"
    "       You may call this more than once to add items in chunks\n"
    "\n"
    "example:\n"
    "  - if not xbmcplugin.addDirectoryItems(int(sys.argv[1]), [(url, listitem, False,)]: raise\n");

  PyObject* XBMCPLUGIN_AddDirectoryItems(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    int handle = -1;
    PyObject *pItems = NULL;

    static const char *keywords[] = { "handle", "items", "totalItems", NULL };

    int totalItems = 0;
    // parse arguments
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"iO|l",
      (char**)keywords,
      &handle,
      &pItems,
      &totalItems
      ))
    {
      return NULL;
    };

    CFileItemList items;
    for (int item = 0; item < PyList_Size(pItems); item++)
    {
      PyObject *pItem = PyList_GetItem(pItems, item);
      PyObject *pURL = NULL;
      char bIsFolder = false;
      // parse arguments
      if (!PyArg_ParseTuple(
        pItem,
        (char*)"OO|b",
        &pURL,
        &pItem,
        &bIsFolder
        ))
      {
        return NULL;
      };

      string url;
      if (!PyXBMCGetUnicodeString(url, pURL, 1) || !ListItem_CheckExact(pItem)) return NULL;

      ListItem *pListItem = (ListItem *)pItem;
      pListItem->item->m_strPath = url;
      pListItem->item->m_bIsFolder = (0 != bIsFolder);
      items.Add(pListItem->item);
    }
    // call the directory class to add our items
    bool bOk = XFILE::CPluginDirectory::AddItems(handle, &items, totalItems);

    return Py_BuildValue((char*)"b", bOk);
  }

  PyDoc_STRVAR(endOfDirectory__doc__,
    "endOfDirectory(handle[, succeeded, updateListing, cacheToDisc]) -- Callback function to tell XBMC that the end of the directory listing in a virtualPythonFolder module is reached.\n"
    "\n"
    "handle           : integer - handle the plugin was started with.\n"
    "succeeded        : [opt] bool - True=script completed successfully(Default)/False=Script did not.\n"
    "updateListing    : [opt] bool - True=this folder should update the current listing/False=Folder is a subfolder(Default).\n"
    "cacheToDisc      : [opt] bool - True=Folder will cache if extended time(default)/False=this folder will never cache to disc.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.endOfDirectory(int(sys.argv[1]), cacheToDisc=False)\n");

  PyObject* XBMCPLUGIN_EndOfDirectory(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "handle", "succeeded", "updateListing", "cacheToDisc", NULL };
    int handle = -1;
    char bSucceeded = true;
    char bUpdateListing = false;
    char bCacheToDisc = true;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"i|bbb",
      (char**)keywords,
      &handle,
      &bSucceeded,
      &bUpdateListing,
      &bCacheToDisc
      ))
    {
      return NULL;
    };

    // tell the directory class that we're done
    XFILE::CPluginDirectory::EndOfDirectory(handle, 0 != bSucceeded, 0 != bUpdateListing, 0 != bCacheToDisc);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setResolvedUrl__doc__,
    "setResolvedUrl(handle, succeeded, listitem) -- Callback function to tell XBMC that the file plugin has been resolved to a url\n"
    "\n"
    "handle           : integer - handle the plugin was started with.\n"
    "succeeded        : bool - True=script completed successfully/False=Script did not.\n"
    "listitem         : ListItem - item the file plugin resolved to for playback.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.setResolvedUrl(int(sys.argv[1]), True, listitem)\n");

  PyObject* XBMCPLUGIN_SetResolvedUrl(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "handle", "succeeded", "listitem", NULL };
    int handle = -1;
    char bSucceeded = true;
    PyObject *pItem = NULL;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"ibO",
      (char**)keywords,
      &handle,
      &bSucceeded,
      &pItem
      ))
    {
      return NULL;
    };

    ListItem *pListItem = (ListItem *)pItem;

    XFILE::CPluginDirectory::SetResolvedUrl(handle, 0 != bSucceeded, pListItem->item.get());

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(addSortMethod__doc__,
    "addSortMethod(handle, sortMethod, label2) -- Adds a sorting method for the media list.\n"
    "\n"
    "handle      : integer - handle the plugin was started with.\n"
    "sortMethod  : integer - number for sortmethod see FileItem.h.\n"
    "label2Mask  : [opt] string - the label mask to use for the second label.  Defaults to '%D'\n"
    "              applies to: SORT_METHOD_NONE, SORT_METHOD_UNSORTED, SORT_METHOD_VIDEO_TITLE,\n"
    "                          SORT_METHOD_TRACKNUM, SORT_METHOD_FILE, SORT_METHOD_TITLE\n"
    "                          SORT_METHOD_TITLE_IGNORE_THE, SORT_METHOD_LABEL\n"
    "                          SORT_METHOD_LABEL_IGNORE_THE\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.addSortMethod(int(sys.argv[1]), xbmcplugin.SORT_METHOD_TITLE)\n");

  PyObject* XBMCPLUGIN_AddSortMethod(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "handle", "sortMethod", "label2Mask", NULL };
    int handle = -1;
    int sortMethod = -1;
    PyObject *mask2 = NULL;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"ii|O",
      (char**)keywords,
      &handle,
      &sortMethod,
      &mask2
      ))
    {
      return NULL;
    };

    string label2Mask;
    if (mask2)
      PyXBMCGetUnicodeString(label2Mask, mask2, 1);
    if (label2Mask.empty())
      label2Mask = "%D";

    // call the directory class to add the sort method.
    if (sortMethod >= SORT_METHOD_NONE && sortMethod < SORT_METHOD_MAX)
      XFILE::CPluginDirectory::AddSortMethod(handle, (SORT_METHOD)sortMethod, label2Mask);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getSetting__doc__,
    "getSetting(handle, id) -- Returns the value of a setting as a string.\n"
    "\n"
    "handle    : integer - handle the plugin was started with.\n"
    "id        : string - id of the setting that the module needs to access.\n"
    "\n"
    "*Note, You can use the above as a keyword.\n"
    "\n"
    "example:\n"
    "  - apikey = xbmcplugin.getSetting(int(sys.argv[1]), 'apikey')\n");

  PyObject* XBMCPLUGIN_GetSetting(PyObject *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "handle", "id", NULL };
    int handle = -1;
    char *id;
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"is",
      (char**)keywords,
      &handle,
      &id
      ))
    {
      return NULL;
    };

    return Py_BuildValue((char*)"s", XFILE::CPluginDirectory::GetSetting(handle, id).c_str());
  }

  PyDoc_STRVAR(setSetting__doc__,
    "setSetting(handle, id, value) -- Sets a plugin setting for the current running plugin.\n"
    "\n"
    "handle    : integer - handle the plugin was started with.\n"
    "id        : string - id of the setting that the module needs to access.\n"
    "value     : string or unicode - value of the setting.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.setSetting(int(sys.argv[1]), id='username', value='teamxbmc')\n");

  PyObject* XBMCPLUGIN_SetSetting(PyObject *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "handle", "id", "value" };
    int handle = -1;
    char *id;
    PyObject *pValue = NULL;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"isO",
      (char**)keywords,
      &handle,
      &id,
      &pValue
      ))
    {
      return NULL;
    };

    CStdString value;
    if (!id || !PyXBMCGetUnicodeString(value, pValue, 1))
      return NULL;

    XFILE::CPluginDirectory::SetSetting(handle, id, value);

    Py_INCREF(Py_None);
    return Py_None;
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
    static const char *keywords[] = { "handle", "content", NULL };
    int handle = -1;
    char *content;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"is",
      (char**)keywords,
      &handle,
      &content
      ))
    {
      return NULL;
    };

    XFILE::CPluginDirectory::SetContent(handle, content);

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
    static const char *keywords[] = { "handle", "category", NULL };
    int handle = -1;
    PyObject *category = NULL;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"iO",
      (char**)keywords,
      &handle,
      &category
      ))
    {
      return NULL;
    };

    CStdString uCategory;
    if (!category || (category && !PyXBMCGetUnicodeString(uCategory, category, 1)))
      return NULL;

    XFILE::CPluginDirectory::SetProperty(handle, "plugincategory", uCategory);

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
    "  - xbmcplugin.setPluginFanart(int(sys.argv[1]), 'special://home/addons/plugins/video/Apple movie trailers II/fanart.png', color2='0xFFFF3300')\n");

  PyObject* XBMCPLUGIN_SetPluginFanart(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "handle", "image", "color1", "color2", "color3", NULL };
    int handle = -1;
    char *image = NULL;
    char *color1 = NULL;
    char *color2 = NULL;
    char *color3 = NULL;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"i|ssss",
      (char**)keywords,
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
      XFILE::CPluginDirectory::SetProperty(handle, "fanart_image", image);
    if (color1)
      XFILE::CPluginDirectory::SetProperty(handle, "fanart_color1", color1);
    if (color2)
      XFILE::CPluginDirectory::SetProperty(handle, "fanart_color2", color2);
    if (color3)
      XFILE::CPluginDirectory::SetProperty(handle, "fanart_color3", color3);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setProperty__doc__,
    "setProperty(handle, key, value) -- Sets a container property for this plugin.\n"
    "\n"
    "handle      : integer - handle the plugin was started with.\n"
    "key         : string - property name.\n"
    "value       : string or unicode - value of property.\n"
    "\n"
    "*Note, Key is NOT case sensitive.\n"
    "       You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - xbmcplugin.setProperty(int(sys.argv[1]), 'Emulator', 'M.A.M.E.')\n");

  PyObject* XBMCPLUGIN_SetProperty(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "handle", "key", "value", NULL };
    int handle = -1;
    char *key = NULL;
    PyObject *pValue = NULL;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"isO",
      (char**)keywords,
      &handle,
      &key,
      &pValue
      ))
    {
      return NULL;
    };

    if (!key || !pValue) return NULL;
    CStdString value;
    if (!PyXBMCGetUnicodeString(value, pValue, 1))
      return NULL;

    CStdString lowerKey = key;
    XFILE::CPluginDirectory::SetProperty(handle, key, value);

    Py_INCREF(Py_None);
    return Py_None;
  }

  // define c functions to be used in python here
  PyMethodDef pluginMethods[] = {
    {(char*)"addDirectoryItem", (PyCFunction)XBMCPLUGIN_AddDirectoryItem, METH_VARARGS|METH_KEYWORDS, addDirectoryItem__doc__},
    {(char*)"addDirectoryItems", (PyCFunction)XBMCPLUGIN_AddDirectoryItems, METH_VARARGS|METH_KEYWORDS, addDirectoryItems__doc__},
    {(char*)"endOfDirectory", (PyCFunction)XBMCPLUGIN_EndOfDirectory, METH_VARARGS|METH_KEYWORDS, endOfDirectory__doc__},
    {(char*)"setResolvedUrl", (PyCFunction)XBMCPLUGIN_SetResolvedUrl, METH_VARARGS|METH_KEYWORDS, setResolvedUrl__doc__},
    {(char*)"addSortMethod", (PyCFunction)XBMCPLUGIN_AddSortMethod, METH_VARARGS|METH_KEYWORDS, addSortMethod__doc__},
    {(char*)"getSetting", (PyCFunction)XBMCPLUGIN_GetSetting, METH_VARARGS|METH_KEYWORDS, getSetting__doc__},
    {(char*)"setSetting", (PyCFunction)XBMCPLUGIN_SetSetting, METH_VARARGS|METH_KEYWORDS, setSetting__doc__},
    {(char*)"setContent", (PyCFunction)XBMCPLUGIN_SetContent, METH_VARARGS|METH_KEYWORDS, setContent__doc__},
    {(char*)"setPluginCategory", (PyCFunction)XBMCPLUGIN_SetPluginCategory, METH_VARARGS|METH_KEYWORDS, setPluginCategory__doc__},
    {(char*)"setPluginFanart", (PyCFunction)XBMCPLUGIN_SetPluginFanart, METH_VARARGS|METH_KEYWORDS, setPluginFanart__doc__},
    {(char*)"setProperty", (PyCFunction)XBMCPLUGIN_SetProperty, METH_VARARGS|METH_KEYWORDS, setProperty__doc__},
    {NULL, NULL, 0, NULL}
  };

/*****************************************************************
 * end of methods and python objects
 * initxbmcplugin(void);
 *****************************************************************/

  PyMODINIT_FUNC
  DeinitPluginModule(void)
  {
    // no need to Py_DECREF our objects (see InitPluginTypes()) as they were created only
    // so that they could be added to the module, which steals a reference.
  }

  PyMODINIT_FUNC
  InitPluginTypes(void)
  {
    // no types here
  }

  PyMODINIT_FUNC
  InitPluginModule()
  {
    // init general xbmc modules
    PyObject* pXbmcPluginModule;

    pXbmcPluginModule = Py_InitModule((char*)"xbmcplugin", pluginMethods);
    if (pXbmcPluginModule == NULL) return;

    // constants
    PyModule_AddStringConstant(pXbmcPluginModule, (char*)"__author__", (char*)PY_XBMC_AUTHOR);
    PyModule_AddStringConstant(pXbmcPluginModule, (char*)"__date__", (char*)"20 August 2007");
    PyModule_AddStringConstant(pXbmcPluginModule, (char*)"__version__", (char*)"1.0");
    PyModule_AddStringConstant(pXbmcPluginModule, (char*)"__credits__", (char*)PY_XBMC_CREDITS);
    PyModule_AddStringConstant(pXbmcPluginModule, (char*)"__platform__", (char*)PY_XBMC_PLATFORM);

    // sort method constants
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_NONE", SORT_METHOD_NONE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_LABEL", SORT_METHOD_LABEL);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_LABEL_IGNORE_THE", SORT_METHOD_LABEL_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_DATE", SORT_METHOD_DATE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_SIZE", SORT_METHOD_SIZE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_FILE", SORT_METHOD_FILE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_DRIVE_TYPE", SORT_METHOD_DRIVE_TYPE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_TRACKNUM", SORT_METHOD_TRACKNUM);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_DURATION", SORT_METHOD_DURATION);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_TITLE", SORT_METHOD_TITLE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_TITLE_IGNORE_THE", SORT_METHOD_TITLE_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_ARTIST", SORT_METHOD_ARTIST);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_ARTIST_IGNORE_THE", SORT_METHOD_ARTIST_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_ALBUM", SORT_METHOD_ALBUM);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_ALBUM_IGNORE_THE", SORT_METHOD_ALBUM_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_GENRE", SORT_METHOD_GENRE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_VIDEO_YEAR", SORT_METHOD_YEAR);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_VIDEO_RATING", SORT_METHOD_VIDEO_RATING);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_PROGRAM_COUNT", SORT_METHOD_PROGRAM_COUNT);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_PLAYLIST_ORDER", SORT_METHOD_PLAYLIST_ORDER);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_EPISODE", SORT_METHOD_EPISODE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_VIDEO_TITLE", SORT_METHOD_VIDEO_TITLE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_PRODUCTIONCODE", SORT_METHOD_PRODUCTIONCODE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_SONG_RATING", SORT_METHOD_SONG_RATING);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_MPAA_RATING", SORT_METHOD_MPAA_RATING);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_VIDEO_RUNTIME", SORT_METHOD_VIDEO_RUNTIME);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_STUDIO", SORT_METHOD_STUDIO);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_STUDIO_IGNORE_THE", SORT_METHOD_STUDIO_IGNORE_THE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_UNSORTED", SORT_METHOD_UNSORTED);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_BITRATE", SORT_METHOD_BITRATE);
    PyModule_AddIntConstant(pXbmcPluginModule, (char*)"SORT_METHOD_LISTENERS", SORT_METHOD_LISTENERS);
  }
}

#ifdef __cplusplus
}
#endif
