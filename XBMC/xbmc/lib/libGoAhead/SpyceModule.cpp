#include "stdafx.h"
#include "SpyceModule.h"

#ifndef __GNUC__
#pragma code_seg("WEB_TEXT")
#pragma data_seg("WEB_DATA")
#pragma bss_seg("WEB_BSS")
#pragma const_seg("WEB_RD")
#endif

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
			PyEval_AcquireLock();
			PyThreadState_Swap(NULL);
			PyThreadState_Clear(spyThreadState);
			PyThreadState_Delete(spyThreadState);
			PyEval_ReleaseLock();
			g_pythonParser.Finalize();
		}
	}

	int spyceRequest(webs_t wp, char_t *lpath)
	{
		// initialize python first
		if(!spyInitialized)
		{
			g_pythonParser.Initialize();

			PyEval_AcquireLock();
			PyInterpreterState * mainInterpreterState = g_pythonParser.getMainThreadState()->interp;
			spyThreadState = PyThreadState_New(mainInterpreterState);
			PyThreadState_Swap(spyThreadState);

			PyObject* pName = PyString_FromString("spyceXbmc");
			PyObject* pModule = PyImport_Import(pName);
			Py_XDECREF(pName);

			if(!pModule) websError(wp, 500, "%s", "Corrupted Spyce installation");
			else
			{
				PyObject* pDict = PyModule_GetDict(pModule);
				Py_XDECREF(pModule);
				spyFunc = PyDict_GetItemString(pDict, "ParseFile");
				if(!spyFunc) websError(wp, 500, "%s", "Corrupted Spyce installation");
				
				else spyInitialized = true;
			}

			PyThreadState_Swap(NULL);
			PyEval_ReleaseLock();
			if(!spyInitialized)
			{
				PyThreadState_Clear(spyThreadState);
				PyThreadState_Delete(spyThreadState);
				g_pythonParser.Finalize();
				return -1;		
			}
			
		}

		PyEval_AcquireLock();
		PyThreadState_Swap(spyThreadState);

		std::string strRequestMethod;
		std::string strQuery = wp->query;
		std::string strCookie;
		int iContentLenght = 0;
		
		if (strlen(wp->query) > 0)
		{
			if(wp->flags & WEBS_POST_REQUEST)	strRequestMethod = "POST";
			else if (wp->flags & WEBS_HEAD_REQUEST) strRequestMethod = "HEAD";
			else strRequestMethod = "GET";
		}

		if (wp->flags & WEBS_COOKIE) strCookie = wp->cookie;
		iContentLenght = strQuery.length();

		// create enviroment and parse file
		PyObject* pEnv = PyDict_New();
		PyObject* pREQUEST_METHOD = PyString_FromString(strRequestMethod.c_str());
		PyObject* pCONTENT_LENGTH = PyInt_FromLong(iContentLenght);
		PyObject* pQUERY_STRING = PyString_FromString(strQuery.c_str());
		PyObject* pHTTP_COOKIE = PyString_FromString(strCookie.c_str());
		PyObject* pCONTENT_TYPE = PyString_FromString(wp->type);
		PyObject* pHTTP_HOST = PyString_FromString(wp->host);
		PyObject* pHTTP_USER_AGENT = PyString_FromString(wp->userAgent ? wp->userAgent : "");
		PyObject* pHTTP_CONNECTION = PyString_FromString((wp->flags & WEBS_KEEP_ALIVE)? "Keep-Alive" : "");

		PyDict_SetItemString(pEnv, "REQUEST_METHOD", pREQUEST_METHOD);
		PyDict_SetItemString(pEnv, "CONTENT_LENGTH", pCONTENT_LENGTH);
		PyDict_SetItemString(pEnv, "QUERY_STRING", pQUERY_STRING);
		PyDict_SetItemString(pEnv, "HTTP_COOKIE", pHTTP_COOKIE);
		//PyDict_SetItemString(pEnv, "CONTENT_TYPE", pCONTENT_TYPE);
		PyDict_SetItemString(pEnv, "HTTP_HOST", pHTTP_HOST);
		PyDict_SetItemString(pEnv, "HTTP_USER_AGENT", pHTTP_USER_AGENT);
		PyDict_SetItemString(pEnv, "HTTP_CONNECTION", pHTTP_CONNECTION);

		PyObject* pResult = PyObject_CallFunction(spyFunc, "sO", lpath, pEnv);

		Py_XDECREF(pREQUEST_METHOD);
		Py_XDECREF(pCONTENT_LENGTH);
		Py_XDECREF(pQUERY_STRING);
		Py_XDECREF(pHTTP_COOKIE);
		Py_XDECREF(pCONTENT_TYPE);
		Py_XDECREF(pHTTP_HOST);
		Py_XDECREF(pHTTP_USER_AGENT);
		Py_XDECREF(pHTTP_CONNECTION);

		Py_XDECREF(pEnv);

		if(!pResult) websError(wp, 500, "%s", "Corrupted Spyce installation");
		else
		{
			char* cResult = PyString_AsString(pResult);
			websWriteBlock(wp, cResult, strlen(cResult));
			Py_XDECREF(pResult);
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

