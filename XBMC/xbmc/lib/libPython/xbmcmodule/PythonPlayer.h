#pragma once

#include "..\python.h"
#include "..\..\..\cores\IPlayer.h"

int Py_XBMC_Event_OnPlayBackStarted(void* arg);
int Py_XBMC_Event_OnPlayBackEnded(void* arg);

class CPythonPlayer : public IPlayerCallback
{
public:
  CPythonPlayer();
  virtual ~CPythonPlayer(void);
	void		SetCallback(PyObject *object);
	void		OnPlayBackStarted();
	void		OnPlayBackEnded();

protected:
	PyObject*		pCallback;
};