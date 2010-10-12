/*
 * MythXmlCommandResult.h
 *
 *  Created on: Oct 7, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMANDRESULT_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMANDRESULT_H_

#include <vector>

#include "../StdString.h"

using std::vector;
class MythXmlCommandResult {
public:
	MythXmlCommandResult(){};
	virtual ~MythXmlCommandResult(){};
	inline bool isSuccess() const { return errors_.empty(); };
	inline vector<CStdString> getErrors() const {return errors_;};
	virtual void parseData(const CStdString& xmlData) = 0;
protected:
	vector<CStdString> errors_;
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_MYTHXMLCOMMANDRESULT_H_ */
