
#include "stdafx.h"
#include "DVDFactorySubtitle.h"

//#include "DVDSubtitleParserSpu.h"
#include "DVDSubtitleParserSubrip.h"
#include "Util.h"

bool CDVDFactorySubtitle::GetSubtitles(VecSubtitleFiles& vecSubtitles, string& strFile)
{
  CLog::Log(LOGINFO, "CDVDFactorySubtitle::GetSubtitles, searching subtitles");
  
  vecSubtitles.clear();
  
  std::string subtitlePrefix = _P("Z:\\subtitle.");
  //std::string filenameWithoutExtension = strFile;
  //int iExtension = filenameWithoutExtension.find_last_of(".");
  //if (iExtension > 0) filenameWithoutExtension.erase(iExtension + 1, filenameWithoutExtension.size());
  
  CStdString strExtensionCached;
  
  CUtil::CacheSubtitles(strFile, strExtensionCached);
  int iSize = strExtensionCached.size();
  int iStart = 0;
  
  while (iStart < iSize)
  {
    int iEnd = strExtensionCached.Find(" ", iStart);
    std::string strExtension = strExtensionCached.substr(iStart, iEnd-iStart);
    iStart = iEnd + 1;
    
    std::string subtitleFile = subtitlePrefix + strExtension;;
    vecSubtitles.push_back(_P(subtitleFile));
  }
  
  CLog::Log(LOGINFO, "CDVDFactorySubtitle::GetSubtitles, searching subtitles done");
  
  return true;
}

CDVDSubtitleParser* CDVDFactorySubtitle::CreateParser(CDVDSubtitleStream* pStream, string& strFile)
{
  char line[1024];
  int i;
  CDVDSubtitleParser* pParser = NULL;
  
  if(!pStream->Open(strFile))
    return false;

  for (int t = 0; !pParser && t < 256; t++)
  {
    if (pStream->ReadLine(line, sizeof(line)))
    {
   // from xine
   //   if ((sscanf (line, "{%d}{}", &i)==1) ||
   //       (sscanf (line, "{%d}{%d}", &i, &i)==2)) {
   //     this->uses_time=0;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "microdvd subtitle format detected\n");
   //     return FORMAT_MICRODVD;
   //   }

      if (sscanf(line, "%d:%d:%d,%d --> %d:%d:%d,%d", &i, &i, &i, &i, &i, &i, &i, &i) == 8)
      {
        pParser = new CDVDSubtitleParserSubrip(pStream, strFile.c_str());
      }

   //   if (sscanf (line, "%d:%d:%d.%d,%d:%d:%d.%d",     &i, &i, &i, &i, &i, &i, &i, &i)==8){
   //     this->uses_time=1;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "subviewer subtitle format detected\n");
   //     return FORMAT_SUBVIEWER;
   //   }

   //   if (sscanf (line, "%d:%d:%d,%d,%d:%d:%d,%d",     &i, &i, &i, &i, &i, &i, &i, &i)==8){
   //     this->uses_time=1;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "subviewer subtitle format detected\n");
   //     return FORMAT_SUBVIEWER;
   //   }

   //   if (strstr (line, "<SAMI>")) {
   //     this->uses_time=1; 
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "sami subtitle format detected\n");
   //     return FORMAT_SAMI;
   //   }
   //   if (sscanf (line, "%d:%d:%d:",     &i, &i, &i )==3) {
   //     this->uses_time=1;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "vplayer subtitle format detected\n");
   //     return FORMAT_VPLAYER;
   //   }
   //   /*
   //   * A RealText format is a markup language, starts with <window> tag,
   //   * options (behaviour modifiers) are possible.
   //   */
   //   if ( !strcasecmp(line, "<window") ) {
   //     this->uses_time=1;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "rt subtitle format detected\n");
   //     return FORMAT_RT;
   //   }
   //   if ((!memcmp(line, "Dialogue: Marked", 16)) || (!memcmp(line, "Dialogue: ", 10))) {
   //     this->uses_time=1; 
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "ssa subtitle format detected\n");
   //     return FORMAT_SSA;
   //   }
   //   if (sscanf (line, "%d,%d,\"%c", &i, &i, (char *) &i) == 3) {
   //     this->uses_time=0;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "pjs subtitle format detected\n");
   //     return FORMAT_PJS;
   //   }
   //   if (sscanf (line, "FORMAT=%d", &i) == 1) {
   //     this->uses_time=0; 
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "mpsub subtitle format detected\n");
   //     return FORMAT_MPSUB;
   //   }
   //   if (sscanf (line, "FORMAT=TIM%c", &p)==1 && p=='E') {
   //     this->uses_time=1; 
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "mpsub subtitle format detected\n");
   //     return FORMAT_MPSUB;
   //   }
   //   if (strstr (line, "-->>")) {
   //     this->uses_time=0; 
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "aqtitle subtitle format detected\n");
   //     return FORMAT_AQTITLE;
   //   }
   //   if (sscanf(line, "@%d @%d", &i, &i) == 2 ||
	  //sscanf(line, "%d:%d:%d.%d %d:%d:%d.%d", &i, &i, &i, &i, &i, &i, &i, &i) == 8) {
   //     this->uses_time = 1;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "jacobsub subtitle format detected\n");
   //     return FORMAT_JACOBSUB;
   //   }
   //   if (sscanf(line, "{T %d:%d:%d:%d",&i, &i, &i, &i) == 4) {
   //     this->uses_time = 1;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "subviewer 2.0 subtitle format detected\n");
   //     return FORMAT_SUBVIEWER2;
   //   }
   //   if (sscanf(line, "[%d:%d:%d]", &i, &i, &i) == 3) {
   //     this->uses_time = 1;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "subrip 0.9 subtitle format detected\n");
   //     return FORMAT_SUBRIP09;
   //   }
   // 
   //   if (sscanf (line, "[%d][%d]", &i, &i) == 2) {
   //     this->uses_time = 1;
   //     xprintf (this->stream->xine, XINE_VERBOSITY_DEBUG, "mpl2 subtitle format detected\n");
   //     return FORMAT_MPL2;
   //   }
    }
    else
    {
      break;
    }
  }
  pStream->Close();
  
  return pParser;
}

