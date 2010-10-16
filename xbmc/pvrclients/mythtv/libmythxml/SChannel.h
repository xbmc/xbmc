#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_SCHANNEL_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_SCHANNEL_H_

#include "../StdString.h"

struct SChannel
{
  int             id;
  CStdString      name;
  CStdString      callsign;
  CStdString      number;

  SChannel() {
	id = 0;
  }
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_SCHANNEL_H_ */
