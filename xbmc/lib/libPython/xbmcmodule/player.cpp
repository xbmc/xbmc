#include "stdafx.h"
#include "..\..\..\application.h"
#include "..\..\..\playlistplayer.h"
#include "..\..\..\util.h"
#include "player.h"
#include "playlist.h"

using namespace std;

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

		return (PyObject*)self;
	}

	void Player_Dealloc(Player* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

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
			if (CUtil::IsPlayList(PyString_AsString(pObject)))
			{
				PyErr_SetString(PyExc_ValueError, "Only python playlists are supported (see xbmc.PlayList)");
				return NULL;
			}
			else
			{
				g_applicationMessenger.MediaPlay(PyString_AsString(pObject));
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Player_Stop(PyObject *self, PyObject *args)
	{
		g_applicationMessenger.MediaStop();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Player_Pause(PyObject *self, PyObject *args)
	{
		g_applicationMessenger.MediaPause();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Player_PlayNext(PyObject *self, PyObject *args)
	{
		g_applicationMessenger.PlayListPlayerNext();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Player_PlayPrevious(PyObject *self, PyObject *args)
	{
		g_applicationMessenger.PlayListPlayerPrevious();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef Player_methods[] = {
		{"play", (PyCFunction)Player_Play, METH_VARARGS, ""},
		{"stop", (PyCFunction)Player_Stop, METH_VARARGS, ""},
		{"pause", (PyCFunction)Player_Pause, METH_VARARGS, ""},
		{"playnext", (PyCFunction)Player_PlayNext, METH_VARARGS, ""},
		{"playprevious", (PyCFunction)Player_PlayPrevious, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

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
			"Player Objects",          /* tp_doc */
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
