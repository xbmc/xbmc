/*
 * GetNumChannelsCommand.h
 *
 *  Created on: Oct 7, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETNUMCHANNELSCOMMAND_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETNUMCHANNELSCOMMAND_H_

#include "MythXmlCommand.h"

class GetNumChannelsCommand: public MythXmlCommand {
public:
	GetNumChannelsCommand(){};
	virtual ~GetNumChannelsCommand(){};
protected:
	virtual const CStdString& getCommand() const{
		static CStdString result = "GetProgramGuide";
		return result;
	}
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETNUMCHANNELSCOMMAND_H_ */
