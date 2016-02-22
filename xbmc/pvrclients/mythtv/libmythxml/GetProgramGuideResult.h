#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETPROGRAMGUIDERESULT_H
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETPROGRAMGUIDERESULT_H

#include <map>

#include "MythXmlCommandResult.h"
#include "SEpg.h"

class GenreIdMapper;

class GetProgramGuideResult:public MythXmlCommandResult
{
public:
  GetProgramGuideResult();
  virtual ~GetProgramGuideResult();
  virtual void parseData(const CStdString& xmlData);
  inline const vector<SEpg>& getEpg() {return epg_;};

private:
  void initialize();
  vector<SEpg> epg_;
  static GenreIdMapper s_mapper_;
};

#endif // XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETPROGRAMGUIDERESULT_H
