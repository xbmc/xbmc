/*
 * GetChannelListCommand.h
 *
 *  Created on: Oct 7, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETCHANNELLISTCOMMAND_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETCHANNELLISTCOMMAND_H_

#include "MythXmlCommand.h"

class GetChannelListCommand: public MythXmlCommand {

public:
	GetChannelListCommand() {};
	virtual ~GetChannelListCommand() {};
protected:
	virtual const CStdString& getCommand() const {
		static CStdString result = "GetProgramGuide";
		return result;
	};
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETCHANNELLISTCOMMAND_H_ */
