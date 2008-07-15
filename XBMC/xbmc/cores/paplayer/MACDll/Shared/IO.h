#ifndef APE_IO_H
#define APE_IO_H

#ifndef FILE_BEGIN
    #define FILE_BEGIN        0
#endif

#ifndef FILE_CURRENT
    #define FILE_CURRENT    1
#endif

#ifndef FILE_END
    #define FILE_END        2
#endif

class CIO
{
    
public:

    //construction / destruction
    CIO() { }
    virtual ~CIO() { };

    // open / close
    virtual int Open(const wchar_t * pName) = 0;
    virtual int Close() = 0;
    
    // read / write
    virtual int Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead) = 0;
    virtual int Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten) = 0;
    
    // seek
    virtual int Seek(int nDistance, unsigned int nMoveMode) = 0;
    
    // creation / destruction
    virtual int Create(const wchar_t * pName) = 0;
    virtual int Delete() = 0;

    // other functions
    virtual int SetEOF() = 0;

    // attributes
    virtual int GetPosition() = 0;
    virtual int GetSize() = 0;
    virtual int GetName(wchar_t * pBuffer) = 0;
};

#endif // #ifndef APE_IO_H
