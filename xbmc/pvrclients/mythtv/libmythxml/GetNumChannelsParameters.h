/*
 * GetNumChannelsParameters.h
 *
 *  Created on: Oct 8, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETNUMCHANNELSPARAMETERS_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETNUMCHANNELSPARAMETERS_H_

#include "GetProgramGuideParameters.h"

class GetNumChannelsParameters: public GetProgramGuideParameters {
public:
	GetNumChannelsParameters(): GetProgramGuideParameters(false) {};
	virtual ~GetNumChannelsParameters() {};
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETNUMCHANNELSPARAMETERS_H_ */
