/*
 * GetChannelListParameters.h
 *
 *  Created on: Oct 8, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETCHANNELLISTPARAMETERS_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETCHANNELLISTPARAMETERS_H_

#include "GetProgramGuideParameters.h"

class GetChannelListParameters: public GetProgramGuideParameters {
public:
	GetChannelListParameters() : GetProgramGuideParameters(false) {};
	virtual ~GetChannelListParameters(){};
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETCHANNELLISTPARAMETERS_H_ */
