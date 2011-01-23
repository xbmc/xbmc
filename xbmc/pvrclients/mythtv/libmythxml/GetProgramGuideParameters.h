/*
 * GetProgramGuideParameters.h
 *
 *  Created on: Oct 8, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETPROGRAMGUIDEPARAMETERS_H_
#define XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETPROGRAMGUIDEPARAMETERS_H_

#include "DateTime.h"

#include "MythXmlCommandParameters.h"

class GetProgramGuideParameters: public MythXmlCommandParameters {
public:
	GetProgramGuideParameters(int channelid, CDateTime starttime, CDateTime endtime, bool retrieveDetails);
	virtual ~GetProgramGuideParameters();

	virtual CStdString createParameterString() const;
	inline const CDateTime& get_starttime() const {return starttime_;};
	inline const CDateTime& get_endtime() const {return endtime_;};
	inline void set_channelid(int channelid) {channelid_ = channelid;};
	inline int get_channelid() const { return channelid_;};
	inline void set_retrievedetailsflag(bool retrievedetails) {retrieveDetails_ = retrievedetails;};
	inline bool get_retrievedetailsflag() const {return retrieveDetails_;};

protected:
	GetProgramGuideParameters(bool retrieveDetails);

private:
	CDateTime starttime_;
	CDateTime endtime_;
	int channelid_;
	bool retrieveDetails_;
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_LIBMYTHXML_GETPROGRAMGUIDEPARAMETERS_H_ */
