#include "../../../stdafx.h"
#include "..\..\..\application.h"
#include "..\..\..\playlistplayer.h"
#include "..\..\..\util.h"
#include "player.h"
#include "pyplaylist.h"
#include "pyutil.h"
#include "infotagvideo.h"
#include "infotagmusic.h"

// player callback class


#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	PyObject* Player_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		Player *self;

		self = (Player*)type->tp_alloc(type, 0);
		if (!self) return NULL;

		self->iPlayList = PLAYLIST_MUSIC;
		self->pPlayer = new CPythonPlayer();
		self->pPlayer->SetCallback((PyObject*)self);

		return (PyObject*)self;
	}

	void Player_Dealloc(Player* self)
	{
		if (self->pPlayer) delete self->pPlayer;
		self->pPlayer = NULL;
		self->ob_type->tp_free((PyObject*)self);
	}

	PyDoc_STRVAR(play__doc__,
		"play([item]) -- Play this item.\n"
		"\n"
		"item can be a filename or a PlayList.\n"
		"If item is not given then the Player will try to play the current item\n"
		"in the current playlist.");

	// play a file or python playlist
	PyObject* Player_Play(Player *self, PyObject *args)
	{
		PyObject *pObject = NULL;

		if (!PyArg_ParseTuple(args, "|O", &pObject)) return NULL;

		if (pObject == NULL)
		{
			// play current file in playlist
			if (g_playlistPlayer.GetCurrentPlaylist() != self->iPlayList)
			{
				g_playlistPlayer.SetCurrentPlaylist(self->iPlayList);
			}
			g_applicationMessenger.PlayListPlayerPlay(g_playlistPlayer.GetCurrentSong());
		}
		else if(PlayList_Check(pObject))
		{
			// play a python playlist (a playlist from playlistplayer.cpp)
			PlayList* pPlayList = (PlayList*)pObject;
			self->iPlayList = pPlayList->iPlayList;
			g_playlistPlayer.SetCurrentPlaylist(pPlayList->iPlayList);
			g_applicationMessenger.PlayListPlayerPlay(0);
		}
		else if (PyString_Check(pObject))
		{
			CFileItem item(PyString_AsString(pObject), false);
			if (item.IsPlayList())
			{
				PyErr_SetString(PyExc_ValueError, "Only python playlists are supported (see xbmc.PlayList)");
				return NULL;
			}
			else
			{
				g_applicationMessenger.MediaPlay(item.m_strPath);
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	// Player_Stop
	PyDoc_STRVAR(stop__doc__,
		"stop() -- Stop playing.");

	PyObject* Player_Stop(PyObject *self, PyObject *args)
	{
		g_applicationMessenger.MediaStop();

		Py_INCREF(Py_None);
		return Py_None;
	}

	// Player_Pause
	PyDoc_STRVAR(pause__doc__,
		"pause() -- Pause playing.");

	PyObject* Player_Pause(PyObject *self, PyObject *args)
	{
		g_applicationMessenger.MediaPause();

		Py_INCREF(Py_None);
		return Py_None;
	}

	// Player_PlayNext
	PyDoc_STRVAR(playnext__doc__,
		"playnext() -- Play next item in playlist.");

	PyObject* Player_PlayNext(PyObject *self, PyObject *args)
	{
		g_applicationMessenger.PlayListPlayerNext();

		Py_INCREF(Py_None);
		return Py_None;
	}

	// Player_PlayPrevious
	PyDoc_STRVAR(playprevious__doc__,
		"playprevious() -- Play previous item in playlist.");

	PyObject* Player_PlayPrevious(PyObject *self, PyObject *args)
	{
		g_applicationMessenger.PlayListPlayerPrevious();

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

	// Player_IsPlaying
	PyDoc_STRVAR(isPlaying__doc__,
		"isPlayingAudio() -- returns True is xbmc is playing a file.");

	PyObject* Player_IsPlaying(PyObject *self, PyObject *args)
	{
		return Py_BuildValue("b", g_application.IsPlaying());
	}

	// Player_IsPlayingAudio
	PyDoc_STRVAR(isPlayingAudio__doc__,
		"isPlayingAudio() -- returns True is xbmc is playing an audio file.");

	PyObject* Player_IsPlayingAudio(PyObject *self, PyObject *args)
	{
		return Py_BuildValue("b", g_application.IsPlayingAudio());
	}
	
	// Player_IsPlayingVideo
	PyDoc_STRVAR(isPlayingVideo__doc__,
		"isPlayingVideo() -- returns True if xbmc is playing a video.");

	PyObject* Player_IsPlayingVideo(PyObject *self, PyObject *args)
	{
		return Py_BuildValue("b", g_application.IsPlayingVideo());
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
		return Py_BuildValue("s", g_application.CurrentFile().c_str());
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

		return (PyObject*)InfoTagVideo_FromCIMDBMovie(*(g_application.GetCurrentMovie()));
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

		return (PyObject*)InfoTagMusic_FromCMusicInfoTag(*(g_application.GetCurrentSong()));
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
		return Py_BuildValue("d", dTime);
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

		if (!PyArg_ParseTuple(args, "d", &pTime)) return NULL;

        g_application.SeekTime( pTime );

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef Player_methods[] = {
		{"play", (PyCFunction)Player_Play, METH_VARARGS, play__doc__},
		{"stop", (PyCFunction)Player_Stop, METH_VARARGS, stop__doc__},
		{"pause", (PyCFunction)Player_Pause, METH_VARARGS, pause__doc__},
		{"playnext", (PyCFunction)Player_PlayNext, METH_VARARGS, playnext__doc__},
		{"playprevious", (PyCFunction)Player_PlayPrevious, METH_VARARGS, playprevious__doc__},
		{"onPlayBackStarted", (PyCFunction)Player_OnPlayBackStarted, METH_VARARGS, onPlayBackStarted__doc__},
		{"onPlayBackEnded", (PyCFunction)Player_OnPlayBackEnded, METH_VARARGS, onPlayBackEnded__doc__},
		{"onPlayBackStopped", (PyCFunction)Player_OnPlayBackStopped, METH_VARARGS, onPlayBackStopped__doc__},
		{"isPlaying", (PyCFunction)Player_IsPlaying, METH_VARARGS, isPlaying__doc__},
		{"isPlayingAudio", (PyCFunction)Player_IsPlayingAudio, METH_VARARGS, isPlayingAudio__doc__},
		{"isPlayingVideo", (PyCFunction)Player_IsPlayingVideo, METH_VARARGS, isPlayingVideo__doc__},
		{"getPlayingFile", (PyCFunction)Player_GetPlayingFile, METH_VARARGS, getPlayingFile__doc__},		
		{"getMusicInfoTag", (PyCFunction)Player_GetMusicInfoTag, METH_VARARGS, getMusicInfoTag__doc__},
		{"getVideoInfoTag", (PyCFunction)Player_GetVideoInfoTag, METH_VARARGS, getVideoInfoTag__doc__},
		{"getTotalTime", (PyCFunction)Player_GetTotalTime, METH_NOARGS, getTotalTime__doc__},
		{"getTime", (PyCFunction)Player_GetTime, METH_NOARGS, getTime__doc__},
		{"seekTime", (PyCFunction)Player_SeekTime, METH_VARARGS, seekTime__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(player__doc__,
		"Player class.\n"
		"\n"
		"Player() -- Creates a new Player with as default the xbmc music playlist.");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject Player_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmc.Player",             /*tp_name*/
			sizeof(Player),            /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)Player_Dealloc,/*tp_dealloc*/
			0,                         /*tp_print*/
			0,                         /*tp_getattr*/
			0,                         /*tp_setattr*/
			0,                         /*tp_compare*/
			0,                         /*tp_repr*/
			0,                         /*tp_as_number*/
			0,                         /*tp_as_sequence*/
			0,                         /*tp_as_mapping*/
			0,                         /*tp_hash */
			0,                         /*tp_call*/
			0,                         /*tp_str*/
			0,                         /*tp_getattro*/
			0,                         /*tp_setattro*/
			0,                         /*tp_as_buffer*/
			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
			player__doc__,             /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			Player_methods,            /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			0,                         /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			Player_New,                /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
