
#include "XHandle.h"

bool CloseHandle(HANDLE hObject) {
	if (hObject) {
		delete hObject;
		return true;
	}

	return false;
}
