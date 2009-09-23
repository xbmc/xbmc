#ifndef QCDTAGS_H
#define QCDTAGS_H

#include "QCDModTagEditor.h"

extern HINSTANCE		hInstance;

void ShutDown_Tag(int flags);
bool Read_Tag(LPCSTR filename, void* tagData);
bool Write_Tag(LPCSTR filename, void* tagData);
bool Strip_Tag(LPCSTR filename);


#endif