#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_SEPG_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_SEPG_H_

#include "../StdString.h"
#include "XBDateTime.h"

struct SEpg
{
  int             id;
  int             chan_num;
  int             genre_type;
  int             genre_subtype;
  int             parental_rating;
  CStdString      title;
  CStdString      subtitle;
  CStdString      description;
  CDateTime       start_time;
  CDateTime       end_time;
  
  SEpg() {
	id = 0;
	chan_num = 0;
	genre_type = 0;
	genre_subtype = 0;
	parental_rating = 0;
  }
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_SEPG_H_ */
