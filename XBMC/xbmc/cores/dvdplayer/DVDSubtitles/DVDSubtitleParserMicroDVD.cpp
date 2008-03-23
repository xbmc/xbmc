
#include "stdafx.h"
#include "DVDSubtitleParserMicroDVD.h"
#include "../DVDCodecs/Overlay/DVDOverlayText.h"
#include "../DVDClock.h"
#include "utils/RegExp.h"
#include "DVDStreamInfo.h"

using namespace std;

CDVDSubtitleParserMicroDVD::CDVDSubtitleParserMicroDVD(CDVDSubtitleStream* stream, const string& filename)
    : CDVDSubtitleParser(stream, filename)
{

}

CDVDSubtitleParserMicroDVD::~CDVDSubtitleParserMicroDVD()
{
  Dispose();
}

bool CDVDSubtitleParserMicroDVD::Open(CDVDStreamInfo &hints)
{
  if (!m_pStream->Open(m_strFileName))
    return false;

  CLog::Log(LOGERROR, "%s - framerate %d:%d", __FUNCTION__, hints.fpsrate, hints.fpsscale);
  if(hints.fpsscale > 0 && hints.fpsrate > 0)
    m_framerate = DVD_TIME_BASE * hints.fpsscale / hints.fpsrate;
  else
    m_framerate = DVD_TIME_BASE / 25.0;

  char line[1024];

  CRegExp reg;
  if (!reg.RegComp("\\{([0-9]+)\\}\\{([0-9]+)\\}([^|]*?)(\\|([^|]*?))?$"))//(\\|([^|]*?))?$"))
    assert(0);

  while (m_pStream->ReadLine(line, sizeof(line)))
  {
    if (reg.RegFind(line) > -1)
    {
      char* startFrame = reg.GetReplaceString("\\1");
      char* endFrame   = reg.GetReplaceString("\\2");
      char* lines[3];
      lines[0] = reg.GetReplaceString("\\3");
      lines[1] = reg.GetReplaceString("\\5");
      lines[2] = reg.GetReplaceString("\\7");
      CDVDOverlayText* pOverlay = new CDVDOverlayText();
      pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

      pOverlay->iPTSStartTime = m_framerate * atoi(startFrame);
      pOverlay->iPTSStopTime  = m_framerate * atoi(endFrame);

      for(int i=0;i<3 && lines[i];i++)
      {
        CStdStringW strUTF16;
        CStdStringA strUTF8;
        g_charsetConverter.subtitleCharsetToW(lines[i], strUTF16);
        g_charsetConverter.wToUTF8(strUTF16, strUTF8);
        if (strUTF8.IsEmpty())
          continue;
CLog::Log(LOGDEBUG, strUTF8);
        // add a new text element to our container
        pOverlay->AddElement(new CDVDOverlayText::CElementText(strUTF8.c_str()));
      }
      free(lines[0]);
      free(lines[1]);
      free(lines[2]);
      free(startFrame);
      free(endFrame);

      m_collection.Add(pOverlay);
    }
  }
  return true;
}

void CDVDSubtitleParserMicroDVD::Dispose()
{
  m_collection.Clear();
}

void CDVDSubtitleParserMicroDVD::Reset()
{
  m_collection.Reset();
}

// parse exactly one subtitle
CDVDOverlay* CDVDSubtitleParserMicroDVD::Parse(double iPts)
{
  return  m_collection.Get(iPts);;
}

