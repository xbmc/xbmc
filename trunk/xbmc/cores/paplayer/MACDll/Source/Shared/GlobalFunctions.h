#ifndef APE_GLOBALFUNCTIONS_H
#define APE_GLOBALFUNCTIONS_H

/*************************************************************************************
Definitions
*************************************************************************************/
class CIO;

/*************************************************************************************
Read / Write from an IO source and return failure if the number of bytes specified
isn't read or written
*************************************************************************************/
int ReadSafe(CIO * pIO, void * pBuffer, int nBytes);
int WriteSafe(CIO * pIO, void * pBuffer, int nBytes);

/*************************************************************************************
Checks for the existence of a file
*************************************************************************************/
BOOL FileExists(wchar_t * pFilename);

#endif // #ifndef APE_GLOBALFUNCTIONS_H
