
#include "common.h"
#include "tsdemux.h"
#include "pesparse.h"
#include "bitstream.h"
#include "accum.h"

#include <list>

#define __TS_MODULE__ "Xdmx Transport Stream Demux"

#define TS_MAX_PIDS 0x2000
/////////////////////////////////////////////////////////////////////////////////////////////////

class CTSFilter : public IPacketFilter
{
public:
  CTSFilter();
  virtual ~CTSFilter();
  bool CheckContinuity(unsigned char counterValue);
protected:
  unsigned char m_ContinuityCounter;
};

typedef std::map<unsigned short, CTSFilter*> TSFilterList;
typedef std::map<unsigned short, CTSFilter*>::iterator TSFilterIterator;

CTSFilter::CTSFilter() :
  m_ContinuityCounter(0)
{
  
}

CTSFilter::~CTSFilter()
{

}

bool CTSFilter::CheckContinuity(unsigned char counterValue)
{
  m_ContinuityCounter = counterValue;
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

class CTableFilter : public CTSFilter
{
public:
  virtual bool Add(unsigned char* pData, unsigned int len, bool newPayloadUnit);
  virtual void Flush();
protected:
  virtual bool ParseSection(unsigned char* pData, unsigned int len) = 0;
  CPayloadAccumulator m_Accum;
};

bool CTableFilter::Add(unsigned char* pData, unsigned int len, bool newPayloadUnit)
{
  if (newPayloadUnit)
  {
    // TODO: Handle packet data before new payload
    unsigned char pointer = pData[0];
    if (pointer)
    {
      pData += (pointer + 1);
      len -= (pointer + 1);
    }
    else
    {
      pData++;
      len--;
    }

    // Peek at the section header to get the size
    unsigned short sectionLen = ((pData[1] & 0x0F) << 8) | pData[2] + 3; // Include header in total size
    m_Accum.StartPayload(sectionLen);
    if (sectionLen < len)
      len = sectionLen;
  }

  // Add the data to the payload accumulator
  bool complete = m_Accum.AddData(pData, &len);

  if (complete) // Payload Unit is complete
  {
    // Pass on to the derived class' handler method
    ParseSection(m_Accum.GetData(), m_Accum.GetLen());
  }
  return true;
}

void CTableFilter::Flush()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////

class CPCRFilter : public CTSFilter
{
public:
  virtual bool Add(unsigned char* pData, unsigned int len, bool newPayloadUnit);
  virtual void Flush();
protected:
};

bool CPCRFilter::Add(unsigned char* pData, unsigned int len, bool newPayloadUnit)
{
  return false;
}

void CPCRFilter::Flush()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////

class CTSFilterRegistry
{
public:
  CTSFilterRegistry();
  virtual ~CTSFilterRegistry();
  bool RegisterFilter(unsigned short pid, CTSFilter* pFilter, bool replace = false);
  void UnregisterFilter(unsigned short pid, bool dispose = false);
  void UnregisterAll(bool dispose = false);
  CTSFilter* GetFilter(unsigned short pid);
  bool RegisterClock(unsigned short pid, CProgramClock* pClock); // TODO: Find a better home for the clocks
  void UnregisterClock(unsigned short pid);
  CProgramClock* GetClock(unsigned short pid);

  void FlushStreamFilters(); // TODO: This needs to go...
protected:
  CTSFilter* m_Filters[TS_MAX_PIDS]; // Use instead of map to improve lookup speed
  std::map<unsigned short, CProgramClock*> m_Clocks;
};

CTSFilterRegistry::CTSFilterRegistry()
{
  memset(m_Filters, 0, sizeof(CTSFilter*) * TS_MAX_PIDS);
}

CTSFilterRegistry::~CTSFilterRegistry()
{
  UnregisterAll(true); // Clean-Up any leftover items
}

bool CTSFilterRegistry::RegisterFilter(unsigned short pid, CTSFilter* pFilter, bool replace/* = false*/)
{
  if (m_Filters[pid])
    if (replace)
      delete m_Filters[pid];
    else
      return false;
  m_Filters[pid] = pFilter;
  return true;
}

void CTSFilterRegistry::UnregisterFilter(unsigned short pid, bool dispose/* = false*/)
{
  if (dispose)
  {
    if (m_Filters[pid])
      delete m_Filters[pid];
  }
  m_Filters[pid] = NULL;
}

void CTSFilterRegistry::UnregisterAll(bool dispose /*= false*/)
{
  // Clean-up filters
  if (dispose)
  {
    for (int f = 0; f < TS_MAX_PIDS; f++)
    {
      if (m_Filters[f])
        delete m_Filters[f];
    }
  }
  memset(m_Filters, 0, sizeof(CTSFilter*) * TS_MAX_PIDS);

  // Clean-up clocks (All are statically defined in Program objects)
  m_Clocks.clear();
}

CTSFilter* CTSFilterRegistry::GetFilter(unsigned short pid)
{
  if (pid & 0xE000) // > TS_MAX_PIDS
    return NULL;

  return m_Filters[pid];
}

bool CTSFilterRegistry::RegisterClock(unsigned short pid, CProgramClock* pClock)
{
  m_Clocks[pid] = pClock;
  return true;
}

void CTSFilterRegistry::UnregisterClock(unsigned short pid)
{
  m_Clocks.erase(pid);
}

CProgramClock* CTSFilterRegistry::GetClock(unsigned short pid)
{
  std::map<unsigned short, CProgramClock*>::iterator iter = m_Clocks.find(pid);
  if (iter != m_Clocks.end())
    return iter->second;
  return NULL;
}

void CTSFilterRegistry::FlushStreamFilters()
{
  // Currently, Flush does nothing for table or PCR filters
  for (int f = 0; f < TS_MAX_PIDS; f++)
  {
    if (m_Filters[f])
    {
      m_Filters[f]->Flush();
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

CTSDescriptor::CTSDescriptor(unsigned char tag, const char* pName) : 
  m_Tag(tag),
  m_Name(pName),
  m_pRawData(NULL),
  m_RawDataLen(0)
{

}

CTSDescriptor::~CTSDescriptor()
{
  delete[] m_pRawData;
}

unsigned char CTSDescriptor::GetTag()
{
  return m_Tag;
}

const char* CTSDescriptor::GetName()
{
  return m_Name.c_str();
}

void CTSDescriptor::SetData(unsigned char* pData, unsigned int len)
{
  if (!pData)
    return;
  if (m_pRawData)
    delete[] m_pRawData;
  m_pRawData = new unsigned char[len];
  m_RawDataLen = len;
  memcpy(m_pRawData, pData, len);
}

const unsigned char* CTSDescriptor::GetData()
{
  return m_pRawData;
}

unsigned int CTSDescriptor::GetDataLen()
{
  return m_RawDataLen;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

const uint64_t CProgramClock::m_SysClockFreq = 27000000LL;

CProgramClock::CProgramClock() :
  m_LastReferencePos(0),
  m_LastReference(0),
  m_DeltaPos(0),
  m_Delta(0),
  m_LastBitrate(0),
  m_FirstReference(0),
  m_Valid(false)
{
  
}

void CProgramClock::AddReference(uint64_t position, uint64_t base, unsigned int extension)
{
  if (m_Valid)
  {
    uint64_t deltaPos = position - m_LastReferencePos;
    uint64_t delta = (base * 300 + extension) - m_LastReference;
    uint64_t rate = 8 * (deltaPos * m_SysClockFreq) / delta;
    m_DeltaPos = deltaPos;
    m_Delta = delta;
    m_LastBitrate = rate;
    m_LastReference += delta;
    m_LastReferencePos += deltaPos;
  }
  else
  {
    m_DeltaPos = 0;
    m_Delta = 0;
    m_LastBitrate = 0;
    m_Valid = true;
    m_LastReferencePos = position;
    m_FirstReference = m_LastReference = base * 300 + extension;
  }
}

uint64_t CProgramClock::GetFirstReference()
{
  return m_FirstReference;
}

double CProgramClock::GetElapsedTime()
{
  return (double)(m_LastReference - m_FirstReference) / (double)m_SysClockFreq;
}

bool CProgramClock::IsValid()
{
  return m_Valid;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

CTSProgram::CTSProgram()
{

}

CTSProgram::~CTSProgram()
{
  // Clean-up contained streams
  while (m_StreamList.size())
  {
    delete m_StreamList.back();
    m_StreamList.pop_back();
  }
}

void CTSProgram::AddStream(CElementaryStream* pStream)
{
  m_StreamList.push_back(pStream);
}

unsigned int CTSProgram::GetStreamCount()
{
  return m_StreamList.size();
}

CElementaryStream* CTSProgram::GetStream(unsigned int index)
{
  return m_StreamList[index];
}

CProgramClock* CTSProgram::GetClock()
{
  return &m_Clock;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
CTransportStream::CTransportStream()
{

}

CTransportStream::~CTransportStream()
{
  // Clean-up contained programs
  while (m_ProgramList.size())
  {
    delete m_ProgramList.back();
    m_ProgramList.pop_back();
  }
}

void CTransportStream::AddProgram(CTSProgram* pProgram)
{
  m_ProgramList.push_back(pProgram);
}

unsigned int CTransportStream::GetProgramCount()
{
  return m_ProgramList.size();
}

CTSProgram* CTransportStream::GetProgram(unsigned int index)
{
  return m_ProgramList[index];
}

/////////////////////////////////////////////////////////////////////////////////////////////////

class CElementaryStreamFilter : public CTSFilter
{
public:
  CElementaryStreamFilter(CElementaryStream* pStream, PayloadList* pPayloadList);
  CElementaryStreamFilter(CElementaryStream* pStream, PayloadList* pPayloadList, IPacketFilter* pInnerFilter);
  virtual ~CElementaryStreamFilter();
  virtual bool Add(unsigned char* pData, unsigned int len, bool newPayloadUnit);
  virtual void Flush();
  CElementaryStream* GetStream();
  uint64_t GetBytesIn();
protected:
  CElementaryStream* m_pStream;
  IPacketFilter* m_pParser;
  uint64_t m_BytesIn;
};

CElementaryStreamFilter::CElementaryStreamFilter(CElementaryStream* pStream, PayloadList* pPayloadList) :
  m_pStream(pStream),
  m_BytesIn(0)
{
  m_pParser = new CPESParserStandard(pStream, pPayloadList);
}

CElementaryStreamFilter::CElementaryStreamFilter(CElementaryStream* pStream, PayloadList* pPayloadList, IPacketFilter* pInnerFilter) :
  m_pStream(pStream),
  m_pParser(pInnerFilter),
  m_BytesIn(0)
{
  if (!pInnerFilter)
    m_pParser = new CPESParserStandard(pStream, pPayloadList);
}

CElementaryStreamFilter::~CElementaryStreamFilter()
{
  delete m_pParser;
}

bool CElementaryStreamFilter::Add(unsigned char* pData, unsigned int len, bool newPayloadUnit)
{
  m_BytesIn += len;
  return m_pParser->Add(pData, len, newPayloadUnit);
}

void CElementaryStreamFilter::Flush()
{
  m_pParser->Flush();
}

CElementaryStream* CElementaryStreamFilter::GetStream()
{
  return m_pStream;
}

uint64_t CElementaryStreamFilter::GetBytesIn()
{
  return m_BytesIn;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

class CTSProgramMapFilter : public CTableFilter
{
public:
  CTSProgramMapFilter(CTSProgram* pProgram, CTSFilterRegistry* pRegistry, PayloadList* pPayloadList);
protected:
  virtual bool ParseSection(unsigned char* pData, unsigned int len);
  unsigned int ParseProgramDescriptor(CSimpleBitstreamReader& reader);
  void ParseStream(CSimpleBitstreamReader& reader);
  unsigned int ParseStreamDescriptor(CSimpleBitstreamReader& reader, CTSDescriptor** ppDesc);
  CTSProgram* m_pProgram;
  unsigned short m_Version;
  CTSFilterRegistry* m_pRegistry;
  PayloadList* m_pPayloadList;
};

CTSProgramMapFilter::CTSProgramMapFilter(CTSProgram* pProgram, CTSFilterRegistry* pRegistry, PayloadList* pPayloadList) :
  m_pProgram(pProgram),
  m_Version(0x100),
  m_pRegistry(pRegistry),
  m_pPayloadList(pPayloadList)
{

}

bool CTSProgramMapFilter::ParseSection(unsigned char* pData, unsigned int len)
{
  CSimpleBitstreamReader reader(pData, len);

  unsigned char tableId = reader.ReadChar(8); // Table Id (0x00)
  reader.ReadBit(); // Syntax (1)
  reader.UpdatePosition(3); // Reserved
  reader.ReadShort(12); // Section Length ( == len)
  if (tableId != 0x02) // private_section
  {
    // TODO: Handle Private Sections 
    // Table Id's
    // ----------------------
    // 0x7f - Selection Information Table (SIT) - Goes with PID 0x1f
    // 
    //XDMX_LOG_DEBUG("%s: Program Map is skipping Private Section (tid: 0x%02x, len: %lu)", __TS_MODULE__, tableId, len);
    return true;
  }
  unsigned short program = reader.ReadShort(16);
  reader.UpdatePosition(2); // Reserved
  unsigned char version = reader.ReadChar(5);
  if (version == m_Version) // (m_Version != 0x100) // Some muxers increment the version of the PMT each time, even if there are no changes
    return true; // This version of the table has already been parsed
  bool current = reader.ReadBit(); // Current/Next Indicator
  if (!current)
    return true; // TODO: Decide how to handle caching the 'next' version
  unsigned char sectionId = reader.ReadChar(8); // Section Id
  unsigned char lastSectionId = reader.ReadChar(8); // Last Section Id
  reader.UpdatePosition(3); // Reserved
  unsigned short pcrPid = reader.ReadShort(13);
  if (pcrPid != 0x1FFF)
    m_pRegistry->RegisterClock(pcrPid, m_pProgram->GetClock()); // Register this program's clock reference stream
  reader.UpdatePosition(4); // Reserved
  XDMX_LOG_INFO("%s: Parsing Program Mapping Table (PMT) for Program %lu (0x%04x).", __TS_MODULE__, program, program);
  
  // Parse Program Descriptors
  unsigned short infoLen = reader.ReadShort(12);
  while (infoLen)
    infoLen -= ParseProgramDescriptor(reader);

  // Parse Streams
  // TODO: Dispose of any existing streams
  while (reader.GetBytesLeft() > 4)
    ParseStream(reader);

  reader.ReadInt32(32); // CRC

  if (sectionId == lastSectionId) // Last section of this table
    m_Version = version; // Store the version to prevent unnecessary re-parsing

  return true;
}

unsigned int CTSProgramMapFilter::ParseProgramDescriptor(CSimpleBitstreamReader& reader)
{
  unsigned char tag = reader.ReadChar(8);
  unsigned char descLen = reader.ReadChar(8);
  char formatId[4];
  switch (tag)
  {
  case 5: // Registration Descriptor
    formatId[0] = reader.ReadChar(8);
    formatId[1] = reader.ReadChar(8);
    formatId[2] = reader.ReadChar(8);
    formatId[3] = reader.ReadChar(8);
    XDMX_LOG_INFO("%s:   Found Program Registration Descriptor. Format: %4.4s, InfoLen: %d.", __TS_MODULE__, formatId, descLen - 4);
    reader.UpdatePosition(descLen * 8 - 32);
    break;
  default:
    XDMX_LOG_INFO("%s:   Found Unknown Program Descriptor. Tag: %d (0x%02lx), Len: %d.", __TS_MODULE__, tag, tag, descLen);
    reader.UpdatePosition(descLen * 8);
  }
  return (descLen + 2);
}

void CTSProgramMapFilter::ParseStream(CSimpleBitstreamReader& reader)
{
  unsigned char streamType = reader.ReadChar(8); // Stream Type
  reader.UpdatePosition(3); // Reserved
  unsigned short pid = reader.ReadShort(13); // Elementary PID
  reader.UpdatePosition(4); // Reserved
  unsigned short infoLen = reader.ReadShort(12); // Stream Info Length

  // Parse Stream Descriptors
  TSDescriptorList descList;
  while (infoLen)
  {
    CTSDescriptor* pDesc;
    infoLen -= ParseStreamDescriptor(reader, &pDesc);
    descList[pDesc->GetTag()] = pDesc;
  }

  // Try to identify the stream
  CElementaryStream* pStream = new CElementaryStream(pid, streamType);
  IPacketFilter* pCustomFilter = NULL;
  const char* pTypeName = "Unknown";
  switch (streamType) // (Table 2-29)
  {
  case ES_STREAM_TYPE_VIDEO_MPEG2_2:
    pTypeName = "MPEG2 Video";
    break;
  case ES_STREAM_TYPE_VIDEO_H264:
    pTypeName = "H264 Video";
    break;
  case ES_STREAM_TYPE_VIDEO_VC1:
    pTypeName = "VC1 Video";
    break;
  case ES_STREAM_TYPE_AUDIO_AC3:
    pTypeName = "AC3 Audio";
    break;
  case ES_STREAM_TYPE_AUDIO_HDMV_AC3_TRUE_HD:
    pTypeName = "True-HD Audio";
    break;
  case ES_STREAM_TYPE_AUDIO_DTS:
    pTypeName = "DTS Audio";
    break;
  case ES_STREAM_TYPE_AUDIO_HDMV_DTS_HD:
    pTypeName = "DTS-HD Audio";
    break;
  case ES_STREAM_TYPE_AUDIO_HDMV_DTS_HD_MASTER:
    pTypeName = "DTS-HD Master Audio";
    break;
  case ES_STREAM_TYPE_AUDIO_MPEG2_AAC:
    pTypeName = "AAC (MPEG-2 Part 7 Audio)";
    break;
  case ES_STREAM_TYPE_PRIVATE_SECTION:
    pTypeName = "Private Section";
    // TODO: This should use a TableFilter
    streamType = 0;
    break;
  case ES_STREAM_TYPE_PRIVATE_DATA:
    pTypeName = "Private Data";
    // TODO: This should use a private stream filter
    streamType = 0;
    break;
  case ES_STREAM_TYPE_USER_PRIVATE:
    {
      TSDescriptorIterator iter = descList.find(TS_DESC_REGISTRATION); // Get stream registration decriptor
      if (iter != descList.end())
      {
        const unsigned char* pData = iter->second->GetData();
        if (pData[0] == 'H' && pData[1] == 'D' && pData[2] == 'M' && pData[3] == 'V')
        {
          streamType = ES_PRIVATE_TYPE_LPCM;
          pCustomFilter = new CPESParserLPCM(pStream, m_pPayloadList);
          pStream->SetElementType(streamType);
          pTypeName = "LPCM Audio";
          break;
        }
      }
      else if (descList.find(TS_DESC_AC3) != descList.end()) // Check for AC-3 descriptor
      {
        streamType = ES_STREAM_TYPE_AUDIO_AC3;
        pStream->SetElementType(streamType);
        pTypeName = "AC3 Audio";
        break;
      }
      streamType = 0;
      break;
    }
  default:
    XDMX_LOG_INFO("%s:   Found Stream. PID: %lu (0x%04x), TypeName: %s, Type: %lu (0x%02x), InfoLen: %lu (0x%04x).", __TS_MODULE__, pid, pid, pTypeName, streamType, streamType, infoLen, infoLen);
    streamType = 0;
    break;
  }
  // TODO: Transfer descriptors to stream?
  if (streamType)
  {
    XDMX_LOG_INFO("%s:   Found Stream. PID: %lu (0x%04x), TypeName: %s, Type: %lu (0x%02x), InfoLen: %lu (0x%04x).", __TS_MODULE__, pid, pid, pTypeName, streamType, streamType, infoLen, infoLen);
    
    // Insert Stream into the hierarchy
    m_pProgram->AddStream(pStream);

    // Create and register a filter to handle the Stream
    CElementaryStreamFilter* pFilter = new CElementaryStreamFilter(pStream, m_pPayloadList, pCustomFilter);
    m_pRegistry->RegisterFilter(pid, pFilter);
  }
  else
  {
    // No need for this stream. We do not know what it is.
    delete pStream;
  }
  // Clean-up the descriptors
  for (TSDescriptorList::iterator iter = descList.begin(); iter != descList.end(); iter++)
    delete iter->second;
}

unsigned int CTSProgramMapFilter::ParseStreamDescriptor(CSimpleBitstreamReader& reader, CTSDescriptor** ppDesc)
{
  char formatId[4];
  char languageId[3];
  unsigned char audioType = 0;
  unsigned char tag = reader.ReadChar(8);
  unsigned char descLen = reader.ReadChar(8);
  CTSDescriptor* pDesc = new CTSDescriptor(tag);
  pDesc->SetData(reader.GetCurrentPointer(), descLen);

  switch (tag)
  {
  case TS_DESC_REGISTRATION: // Registration Descriptor
    // TODO: Parse sub-descriptors
    // struct subDesc{
    //  unsigned char tag;
    //  ...
    // };
    // VC-1 sub-descriptors:
    // 0x01 - Profile Level (+ 8 bits)
    // 0x02 - Alignment (+ 8 bits)
    // 0x03 - Buffer Size (+ 4 bits + 16 bits)
    // 0xff - NULL
    formatId[0] = reader.ReadChar(8);
    formatId[1] = reader.ReadChar(8);
    formatId[2] = reader.ReadChar(8);
    formatId[3] = reader.ReadChar(8);
    XDMX_LOG_INFO("%s:     Found Stream Registration Descriptor. Format: %4.4s, InfoLen: %d.", __TS_MODULE__, formatId, descLen - 4);
    reader.UpdatePosition(descLen * 8 - 32);
    break;
  case TS_DESC_LANGUAGE: // Language Descriptor
    // TODO: There can be multiple languages defined here
    languageId[0] = reader.ReadChar(8);
    languageId[1] = reader.ReadChar(8);
    languageId[2] = reader.ReadChar(8);
    audioType = reader.ReadChar(8);
    XDMX_LOG_INFO("%s:     Found Stream Language Descriptor. Language: %3.3s, AudioType: %d (0x%02lx).", __TS_MODULE__, languageId, audioType, audioType);
    reader.UpdatePosition(descLen * 8 - 32);
    break;
  case TS_DESC_AC3:     // AC-3 Descriptor
    XDMX_LOG_INFO("%s:     Found AC-3 Stream Identification Descriptor.", __TS_MODULE__);
    reader.UpdatePosition(descLen * 8);
    break;
  default:
    XDMX_LOG_INFO("%s:     Found Unknown Stream Descriptor. Tag: %d (0x%02lx), Len: %d.", __TS_MODULE__, tag, tag, descLen);
    reader.UpdatePosition(descLen * 8);
  }
  *ppDesc = pDesc;
  return (descLen + 2);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

class CTSProgramAssociationFilter : public CTableFilter
{
public:
  CTSProgramAssociationFilter(CTransportStream* pTransportStream, CTSFilterRegistry* pRegistry, PayloadList* pPayloadList);
  virtual ~CTSProgramAssociationFilter();
  virtual bool ParseSection(unsigned char* pData, unsigned int len);
protected:
  CTransportStream* m_pTransportStream;
  CTSFilterRegistry* m_pRegistry;
  unsigned short m_Version;
  PayloadList* m_pPayloadList;
};

CTSProgramAssociationFilter::CTSProgramAssociationFilter(CTransportStream* pTransportStream, CTSFilterRegistry* pRegistry, PayloadList* pPayloadList) :
  m_pTransportStream(pTransportStream),
  m_pRegistry(pRegistry),
  m_Version(0x100),
  m_pPayloadList(pPayloadList)
{

}

CTSProgramAssociationFilter::~CTSProgramAssociationFilter()
{

}

bool CTSProgramAssociationFilter::ParseSection(unsigned char* pData, unsigned int len)
{
  CSimpleBitstreamReader reader(pData, len);

  reader.ReadChar(8); // Table Id (0x00)
  reader.ReadBit(); // Syntax (1)
  reader.UpdatePosition(3);
  reader.ReadShort(12); // Section Length (len)
  reader.ReadShort(16); // Transport Stream Id
  reader.UpdatePosition(2);
  unsigned char version = reader.ReadChar(5);
  if (m_Version == version) // This version has already been processed.
    return true;
  bool current = reader.ReadBit(); // Current/Next Indicator
  unsigned char sectionId = reader.ReadChar(8); // Section Id
  unsigned char lastSectionId = reader.ReadChar(8); // Last Section Id
  XDMX_LOG_INFO("%s: Parsing Program Association Table (PAT) section %d.", __TS_MODULE__, sectionId);
  while (reader.GetBytesLeft() > 4)
  {
    unsigned short program = reader.ReadShort(16); // Program Number
    reader.UpdatePosition(3);
    unsigned short pid = reader.ReadShort(13); // Network PID (if Program Number == 0) or Program Map PID
    if (current) // TODO: Handle caching of 'next' PAT
    {
      // Create 'Program' Object
      CTSProgram* pProgram = new CTSProgram();
      m_pTransportStream->AddProgram(pProgram);

      // Create and register a mapping filter for the Program
      CTSProgramMapFilter* pFilter = new CTSProgramMapFilter(pProgram, m_pRegistry, m_pPayloadList);
      m_pRegistry->RegisterFilter(pid, pFilter);
      
      XDMX_LOG_INFO("%s: Found Program %lu (0x%04x). PID: %lu (0x%04x).", __TS_MODULE__, program, program, pid, pid);
    }
  }
  reader.ReadInt32(32); // CRC

  if (sectionId == lastSectionId) // Last section of this table
    m_Version = version; // Store the version to prevent unnecessary re-parsing

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
class CTransportStreamDemux : public ITransportStreamDemux
{
public:
  CTransportStreamDemux(TSTransportType type = TS_TYPE_UNKNOWN);
  virtual ~CTransportStreamDemux();
  
  bool Open(IXdmxInputStream* pInput);
  void Close();
  CParserPayload* GetPayload();
  CTransportStream* GetTransportStream();
  CElementaryStream* GetStreamById(unsigned short id);
  double GetTotalTime();
  double SeekTime(double time);
protected:
  void ProbeDuration();
  bool FillCache(unsigned int offset = 0);
  bool ProcessNext();
  unsigned char* ReadPacketSync(unsigned int* syncBytes); // Read a single packet from the input stream. Sync if necessary.
  bool ProcessNextPacket();
  bool ProbeStreams(unsigned int program);
  void FlushPayloads();
  void FlushStreams();


  // System Layer Information
  CTSFilterRegistry m_FilterRegistry;
  CTransportStream* m_pTransportStream;
  CTSProgramAssociationFilter* m_pPATFilter;
  PayloadList m_PayloadList;

  // Input Configuration
  int m_StreamType;
  unsigned int m_PacketSize;
  int m_SyncOffset;
  IXdmxInputStream* m_pInput;

  // Input Caching
  unsigned char* m_pCache;
  unsigned int m_MaxCacheSize;
  unsigned int m_CacheSize;
  unsigned int m_CacheOffset;

  // Stats
  uint64_t m_PacketCount;
  std::map<unsigned short, uint64_t> m_StreamCounterList;

  // Timing
  unsigned int m_HDMVArrivalTime;
  double m_Duration; // Seconds
};

CTransportStreamDemux::CTransportStreamDemux(TSTransportType type /*= TS_TYPE_UNKNOWN*/) :
  m_pTransportStream(NULL),
  m_pPATFilter(NULL),
  m_StreamType(type),
  m_PacketSize(188),
  m_SyncOffset(0),
  m_pInput(NULL),
  m_pCache(0),
  m_MaxCacheSize(0),
  m_CacheSize(0),
  m_CacheOffset(0),
  m_PacketCount(0),
  m_HDMVArrivalTime(0),
  m_Duration(0.0)
{
  
}

CTransportStreamDemux::~CTransportStreamDemux()
{
  Close();
}

bool CTransportStreamDemux::FillCache(unsigned int offset /*= 0*/)
{
  if (offset)
    memmove(m_pCache, m_pCache + m_CacheOffset, m_CacheSize - m_CacheOffset); // Move remaining data to the beginning of the buffer
  m_CacheSize = m_pInput->Read(m_pCache + offset, m_MaxCacheSize - offset);
  m_CacheOffset = 0;
  if (!m_CacheSize)
    return false;
  return true;
}

bool CTransportStreamDemux::Open(IXdmxInputStream* pInput)
{
  switch (m_StreamType)
  {
  case TS_TYPE_M2TS:
    m_PacketSize = 192;
    m_SyncOffset = 4;
    break;
  case TS_TYPE_DVB:
    m_PacketSize = 204;
    m_SyncOffset = 0;
    break;
  case TS_TYPE_ATSC:
    m_PacketSize = 208;
    m_SyncOffset = 0;
    break;
  case TS_TYPE_UNKNOWN: // TODO: See if we can probe to determine the likely format
  case TS_TYPE_STD:
  default:
    m_PacketSize = 188;
    m_SyncOffset = 0;
    break;
  }

  m_pInput = pInput;
  ProbeDuration();

  m_MaxCacheSize = m_PacketSize * 188; // TODO: Optimize
  m_pCache = new unsigned char[m_MaxCacheSize];
  if (!FillCache()) // TODO: Should we wait to fill the cache?
    return false;

  // Create stream hierarchy root object
  m_pTransportStream = new CTransportStream();

  // Register known PID filters
  m_pPATFilter = new CTSProgramAssociationFilter(m_pTransportStream, &m_FilterRegistry, &m_PayloadList);
  m_FilterRegistry.RegisterFilter(0x000, m_pPATFilter);

  // Read in the Program Association Table. We cannot parse anything without it.
  while (m_pTransportStream->GetProgramCount() == 0)
  {
    if (!ProcessNext())
      return false;
  }

  // Read packets until each PMT has been received
  // TODO: Find a better way to decide when we have enough information
  while (m_pTransportStream->GetProgram(m_pTransportStream->GetProgramCount() - 1)->GetStreamCount() == 0) 
  {
    if (!ProcessNext())
      return false;
  }

  // Try to probe streams for format information
  return true;//ProbeStreams(m_pTransportStream->GetProgramCount() - 1);
}

void CTransportStreamDemux::ProbeDuration()
{
  // TODO: Should we make use of the M2TS arrival time values?

  // Store current file position to return to after probe
  uint64_t lastPos = m_pInput->GetPosition();

  // Go to the beginning of the input
  if (m_pInput->Seek(0, SEEK_SET) != 0)
    return; // No way to tell the length of the stream if we can't seek

  unsigned char buf[TS_MAX_PACKET_LEN];
  uint64_t startTime = 0;
  uint64_t endTime = 0;

  // Find the first and last PCR from the same PID and calculate the time difference
  for (;;)
  {
    int i = 0;
    unsigned short pid = 0;

    for (;;)
    {
      // TODO: Reading 1 byte at a time is going to get awfully slow...
      for (i = m_pInput->Read(buf, m_SyncOffset + 1); (i > 0) && (buf[m_SyncOffset] != TS_SYNC_WORD); i = m_pInput->Read(buf + m_SyncOffset, 1))
        ; // Sync the stream

      if (i == 0)
        break; // End of Stream
      
      if (m_pInput->Read(buf + m_SyncOffset + 1, m_PacketSize - (m_SyncOffset + 1)) < (m_PacketSize - (m_SyncOffset + 1)))
        break; // End of Stream

      // Check for PCR
      if ((buf[m_SyncOffset + 3] & 0x20) && // Adaptation field present
          (buf[m_SyncOffset + 4]) &&        // length > 0
          (buf[m_SyncOffset + 5] & 0x10))   // has PCR
      {
        startTime = ((uint64_t)buf[m_SyncOffset + 6] << 25) | ((uint32_t)buf[m_SyncOffset + 7] << 17) | ((uint32_t)buf[m_SyncOffset + 8] << 9) | ((uint32_t)buf[m_SyncOffset + 9] << 1) | ((uint32_t)buf[m_SyncOffset + 10] >> 7);
        startTime *= 300;
        startTime += (((unsigned short)buf[m_SyncOffset + 10] & 0x1) << 8) | buf[m_SyncOffset + 11];
        pid = (((unsigned short)buf[m_SyncOffset + 1] & 0x1f) << 8) + buf[m_SyncOffset + 2];
        break;
      }
    }
    if (!startTime)
      break;

    for (i = m_PacketSize; i < m_pInput->GetLength(); i += m_PacketSize)
    {
      m_pInput->Seek(0 - i, SEEK_END);
      if (m_pInput->Read(buf, m_PacketSize) != m_PacketSize)
        break;

      if (buf[m_SyncOffset] != TS_SYNC_WORD)
      {
        // Need sync first
        i -= (m_PacketSize - 1);
        continue;
      }

      // Check for PCR
      if ((buf[m_SyncOffset + 3] & 0x20) && // Adaptation field present
          (buf[m_SyncOffset + 4]) &&        // length > 0
          (buf[m_SyncOffset + 5] & 0x10))   // has PCR
      {
        if (pid == ((((unsigned short)buf[m_SyncOffset + 1] & 0x1f) << 8) + buf[m_SyncOffset + 2]))
        {
          endTime = ((uint64_t)buf[m_SyncOffset + 6] << 25) | ((uint32_t)buf[m_SyncOffset + 7] << 17) | ((uint32_t)buf[m_SyncOffset + 8] << 9) | ((uint32_t)buf[m_SyncOffset + 9] << 1) | ((uint32_t)buf[m_SyncOffset + 10] >> 7);
          endTime *= 300;
          endTime += (((unsigned short)buf[m_SyncOffset + 10] & 0x1) << 8) | buf[m_SyncOffset + 11];
          break;
        }
      }
    }
    if (!endTime)
      break;

    m_Duration = (double)(endTime - startTime)/ 27000000.0;
    break;
  }

  // Return to the previous position before leaving
  m_pInput->Seek(lastPos, SEEK_SET);
}

void CTransportStreamDemux::Close()
{
  // Reset members
  m_StreamType = TS_TYPE_UNKNOWN;
  m_PacketSize = 188;
  m_SyncOffset = 0;
  m_pInput = NULL; // TODO: Is this cleaned-up by the caller?
  m_MaxCacheSize = 0;
  m_CacheSize = 0;
  m_CacheOffset = 0;
  m_PacketCount = 0;
  m_HDMVArrivalTime = 0;

  // Clean-up input cache
  delete[] m_pCache;
  m_pCache = NULL;

  // Clean-up any remaining payload data
  FlushPayloads();

  // Reset the filter registry
  m_FilterRegistry.UnregisterAll(true);

  // Clean-Up stream hierarchy
  delete m_pTransportStream;
  m_pTransportStream = NULL;
}

CParserPayload* CTransportStreamDemux::GetPayload()
{
  do
  {
    if (m_PayloadList.size())
    {
      CParserPayload* pPayload = m_PayloadList.front();
      m_PayloadList.pop_front();
      return pPayload;
    }
  } while (ProcessNext());
  return NULL;
}

void CTransportStreamDemux::FlushPayloads()
{
  while (m_PayloadList.size())
  {
    delete m_PayloadList.front();
    m_PayloadList.pop_front();
  }
}

void CTransportStreamDemux::FlushStreams()
{
  for (unsigned int p = 0; p < m_pTransportStream->GetProgramCount(); p++)
  {
    CTSProgram* pProgram = m_pTransportStream->GetProgram(p);
    for (unsigned int s = 0; s < pProgram->GetStreamCount(); s++)
    {
      // TODO: Find a clean way to flush the stream filters
    }
  }
}

bool CTransportStreamDemux::ProcessNext()
{
  // Read until we find a valid TS packet or run out of input
  for (;;)
  {
    // Fetch a packet from the input stream
    unsigned int syncBytes = 0;
    unsigned char* pHeader = ReadPacketSync(&syncBytes);
    if (!pHeader)
      break; // Give up

    unsigned int bytesLeft = m_PacketSize;
    // Read the M2TS time code if it exists.
    if (m_StreamType == TS_TYPE_M2TS)
    {
      // TODO: What do we do with this now?
      unsigned int arrivalTime = ((unsigned int)(pHeader[0] & 0x3f) << 24) | ((unsigned int)pHeader[1] << 16) | ((unsigned int)pHeader[2] << 8) | pHeader[3];
      pHeader += 4;
      bytesLeft -= 4;
      if (m_HDMVArrivalTime)
      {
        //unsigned int arrivalDelta = arrivalTime - m_HDMVArrivalTime;
      }
      m_HDMVArrivalTime = arrivalTime;
    }

    // Parse the TS packet header (ISO/IEC 13818-1, Table 2-2)
    /*
    0:7   - Sync Byte (0x47)
    8     - Transport Error Indicator
    9     - Payload Unit Start Indicator
    10    - Transport Priority
    11:23 - Packet Id (PID) - Big Endian
    24:25 - Scrambling Control
    26    - Adaptation Field Flag
    27    - Payload Flag
    28:31 - Continuity Counter - Big Endian
    */

    // Skip the sync word. We do not need it.
    pHeader += 1;
    bytesLeft -= 1;

    bool trasportError = (pHeader[0] & 0x80) == 0x80;
    if (trasportError) // This packet contains an error.
      continue; // Try again

    bool newPayload = (pHeader[0] & 0x40) == 0x40;
    // Transport Priority - 1 bit
    unsigned short pid = ((unsigned short)(pHeader[0] & 0x1f) << 8) | pHeader[1];
    pHeader += 2;
    bytesLeft -=2;
    if (pid == 0x1FFF) // NULL Packet
      continue; // Try again

    // Determine the type of this packet
    CTSFilter* pFilter = m_FilterRegistry.GetFilter(pid);

    unsigned char scrambling = pHeader[0] >> 6;
    bool handleAF = (pHeader[0] & 0x20) == 0x20; // An adaptation field (AF) will follow the header
    bool handlePayload = (pHeader[0] & 0x10) == 0x10;
    unsigned char continuityCounter = pHeader[0] & 0xf;
    pHeader++;
    bytesLeft--;

    if (scrambling) // This pid's data is scrambled
    {
      if (m_FilterRegistry.GetFilter(0x001) == NULL) // Check for CAT
        continue; // Won't be able to descramble it if no CAT is present
    }
    // Parse the adaptation field, if one is present
    if (handleAF)
    {
      unsigned char fieldLen = pHeader[0];
      pHeader++;
      bytesLeft--;
      if (fieldLen)
      {
        unsigned char adapFlags = pHeader[0];
        pHeader++;
        bytesLeft--;
        if (adapFlags & 0x10) // Check PCR Flag
        {
          if (adapFlags & 0x80) // Discontinuity
          {
            XDMX_LOG_DEBUG("%s: Detected Discontinuity in PCR", __TS_MODULE__);
          }
          CProgramClock* pClock =  m_FilterRegistry.GetClock(pid);
          if (pClock)
          {
            // TODO: Detect and handle discontinuities
            uint64_t pcrBase = ((uint64_t)pHeader[0] << 25) | ((unsigned int)pHeader[1] << 17) | ((unsigned int)pHeader[2] << 9) | ((unsigned int)pHeader[3] << 1) | ((unsigned int)pHeader[4] >> 7);
            unsigned short pcrExt = (((unsigned short)pHeader[4] & 0x1) << 8) | pHeader[5];
            pClock->AddReference(m_PacketCount * m_PacketSize - bytesLeft, pcrBase, pcrExt);
            fieldLen -= 6;
          }
        }
        pHeader += (fieldLen - 1);
        bytesLeft -= (fieldLen - 1);
        // TODO: Be sure to skip stuffing bytes. See 2.4.3.5 in spec.
      }
    }

    if (!pFilter) // Unknown PID
      continue; // We don't know what to do with it

    // If the packet has a payload, pass it on to the appropriate filter
    if (handlePayload)
    {
      // Check the continuity counter
      if (!pFilter->CheckContinuity(continuityCounter))
      {
        // TODO: Handle these
      }
      pFilter->Add(pHeader, bytesLeft, newPayload);
    }
    return true;
  }

  // We were not able to find an appropriate packet
  return false;
}

unsigned char* CTransportStreamDemux::ReadPacketSync(unsigned int* syncBytes)
{
  // if necessary, sync the stream
  unsigned int bytesToSync = 0;
  do
  {
    // Look for the syncword in the cache 
    for (unsigned char* pByte = m_pCache + m_CacheOffset; (m_CacheSize - m_CacheOffset) >= m_PacketSize; m_CacheOffset++)
    {
      if (pByte[m_SyncOffset] == TS_SYNC_WORD)
      {
        // Found it!
        m_CacheOffset += m_PacketSize;
        if (syncBytes)
          *syncBytes = bytesToSync;
        m_PacketCount++;
        return pByte;
      }
      bytesToSync++;
      if (syncBytes && *syncBytes && bytesToSync > *syncBytes)
        break; // Stop searching after maximum number of bytes
    }
  } while (FillCache(m_CacheSize - m_CacheOffset));

  return false; // Could not find the sync word in the available data
}

CTransportStream* CTransportStreamDemux::GetTransportStream()
{
  return m_pTransportStream;
}

CElementaryStream* CTransportStreamDemux::GetStreamById(unsigned short id)
{
  CElementaryStreamFilter* pFilter = dynamic_cast<CElementaryStreamFilter*>(m_FilterRegistry.GetFilter(id));
  if (!pFilter)
    return NULL;
  return pFilter->GetStream();
}

double CTransportStreamDemux::GetTotalTime()
{
  return m_Duration;
}

double CTransportStreamDemux::SeekTime(double time)
{
  // TODO: Make use of the M2TS timestamps
  CProgramClock* pClock = m_pTransportStream->GetProgram(m_pTransportStream->GetProgramCount()-1)->GetClock(); // TODO: Which clock should be used for reference?
  double deltaTime = time - pClock->GetElapsedTime();
  double avgBytesPerSec = (double)m_pInput->GetLength() / m_Duration;
  
  // Do a rough seek to get us close...
  int64_t deltaBytes = (int64_t)(deltaTime * avgBytesPerSec);
  deltaBytes -= (deltaBytes % m_PacketSize);
  m_pInput->Seek(deltaBytes, SEEK_CUR);

  double actualTime = 0.0;
  double error = 0.0;
  unsigned char buf[TS_MAX_PACKET_LEN];
  int i = 0;
  unsigned short pid = 0;
  uint64_t lastPcr = 0;
  // Find the next PCR in the stream
  for (int loop = 0; loop < 10; loop++) // Try a max of 10 times
  {
    for (i = m_pInput->Read(buf, m_SyncOffset + 1); (i > 0) && (buf[m_SyncOffset] != TS_SYNC_WORD); i = m_pInput->Read(buf + m_SyncOffset, 1))
      ; // Sync the stream

    if (i == 0)
    {
      actualTime = m_Duration;
      break;
    }
    
    if (m_pInput->Read(buf + m_SyncOffset + 1, m_PacketSize - (m_SyncOffset + 1)) < (m_PacketSize - (m_SyncOffset + 1)))
    {
      actualTime = m_Duration;
      break;
    }

    // Check for PCR
    if ((buf[m_SyncOffset + 3] & 0x20) && // Adaptation field present
        (buf[m_SyncOffset + 4]) &&        // length > 0
        (buf[m_SyncOffset + 5] & 0x10))   // has PCR
    {
      uint64_t pcr = ((uint64_t)buf[m_SyncOffset + 6] << 25) | ((uint32_t)buf[m_SyncOffset + 7] << 17) | ((uint32_t)buf[m_SyncOffset + 8] << 9) | ((uint32_t)buf[m_SyncOffset + 9] << 1) | ((uint32_t)buf[m_SyncOffset + 10] >> 7);
      pcr *= 300;
      pcr += (((unsigned short)buf[m_SyncOffset + 10] & 0x1) << 8) | buf[m_SyncOffset + 11];
      pid = (((unsigned short)buf[m_SyncOffset + 1] & 0x1f) << 8) + buf[m_SyncOffset + 2];
      actualTime = (double)(pcr - pClock->GetFirstReference()) / 27000000.0;
      error = actualTime - time;
      if (error <= 0.5 && error >= -0.5)
        break; // Accept +- .5 seconds

      if (pcr == lastPcr)
        break; // We're stuck. Give up.

      // Go half the distance to the goal...
      deltaBytes = (int64_t)((0.0 - error) * avgBytesPerSec) / 2;
      deltaBytes -= (deltaBytes % m_PacketSize);
      m_pInput->Seek(deltaBytes, SEEK_CUR);

      lastPcr = pcr;
    }
  }

  FlushPayloads(); // These are invalid now
  FillCache(); // Re-fill the input cache

  // TODO: Notify clocks of discontinuity
  // TODO: Flush PES filters
  m_FilterRegistry.FlushStreamFilters();

  return actualTime;
}

bool CTransportStreamDemux::ProbeStreams(unsigned int program)
{
  // Read packets until a payload has been received by each stream, this will allow 
  // each parser to probe for stream parameters
  CTSProgram* pProgram = m_pTransportStream->GetProgram(program);
  unsigned int streamsToProbe = pProgram->GetStreamCount();
  CElementaryStream** pStreams = new CElementaryStream*[streamsToProbe];
  for (unsigned int s = 0; s < streamsToProbe; s++)
    pStreams[s] = pProgram->GetStream(s);

  while (streamsToProbe)
  {
    unsigned int listLen = m_PayloadList.size();
    // Process packets until a new payload shows up
    do
    {
      if (!ProcessNext())
        return false;
      
    } while (m_PayloadList.size() == listLen);

    // See if the latest payload is for a stream we want
    // TODO: Find a better way to manage which streams have been found
    CElementaryStream* pStream = m_PayloadList.back()->GetStream();
    for (unsigned int s = 0; s < pProgram->GetStreamCount(); s++)
    {
      if (pStream && (pStream == pStreams[s])) // Found a new one
      {
        pStreams[s] = NULL; // No need to look for this one any more
        streamsToProbe--; // One less to go
        break;
      }
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////

ITransportStreamDemux* CreateTSDemux(TSTransportType type /*= TS_TYPE_UNKNOWN*/)
{
  return new CTransportStreamDemux(type);
}
