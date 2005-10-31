#include "../../../../stdafx.h"
#include "DVDVideoPPFFmpeg.h"

#define uint8_t unsigned char
#define int8_t char

#include "..\..\ffmpeg\postprocess.h"


CDVDVideoPPFFmpeg::CDVDVideoPPFFmpeg(EPPTYPE mType)
{
  m_eType = mType;
  m_pMode = m_pContext = NULL;
  m_pSource = m_pTarget = NULL;
  m_iInitWidth = m_iInitHeight = 0;
  memset(&m_FrameBuffer, 0, sizeof(DVDVideoPicture));
}
CDVDVideoPPFFmpeg::~CDVDVideoPPFFmpeg()
{
  Dispose();
}
void CDVDVideoPPFFmpeg::Dispose()
{
  if (m_pMode)
  {
    m_dll.pp_free_mode(m_pMode);
    m_pMode = NULL;
  }
  if(m_pContext)
  {
    m_dll.pp_free_context(m_pContext);
    m_pContext = NULL;
  }
  
  if( m_FrameBuffer.iFlags & DVP_FLAG_ALLOCATED )
  {
    for( int i = 0; i<4; i++ )
    {
      if( m_FrameBuffer.data[i] )
      {
        delete m_FrameBuffer.data[i];
        m_FrameBuffer.data[i] = NULL;
        m_FrameBuffer.iLineSize[i] = 0;
      }
    }
    m_FrameBuffer.iFlags &= ~DVP_FLAG_ALLOCATED;
  }

  m_iInitWidth = 0;
  m_iInitHeight = 0;

  m_dll.Unload();
}

bool CDVDVideoPPFFmpeg::CheckInit(int iWidth, int iHeight)
{
  if (!m_dll.IsLoaded() && !m_dll.Load()) return false;
  
  if(m_iInitWidth != iWidth || m_iInitHeight != iHeight)
  {
    if(m_pContext || m_pMode)
    {
      Dispose();
    }

    m_pContext = m_dll.pp_get_context(m_pSource->iWidth, m_pSource->iHeight, PP_CPU_CAPS_MMX | PP_CPU_CAPS_MMX2 | PP_FORMAT_420);

    m_iInitWidth = m_pSource->iWidth;
    m_iInitHeight = m_pSource->iHeight;

    switch(m_eType)
    {
    case ED_DEINT_FFMPEG:
      m_pMode = m_dll.pp_get_mode_by_name_and_quality("ffmpegdeint", PP_QUALITY_MAX);
      break;
    case ED_DEINT_CUBICIPOL:
      m_pMode = m_dll.pp_get_mode_by_name_and_quality("cubicipoldeint", PP_QUALITY_MAX);
      break;
    case ED_DEINT_LINBLEND:
      m_pMode = m_dll.pp_get_mode_by_name_and_quality("linblenddeint", PP_QUALITY_MAX);
      break;
    default:
      Dispose();
      break;
    }
  }


  if(m_pMode) 
    return true;
  else
    return false;
}


void CDVDVideoPPFFmpeg::Process(DVDVideoPicture* pPicture)
{


  m_pSource =  pPicture;

  if( !CheckInit(m_pSource->iWidth, m_pSource->iHeight) )
  {
    CLog::Log(LOGERROR, "Initialization of ffmpeg postprocessing failed");
    return;
  }

  //If no target was set or we are using internal buffer, make sure it's correctly sized
  if(m_pTarget == &m_FrameBuffer || !m_pTarget)
  {
    if(CheckFrameBuffer(m_pSource))
      m_pTarget = &m_FrameBuffer;
    else
    {
      m_pTarget = NULL;
      return;
    }
  }

  m_dll.pp_postprocess(m_pSource->data, m_pSource->iLineSize, 
                m_pTarget->data, m_pTarget->iLineSize,        
                m_pSource->iWidth, m_pSource->iHeight,
                0, 0,
                m_pMode, m_pContext, 
                PP_PICT_TYPE_QP2); //m_pSource->iFrameType);

  //Copy frame information over to target, but make sure it is set as allocated should decoder have forgotten
  m_pTarget->iFlags = m_pSource->iFlags | DVP_FLAG_ALLOCATED;
  m_pTarget->iFrameType = m_pSource->iFrameType;
  m_pTarget->iRepeatPicture = m_pSource->iRepeatPicture;;
  m_pTarget->iDuration = m_pSource->iDuration;
  m_pTarget->iDisplayHeight = m_pSource->iDisplayHeight;
  m_pTarget->iDisplayWidth = m_pSource->iDisplayWidth;
  m_pTarget->pts = m_pSource->pts;


}
 


bool CDVDVideoPPFFmpeg::CheckFrameBuffer(const DVDVideoPicture* pSource)
{
  if( m_FrameBuffer.iFlags & DVP_FLAG_ALLOCATED && (m_FrameBuffer.iWidth != pSource->iWidth || m_FrameBuffer.iHeight != pSource->iHeight))
  {
    m_FrameBuffer.iFlags &= ~DVP_FLAG_ALLOCATED;
    for(int i = 0;i<3;i++)
      if(m_FrameBuffer.data[i])
      {
        delete[] m_FrameBuffer.data[i];
        m_FrameBuffer.data[i] = NULL;
      }
  }

  if(!(m_FrameBuffer.iFlags & DVP_FLAG_ALLOCATED))
  {
    memset(&m_FrameBuffer, 0, sizeof(DVDVideoPicture));

    m_FrameBuffer.iLineSize[0] = pSource->iLineSize[0];   //Y
    m_FrameBuffer.iLineSize[1] = pSource->iLineSize[1]; //U
    m_FrameBuffer.iLineSize[2] = pSource->iLineSize[2]; //V
    m_FrameBuffer.iLineSize[3] = 0;

    m_FrameBuffer.iWidth = pSource->iWidth;
    m_FrameBuffer.iHeight = pSource->iHeight;
    
    unsigned int iPixels = pSource->iWidth*pSource->iHeight;

    m_FrameBuffer.data[0] = new BYTE[iPixels];    //Y
    m_FrameBuffer.data[1] = new BYTE[iPixels/4];  //U
    m_FrameBuffer.data[2] = new BYTE[iPixels/4];  //V
  
    if( !m_FrameBuffer.data[0] || !m_FrameBuffer.data[1] || !m_FrameBuffer.data[2])
    {
      CLog::Log(LOGERROR, "CDVDVideoDeinterlace::AllocBufferOfType - Unable to allocate framebuffer, bailing");
      return false;
    }

    //Set all data to 0 for less artifacts.. hmm.. what is black in YUV??
    memset( m_FrameBuffer.data[0], 0, iPixels );
    memset( m_FrameBuffer.data[1], 0, iPixels/4 );
    memset( m_FrameBuffer.data[2], 0, iPixels/4 );
    m_FrameBuffer.iFlags |= DVP_FLAG_ALLOCATED;
  }

  return true;
}


bool CDVDVideoPPFFmpeg::GetPicture(DVDVideoPicture* pPicture)
{
  if( m_pTarget )
  {
    memmove(pPicture, m_pTarget, sizeof(DVDVideoPicture));
    return true;
  }
  return false;
}