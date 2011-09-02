#pragma once

extern BOOL getFileExtraData(PCTSTR fname, PVOID *extraData, PDWORD dwSize);
extern BOOL appendFileExtraData(PCTSTR fname, PVOID extraData, DWORD size);
