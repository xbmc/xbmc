
#include "XHandle.h"

bool CloseHandle(HANDLE hObject) {

    if (hObject == INVALID_HANDLE_VALUE || hObject == (HANDLE)-1)
	  return true;

	if (hObject) {
		delete hObject;
		return true;
	}

	return false;
}
