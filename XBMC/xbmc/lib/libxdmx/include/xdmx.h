#ifndef LIBXDMX_H_
#define LIBXDMX_H_

#if defined(WIN32)
  typedef __int64 int64_t;
  typedef __int32 int32_t;
  typedef unsigned __int64 uint64_t;
  typedef unsigned __int32 uint32_t;
#endif

enum
{
  XDMX_LOG_LEVEL_NONE,
  XDMX_LOG_LEVEL_ERROR,
  XDMX_LOG_LEVEL_WARNING,
  XDMX_LOG_LEVEL_INFO,
  XDMX_LOG_LEVEL_DEBUG
};

void XdmxSetLogLevel(int level);

typedef int (*XdmxLogFuncPtr)(const char* format, ...);
void XdmxSetLogFunc(XdmxLogFuncPtr func);

class IXdmxInputStream
{
public:
  virtual unsigned int Read(unsigned char* buf, unsigned int buf_size) = 0;
  virtual int64_t Seek(int64_t offset, int whence) = 0;
  virtual int64_t GetLength() = 0;
  virtual bool IsEOF() = 0;
protected:
  IXdmxInputStream() {}
  virtual ~IXdmxInputStream() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class CElementaryStream
{
public:
  CElementaryStream(unsigned int id, unsigned char elementType);
  virtual ~CElementaryStream();
  unsigned int GetId();
  unsigned char GetElementType();
protected:
  unsigned int m_Id;
  unsigned char m_ElementType;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class CParserPayload
{
public:
  CParserPayload(CElementaryStream* pStream);
  CParserPayload(CElementaryStream* pStream, unsigned char* pData, unsigned int size);
  virtual ~CParserPayload();
  void Attach(unsigned char* pData, unsigned int size);
  unsigned char* GetData(unsigned int size = 0);
  unsigned char* Detach();
  unsigned int GetSize();
  void SetPts(double pts);
  void SetDts(double dts);
  double GetPts();
  double GetDts();
  CElementaryStream* GetStream();
protected:
  CElementaryStream* m_pStream; 
  unsigned char* m_pData;
  unsigned int m_Size;
  double m_Pts; // Seconds
  double m_Dts; // Seconds
};

#endif /*LIBXDMX_H_*/


