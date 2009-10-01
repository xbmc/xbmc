#ifdef IO_USE_STD_LIB_FILE_IO

#ifndef APE_STDLIBFILEIO_H
#define APE_STDLIBFILEIO_H

#include "IO.h"

typedef char* LPCTSTR;

class CStdLibFileIO : public CIO
{
public:

    // construction / destruction
    CStdLibFileIO();
    virtual ~CStdLibFileIO();

    // open / close
    virtual int Open(const wchar_t* pName);
    virtual int Close();
    
    // read / write
    virtual int Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead);
    virtual int Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten);
    
    // seek
    virtual int Seek(int nDistance, unsigned int nMoveMode);
    
    // other functions
    virtual int SetEOF();

    // creation / destruction
    virtual int Create(const wchar_t * pName);
    virtual int Delete();

    // attributes
    virtual int GetPosition();
    virtual int GetSize();
    virtual int GetName(wchar_t * pBuffer);
    int GetHandle();

private:
    
    char m_cFileName[MAX_PATH];
    BOOL m_bReadOnly;
    FILE * m_pFile;
};

#endif // #ifndef APE_STDLIBFILEIO_H

#endif // #ifdef IO_USE_STD_LIB_FILE_IO

