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

#include "pyutil.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "PlayListPlayer.h"
#include "player.h"
#include "pyplaylist.h"
#include "infotagvideo.h"
#include "infotagmusic.h"
#include "listitem.h"
#include "FileItem.h"
#include "utils/LangCodeExpander.h"
#include "settings/Settings.h"
#include "pythreadstate.h"
#include "utils/log.h"

using namespace MUSIC_INFO;

// player callback class


#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* Player_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Player *self;
    int playerCore=EPC_NONE;

    self = (Player*)type->tp_alloc(type, 0);
    if (!self) return NULL;

    if (!PyArg_ParseTuple(args, (char*)"|i", &playerCore)) return NULL;

    self->iPlayList = PLAYLIST_MUSIC;

    CPyThreadState pyState;
    self->pPlayer = new CPythonPlayer();
    pyState.Restore();

    self->pPlayer->SetCallback(PyThreadState_Get(), (PyObject*)self);
    self->playerCore = EPC_NONE;

    if (playerCore == EPC_DVDPLAYER ||
        playerCore == EPC_MPLAYER ||
        playerCore == EPC_PAPLAYER)
    {
      self->playerCore = (EPLAYERCORES)playerCore;
    }

    return (PyObject*)self;
  }

  void Player_Dealloc(Player* self)
  {
    self->pPlayer->SetCallback(NULL, NULL);

    CPyThreadState pyState;
    self->pPlayer->Release();
    pyState.Restore();
      
    self->pPlayer = NULL;
    self->ob_type->tp_free((PyObject*)self);
  }

  PyDoc_STRVAR(play__doc__,
    "play([item, listitem, windowed]) -- Play this item.\n"
    "\n"
    "item           : [opt] string - filename, url or playlist.\n"
    "listitem       : [opt] listitem - used with setInfo() to set different infolabels.\n"
    "windowed       : [opt] bool - true=play video windowed, false=play users preference.(default)\n"
    "\n"
    "*Note, If item is not given then the Player will try to play the current item\n"
    "       in the current playlist.\n"
    "\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - listitem = xbmcgui.ListItem('Ironman')\n"
    "  - listitem.setInfo('video', {'Title': 'Ironman', 'Genre': 'Science Fiction'})\n"
    "  - xbmc.Player( xbmc.PLAYER_CORE_MPLAYER ).play(url, listitem, windowed)\n");

  // play a file or python playlist
  PyObject* Player_Play(Player *self, PyObject *args, PyObject *kwds)
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

    // set fullscreen or windowed
    g_settings.m_bStartVideoWindowed = (0 != bWindowed);

    // force a playercore before playing
    g_application.m_eForcedNextPlayer = self->playerCore;

    if (pObject == NULL)
    {
      // play current file in playlist
      if (g_playlistPlayer.GetCurrentPlaylist() != self->iPlayList)
      {
        g_playlistPlayer.SetCurrentPlaylist(self->iPlayList);
      }

      CPyThreadState pyState;
      g_application.getApplicationMessenger().PlayListPlayerPlay(g_playlistPlayer.GetCurrentSong());
    }
    else if ((PyString_Check(pObject) || PyUnicode_Check(pObject)) && pObjectListItem != NULL && ListItem_CheckExact(pObjectListItem))
    {
      // an optional listitem was passed
      ListItem* pListItem = NULL;
      pListItem = (ListItem*)pObjectListItem;

      // set m_strPath to the passed url
      pListItem->item->SetPath(PyString_AsString(pObject));

      CPyThreadState pyState;
      g_application.getApplicationMessenger().PlayFile((const CFileItem)*pListItem->item, false);
    }
    else if (PyString_Check(pObject) || PyUnicode_Check(pObject))
    {
      CFileItem item(PyString_AsString(pObject), false);
      
      CPyThreadState pyState;
      g_application.getApplicationMessenger().MediaPlay(item.GetPath());
    }
    else if (PlayList_Check(pObject))
    {
      // play a python playlist (a playlist from playlistplayer.cpp)
      PlayList* pPlayList = (PlayList*)pObject;
      self->iPlayList = pPlayList->iPlayList;
      g_playlistPlayer.SetCurrentPlaylist(pPlayList->iPlayList);

      CPyThreadState pyState;
      g_application.getApplicationMessenger().PlayListPlayerPlay();
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_Stop
  PyDoc_STRVAR(stop__doc__,
    "stop() -- Stop playing.");

  PyObject* pyPlayer_Stop(PyObject *self, PyObject *args)
  {
    CPyThreadState pyState;
    g_application.getApplicationMessenger().MediaStop();
    pyState.Restore();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_Pause
  PyDoc_STRVAR(pause__doc__,
    "pause() -- Pause playing.");

  PyObject* Player_Pause(PyObject *self, PyObject *args)
  {
    CPyThreadState pyState;
    g_application.getApplicationMessenger().MediaPause();
    pyState.Restore();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_PlayNext
  PyDoc_STRVAR(playnext__doc__,
    "playnext() -- Play next item in playlist.");

  PyObject* Player_PlayNext(Player *self, PyObject *args)
  {
    // force a playercore before playing
    g_application.m_eForcedNextPlayer = self->playerCore;

    CPyThreadState pyState;
    g_application.getApplicationMessenger().PlayListPlayerNext();
    pyState.Restore();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_PlayPrevious
  PyDoc_STRVAR(playprevious__doc__,
    "playprevious() -- Play previous item in playlist.");

  PyObject* Player_PlayPrevious(Player *self, PyObject *args)
  {
    // force a playercore before playing
    g_application.m_eForcedNextPlayer = self->playerCore;

    CPyThreadState pyState;
    g_application.getApplicationMessenger().PlayListPlayerPrevious();
    pyState.Restore();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_PlaySelected
  PyDoc_STRVAR(playselected__doc__,
    "playselected() -- Play a certain item from the current playlist.");

  PyObject* Player_PlaySelected(Player *self, PyObject *args)
  {
    int iItem;
    if (!PyArg_ParseTuple(args, (char*)"i", &iItem)) return NULL;

    // force a playercore before playing
    g_application.m_eForcedNextPlayer = self->playerCore;

    if (g_playlistPlayer.GetCurrentPlaylist() != self->iPlayList)
    {
      g_playlistPlayer.SetCurrentPlaylist(self->iPlayList);
    }
    g_playlistPlayer.SetCurrentSong(iItem);

    CPyThreadState pyState;
    g_application.getApplicationMessenger().PlayListPlayerPlay(iItem);
    pyState.Restore();

    //g_playlistPlayer.Play(iItem);
    //CLog::Log(LOGNOTICE, "Current Song After Play: %i", g_playlistPlayer.GetCurrentSong());

    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_OnPlayBackStarted
  PyDoc_STRVAR(onPlayBackStarted__doc__,
    "onPlayBackStarted() -- onPlayBackStarted method.\n"
    "\n"
    "Will be called when xbmc starts playing a file");

  PyObject* Player_OnPlayBackStarted(PyObject *self, PyObject *args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_OnPlayBackEnded
  PyDoc_STRVAR(onPlayBackEnded__doc__,
    "onPlayBackEnded() -- onPlayBackEnded method.\n"
    "\n"
    "Will be called when xbmc stops playing a file");

  PyObject* Player_OnPlayBackEnded(PyObject *self, PyObject *args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_OnPlayBackStopped
  PyDoc_STRVAR(onPlayBackStopped__doc__,
    "onPlayBackStopped() -- onPlayBackStopped method.\n"
    "\n"
    "Will be called when user stops xbmc playing a file");

  PyObject* Player_OnPlayBackStopped(PyObject *self, PyObject *args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_OnPlayBackPaused
  PyDoc_STRVAR(onPlayBackPaused__doc__,
    "onPlayBackPaused() -- onPlayBackPaused method.\n"
    "\n"
    "Will be called when user pauses a playing file");

  PyObject* Player_OnPlayBackPaused(PyObject *self, PyObject *args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_OnPlayBackResumed
  PyDoc_STRVAR(onPlayBackResumed__doc__,
    "onPlayBackResumed() -- onPlayBackResumed method.\n"
    "\n"
    "Will be called when user resumes a paused file");

  PyObject* Player_OnPlayBackResumed(PyObject *self, PyObject *args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_IsPlaying
  PyDoc_STRVAR(isPlaying__doc__,
    "isPlaying() -- returns True is xbmc is playing a file.");

  PyObject* Player_IsPlaying(PyObject *self, PyObject *args)
  {
    return Py_BuildValue((char*)"b", g_application.IsPlaying());
  }

  // Player_IsPlayingAudio
  PyDoc_STRVAR(isPlayingAudio__doc__,
    "isPlayingAudio() -- returns True is xbmc is playing an audio file.");

  PyObject* Player_IsPlayingAudio(PyObject *self, PyObject *args)
  {
    return Py_BuildValue((char*)"b", g_application.IsPlayingAudio());
  }

  // Player_IsPlayingVideo
  PyDoc_STRVAR(isPlayingVideo__doc__,
    "isPlayingVideo() -- returns True if xbmc is playing a video.");

  PyObject* Player_IsPlayingVideo(PyObject *self, PyObject *args)
  {
    return Py_BuildValue((char*)"b", g_application.IsPlayingVideo());
  }

  // Player_GetPlayingFile
  PyDoc_STRVAR(getPlayingFile__doc__,
    "getPlayingFile() -- returns the current playing file as a string.\n"
    "\n"
    "Throws: Exception, if player is not playing a file.\n"
    "");

  PyObject* Player_GetPlayingFile(PyObject *self, PyObject *args)
  {
    if (!g_application.IsPlaying())
    {
      PyErr_SetString(PyExc_Exception, "XBMC is not playing any file");
      return NULL;
    }
    return Py_BuildValue((char*)"s", g_application.CurrentFile().c_str());
  }

  // Player_GetVideoInfoTag
  PyDoc_STRVAR(getVideoInfoTag__doc__,
    "getVideoInfoTag() -- returns the VideoInfoTag of the current playing Movie.\n"
    "\n"
    "Throws: Exception, if player is not playing a file or current file is not a movie file.\n"
    "\n"
    "Note, this doesn't work yet, it's not tested\n"
    "");

  PyObject* Player_GetVideoInfoTag(PyObject *self, PyObject *args)
  {
    if (!g_application.IsPlayingVideo())
    {
      PyErr_SetString(PyExc_Exception, "XBMC is not playing any videofile");
      return NULL;
    }

    const CVideoInfoTag* movie = g_infoManager.GetCurrentMovieTag();
    if (movie)
      return (PyObject*)InfoTagVideo_FromCVideoInfoTag(*movie);

    CVideoInfoTag movie2;
    return (PyObject*)InfoTagVideo_FromCVideoInfoTag(movie2);
  }

  // Player_GetMusicInfoTag
  PyDoc_STRVAR(getMusicInfoTag__doc__,
    "getMusicInfoTag() -- returns the MusicInfoTag of the current playing 'Song'.\n"
    "\n"
    "Throws: Exception, if player is not playing a file or current file is not a music file.\n"
    "");

  PyObject* Player_GetMusicInfoTag(PyObject *self, PyObject *args)
  {
    if (g_application.IsPlayingVideo() || !g_application.IsPlayingAudio())
    {
      PyErr_SetString(PyExc_Exception, "XBMC is not playing any music file");
      return NULL;
    }

    const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
    if (tag)
      return (PyObject*)InfoTagMusic_FromCMusicInfoTag(*tag);

    CMusicInfoTag tag2;
    return (PyObject*)InfoTagMusic_FromCMusicInfoTag(tag2);
  }

  // Player_GetTotalTime
  PyDoc_STRVAR(getTotalTime__doc__,
    "getTotalTime() -- Returns the total time of the current playing media in\n"
        "                  seconds.  This is only accurate to the full second.\n"
    "\n"
    "Throws: Exception, if player is not playing a file.\n"
    "");

  PyObject* Player_GetTotalTime(PyObject *self)
  {
    if (!g_application.IsPlaying())
    {
      PyErr_SetString(PyExc_Exception, "XBMC is not playing any media file");
      return NULL;
    }

    return PyFloat_FromDouble(g_application.GetTotalTime());
  }

  // Player_GetTime
  PyDoc_STRVAR(getTime__doc__,
    "getTime() -- Returns the current time of the current playing media as fractional seconds.\n"
    "\n"
    "Throws: Exception, if player is not playing a file.\n"
    "");

  PyObject* Player_GetTime(PyObject *self)
  {
        double dTime = 0.0;

    if (!g_application.IsPlaying())
    {
      PyErr_SetString(PyExc_Exception, "XBMC is not playing any media file");
      return NULL;
    }

        dTime = g_application.GetTime();
    return Py_BuildValue((char*)"d", dTime);
  }

  // Player_SeekTime
  PyDoc_STRVAR(seekTime__doc__,
    "seekTime() -- Seeks the specified amount of time as fractional seconds.\n"
        "              The time specified is relative to the beginning of the\n"
        "              currently playing media file.\n"
    "\n"
    "Throws: Exception, if player is not playing a file.\n"
    "");

  PyObject* Player_SeekTime(PyObject *self, PyObject *args)
  {
        double pTime = 0.0;

    if (!g_application.IsPlaying())
    {
      PyErr_SetString(PyExc_Exception, "XBMC is not playing any media file");
      return NULL;
    }

    if (!PyArg_ParseTuple(args, (char*)"d", &pTime)) return NULL;

        g_application.SeekTime( pTime );

    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_SetSubtitles
  PyDoc_STRVAR(setSubtitles__doc__,
    "setSubtitles(path) -- set subtitle file and enable subtitles\n"
    "\n"
    "path           : string or unicode - Path to subtitle\n"
    "\n"
    "example:\n"
    "  - setSubtitles('/path/to/subtitle/test.srt')\n");

  PyObject* Player_SetSubtitles(PyObject *self, PyObject *args)
  {
    char *cLine = NULL;
    if (!PyArg_ParseTuple(args, (char*)"s", &cLine)) return NULL;

    if (g_application.m_pPlayer)
    {
      int nStream = g_application.m_pPlayer->AddSubtitle(cLine);
      if(nStream >= 0)
      {
        g_application.m_pPlayer->SetSubtitle(nStream);
        g_application.m_pPlayer->SetSubtitleVisible(true);
        g_settings.m_currentVideoSettings.m_SubtitleDelay = 0.0f;
        g_application.m_pPlayer->SetSubTitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);
      }
    }

    Py_INCREF(Py_None);
    return Py_None;
  }


  // Player_GetSubtitles
  PyDoc_STRVAR(getSubtitles__doc__,
    "getSubtitles() -- get subtitle stream name\n");

  PyObject* Player_GetSubtitles(PyObject *self)
  {
    if (g_application.m_pPlayer)
    {
      int i = g_application.m_pPlayer->GetSubtitle();
      CStdString strName;
      g_application.m_pPlayer->GetSubtitleName(i, strName);

      if (strName == "Unknown(Invalid)")
        strName = "";
      return Py_BuildValue((char*)"s", strName.c_str());
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_ShowSubtitles
  PyDoc_STRVAR(showSubtitles__doc__,
    "showSubtitles(visible) -- enable/disable subtitles\n"
    "\n"
    "visible        : boolean - True for visible subtitles.\n"
    "example:\n"
    "  - xbmc.Player().showSubtitles(True)");

  PyObject* Player_ShowSubtitles(PyObject *self, PyObject *args)
  {
    char bVisible;
    if (!PyArg_ParseTuple(args, (char*)"b", &bVisible)) return NULL;
    if (g_application.m_pPlayer)
    {
      g_settings.m_currentVideoSettings.m_SubtitleOn = (bVisible != 0);
      g_application.m_pPlayer->SetSubtitleVisible(bVisible != 0);

      Py_INCREF(Py_None);
      return Py_None;
    }
    return NULL;
  }

  // Player_DisableSubtitles
  PyDoc_STRVAR(DisableSubtitles__doc__,
    "DisableSubtitles() -- disable subtitles\n");

  PyObject* Player_DisableSubtitles(PyObject *self)
  {
    CLog::Log(LOGWARNING,"'xbmc.Player().disableSubtitles()' is deprecated and will be removed in future releases, please use 'xbmc.Player().showSubtitles(false)' instead");
    if (g_application.m_pPlayer)
    {
      g_settings.m_currentVideoSettings.m_SubtitleOn = false;
      g_application.m_pPlayer->SetSubtitleVisible(false);

      Py_INCREF(Py_None);
      return Py_None;
    }
    return NULL;
  }

  // Player_getAvailableAudioStreams
  PyDoc_STRVAR(getAvailableAudioStreams__doc__,
    "getAvailableAudioStreams() -- get Audio stream names\n");
  
  PyObject* Player_getAvailableAudioStreams(PyObject *self)
  {
    if (g_application.m_pPlayer)
    {
      PyObject *list = PyList_New(0);
      for (int iStream=0; iStream < g_application.m_pPlayer->GetAudioStreamCount(); iStream++)
      {  
        CStdString strName;
        CStdString FullLang;
        g_application.m_pPlayer->GetAudioStreamLanguage(iStream, strName);
        g_LangCodeExpander.Lookup(FullLang, strName);
        if (FullLang.IsEmpty())
          g_application.m_pPlayer->GetAudioStreamName(iStream, FullLang);
        PyList_Append(list, Py_BuildValue((char*)"s", FullLang.c_str()));
      }
      return list;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
  }  

  // Player_setAudioStream
  PyDoc_STRVAR(setAudioStream__doc__,
    "setAudioStream(stream) -- set Audio Stream \n"
    "\n"
    "stream           : int\n"
    "\n"
    "example:\n"
    "  - setAudioStream(1)\n");
  
  PyObject* Player_setAudioStream(PyObject *self, PyObject *args)
  {
    int iStream;
    if (!PyArg_ParseTuple(args, (char*)"i", &iStream)) return NULL;
    
    if (g_application.m_pPlayer)
    {
      int streamCount = g_application.m_pPlayer->GetAudioStreamCount();
      if(iStream < streamCount)
        g_application.m_pPlayer->SetAudioStream(iStream);
    }
    
    Py_INCREF(Py_None);
    return Py_None;
  }  

  // Player_getAvailableSubtitleStreams
  PyDoc_STRVAR(getAvailableSubtitleStreams__doc__,
    "getAvailableSubtitleStreams() -- get Subtitle stream names\n");

  PyObject* Player_getAvailableSubtitleStreams(PyObject *self)
  {
    if (g_application.m_pPlayer)
    {
      PyObject *list = PyList_New(0);
      for (int iStream=0; iStream < g_application.m_pPlayer->GetSubtitleCount(); iStream++)
      {
        CStdString strName;
        CStdString FullLang;
        g_application.m_pPlayer->GetSubtitleName(iStream, strName);
        if (!g_LangCodeExpander.Lookup(FullLang, strName))
          FullLang = strName;
        PyList_Append(list, Py_BuildValue((char*)"s", FullLang.c_str()));
      }
      return list;
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

  // Player_setSubtitleStream
  PyDoc_STRVAR(setSubtitleStream__doc__,
    "setSubtitleStream(stream) -- set Subtitle Stream \n"
    "\n"
    "stream           : int\n"
    "\n"
    "example:\n"
    "  - setSubtitleStream(1)\n");

  PyObject* Player_setSubtitleStream(PyObject *self, PyObject *args)
  {
    int iStream;
    if (!PyArg_ParseTuple(args, (char*)"i", &iStream)) return NULL;
  
    if (g_application.m_pPlayer)
    {
      int streamCount = g_application.m_pPlayer->GetSubtitleCount();
      if(iStream < streamCount)
      {
        g_application.m_pPlayer->SetSubtitle(iStream);
        g_application.m_pPlayer->SetSubtitleVisible(true);
      }
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef Player_methods[] = {
    {(char*)"play", (PyCFunction)Player_Play, METH_VARARGS|METH_KEYWORDS, play__doc__},
    {(char*)"stop", (PyCFunction)pyPlayer_Stop, METH_VARARGS, stop__doc__},
    {(char*)"pause", (PyCFunction)Player_Pause, METH_VARARGS, pause__doc__},
    {(char*)"playnext", (PyCFunction)Player_PlayNext, METH_VARARGS, playnext__doc__},
    {(char*)"playprevious", (PyCFunction)Player_PlayPrevious, METH_VARARGS, playprevious__doc__},
    {(char*)"playselected", (PyCFunction)Player_PlaySelected, METH_VARARGS, playselected__doc__},
    {(char*)"onPlayBackStarted", (PyCFunction)Player_OnPlayBackStarted, METH_VARARGS, onPlayBackStarted__doc__},
    {(char*)"onPlayBackEnded", (PyCFunction)Player_OnPlayBackEnded, METH_VARARGS, onPlayBackEnded__doc__},
    {(char*)"onPlayBackStopped", (PyCFunction)Player_OnPlayBackStopped, METH_VARARGS, onPlayBackStopped__doc__},
    {(char*)"onPlayBackPaused", (PyCFunction)Player_OnPlayBackPaused, METH_VARARGS, onPlayBackPaused__doc__},
    {(char*)"onPlayBackResumed", (PyCFunction)Player_OnPlayBackResumed, METH_VARARGS, onPlayBackResumed__doc__},
    {(char*)"isPlaying", (PyCFunction)Player_IsPlaying, METH_VARARGS, isPlaying__doc__},
    {(char*)"isPlayingAudio", (PyCFunction)Player_IsPlayingAudio, METH_VARARGS, isPlayingAudio__doc__},
    {(char*)"isPlayingVideo", (PyCFunction)Player_IsPlayingVideo, METH_VARARGS, isPlayingVideo__doc__},
    {(char*)"getPlayingFile", (PyCFunction)Player_GetPlayingFile, METH_VARARGS, getPlayingFile__doc__},
    {(char*)"getMusicInfoTag", (PyCFunction)Player_GetMusicInfoTag, METH_VARARGS, getMusicInfoTag__doc__},
    {(char*)"getVideoInfoTag", (PyCFunction)Player_GetVideoInfoTag, METH_VARARGS, getVideoInfoTag__doc__},
    {(char*)"getTotalTime", (PyCFunction)Player_GetTotalTime, METH_NOARGS, getTotalTime__doc__},
    {(char*)"getTime", (PyCFunction)Player_GetTime, METH_NOARGS, getTime__doc__},
    {(char*)"seekTime", (PyCFunction)Player_SeekTime, METH_VARARGS, seekTime__doc__},
    {(char*)"setSubtitles", (PyCFunction)Player_SetSubtitles, METH_VARARGS, setSubtitles__doc__},
    {(char*)"getSubtitles", (PyCFunction)Player_GetSubtitles, METH_NOARGS, getSubtitles__doc__},
    {(char*)"showSubtitles", (PyCFunction)Player_ShowSubtitles, METH_VARARGS, showSubtitles__doc__},
    {(char*)"disableSubtitles", (PyCFunction)Player_DisableSubtitles, METH_NOARGS, DisableSubtitles__doc__},
    {(char*)"getAvailableAudioStreams", (PyCFunction)Player_getAvailableAudioStreams, METH_NOARGS, getAvailableAudioStreams__doc__},
    {(char*)"setAudioStream", (PyCFunction)Player_setAudioStream, METH_VARARGS, setAudioStream__doc__},
    {(char*)"getAvailableSubtitleStreams", (PyCFunction)Player_getAvailableSubtitleStreams, METH_NOARGS, getAvailableSubtitleStreams__doc__},
    {(char*)"setSubtitleStream", (PyCFunction)Player_setSubtitleStream, METH_VARARGS, setSubtitleStream__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(player__doc__,
    "Player class.\n"
    "\n"
    "Player([core]) -- Creates a new Player with as default the xbmc music playlist.\n"
    "\n"
    "core     : (optional) Use a specified playcore instead of letting xbmc decide the playercore to use.\n"
    "         : - xbmc.PLAYER_CORE_AUTO\n"
    "         : - xbmc.PLAYER_CORE_DVDPLAYER\n"
    "         : - xbmc.PLAYER_CORE_MPLAYER\n"
    "         : - xbmc.PLAYER_CORE_PAPLAYER\n");

// Restore code and data sections to normal.

  PyTypeObject Player_Type;

  void initPlayer_Type()
  {
    PyXBMCInitializeTypeObject(&Player_Type);

    Player_Type.tp_name = (char*)"xbmc.Player";
    Player_Type.tp_basicsize = sizeof(Player);
    Player_Type.tp_dealloc = (destructor)Player_Dealloc;
    Player_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Player_Type.tp_doc = player__doc__;
    Player_Type.tp_methods = Player_methods;
    Player_Type.tp_base = 0;
    Player_Type.tp_new = Player_New;
  }
}

#ifdef __cplusplus
}
#endif
