
#include "stdafx.h"
#include "DVDSubtitleParserSami.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDClock.h"
#include "utils/RegExp.h"
#include "DVDStreamInfo.h"

using namespace std;

CDVDSubtitleParserSami::CDVDSubtitleParserSami(CDVDSubtitleStream* stream, const string& filename)
    : CDVDSubtitleParser(stream, filename)
{

}

CDVDSubtitleParserSami::~CDVDSubtitleParserSami()
{
  Dispose();
}

bool CDVDSubtitleParserSami::Open(CDVDStreamInfo &hints)
{
  if (!m_pStream->Open(m_strFileName))
    return false;

  char line[1024];
  char text[1024];

  CRegExp reg;
  if (!reg.RegComp("<SYNC START=([0-9]+)>"))
    assert(0);

  bool reuse=false;
  while (reuse || m_pStream->ReadLine(line, sizeof(line)))
  {
    if (reg.RegFind(line) > -1)
    {
      char* startFrame = reg.GetReplaceString("\\1");
      m_pStream->ReadLine(text,sizeof(text));
      m_pStream->ReadLine(line,sizeof(line));
      if (reg.RegFind(line) > -1)
      {
        char* endFrame   = reg.GetReplaceString("\\1");
      
        CDVDOverlayText* pOverlay = new CDVDOverlayText();
        pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay

        pOverlay->iPTSStartTime = atoi(startFrame)*DVD_TIME_BASE/1000; 
        pOverlay->iPTSStopTime  = atoi(endFrame)*DVD_TIME_BASE/1000;

        CStdStringW strUTF16;
        CStdStringA strUTF8;
        g_charsetConverter.subtitleCharsetToW(text, strUTF16);
        g_charsetConverter.wToUTF8(strUTF16, strUTF8);
        if (strUTF8.IsEmpty())
          continue;
        // add a new text element to our container
        pOverlay->AddElement(new CDVDOverlayText::CElementText(strUTF8.c_str()));
        reuse = true;
        free(endFrame);
      
        m_collection.Add(pOverlay);
      }
      free(startFrame);
    }
    else
      reuse = false;
  }

  return true;
}

void CDVDSubtitleParserSami::Dispose()
{
  m_collection.Clear();
}

void CDVDSubtitleParserSami::Reset()
{
  m_collection.Reset();
}

// parse exactly one subtitle
CDVDOverlay* CDVDSubtitleParserSami::Parse(double iPts)
{
  return  m_collection.Get(iPts);;
}

