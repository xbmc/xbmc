#include "stdafx.h"
#include "infotagmusic.h"

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
	/*
	 * allocate a new InfoTagMusic. Used for c++ and not the python user
	 * returns a new reference
	 */
	InfoTagMusic* InfoTagMusic_FromCMusicInfoTag(const MUSIC_INFO::CMusicInfoTag& infoTag)
	{
		InfoTagMusic* self = (InfoTagMusic*)InfoTagMusic_Type.tp_alloc(&InfoTagMusic_Type, 0);
		if (!self) return NULL;

		self->infoTag = infoTag;

		return self;
	}

	void InfoTagMusic_Dealloc(InfoTagMusic* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

	// InfoTagMusic_GetURL
	PyDoc_STRVAR(getURL__doc__,
		"getURL() -- returns a string.\n");

	PyObject* InfoTagMusic_GetURL(InfoTagMusic *self, PyObject *args)
	{
		return Py_BuildValue("s", self->infoTag.GetURL().c_str());
	}

	// InfoTagMusic_GetTitle
	PyDoc_STRVAR(getTitle__doc__,
		"getTitle() -- returns a string.\n");

	PyObject* InfoTagMusic_GetTitle(InfoTagMusic *self, PyObject *args)
	{
		return Py_BuildValue("s", self->infoTag.GetTitle().c_str());
	}

	// InfoTagMusic_GetArtist
	PyDoc_STRVAR(getArtist__doc__,
		"getArtist() -- returns a string.\n");

	PyObject* InfoTagMusic_GetArtist(InfoTagMusic *self, PyObject *args)
	{
		return Py_BuildValue("s", self->infoTag.GetArtist().c_str());
	}

	// InfoTagMusic_GetAlbum
	PyDoc_STRVAR(getAlbum__doc__,
		"getAlbum() -- returns a string.\n");

	PyObject* InfoTagMusic_GetAlbum(InfoTagMusic *self, PyObject *args)
	{
		return Py_BuildValue("s", self->infoTag.GetAlbum().c_str());
	}

	// InfoTagMusic_GetGenre
	PyDoc_STRVAR(getGenre__doc__,
		"getAlbum() -- returns a string.\n");

	PyObject* InfoTagMusic_GetGenre(InfoTagMusic *self, PyObject *args)
	{
		return Py_BuildValue("s", self->infoTag.GetGenre().c_str());
	}

	// InfoTagMusic_GetDuration
	PyDoc_STRVAR(getDuration__doc__,
		"getDuration() -- returns an integer.\n");

	PyObject* InfoTagMusic_GetDuration(InfoTagMusic *self, PyObject *args)
	{
		return Py_BuildValue("i", self->infoTag.GetDuration());
	}

	// InfoTagMusic_GetTrack
	PyDoc_STRVAR(getTrack__doc__,
		"getTrack() -- returns an integer.\n");

	PyObject* InfoTagMusic_GetTrack(InfoTagMusic *self, PyObject *args)
	{
		return Py_BuildValue("s", self->infoTag.GetTrackNumber());
	}

	// InfoTagMusic_ReleaseDate
	PyDoc_STRVAR(getReleaseDate__doc__,
		"getReleaseDate() -- returns a string.\n");

	PyObject* InfoTagMusic_GetReleaseDate(InfoTagMusic *self, PyObject *args)
	{
		return Py_BuildValue("s", "");
	}

	PyMethodDef InfoTagMusic_methods[] = {
		{"getURL", (PyCFunction)InfoTagMusic_GetURL, METH_VARARGS, getURL__doc__},
		{"getTitle", (PyCFunction)InfoTagMusic_GetTitle, METH_VARARGS, getTitle__doc__},
		{"getAlbum", (PyCFunction)InfoTagMusic_GetAlbum, METH_VARARGS, getAlbum__doc__},
		{"getArtist", (PyCFunction)InfoTagMusic_GetArtist, METH_VARARGS, getArtist__doc__},
		{"getGenre", (PyCFunction)InfoTagMusic_GetGenre, METH_VARARGS, getGenre__doc__},
		{"getDuration", (PyCFunction)InfoTagMusic_GetDuration, METH_VARARGS, getDuration__doc__},
		{"getTrack", (PyCFunction)InfoTagMusic_GetTrack, METH_VARARGS, getTrack__doc__},
		{"getReleaseDate", (PyCFunction)InfoTagMusic_GetReleaseDate, METH_VARARGS, getReleaseDate__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(musicInfoTag__doc__,
		"InfoTagMusic class.\n"
		"\n"
		"");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject InfoTagMusic_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmc.InfoTagMusic",       /*tp_name*/
			sizeof(InfoTagMusic),      /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)InfoTagMusic_Dealloc, /*tp_dealloc*/
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
			musicInfoTag__doc__,       /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			InfoTagMusic_methods,      /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			0,                         /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			0,                         /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
