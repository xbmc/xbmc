
#pragma once

class CDVDSubtitleParser;
class CDVDSubtitleStream;

typedef std::vector<std::string> VecSubtitleFiles;
typedef std::vector<std::string>::iterator VecSubtitleFilesIter;

class CDVDFactorySubtitle
{
public:
  static bool GetSubtitles(VecSubtitleFiles& vecSubtitles, const char* strFile);
  static CDVDSubtitleParser* CreateParser(CDVDSubtitleStream* pStream, const char* strFile);
};

