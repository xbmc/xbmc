#ifndef TSDEMUX_H_
#define TSDEMUX_H_

#include "xdmx.h"
#include <string>
#include <map>
#include <vector>

#define TS_SYNC_WORD 0x47
#define TS_MAX_PACKET_LEN 208

// Transport Stream Types
enum TSTransportType
{
  TS_TYPE_UNKNOWN,
  TS_TYPE_STD,
  TS_TYPE_M2TS, // also known as HDMV 
  TS_TYPE_DVB,
  TS_TYPE_ATSC,
  TS_TYPE_HDMV = TS_TYPE_M2TS
};

// Transport Stream Element Types
enum TSElementType
{
  TS_ELEMENT_TYPE_UNKNOWN,
  TS_ELEMENT_TYPE_PSI,
  TS_ELEMENT_TYPE_PES,
  TS_ELEMENT_TYPE_PRIVATE
};

// Program Element Types (ISO/IEC 13818-1, Table 2-29)
// TODO: Move to mpeg2 header when other systems are added
enum
{
  // Standard Values
  ES_STREAM_TYPE_RESERVED         = 0x00,
  ES_STREAM_TYPE_VIDEO_MPEG1      = 0x01, // MPEG-1 Part 2 Video
  ES_STREAM_TYPE_VIDEO_MPEG2_2    = 0x02, // MPEG-2 Part 2 Video
  ES_STREAM_TYPE_AUDIO_MPEG1      = 0x03, // MPEG-1 Part 3 Audio
  ES_STREAM_TYPE_AUDIO_MPEG2_3    = 0x04, // MPEG-2 Part 3 Audio
  ES_STREAM_TYPE_PRIVATE_SECTION  = 0x05,
  ES_STREAM_TYPE_PRIVATE_DATA     = 0x06, 
  ES_STREAM_TYPE_MHEG             = 0x07,
  ES_STREAM_TYPE_DSM_CC           = 0x08,
  ES_STREAM_TYPE_H222_1           = 0x09,
  ES_STREAM_TYPE_VIDEO_MPEG2_A    = 0x0a,
  ES_STREAM_TYPE_VIDEO_MPEG2_B    = 0x0b,
  ES_STREAM_TYPE_VIDEO_MPEG2_C    = 0x0c,
  ES_STREAM_TYPE_VIDEO_MPEG2_D    = 0x0d,
  ES_STREAM_TYPE_MPEG2_AUX        = 0x0e,
  ES_STREAM_TYPE_AUDIO_MPEG2_AAC  = 0x0f, // MPEG-2 Part 7 Audio (ADTS Syntax)
  ES_STREAM_TYPE_VIDEO_MPEG4      = 0x10, // MPEG-4 Part 2 Visual
  ES_STREAM_TYPE_AUDIO_MPEG4_AAC  = 0x11, // MPEG-4 Part 3 Audio (LATM Syntax)
  ES_STREAM_TYPE_SL_PES           = 0x12,
  ES_STREAM_TYPE_SL_SECTION       = 0x13,
  ES_STREAM_TYPE_DOWNLOAD         = 0x14,

  // Reserved Values (0x15 - 0x7f)
  ES_STREAM_TYPE_VIDEO_H264       = 0x1b, // MPEG-4 Part 10 AVC
  
  // User Private Values (0x80 - 0xff)
  ES_STREAM_TYPE_USER_PRIVATE             = 0x80, // LPCM if Stream Registration Descriptor Format == "HDMV"
  ES_STREAM_TYPE_AUDIO_AC3                = 0x81,
  ES_STREAM_TYPE_AUDIO_HDMV_DTS           = 0x82,
  ES_STREAM_TYPE_AUDIO_HDMV_AC3_TRUE_HD   = 0x83, // True-HD with AC3 core (BluRay Only)
  ES_STREAM_TYPE_AUDIO_HDMV_AC3_PLUS      = 0x84,
  ES_STREAM_TYPE_AUDIO_HDMV_DTS_HD        = 0x85,
  ES_STREAM_TYPE_AUDIO_HDMV_DTS_HD_MASTER = 0x86,
  ES_STREAM_TYPE_AUDIO_EAC3               = 0x87,
  ES_STREAM_TYPE_AUDIO_DTS                = 0x8a,
  ES_STREAM_TYPE_AUDIO_HDMV_PGS           = 0x90, // Blu-Ray PGS Subpicture
  ES_STREAM_TYPE_AUDIO_EAC3_SECONDARY     = 0xa1,
  ES_STREAM_TYPE_AUDIO_DTS_SECONDARY      = 0xa2,
  ES_STREAM_TYPE_VIDEO_DIRAC              = 0xd1,
  ES_STREAM_TYPE_VIDEO_VC1                = 0xea,
};

// Private Stream Types
enum
{
  ES_PRIVATE_TYPE_AC3,
  ES_PRIVATE_TYPE_LPCM
};

enum
{
  TS_DESC_VIDEO_STREAM                = 0x02,
  TS_DESC_AUDIO_STREAM                = 0x03,
  TS_DESC_HIERARCHY                   = 0x04,
  TS_DESC_REGISTRATION                = 0x05,
  TS_DESC_LANGUAGE                    = 0x0A,
  TS_DESC_DVB_AC3                     = 0x6A,
  TS_DESC_DVB_EAC3                    = 0x7A,
  TS_DESC_DVB_DTS                     = 0x7B,
  TS_DESC_DVB_AAC                     = 0x7C,
  TS_DESC_AC3                         = 0x81,
  TS_DESC_ATSC_CAPT_SVC               = 0x86
};


/////////////////////////////////////////////////////////////////////////////////////////////////

class CTSDescriptor
{
public:
  CTSDescriptor(unsigned char tag, const char* pName = "");
  virtual ~CTSDescriptor();
  unsigned char GetTag();
  virtual const char* GetName();
  virtual void SetData(unsigned char* pData, unsigned int len);
  const unsigned char* GetData();
  unsigned int GetDataLen();
protected:
  unsigned char m_Tag;
  std::string m_Name;
  unsigned char* m_pRawData;
  unsigned int m_RawDataLen;
};

typedef std::map<unsigned char, CTSDescriptor*> TSDescriptorList;
typedef std::map<unsigned char, CTSDescriptor*>::iterator TSDescriptorIterator;

/////////////////////////////////////////////////////////////////////////////////////////////////

class CProgramClock
{
public:
  CProgramClock();
  void AddReference(uint64_t position, uint64_t base, unsigned int extension); 
  uint64_t GetFirstReference();
  double GetElapsedTime();
  bool IsValid();
protected:
  uint64_t m_LastReferencePos; // i''
  uint64_t m_LastReference; // PCR(i'')
  uint64_t m_DeltaPos; // i' - i''
  uint64_t m_Delta; // PCR(i') - PCR(i'')
  uint64_t m_LastBitrate; // Technically valid for any i where i'' - deltaPos < i <= i''
  uint64_t m_FirstReference;
  bool m_Valid;
  static const uint64_t m_SysClockFreq;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class CTSProgram
{
public:
  CTSProgram();
  virtual ~CTSProgram();
  void AddStream(CElementaryStream* pStream);
  unsigned int GetStreamCount();
  CElementaryStream* GetStream(unsigned int index);
  CProgramClock* GetClock();
protected:
  std::vector<CElementaryStream*> m_StreamList;
  CProgramClock m_Clock;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class CTransportStream
{
public:
  CTransportStream();
  virtual ~CTransportStream();
  void AddProgram(CTSProgram* pProgram);
  unsigned int GetProgramCount();
  CTSProgram* GetProgram(unsigned int index);
protected:
  std::vector<CTSProgram*> m_ProgramList;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class ITransportStreamDemux
{
public:
  virtual ~ITransportStreamDemux() {}
  virtual bool Open(IXdmxInputStream* pInput) = 0;
  virtual void Close() = 0;
  // Retrieve the next Elementary Stream Payload
  virtual CParserPayload* GetPayload() = 0;
  // Program Stream Information
  virtual CTransportStream* GetTransportStream() = 0;
  virtual CElementaryStream* GetStreamById(unsigned short id) = 0;
  virtual double GetTotalTime() = 0;
  virtual double SeekTime(double time) = 0;

protected:
  ITransportStreamDemux() {}
};

extern ITransportStreamDemux* CreateTSDemux(TSTransportType type = TS_TYPE_UNKNOWN);

#endif /*TSDEMUX_H_*/
