#ifndef LIBXDMX_H_
#define LIBXDMX_H_

#pragma once

#include <stdint.h>
#include <string>

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

/////////////////////////////////////////////////////////////////////////////////////////////////

enum XdmxPropertyType
{
  XDMX_PROP_TYPE_ANY,
  XDMX_PROP_TYPE_BOOL,
  XDMX_PROP_TYPE_CHAR,
  XDMX_PROP_TYPE_INT32,
//  XDMX_PROP_TYPE_INT64,
  XDMX_PROP_TYPE_FLOAT,
//  XDMX_PROP_TYPE_DOUBLE,
  XDMX_PROP_TYPE_STRING
};

struct XdmxPropertyValue
{
  XdmxPropertyType type;
  union
  {
    bool boolVal;
    char charVal;
    int32_t int32Val;
//    int64_t int64Val;
    float floatVal;
//    double doubleVal;
    char* stringVal;
  };
};

#define FOURCC(a,b,c,d) ((uint32_t)(((d)<<24) | ((c)<<16) | ((b)<<8) | (a)))

typedef uint32_t XdmxPropertyTag;

// TODO: Implement more-efficient custom hash map
#include <map>

class CXdmxPropertyBag
{
public:
  CXdmxPropertyBag();
  void SetValue(XdmxPropertyTag tag, XdmxPropertyValue& val);
  const XdmxPropertyValue& GetValue(XdmxPropertyTag tag);
  bool IsEmpty(const XdmxPropertyValue& val);
protected:
  static const XdmxPropertyValue m_EmptyValue;
  std::map<XdmxPropertyTag, XdmxPropertyValue> m_Values;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// Standard Property Tags

enum
{
  // Common
  XDMX_PROP_TAG_FOURCC      = FOURCC('f','r','c','c'),   // int32
  XDMX_PROP_TAG_STREAM_TYPE = FOURCC('s','t','y','p'),   // int32
  XDMX_PROP_TAG_DURATION    = FOURCC('d','u','r',' '),   // float
  XDMX_PROP_TAG_LANGUAGE    = FOURCC('l','a','n','g'),   // int32
  XDMX_PROP_TAG_BITRATE     = FOURCC('b','r','a','t'),   // float
  XDMX_PROP_TAG_VAR_BITRATE = FOURCC('v','b','r',' '),   // bool

  // Audio
  XDMX_PROP_TAG_CHANNELS    = FOURCC('c','h','a','n'),   // int32
  XDMX_PROP_TAG_SAMPLE_RATE = FOURCC('s','r','a','t'),   // int32
  XDMX_PROP_TAG_FRAME_SIZE  = FOURCC('f','m','s','z'),   // int32
  XDMX_PROP_TAG_BIT_DEPTH   = FOURCC('d','p','t','h'),   // int32

  // Video
  XDMX_PROP_TAG_FRAMES_PER_SEC  = FOURCC('f','p','s',' '), // float
  XDMX_PROP_TAG_HEIGHT          = FOURCC('h','g','h','t'), // int32
  XDMX_PROP_TAG_WIDTH           = FOURCC('w','d','t','h'), // int32
  XDMX_PROP_TAG_ASPECT_RATIO    = FOURCC('a','s','p','t'), // float
  XDMX_PROP_TAG_VAR_FPS         = FOURCC('v','f','p','s')  // bool
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class IXdmxInputStream
{
public:
  virtual unsigned int Read(unsigned char* buf, unsigned int buf_size) = 0;
  virtual int64_t Seek(int64_t offset, int whence) = 0;
  virtual int64_t GetLength() = 0;
  virtual int64_t GetPosition() = 0;
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
  void SetId(unsigned int id);
  unsigned int GetId();
  void SetElementType(unsigned char elementType);
  unsigned char GetElementType();
  void SetProperty(XdmxPropertyTag tag, XdmxPropertyValue& val);
  void SetProperty(XdmxPropertyTag tag, bool val);
  void SetProperty(XdmxPropertyTag tag, char val);
  void SetProperty(XdmxPropertyTag tag, int32_t val);
  void SetProperty(XdmxPropertyTag tag, float val);
  const XdmxPropertyValue& GetProperty(XdmxPropertyTag tag);
  bool IsValueEmpty(const XdmxPropertyValue& val);
protected:
  unsigned int m_Id;
  unsigned char m_ElementType;
  CXdmxPropertyBag m_Props;
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


