#include "stdafx.h"
#include "pyutil.h"
#include "pythonplayer.h"
#include "..\xbpython.h"

using namespace PYXBMC;

CPythonPlayer::CPythonPlayer()
{
	pCallback = NULL;
}

CPythonPlayer::~CPythonPlayer(void)
{
	g_pythonParser.UnregisterPythonPlayerCallBack(this);
}

void CPythonPlayer::OnPlayBackStarted()
{
	// aquire lock?
	Py_AddPendingCall(Py_XBMC_Event_OnPlayBackStarted, pCallback);
}

void CPythonPlayer::OnPlayBackEnded()
{
	// aquire lock?
	Py_AddPendingCall(Py_XBMC_Event_OnPlayBackEnded, pCallback);
}

void CPythonPlayer::SetCallback(PyObject *object)
{
	pCallback = object;
	g_pythonParser.RegisterPythonPlayerCallBack(this);
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnPlayBackStarted(void* playerObject)
{
	if (playerObject != NULL) PyObject_CallMethod((PyObject*)playerObject, "onPlayBackStarted", NULL);
	return 0;
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnPlayBackEnded(void* playerObject)
{
	if (playerObject != NULL) PyObject_CallMethod((PyObject*)playerObject, "onPlayBackEnded", NULL);
	return 0;
}
