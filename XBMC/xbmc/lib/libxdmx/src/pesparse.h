#ifndef PESPARSE_H_
#define PESPARSE_H_

#include "accum.h"
#include <deque>

// PES Stream Id (ISO/IEC 13818-1, Table 2-18)
enum
{
  // Program Stream Only
  PES_ID_END_CODE                = 0xb9,
  PES_ID_PACK_START              = 0xba,
  PES_ID_SYSTEM_HEADER_START     = 0xbb,

  PES_ID_PROGRAM_STREAM_MAP      = 0xbc, // Syntax in ISO/IEC 13818-1, Section 2.5.4.1
  PES_ID_PRIVATE_STREAM_1        = 0xbd, // Standard Syntax
  PES_ID_PADDING_STREAM          = 0xbe,
  PES_ID_PRIVATE_STREAM_2        = 0xbf, // Raw data after PES_packet_length
  PES_ID_AUDIO_STREAM_MPEG124_3  = 0xc0, // Standard Syntax
  PES_ID_VIDEO_STREAM_MPEG124_2  = 0xe0, // Standard Syntax
  PES_ID_ECM_STREAM              = 0xf0, // Raw data after PES_packet_length
  PES_ID_EMM_STREAM              = 0xf1, // Raw data after PES_packet_length
  PES_ID_DSMCC_STREAM            = 0xf2, // Syntax in ISO/IEC 13818-6
  PES_ID_MHEG                    = 0xf3, // Standard Syntax
  PES_ID_STREAM_H222_1_TYPE_A    = 0xf4,
  PES_ID_STREAM_H222_1_TYPE_B    = 0xf5,
  PES_ID_STREAM_H222_1_TYPE_C    = 0xf6,
  PES_ID_STREAM_H222_1_TYPE_D    = 0xf7,
  PES_ID_STREAM_H222_1_TYPE_E    = 0xf8,
  PES_ID_ANCILLARY_STREAM        = 0xf9, // Syntax in ISO/IEC 13818-1, Section 2.4.3.7
  PES_ID_SL_PACKETIZED_STREAM    = 0xfa,
  PES_ID_FLEX_MUX_STREAM         = 0xfb,
  PES_ID_RESERVED_STREAM_1       = 0xfc,
  PES_ID_EXTENDED                = 0xfd,
  PES_ID_RESERVED_STREAM_3       = 0xfe,
  PES_ID_PS_DIRECTORY            = 0xff  // Syntax in ISO/IEC 13818-1, Section 2.5.5
};

// PES Packet Syntax
enum
{
  PES_SYNTAX_UNKNOWN,
  PES_SYNTAX_STANDARD,
  PES_SYNTAX_EXTENDED,
  PES_SYNTAX_RAW,
  PES_SYNTAX_SL,
  PES_SYNTAX_PROGRAM_MAP,
  PES_SYNTAX_DSMCC,
  PES_SYNTAX_ANCILLARY,
  PES_SYNTAX_PS_DIRECTORY
};

// Extended PES Stream Id (Stream Id == 0xfd) - Defined in ISO/IEC 13818-1 Amendment 2
// 0x00 - IPMP Control Stream
// 0x01 to 0x3f - reserved_data_stream
// 0x40 to 0x7f - private_stream
enum
{
  PES_ID_EXT_DIRAC_0             = 0x60,
  PES_ID_EXT_DIRAC_15            = 0x6f,
  PES_ID_EXT_AC3                 = 0x71,
  PES_ID_EXT_HDMV_DTS_HD_TRUE_HD = 0x72,
  PES_ID_EXT_HDMV_AC3_CORE       = 0x76
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class IPacketFilter
{
public:
  virtual ~IPacketFilter() {}
  virtual unsigned int Add(unsigned char* pData, unsigned int len, bool newPayloadUnit) = 0;
  virtual void Flush() = 0;
protected:
  IPacketFilter() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////
typedef std::deque<CParserPayload*> PayloadList;
typedef std::deque<CParserPayload*>::iterator PayloadIterator;

class CPESParser : public IPacketFilter
{
public:
  CPESParser(CElementaryStream* pStream, PayloadList* pPayloadList);
  virtual ~CPESParser();
  virtual unsigned int Add(unsigned char* pData, unsigned int len, bool newPayloadUnit);
  virtual void Flush();
  virtual bool ProbeFormat(unsigned char* pData, unsigned int len) = 0;
protected:
  virtual bool Parse(unsigned char* pHeader, unsigned int headerLen, unsigned char* pData, unsigned int dataLen);
  bool CompletePayload();
  void SetTimeStamps(CParserPayload* pPayload, uint64_t pts, uint64_t dts);

  CElementaryStream* m_pStream;
  CPayloadAccumulator m_Accum;
  unsigned char m_Header[264];
  unsigned int m_HeaderLen;
  unsigned int m_HeaderOffset;

  uint64_t m_BytesIn;
  uint64_t m_BytesOut;
  unsigned int m_MaxPayloadLen;
  PayloadList* m_pPayloadList;

  bool m_NeedProbe;
};

class CPESParserStandard : public CPESParser
{
public:
  virtual ~CPESParserStandard() {}
  CPESParserStandard(CElementaryStream* pStream, PayloadList* pPayloadList);
protected:
  virtual bool ProbeFormat(unsigned char* pData, unsigned int len);
};

class CPESParserLPCM : public CPESParser
{
public:
  virtual ~CPESParserLPCM() {}
  CPESParserLPCM(CElementaryStream* pStream, PayloadList* pPayloadList);
  virtual unsigned int Add(unsigned char* pData, unsigned int len, bool newPayloadUnit);
protected:
  virtual bool Parse(unsigned char* pHeader, unsigned int headerLen, unsigned char* pData, unsigned int dataLen);
  virtual bool ProbeFormat(unsigned char* pData, unsigned int len);
  int32_t m_Channels;
  int32_t m_BitDepth;
  int32_t m_SampleRate;
  int32_t m_BlockSize;
};

#endif // PESPARSE_H_
