
#include "common.h"
#include "pesparse.h"

#define __PES_MODULE__ "Xdmx PES Parser"

CPESParser::CPESParser(CElementaryStream* pStream, PayloadList* pPayloadList) :
  m_pStream(pStream),
  m_HeaderLen(0),
  m_HeaderOffset(0),
  m_BytesIn(0),
  m_BytesOut(0),
  m_MaxPayloadLen(0),
  m_pPayloadList(pPayloadList),
  m_NeedProbe(true)
{

}

CPESParser::~CPESParser()
{

}

unsigned int CPESParser::Add(unsigned char* pData, unsigned int len, bool newPayloadUnit)
{
  unsigned int inLen = len;
  if (newPayloadUnit)
  {
    // If an unbounded payload is currently in-process, this signals its end
    if (m_Accum.IsUnbounded() && m_Accum.GetLen()) 
    {
      XDMX_LOG_DEBUG("%s: Completing unbounded payload. Len: %lu (0x%04x). Total Bytes in: %lu (0x%04x)", __PES_MODULE__, m_Accum.GetLen(), m_Accum.GetLen(), (unsigned int)m_BytesIn, (unsigned int)m_BytesIn);
      CompletePayload();
    }
    else if (m_Accum.GetPayloadLen())
    {
      XDMX_LOG_DEBUG("%s: Abandoning incomplete payload.", __PES_MODULE__);
    }

    // Peek at the packet header to get the size
    unsigned short packetLen = (((unsigned short)(pData[4])) << 8) | pData[5];
    // TODO: Not all streams use this syntax after the packet size
    m_HeaderLen = pData[8] + 9;
    if (m_HeaderLen > len)
      m_HeaderOffset = len;
    else
      m_HeaderOffset = m_HeaderLen;

    memcpy(m_Header, pData, m_HeaderOffset);
    len -= m_HeaderOffset;
    pData += m_HeaderOffset;

    if (!packetLen)
    {
      XDMX_LOG_DEBUG("%s: New Unbounded Payload.", __PES_MODULE__);
      m_Accum.StartPayloadUnbounded();
    }
    else
    {
      XDMX_LOG_DEBUG("%s: New Payload. Len: %lu (0x%04x).", __PES_MODULE__, packetLen - m_HeaderLen - 3, packetLen - m_HeaderLen - 3);
      m_Accum.StartPayload(packetLen - m_HeaderLen + 6); // Exclude Header
    }

    if (m_NeedProbe)
      ProbeFormat(pData, len);
  }
  else if (!m_HeaderLen) // This is the first packet added
    return inLen; // Wait for a new payload. Consume all bytes.

  if (m_HeaderOffset < m_HeaderLen) // Still need some header data
  {
    unsigned int headerBytes = m_HeaderLen - m_HeaderOffset;
    if (headerBytes > len)
      headerBytes = len;
    memcpy(&m_Header[0] + m_HeaderOffset, pData, len);
    m_HeaderOffset += headerBytes;
    len -= headerBytes;
    pData += headerBytes;
  }

  if (!len)
    return inLen;

  // Add the data to the payload accumulator
  m_BytesIn += len;
  //XDMX_LOG_DEBUG("%s: Adding chunk to payload. Packet: %lu, Len: %lu (0x%04x), Current Len: %lu (0x%04x), Total Bytes in: %lu (0x%04x)", __PES_MODULE__, current_packet, len, len, m_Accum.GetLen(), m_Accum.GetLen(), (unsigned int)m_BytesIn, (unsigned int)m_BytesIn);
  bool complete = m_Accum.AddData(pData, &len);

  // There should not be any data left. If there is, it likely signals a problem
  if (len)
    XDMX_LOG_ERROR("%s: %lu (0x%04x) Bytes left after add.", __PES_MODULE__, len, len);

  if (complete) // Access Unit is complete
  {
    XDMX_LOG_DEBUG("%s: Completing Payload. Len: %lu (0x%04x). Total Bytes in: %lu (0x%04x)", __PES_MODULE__, m_Accum.GetLen(), m_Accum.GetLen(), (unsigned int)m_BytesIn, (unsigned int)m_BytesIn);
    // Pass on to the derived class' handler method
    CompletePayload();
  }
  return inLen - len;
}

void CPESParser::Flush()
{
  m_Accum.Reset();
  m_HeaderLen = 0;
  m_HeaderOffset = 0;
}

void CPESParser::SetTimeStamps(CParserPayload* pPayload, uint64_t pts, uint64_t dts)
{
  // Scale timestamps to 90 KHz Clock
  pPayload->SetPts((double)pts/90000.0);
  if (dts == 0)
    dts = pts;
  pPayload->SetDts((double)dts/90000.0);
 
}

bool CPESParser::Parse(unsigned char* pHeader, unsigned int headerLen, unsigned char* pData, unsigned int dataLen)
{
  if (!m_pPayloadList)
    return false; // Nothing to do with it anyway

  // Parse the header
  unsigned char extStreamId = 0;
  uint64_t pts = 0;
  uint64_t dts = 0;

  unsigned int startCode = ((uint32_t)pHeader[0] << 16) | ((uint32_t)pHeader[1] << 8) | pHeader[2];
  if (startCode != 0x000001)
    return false; // Invalid packet

  unsigned char streamId = pHeader[3];
//  unsigned short packetLen = ((unsigned short)pHeader[4] << 8) | pHeader[5];

  unsigned char startBits = (pHeader[6] & 0xc0) >> 6;
  if (startBits != 0x2) // MPEG-2 start bits
    return false;
//  unsigned char scramblingCtl = (pHeader[6] & 0x30) >> 4;

  // PES Header Flags
//  bool pes_priority = (pHeader[6] & 0x08) == 0x08;
//  bool data_alignment = (pHeader[6] & 0x04) == 0x04;
//  bool copyright = (pHeader[6] & 0x02) == 0x02;
//  bool original = (pHeader[6] & 0x01) == 0x01;
  bool has_pts = (pHeader[7] & 0x80) == 0x80;
  bool has_dts = (pHeader[7] & 0x40) == 0x40;
  bool escr_flag = (pHeader[7] & 0x20) == 0x20;
  bool es_rate_flag = (pHeader[7] & 0x10) == 0x10;
  bool trick_mode_flag = (pHeader[7] & 0x08) == 0x08;
  bool additional_copy_info_flag = (pHeader[7] & 0x04) == 0x04;
  bool has_pes_crc = (pHeader[7] & 0x02) == 0x02;
  bool pes_ext_flag = (pHeader[7] & 0x01) == 0x01;

  // PES Header
//  unsigned char optLen = pHeader[8];
  pHeader += 9;
  headerLen -= 9;
  if (has_pts)
  {
    pts = (((uint64_t)pHeader[0] & 0xe) << 29) | ((uint32_t)pHeader[1] << 22) | (((uint32_t)pHeader[2] & 0xfe) << 14) | ((uint32_t)pHeader[3] << 7) | (((uint32_t)pHeader[4] & 0xfe) >> 1);
    pHeader += 5;
    headerLen -= 5;
  }

  if (has_dts)
  {
    dts = (((uint64_t)pHeader[0] & 0xe) << 29) | ((uint32_t)pHeader[1] << 22) | (((uint32_t)pHeader[2] & 0xfe) << 14) | ((uint32_t)pHeader[3] << 7) | (((uint32_t)pHeader[4] & 0xfe) >> 1);
    pHeader += 5;
    headerLen -= 5;
  }

  if (escr_flag)
  {
    // Reserved - 2 bits
    // ESCR - 36 bits (3 markers)
    // Extension - 9 bits
    // Marker - 1 bit
    pHeader += 6;
    headerLen -= 6;
  }

  if (es_rate_flag)
  {
    // Marker - 1 bit
    // ES Rate - 22 bits
    // Marker - 1 bit
    pHeader += 3;
    headerLen -= 3;
  }

  if (trick_mode_flag)
  {
    // trick_mode - 8 bits
    pHeader += 1;
    headerLen -= 1;
  }

  if (additional_copy_info_flag)
  {
    // Additional Copy Info - 7 bits
    pHeader += 1;
    headerLen -= 1;
  }

  if (has_pes_crc)
  {
    // Previous Packet CRC - 16 bits
    pHeader += 2;
    headerLen -= 2;
  }

  // PES Extended Header
  if (pes_ext_flag)
  {
    // Extended Flags
    bool private_data_flag = (pHeader[0] & 0x80) == 0x80;
    bool pack_header_flag = (pHeader[0] & 0x40) == 0x40;
    bool seq_counter_flag = (pHeader[0] & 0x20) == 0x20;
    bool pstd_buffer_flag = (pHeader[0] & 0x10) == 0x10;
    bool pes_ext_flag_2 = (pHeader[0] & 0x01) == 0x01;
    pHeader += 1;
    headerLen -= 1;

    if (private_data_flag)
    {
      // PES Private Data
      pHeader += 16;
      headerLen -= 16;
    }

    if (pack_header_flag)
    {
      // Pack Field Length - 8 bits
      pHeader += 1;
      headerLen -= 1;
    }

    if (seq_counter_flag)
    {
      // Marker - 1 bit
      // Program Packet Sequence Counter - 7 bits
      // Marker - 1 bit
      // MPEG Version - 1 bit
      // Original Stuffing Length - 6 bits
      pHeader += 2;
      headerLen -= 2;
    } 

    if (pstd_buffer_flag)
    {
      // Sync word - 2 bits
      // P-STD Buffer Scale - 1 bit
      // P-STD Buffer Size - 13 bits
      pHeader += 2;
      headerLen -= 2;
    }

    if (pes_ext_flag_2)
    {
      unsigned char extensionLen = pHeader[0] & 0x7f; // Extension Field Length
      bool id_ext_flag = ((pHeader[0] & 0x60) != 0x60);
      if (streamId == PES_ID_EXTENDED && id_ext_flag) // Extended Stream Id
      {
        // The extended id serves many purposes. It's meaning depends on the stream type
        // For True-HD and DTS-HD streams, it indicates the sub-stream
        extStreamId = pHeader[1] & 0x7f;
      }
      pHeader += (extensionLen + 1);
      headerLen -= (extensionLen + 1);
    }
  }

  if (m_pStream->GetElementType() == 0x83 ||
      m_pStream->GetElementType() == 0x85 ||
      m_pStream->GetElementType() == 0x86)
  {
    // Only pass-on those packets belonging to the active sub-stream
    // TODO: How do we decide which substream to keep
    // TODO: This is a hack and needs to be put in the correct place
    // Really should expose both the HD stream and the core separately
    // For now, always strip out AC3/DTS core and provide that to caller
    if (extStreamId == 0x72)
      return false;
  }

  CParserPayload* pPayload = new CParserPayload(m_pStream, pData, dataLen);

  SetTimeStamps(pPayload, pts, dts);

  m_BytesOut += pPayload->GetSize();
  if (pPayload->GetSize() > m_MaxPayloadLen)
    m_MaxPayloadLen = pPayload->GetSize();

  m_pPayloadList->push_back(pPayload);
  
  return true;
}

bool CPESParser::CompletePayload()
{
  bool consumed = Parse(m_Header, m_HeaderOffset, m_Accum.GetData(), m_Accum.GetLen());
  m_Accum.Detach(!consumed); // If data was not consumed, it must be freed
  return consumed;
}

////////////////////////////////////////////////////////////////////////////////////////

CPESParserStandard::CPESParserStandard(CElementaryStream* pStream, PayloadList* pPayloadList) :
  CPESParser(pStream, pPayloadList)
{

}

bool CPESParserStandard::ProbeFormat(unsigned char* pData, unsigned int len)
{
  m_NeedProbe = false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////

CPESParserLPCM::CPESParserLPCM(CElementaryStream* pStream, PayloadList* pPayloadList) :
  CPESParser(pStream, pPayloadList),
  m_Channels(0),
  m_BitDepth(0),
  m_SampleRate(0)
{

}

unsigned int CPESParserLPCM::Add(unsigned char* pData, unsigned int len, bool newPayloadUnit)
{
  if (newPayloadUnit)
  {
    unsigned short packetLen = (((unsigned short)(pData[4])) << 8) | pData[5];
    m_HeaderLen = (unsigned short)pData[8] + 9;
    memcpy(m_Header, pData, m_HeaderLen);
    m_HeaderOffset = m_HeaderLen;
    pData += m_HeaderLen;
    len -= m_HeaderLen;

    if (!m_Channels) // Pull stream format from packet header
      ProbeFormat(pData, len);
    // Skip LPCM header
    pData += 4;
    len -= 4;
    m_Accum.StartPayload(packetLen - m_HeaderLen + 6 - 4); // Exclude Headers
  }
  return CPESParser::Add(pData, len, false);
}

bool CPESParserLPCM::Parse(unsigned char* pHeader, unsigned int headerLen, unsigned char* pData, unsigned int dataLen)
{
  return CPESParser::Parse(pHeader, headerLen, pData, dataLen);
}


bool CPESParserLPCM::ProbeFormat(unsigned char* pData, unsigned int len)
{
  // LPCM Block Header
  // 8 : Frame Size
  // 8 : Flags
  //   4 : Channel Mode
  //   4 : Sample Rate
  //   2 : Bit Depth
  //   6 : Unknown/Reserved
  //
  // ab cd ef gh

  // abcd - Frame Size
  m_BlockSize = ((int32_t)pData[0] << 8) | pData[1];

  // e - Audio Mode
  switch(pData[2] >> 4)
  {
	case 1: // 1/0
		m_Channels = 1;
		break;
	case 3: // 2/0
		m_Channels = 2;
		break;
	case 4: // 3/0
		m_Channels = 3;
		break;
	case 5: // 2/1
		m_Channels = 3;
		break;
	case 6: // 3/1
		m_Channels = 4;
		break;
	case 7: // 2/2 (Dual Stereo)
		m_Channels = 4;
		break;
	case 8: // 3/2
		m_Channels = 5;
		break;
	case 9: // 3/2+LFE
		m_Channels = 6;
		break;
	case 10: // 3/4
		m_Channels = 7;
		break;
	case 11: // 3/4+LFE
		m_Channels = 8;
		break;
	default: // Reserved
		m_Channels = 0;
		return false;
	}

  // f - Sample Rate
  switch(pData[2] & 0x0f)
  {
	case 1:
    m_SampleRate = 48000;
		break;
	case 4:
    m_SampleRate = 96000;
		break;
	case 5:
    m_SampleRate = 192000;
		break;
	default:
    m_SampleRate = 0;
		return false;
  }
  // g - Bit Depth and Reserved 
  switch(pData[3] & 0xc0)
  {
  case 0x40:
    m_BitDepth = 16;
		break;
  case 0x80:
    m_BitDepth = 24;
		break;
  case 0xc0:
    m_BitDepth = 32;
		break;
  default:
    m_BitDepth = 0;
		return false;
  }
  // h - Unknown

  // Set the stream properties to reflect the detected format
  m_pStream->SetProperty(XDMX_PROP_TAG_CHANNELS, m_Channels);
  m_pStream->SetProperty(XDMX_PROP_TAG_BIT_DEPTH, m_BitDepth);
  m_pStream->SetProperty(XDMX_PROP_TAG_SAMPLE_RATE, m_SampleRate);
  m_pStream->SetProperty(XDMX_PROP_TAG_FRAME_SIZE, (m_BitDepth >> 3) * m_Channels);
  m_pStream->SetProperty(XDMX_PROP_TAG_BITRATE, (m_BitDepth >> 3) * m_Channels * m_SampleRate);

  return true;
}
