/*
 * GetNumChannelsResult.h
 *
 *  Created on: Oct 7, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETNUMCHANNELSRESULT_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETNUMCHANNELSRESULT_H_

#include "MythXmlCommandResult.h"

class GetNumChannelsResult: public MythXmlCommandResult {
public:
	GetNumChannelsResult();
	virtual ~GetNumChannelsResult();
	virtual void parseData(const CStdString& xmlData);
	inline int getNumberOfChannels() const {return numberOfChannels_;};
private:
	int numberOfChannels_;
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETNUMCHANNELSRESULT_H_ */
