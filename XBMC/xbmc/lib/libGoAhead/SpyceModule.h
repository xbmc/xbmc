#include "defines.h"
#include "webs.h"
#include "..\libPython\python.h"

#ifdef __cplusplus
extern "C" {
#endif

	namespace WEBS_SPYCE
	{
		static bool spyInitialized = false;
		static PyThreadState* spyThreadState = NULL;
		static PyObject* spyFunc;
		int spyceOpen();
		void spyceClose();
		int spyceRequest(webs_t wp, char_t *lpath);
	}

#if defined(__cplusplus)
}
#endif 