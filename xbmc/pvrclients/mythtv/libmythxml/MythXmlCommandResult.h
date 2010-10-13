/*
 * MythXmlCommandResult.h
 *
 *  Created on: Oct 7, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMANDRESULT_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMANDRESULT_H_

#include <vector>
#include "DateTime.h"
#include "../StdString.h"

using std::vector;
class MythXmlCommandResult {
public:
	static CDateTime convertTimeStringToObject(const CStdString& time)
	{
	  int year = 0, month = 0, day = 0;
	  int hour = 0, minute = 0, second = 0;
	  sscanf(time.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);
	  return CDateTime(year, month, day,hour,minute, second);
	}
	
	MythXmlCommandResult(){};
	virtual ~MythXmlCommandResult(){};
	inline bool isSuccess() const { return errors_.empty(); };
	inline vector<CStdString> getErrors() const {return errors_;};
	virtual void parseData(const CStdString& xmlData) = 0;
protected:
	vector<CStdString> errors_;
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMANDRESULT_H_ */
