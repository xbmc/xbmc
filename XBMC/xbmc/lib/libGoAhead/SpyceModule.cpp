#include "..\..\application.h"
#include "..\..\sectionLoader.h"
#include "SpyceModule.h"


namespace WEBS_SPYCE
{
	int spyceOpen()
	{
		return 0;
	}

	void spyceClose()
	{
		if(spyInitialized)
		{
			// grab the lock
			PyEval_AcquireLock();
			// swap my thread state out of the interpreter
			PyThreadState_Swap(NULL);
			// clear out any cruft from thread state object
			PyThreadState_Clear(spyThreadState);
			// delete my thread state object
			PyThreadState_Delete(spyThreadState);
			// release the lock
			PyEval_ReleaseLock();
			m_pythonParser.Finalize();
		}
	}

	int spyceRequest(webs_t wp, char_t *lpath)
	{
		if(!spyInitialized)
		{
			m_pythonParser.Initialize();

			// get the global lock
			PyEval_AcquireLock();
			// get a reference to the PyInterpreterState
			PyInterpreterState * mainInterpreterState = m_pythonParser.getMainThreadState()->interp;
			// create a thread state object for this thread
			spyThreadState = PyThreadState_New(mainInterpreterState);
			// clear the thread state and free the lock
			PyThreadState_Swap(NULL);
			PyEval_ReleaseLock();
			spyInitialized = true;
		}

		PyEval_AcquireLock();
		PyThreadState_Swap(spyThreadState);

		char sourcedir[1024];
		strcpy(sourcedir, "Q:\\scripts\\spyce;");
		strcat(sourcedir, getenv("PYTHONPATH"));
		PySys_SetPath(sourcedir);
		chdir("Q:\\scripts\\spyce");

		PyObject* pName = PyString_FromString("spyceXbmc");
		PyObject* pModule = PyImport_Import(pName);
		Py_XDECREF(pName);

		if(!pModule) websWrite(wp, "%s", "<body><html>Error: Unable to find spyceXbmc.py</body></html>");
		else
		{
			PyObject* pDict = PyModule_GetDict(pModule);
			Py_XDECREF(pModule);
	    PyObject* pFunc = PyDict_GetItemString(pDict, "ParseFile");
			if(!pFunc) websWrite(wp, "%s", "<body><html>Error: Corrupted Spyce installation</body></html>");
			else
			{
				PyObject* pResult = PyObject_CallFunction(pFunc, "s", lpath);
				if(!pResult) websWrite(wp, "%s", "<body><html>Error: Corrupted Spyce installation</body></html>");
				else
				{
					char* cResult = PyString_AsString(pResult);
					websWriteBlock(wp, cResult, strlen(cResult));
					Py_XDECREF(pResult);
				}
			}
		}

		PyThreadState_Swap(NULL);
		PyEval_ReleaseLock();

	/*
	*	Common exit and cleanup
	*/
		if (websValid(wp)) {
			websPageClose(wp);
		}
		return 0;
	}
}