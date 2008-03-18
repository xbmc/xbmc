
#pragma once

class CDVDSubtitleParser;
class CDVDSubtitleStream;

typedef std::vector<std::string> VecSubtitleFiles;
typedef std::vector<std::string>::iterator VecSubtitleFilesIter;

class CDVDFactorySubtitle
{
public:
  static bool GetSubtitles(VecSubtitleFiles& vecSubtitles, std::string& strFile);
  static CDVDSubtitleParser* CreateParser(std::string& strFile);
};

