#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETCHANNELLISTRESULT_H
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETCHANNELLISTRESULT_H

#include "MythXmlCommandResult.h"
#include "SChannel.h"

class GetChannelListResult: public MythXmlCommandResult {
public:
  GetChannelListResult();
  virtual ~GetChannelListResult();
  virtual void parseData(const CStdString& xmlData);
  inline const vector<SChannel>& getChannels() {return channels_;};
private:
  vector<SChannel> channels_;
};

#endif // XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETCHANNELLISTRESULT_H
