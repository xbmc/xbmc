#include "..\python.h"
#include "..\..\..\playlist.h"
#pragma once

#define PlayList_Check(op) PyObject_TypeCheck(op, &PlayList_Type)
#define PlayList_CheckExact(op) ((op)->ob_type == &PlayList_Type)
#define PlayListItem_Check(op) PyObject_TypeCheck(op, &PlayListItem_Type)
#define PlayListItem_CheckExact(op) ((op)->ob_type == &PlayListItem_Type)

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{

	extern PyTypeObject PlayList_Type;
	extern PyTypeObject PlayListItem_Type;

	typedef struct {
    PyObject_HEAD
		PLAYLIST::CPlayList *pPlayList;
		int iPlayList;
	} PlayList;

	typedef struct {
    PyObject_HEAD
		PLAYLIST::CPlayList::CPlayListItem* item;
	} PlayListItem;


}

#ifdef __cplusplus
}
#endif
