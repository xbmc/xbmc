/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/SingleLock.h"
#include "DVDPlayerAudio.h"
#include "DVDPlayer.h"
#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDPerformanceCounter.h"
#include "GUISettings.h"
#include "VideoReferenceClock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include <sstream>
#include <iomanip>

using namespace std;

CPTSOutputQueue::CPTSOutputQueue()
{
  Flush();
}

void CPTSOutputQueue::Add(double pts, double delay, double duration)
{
  CSingleLock lock(m_sync);

  TPTSItem item;
  item.pts = pts;
  item.timestamp = CDVDClock::GetAbsoluteClock() + delay;
  item.duration = duration;

  // first one is applied directly
  if(m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
    m_current = item;
  else
    m_queue.push(item);

  // call function to make sure the queue
  // doesn't grow should nobody call it
  Current();
}
void CPTSOutputQueue::Flush()
{
  CSingleLock lock(m_sync);

  while( !m_queue.empty() ) m_queue.pop();
  m_current.pts = DVD_NOPTS_VALUE;
  m_current.timestamp = 0.0;
  m_current.duration = 0.0;
}

double CPTSOutputQueue::Current()
{
  CSingleLock lock(m_sync);

  if(!m_queue.empty() && m_current.pts == DVD_NOPTS_VALUE)
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  while( !m_queue.empty() && CDVDClock::GetAbsoluteClock() >= m_queue.front().timestamp )
  {
    m_current = m_queue.front();
    m_queue.pop();
  }

  if( m_current.timestamp == 0 ) return m_current.pts;

  return m_current.pts + min(m_current.duration, (CDVDClock::GetAbsoluteClock() - m_current.timestamp));
}

void CPTSInputQueue::Add(__int64 bytes, double pts)
{
  CSingleLock lock(m_sync);

  m_list.insert(m_list.begin(), make_pair(bytes, pts));
}

void CPTSInputQueue::Flush()
{
  CSingleLock lock(m_sync);

  m_list.clear();
}
double CPTSInputQueue::Get(__int64 bytes, bool consume)
{
  CSingleLock lock(m_sync);

  IT it = m_list.begin();
  for(; it != m_list.end(); it++)
  {
    if(bytes <= it->first)
    {
      double pts = it->second;
      if(consume)
      {
        it->second = DVD_NOPTS_VALUE;
        m_list.erase(++it, m_list.end());
      }
      return pts;
    }
    bytes -= it->first;
  }
  return DVD_NOPTS_VALUE;
}

////////////////////////////////////
CDVDAudioPacketHandler::CDVDAudioPacketHandler(CDVDStreamInfo& hints) :
  m_StreamInfo(hints)
{
  // Clear output format
  memset(&m_OutputFormat, 0, sizeof(DVDAudioFormat));;
}

///////////////////

CDVDAudioDecodePacketHandler::CDVDAudioDecodePacketHandler(CDVDStreamInfo& hints, CDVDAudioCodec* pDecoder) :
  CDVDAudioPacketHandler(hints),
  m_pDecoder(pDecoder)
{
  
}

CDVDAudioDecodePacketHandler::~CDVDAudioDecodePacketHandler()
{
  Close();
}

bool CDVDAudioDecodePacketHandler::Init(void* pData, unsigned int size)
{
  if (!m_pDecoder)
    return false;
  
  // TODO: prime decoder before filling-out format
  m_OutputFormat.streamType = DVDAudioStreamType_PCM;
  m_OutputFormat.bitrate = m_pDecoder->GetSampleRate() * m_pDecoder->GetChannels() * (sizeof(float)<<3); // All decoders return float samples
  m_OutputFormat.pcm.channels =  m_pDecoder->GetChannels();
  m_OutputFormat.pcm.sampleType = DVDAudioPCMSampleType_IEEEFloat; // All decoders return float samples
  m_OutputFormat.pcm.channel_map = m_pDecoder->GetChannelMap();
  
  return true;
}

unsigned int CDVDAudioDecodePacketHandler::AddPacket(void* pData, unsigned int size)
{
  // Validate parameters and pass data to inner decoder
  if (m_pDecoder && pData  && size)
  {
    return m_pDecoder->Decode((BYTE*)pData, (int)size);
  }
  return 0;
}

bool CDVDAudioDecodePacketHandler::GetFrame(DVDAudioFrame& frame)
{
  // Fetch a frame from the decoder and set-up an audioframe
  if (m_pDecoder)
  {
    frame.size = m_pDecoder->GetData((float**)&frame.data);
    if (frame.data && frame.size)
    {
      frame.format = m_OutputFormat;
      return true;
    }
  }
  return false;
}

void CDVDAudioDecodePacketHandler::Close()
{
  m_pDecoder->Dispose();
  SAFE_DELETE(m_pDecoder);
  m_pDecoder = NULL;
}

void CDVDAudioDecodePacketHandler::Reset()
{
  if (m_pDecoder)
    m_pDecoder->Reset();
}

unsigned int CDVDAudioDecodePacketHandler::GetCacheSize()
{
  if (m_pDecoder)
    return m_pDecoder->GetBufferSize();
  return 0;
}

//////////////////////////

CDVDAudioPassthroughParser::CDVDAudioPassthroughParser(CDVDStreamInfo& hints) :
  CDVDAudioPacketHandler(hints),
  m_pCurrentFrame(NULL),
  m_CurrentFrameSize(0)
{
  
}

CDVDAudioPassthroughParser::~CDVDAudioPassthroughParser()
{
  Close();
}

bool CDVDAudioPassthroughParser::Init(void* pData, unsigned int size)
{
  if (!pData || !size)
    return false; // Nothing to probe
  
  unsigned char* pFrame = (unsigned char*)pData;
  //TODO: Parse packet header to determine format
  // Try to parse the probe data passed in. If we don't find what we need, we will have to fall-back to the hints
  switch (m_StreamInfo.codec)
  {
    case CODEC_ID_AAC:
      // TODO: Not every AAC frame contains stream info, but look for it anyway
      m_OutputFormat.bitrate = m_StreamInfo.bitrate;      
      m_OutputFormat.encoded.encodingType = DVDAudioEncodingType_AAC;
      break;
    case CODEC_ID_AC3:
    {
      // [0-1] Sync Word (0x0B77), [2-3] CRC1, [4] Sample Rate / Frame Size
      char sr = pFrame[4] & 0xC0 >> 6;
      char fs = pFrame[4] & 0x30;
      m_OutputFormat.bitrate = 448000; // TODO: Read from stream...
      m_OutputFormat.encoded.encodingType = DVDAudioEncodingType_AC3;
      break;
    }
    case CODEC_ID_DTS:
    {
      // [0-3] Sync Word, [4-9] Frame Info
      // TODO: Check sync word to determine format
      unsigned char blocks = pFrame[5] >> 2 | (0x1 & pFrame[4]) << 6;
      unsigned short frameSize = (((short)pFrame[5] & 0x3) << 12 ) | ((short)pFrame[6] << 4) | (pFrame[7] >> 4);
      unsigned char rate = pFrame[9] >> 5 | ((pFrame[8] & 0x3) << 3);
      m_OutputFormat.encoded.encodingType = DVDAudioEncodingType_DTS;
      m_OutputFormat.bitrate = 448000; // TODO: Read from stream...
      break;
    }
    case CODEC_ID_MP1:
      m_OutputFormat.encoded.encodingType = DVDAudioEncodingType_MP1;
      return false; // Not a supported passthrough format (yet)
      break;
    case CODEC_ID_MP2:
      m_OutputFormat.encoded.encodingType = DVDAudioEncodingType_MP2;
      return false; // Not a supported passthrough format (yet)
      break;
    case CODEC_ID_MP3:
      m_OutputFormat.encoded.encodingType = DVDAudioEncodingType_MP3;
      return false; // Not a supported passthrough format (yet)
      break;
    default:
      return false; // Not a supported passthrough format
  }
  
  m_OutputFormat.streamType = DVDAudioStreamType_Encoded;
  m_OutputFormat.encoded.containedType = DVDAudioEncodingType_Raw;
  
  return true;
}

unsigned int CDVDAudioPassthroughParser::AddPacket(void* pData, unsigned int size)
{
  m_pCurrentFrame = pData;
  m_CurrentFrameSize = size;
  
  return size;
}

bool CDVDAudioPassthroughParser::GetFrame(DVDAudioFrame& frame)
{
  if (m_pCurrentFrame && m_CurrentFrameSize)
  {
    frame.format = m_OutputFormat;
    frame.data = m_pCurrentFrame;
    m_pCurrentFrame = NULL;
    frame.size = m_CurrentFrameSize;
    m_CurrentFrameSize = 0;
    return true;
  }
  return false;
}

void CDVDAudioPassthroughParser::Close()
{
  m_pCurrentFrame = NULL;
  m_CurrentFrameSize = 0;  
}

//////////////////////////////////

CDVDAudioPostProc::CDVDAudioPostProc(DVDAudioFormat inFormat, DVDAudioFormat outFormat) :
  m_InputFormat(inFormat),
  m_OutputFormat(outFormat)
{
  
}

//////////////////////////////////

// TODO: Define packetizer
CDVDAudioPacketizerSPDIF::CDVDAudioPacketizerSPDIF(DVDAudioFormat inFormat, DVDAudioFormat outFormat) :
  CDVDAudioPostProc(inFormat, outFormat)
{
  
}

CDVDAudioPacketizerSPDIF::~CDVDAudioPacketizerSPDIF()
{
  
}

bool CDVDAudioPacketizerSPDIF::Init()
{
  return false;
}

bool CDVDAudioPacketizerSPDIF::AddFrame(DVDAudioFrame& frame)
{
  return false;
}

bool CDVDAudioPacketizerSPDIF::GetFrame(DVDAudioFrame& frame)
{
  return false;
}

void CDVDAudioPacketizerSPDIF::Close()
{
}

///////////////////////////////////

CDVDPlayerAudio::CDVDPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent)
: CThread()
, m_messageQueue("audio")
, m_messageParent(parent)
, m_dvdAudio((bool&)m_bStop)
{
  m_pClock = pClock;
  m_nextstream.Clear();
  m_audioClock = 0;
  m_droptime = 0;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_stalled = true;
  m_started = false;
  m_duration = 0.0;
  
  memset(&m_InputFormat, 0, sizeof(DVDAudioFormat));
  memset(&m_OutputFormat, 0, sizeof(DVDAudioFormat));
  m_pInputHandler = NULL;
  m_pPostProc = NULL;
  m_pConverter = NULL;
  
  m_freq = CurrentHostFrequency();

  m_messageQueue.SetMaxDataSize(6 * 1024 * 1024);
  m_messageQueue.SetMaxTimeSize(8.0);
  g_dvdPerformanceCounter.EnableAudioQueue(&m_messageQueue);
}

CDVDPlayerAudio::~CDVDPlayerAudio()
{
  StopThread();
  g_dvdPerformanceCounter.DisableAudioQueue();

  // close the stream, and don't wait for the audio to be finished
  // CloseStream(true);
}

bool CDVDPlayerAudio::OpenStream( CDVDStreamInfo &hints )
{
  // Store stream information until we have a packet to parse
  m_nextstream = hints;
  
  m_messageQueue.Init();

  m_droptime = 0;
  m_audioClock = 0;
  m_stalled = true;
  m_started = false;

  m_synctype = SYNC_DISCON;
  m_setsynctype = g_guiSettings.GetInt("videoplayer.synctype");
  m_prevsynctype = -1;
  m_resampler.SetQuality(g_guiSettings.GetInt("videoplayer.resamplequality"));

  m_error = 0;
  m_errorbuff = 0;
  m_errorcount = 0;
  m_integral = 0;
  m_skipdupcount = 0;
  m_prevskipped = false;
  m_syncclock = true;
  m_errortime = CurrentHostCounter();

  m_maxspeedadjust = g_guiSettings.GetFloat("videoplayer.maxspeedadjust");

  CLog::Log(LOGNOTICE, "Creating audio thread");
  Create();

  return true;
}

void CDVDPlayerAudio::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0) m_messageQueue.WaitUntilEmpty();

  // send abort message to the audio queue
  m_messageQueue.Abort();

  CLog::Log(LOGNOTICE, "Waiting for audio thread to exit");

  // shut down the adio_decode thread and wait for it
  StopThread(); // will set this->m_bStop to true

  // destroy audio device
  CLog::Log(LOGNOTICE, "Closing audio device");
  if (bWaitForBuffers && m_speed > 0)
  {
    m_bStop = false;
    m_dvdAudio.Drain();
    m_bStop = true;
  }
  m_dvdAudio.Destroy();

  // uninit queue
  m_messageQueue.End();

  // Destroy processing elements
  CloseAudioPath();

  // flush any remaining pts values
  m_ptsOutput.Flush();
  m_resampler.Flush();
}

bool CDVDPlayerAudio::OpenAudioPath(CDVDStreamInfo &hints, BYTE* pData /*= NULL*/, unsigned int size /*= 0*/)
{  
  // Make sure any old elements were cleaned-up properly
  CloseAudioPath();
  
  // If input stream is in a format we want to pass-through, try to pass it through
  if ((g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL) && 
      ((hints.codec == CODEC_ID_AC3 && g_guiSettings.GetBool("audiooutput.ac3passthrough")) ||
       (hints.codec == CODEC_ID_DTS && g_guiSettings.GetBool("audiooutput.dtspassthrough")) ||
       (hints.codec == CODEC_ID_AAC && g_guiSettings.GetBool("audiooutput.aacpassthrough")) ||
       (hints.codec == CODEC_ID_MP1 && g_guiSettings.GetBool("audiooutput.mp1passthrough")) ||
       (hints.codec == CODEC_ID_MP2 && g_guiSettings.GetBool("audiooutput.mp2passthrough")) ||
       (hints.codec == CODEC_ID_MP3 && g_guiSettings.GetBool("audiooutput.mp3passthrough")))) 
  {
    // Create handler
    m_pInputHandler = new CDVDAudioPassthroughParser(hints);
    
    // Try to initialize the parser with the provided packet
    // if successful, try to build the rest of the path
    if (m_pInputHandler->Init(pData, size))
    {      
      // Populate/fetch format structures
      m_InputFormat = m_pInputHandler->GetOutputFormat();
      m_OutputFormat = m_InputFormat; // TODO: Update based on desired output bitrate, etc...
      
      // Configure the renderer
      bool ret = m_dvdAudio.Create(m_OutputFormat, m_pInputHandler->GetStreamInfo().codec);
      
      // if successful, keep going
      if (ret) // TODO: Check that the configured renderer is not the NULL renderer
      {
        // Create a packetizer, if necessary
        m_pPostProc = new CDVDAudioPacketizerSPDIF(m_InputFormat, m_OutputFormat);
        // Try to initialize the packetizer
        // If successful, we are ready to go...
        if (m_pPostProc->Init())
          return true;
        // if not, we cannot complete the path...fall into the next option
      }
    }
    // if not, we cannot complete the passthrough path...fall into the next option
  }
  
  // Fall-back to PCM or use provided raw PCM stream
  
  // Create a decoder for the input stream, and then wrap the decoder with a parser to standardize the interface
  m_pInputHandler = new CDVDAudioDecodePacketHandler(hints, CDVDFactoryCodec::CreateAudioCodec(hints));
    
  // if successful, try to build the rest of the path
  if (m_pInputHandler->Init(pData, size))
  {
    // Fetch input format structure
    m_InputFormat = m_pInputHandler->GetOutputFormat();
    m_OutputFormat = m_InputFormat; // Start by assuming no format changes
    m_OutputFormat.pcm.sampleType = DVDAudioPCMSampleType_S16LE; // Force S16LE for now
    m_OutputFormat.bitrate /= 2; // Adjust for conversion
    
    // TODO: Examine configured speaker layout and implement mixing
    // if the output speaker configuration supports the number/layout of decoded channels, no mixing/mapping is needed
    {
      // Configure the renderer
      // if successful, we are ready to go...
      if (m_dvdAudio.Create(m_OutputFormat, m_pInputHandler->GetStreamInfo().codec))
      {
        // TODO: Move this to the right place (post-proc container)
        // Create format converter
        m_pConverter = new CPCMSampleConverter();
        if (m_pConverter->Initialize(m_InputFormat, m_OutputFormat))
          return true;
      }
      // if not, the user has likely provided an incorrect speaker configuration, fall-back to 2-channel stereo
      // Update the output format  
      // outFormat.pcm.channels = 2;
    }
    // if not, try to use the configured channel count/layout
      // Update the output format
    // Configure the renderer
    // m_dvdaudio.Create(outFormat)
    // if not successful, and not already trying 2-channel stereo, fall back to 2-channel stereo (last resort)
      // Update the output format  
      // outFormat.pcm.channels = 2;
      // Configure the renderer
      // m_dvdaudio.Create(outFormat)
      // if still not successful, we did our best. Let the NULL renderer stay and give up.
        // break; // or return...?
    // Create the matrix mixer to convert from the input channel configuration to the output configuration
    // pPostProc = new CDVDAudioMatrixMixer();
    // Try to initialize the matrix mixer
    // pPostProc->Init(inFormat, outFormat);
    // if successful, , we are ready to go...
      // break; // or return...?
  }
  // if not, we cannot complete the path...give up
  CloseAudioPath();
  return false;
  
  // After the path is constructed, it may be ready to provide an output frame, so be sure to check before adding new data
}

void CDVDPlayerAudio::ResetAudioPath()
{
  if (m_pInputHandler)
    m_pInputHandler->Reset();

  if (m_pPostProc)
    m_pPostProc->Reset();
}

void CDVDPlayerAudio::CloseAudioPath()
{
  if (m_pInputHandler)
    m_pInputHandler->Close();
  delete m_pInputHandler;
  m_pInputHandler = NULL;

  delete m_pConverter;
  m_pConverter = NULL;    
  
  if (m_pPostProc)
    m_pPostProc->Close();
  delete m_pPostProc;
  m_pPostProc = NULL;
}

// decode one audio frame and return its uncompressed size (handle and messages encountered along the way)
int CDVDPlayerAudio::DecodeFrame(DVDAudioFrame &audioframe, bool bDropPacket)
{
  int result = 0;

  // make sure the sent frame is clean
  memset(&audioframe, 0, sizeof(DVDAudioFrame));

  while (!m_bStop)
  {
    // NOTE: the audio packet can contain several frames
    while( !m_bStop && m_decode.size > 0 )
    {
      // If we have new stream information, try to open an audio path
      if (m_nextstream.codec)
      {
        OpenAudioPath(m_nextstream, m_decode.data, m_decode.size);
        // TODO: Is this the right place for this?
        m_messageQueue.SetMaxTimeSize(8.0 - m_dvdAudio.GetCacheTotal());
        m_nextstream.Clear(); // only try once, succeed or fail
      }

      // If there is no input handler, we cannot really do much
      if (m_pInputHandler)
      {
        // if the input handler already has a frame ready, do not process another packet yet
        // TODO: Should we force each frame to 'expire'?
        if (1)//!m_pInputHandler->GetFrame(audioframe))
        {
          int consumedBytes = 0;
          
          // the packet dts refers to the first audioframe that starts in the packet
          double dts = m_ptsInput.Get(m_decode.size + m_pInputHandler->GetCacheSize(), true);
          if (dts != DVD_NOPTS_VALUE)
            m_audioClock = dts;
        
          consumedBytes = m_pInputHandler->AddPacket(m_decode.data, m_decode.size);
          if (consumedBytes < 0)
          {
            /* if error, we skip the packet */
            CLog::Log(LOGERROR, "CDVDPlayerAudio::DecodeFrame - Decode Error. Skipping audio packet");
            m_decode.Release();
            m_pInputHandler->Reset();
            return DECODE_FLAG_ERROR;
          }
          
          // fix for fucked up decoders
          if( consumedBytes > m_decode.size )
          {
            CLog::Log(LOGERROR, "CDVDPlayerAudio:DecodeFrame - Codec tried to consume more data than available. Potential memory corruption");
            m_decode.Release();
            m_pInputHandler->Reset();
            assert(0);
          }
          
          m_decode.data += consumedBytes;
          m_decode.size -= consumedBytes;

          m_audioStats.AddSampleBytes(consumedBytes);
          
          // TODO: How do we (can we?) detect and handle mid-stream format changes?
          if (!m_pInputHandler->GetFrame(audioframe))
            continue;
        }
        audioframe.pts = m_audioClock;
      }
      else
        return DECODE_FLAG_ERROR;

      if (audioframe.format.bitrate > 0)
      {
        audioframe.duration = ((double)audioframe.size * DVD_TIME_BASE) / (audioframe.format.bitrate>>3);
        
        // increase audioclock to after the packet
        m_audioClock += audioframe.duration;
      }
      
      if(audioframe.duration > 0)
        m_duration = audioframe.duration;
      
      // if demux source wants us to not display this, continue
      if(m_decode.msg->GetPacketDrop())
        continue;       
      
      //If we are asked to drop this packet, return a size of zero. then it won't be played
      //we currently still decode the audio.. this is needed since we still need to know it's
      //duration to make sure clock is updated correctly.
      if( bDropPacket )
        result |= DECODE_FLAG_DROP;

      return result;
    }
    // free the current packet
    m_decode.Release();

    if (m_messageQueue.ReceivedAbortRequest()) return DECODE_FLAG_ABORT;

    CDVDMsg* pMsg;
    int priority = (m_speed == DVD_PLAYSPEED_PAUSE && m_started) ? 1 : 0;

    int timeout;
    if(m_duration > 0)
      timeout = (int)(1000 * (m_duration / DVD_TIME_BASE + m_dvdAudio.GetCacheTime()));
    else
      timeout = 1000;

    // read next packet and return -1 on error
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, timeout, priority);

    if (ret == MSGQ_TIMEOUT)
      return DECODE_FLAG_TIMEOUT;

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT)
      return DECODE_FLAG_ABORT;

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      m_decode.Attach((CDVDMsgDemuxerPacket*)pMsg);
      m_ptsInput.Add( m_decode.size, m_decode.dts );
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_STREAMCHANGE))
    {
      CDVDMsgGeneralStreamChange* pMsgStreamChange = (CDVDMsgGeneralStreamChange*)pMsg;
      CDVDStreamInfo* hints = pMsgStreamChange->GetStreamInfo();

      // Received a stream change. 
      // Close the current audio path and store new stream hints until the next packet arrives
      CloseAudioPath();
      m_nextstream = *hints;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      ((CDVDMsgGeneralSynchronize*)pMsg)->Wait( &m_bStop, SYNCSOURCE_AUDIO );
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_SYNCHRONIZE");
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESYNC))
    { //player asked us to set internal clock
      CDVDMsgGeneralResync* pMsgGeneralResync = (CDVDMsgGeneralResync*)pMsg;

      if (pMsgGeneralResync->m_timestamp != DVD_NOPTS_VALUE)
        m_audioClock = pMsgGeneralResync->m_timestamp;

      m_ptsOutput.Add(m_audioClock, m_dvdAudio.GetDelay(), 0);
      if (pMsgGeneralResync->m_clock)
      {
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 1)", m_audioClock);
        m_pClock->Discontinuity(CLOCK_DISC_NORMAL, m_ptsOutput.Current(), 0);
      }
      else
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_RESYNC(%f, 0)", m_audioClock);
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      ResetAudioPath();
      m_decode.Release();
      m_started = false;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
    {
      m_dvdAudio.Flush();
      m_ptsOutput.Flush();
      m_ptsInput.Flush();
      m_resampler.Flush();
      m_syncclock = true;
      m_stalled   = true;
      m_started   = false;

      ResetAudioPath();

      m_decode.Release();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_EOF))
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_EOF");
      m_dvdAudio.Finish();
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_DELAY))
    {
      if (m_speed != DVD_PLAYSPEED_PAUSE)
      {
        double timeout = static_cast<CDVDMsgDouble*>(pMsg)->m_value;

        CLog::Log(LOGDEBUG, "CDVDPlayerAudio - CDVDMsg::GENERAL_DELAY(%f)", timeout);

        timeout *= (double)DVD_PLAYSPEED_NORMAL / abs(m_speed);
        timeout += CDVDClock::GetAbsoluteClock();

        while(!m_bStop && CDVDClock::GetAbsoluteClock() < timeout)
          Sleep(1);
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;

      if (m_speed == DVD_PLAYSPEED_PAUSE)
      {
        m_ptsOutput.Flush();
        m_resampler.Flush();
        m_syncclock = true;
        m_dvdAudio.Pause();
      }
      else
        m_dvdAudio.Resume();
    }
    pMsg->Release();
  }
  return 0;
}

void CDVDPlayerAudio::OnStartup()
{
  CThread::SetName("CDVDPlayerAudio");

  m_decode.msg = NULL;
  m_decode.Release();

  g_dvdPerformanceCounter.EnableAudioDecodePerformance(ThreadHandle());
}

void CDVDPlayerAudio::Process()
{
  CLog::Log(LOGNOTICE, "running thread: CDVDPlayerAudio::Process()");

  int result;
  bool packetadded(false);

  DVDAudioFrame audioframe;
  m_audioStats.Start();

  while (!m_bStop)
  {
    result = DecodeFrame(audioframe, m_speed > DVD_PLAYSPEED_NORMAL || m_speed < 0); // blocks if no audio is available, but leaves critical section before doing so

    if( result & DECODE_FLAG_ERROR )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Decode Error");
      continue;
    }

    if( result & DECODE_FLAG_TIMEOUT )
    {
      m_stalled = true;
      continue;
    }

    if( result & DECODE_FLAG_ABORT )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio::Process - Abort received, exiting thread");
      break;
    }

#ifdef PROFILE /* during profiling we just drop all packets, after having decoded */
    m_pClock->Discontinuity(CLOCK_DISC_NORMAL, audioframe.pts, 0);
    continue;
#endif

    if( audioframe.size == 0 )
      continue;

    packetadded = true;

    if( result & DECODE_FLAG_DROP )
    {
      //frame should be dropped. Don't let audio move ahead of the current time though
      //we need to be able to start playing at any time
      //when playing backwords, we try to keep as small buffers as possible

      if(m_droptime == 0.0)
        m_droptime = m_pClock->GetAbsoluteClock();
      if(m_speed > 0)
        m_droptime += audioframe.duration * DVD_PLAYSPEED_NORMAL / m_speed;
      while( !m_bStop && m_droptime > m_pClock->GetAbsoluteClock() ) Sleep(1);

      m_stalled = false;
    }
    else // Render frame
    {
      m_droptime = 0.0;

      SetSyncType(m_OutputFormat.streamType == DVDAudioStreamType_Encoded);

      // Handle a/v sync    
      if (m_synctype == SYNC_DISCON)
      {
        OutputFrame(audioframe);
        packetadded = true;
      }
      else if (m_synctype == SYNC_SKIPDUP)
      {
        if (m_skipdupcount < 0)
        {
          m_prevskipped = !m_prevskipped;
          if (!m_prevskipped)
          {
            OutputFrame(audioframe);
            m_skipdupcount++;
          }
        }
        else if (m_skipdupcount > 0)
        {
          OutputFrame(audioframe, 1);
          m_skipdupcount--;
        }
        else if (m_skipdupcount == 0)
        {
          OutputFrame(audioframe);
        }
        packetadded = true;
      }
      else if (m_synctype == SYNC_RESAMPLE)
      {
        double proportional = 0.0, proportionaldiv;
        
        //on big errors use more proportional
        if (fabs(m_error / DVD_TIME_BASE) > 0.0)
        {
          proportionaldiv = PROPORTIONAL * (PROPREF / fabs(m_error / DVD_TIME_BASE));
          if (proportionaldiv < PROPDIVMIN) proportionaldiv = PROPDIVMIN;
          else if (proportionaldiv > PROPDIVMAX) proportionaldiv = PROPDIVMAX;
          
          proportional = m_error / DVD_TIME_BASE / proportionaldiv;
        }
        m_resampler.SetRatio(1.0 / g_VideoReferenceClock.GetSpeed() + proportional + m_integral);
        
        //add to the resampler
        m_resampler.Add(audioframe, audioframe.pts);
        //give any packets from the resampler to the audiorenderer
        bool packetadded = false;
        while(m_resampler.Retrieve(audioframe, audioframe.pts))
        {
          OutputFrame(audioframe);
          packetadded = true;
        }
      }      

      // we are not running until something is cached in output device
      if(m_stalled && m_dvdAudio.GetCacheTime() > 0.0)
        m_stalled = false;
    }

    // store the delay for this pts value so we can calculate the current playing
    if(packetadded)
    {
      if(m_speed == DVD_PLAYSPEED_PAUSE)
        m_ptsOutput.Add(audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration, 0);
      else
        m_ptsOutput.Add(audioframe.pts, m_dvdAudio.GetDelay() - audioframe.duration, audioframe.duration);
    }

    // signal to our parent that we have initialized
    if(m_started == false)
    {
      m_started = true;
      m_messageParent.Put(new CDVDMsgInt(CDVDMsg::PLAYER_STARTED, DVDPLAYER_AUDIO));
    }

    if( m_ptsOutput.Current() == DVD_NOPTS_VALUE )
      continue;

    if( m_speed != DVD_PLAYSPEED_NORMAL )
      continue;

    if (packetadded)
      HandleSyncError(audioframe.duration);
  }
}

void CDVDPlayerAudio::SetSyncType(bool passthrough)
{
  //set the synctype from the gui
  //use skip/duplicate when resample is selected and passthrough is on
  m_synctype = m_setsynctype;
  if (passthrough && m_synctype == SYNC_RESAMPLE)
    m_synctype = SYNC_SKIPDUP;

  //tell dvdplayervideo how much it can change the speed
  //if SetMaxSpeedAdjust returns false, it means no video is played and we need to use clock feedback
  double maxspeedadjust = 0.0;
  if (m_synctype == SYNC_RESAMPLE)
    maxspeedadjust = m_maxspeedadjust;

  if (!m_pClock->SetMaxSpeedAdjust(maxspeedadjust))
    m_synctype = SYNC_DISCON;

  if (m_synctype != m_prevsynctype)
  {
    const char *synctypes[] = {"clock feedback", "skip/duplicate", "resample", "invalid"};
    int synctype = (m_synctype >= 0 && m_synctype <= 2) ? m_synctype : 3;
    CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: synctype set to %i: %s", m_synctype, synctypes[synctype]);
    m_prevsynctype = m_synctype;
  }
}

void CDVDPlayerAudio::HandleSyncError(double duration)
{
  double clock = m_pClock->GetClock();
  double error = m_ptsOutput.Current() - clock;
  int64_t now;

  if( fabs(error) > DVD_MSEC_TO_TIME(100) || m_syncclock )
  {
    m_pClock->Discontinuity(CLOCK_DISC_NORMAL, clock+error, 0);
    if(m_speed == DVD_PLAYSPEED_NORMAL)
      CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuity - was:%f, should be:%f, error:%f", clock, clock+error, error);

    m_errorbuff = 0;
    m_errorcount = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_syncclock = false;
    m_errortime = CurrentHostCounter();

    return;
  }

  if (m_speed != DVD_PLAYSPEED_NORMAL)
  {
    m_errorbuff = 0;
    m_errorcount = 0;
    m_integral = 0;
    m_skipdupcount = 0;
    m_error = 0;
    m_resampler.Flush();
    m_errortime = CurrentHostCounter();
    return;
  }

  m_errorbuff += error;
  m_errorcount++;

  //check if measured error for 1 second
  now = CurrentHostCounter();
  if ((now - m_errortime) >= m_freq)
  {
    m_errortime = now;
    m_error = m_errorbuff / m_errorcount;

    m_errorbuff = 0;
    m_errorcount = 0;

    if (m_synctype == SYNC_DISCON && fabs(m_error) > DVD_MSEC_TO_TIME(10))
    {
      m_pClock->Discontinuity(CLOCK_DISC_NORMAL, clock+m_error, 0);
      if(m_speed == DVD_PLAYSPEED_NORMAL)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Discontinuity - was:%f, should be:%f, error:%f", clock, clock+m_error, m_error);
    }
    else if (m_synctype == SYNC_SKIPDUP && m_skipdupcount == 0 && fabs(m_error) > DVD_MSEC_TO_TIME(10))
    {
      //check how many packets to skip/duplicate
      m_skipdupcount = (int)(m_error / duration);
      //if less than one frame off, see if it's more than two thirds of a frame, so we can get better in sync
      if (m_skipdupcount == 0 && fabs(m_error) > duration / 3 * 2)
        m_skipdupcount = (int)(m_error / (duration / 3 * 2));

      if (m_skipdupcount > 0)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Duplicating %i packet(s) of %.2f ms duration",
                  m_skipdupcount, duration / DVD_TIME_BASE * 1000.0);
      else if (m_skipdupcount < 0)
        CLog::Log(LOGDEBUG, "CDVDPlayerAudio:: Skipping %i packet(s) of %.2f ms duration ",
                  m_skipdupcount * -1,  duration / DVD_TIME_BASE * 1000.0);
    }
    else if (m_synctype == SYNC_RESAMPLE)
    {
      //reset the integral on big errors, failsafe
      if (fabs(m_error) > DVD_TIME_BASE)
        m_integral = 0;
      else if (fabs(m_error) > DVD_MSEC_TO_TIME(5))
        m_integral += m_error / DVD_TIME_BASE / INTEGRAL;
    }
  }
}

bool CDVDPlayerAudio::OutputFrame(DVDAudioFrame &audioframe, int dupCount /*= 0*/)
{
  DVDAudioFrame outFrame;
  memset(&outFrame, 0, sizeof(DVDAudioFrame));
  
  switch (m_OutputFormat.streamType)
  {
    case DVDAudioStreamType_PCM:
    {
      // This is currently where any pcm post-processing would occur...
      
      if(!m_pConverter->AddFrame(audioframe))
        return false;
      
      outFrame.format = m_OutputFormat;
      outFrame.pts = audioframe.pts;
      m_pConverter->GetFrame(outFrame);
      // TODO: Validate frame
      outFrame.duration = audioframe.duration;
      break;
    }
    case DVDAudioStreamType_Encoded: // Passthrough
      outFrame = audioframe;
      break;
    default:
      return false;
  }

  // Render the frame data
  m_dvdAudio.AddPackets((const unsigned char*)outFrame.data, outFrame.size);
  
  // This is necessary to keep video sync code from forcing post-proc twice 
  if (dupCount)
    for (;dupCount > 0; dupCount--)
      m_dvdAudio.AddPackets((const unsigned char*)outFrame.data, outFrame.size);
      
  return true;
}

void CDVDPlayerAudio::OnExit()
{
  g_dvdPerformanceCounter.DisableAudioDecodePerformance();

  CLog::Log(LOGNOTICE, "thread end: CDVDPlayerAudio::OnExit()");
}

void CDVDPlayerAudio::SetSpeed(int speed)
{
  if(m_messageQueue.IsInited())
    m_messageQueue.Put( new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed), 1 );
  else
    m_speed = speed;
}

void CDVDPlayerAudio::Flush()
{
  m_messageQueue.Flush();
  m_messageQueue.Put( new CDVDMsg(CDVDMsg::GENERAL_FLUSH), 1);
}

void CDVDPlayerAudio::WaitForBuffers()
{
  // make sure there are no more packets available
  m_messageQueue.WaitUntilEmpty();

  // make sure almost all has been rendered
  // leave 500ms to around buffer underruns
  double delay = m_dvdAudio.GetCacheTime();
  if(delay > 0.5)
    Sleep((int)(1000 * (delay - 0.5)));
}

string CDVDPlayerAudio::GetPlayerInfo()
{
  std::ostringstream s;
  s << "aq:"     << setw(2) << min(99,m_messageQueue.GetLevel()) << "%";
  s << ", kB/s:" << fixed << setprecision(2) << (double)GetAudioBitrate() / 1024.0;
  return s.str();
}

int CDVDPlayerAudio::GetAudioBitrate()
{
  return (int)m_audioStats.GetBitrate();
}

bool CDVDPlayerAudio::IsPassthrough() const
{
  return (m_OutputFormat.streamType == DVDAudioStreamType_Encoded);
}
