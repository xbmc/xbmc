/*
 * GetProgramGuideParameters.cpp
 *
 *  Created on: Oct 8, 2010
 *      Author: tafypz
 */

#include "GetProgramGuideParameters.h"
#include "utils/TimeUtils.h"

GetProgramGuideParameters::GetProgramGuideParameters(bool retrieveDetails) : MythXmlCommandParameters(true){
	CDateTime now = CTimeUtils::GetLocalTime(time(NULL));
	channelid_ = -1;
	starttime_ = now;
	endtime_ = now;
	retrieveDetails_ = retrieveDetails_;
}

GetProgramGuideParameters::GetProgramGuideParameters(int channelid, CDateTime starttime, CDateTime endtime, bool retrieveDetails) : MythXmlCommandParameters(true){
	channelid_ = channelid;
	retrieveDetails_ = retrieveDetails;
	starttime_ = starttime;
	endtime_ = endtime;
}

GetProgramGuideParameters::~GetProgramGuideParameters() {
}

CStdString GetProgramGuideParameters::createParameterString() const{
	CStdString result = "?StartTime=%s&EndTime=%s&NumOfChannels=%i";
	CStdString start = MythXmlCommandParameters::convertTimeToString(starttime_);
	CStdString end = MythXmlCommandParameters::convertTimeToString(endtime_);
	int numChannels = 1;
	if(channelid_ == -1)
		numChannels = -1;
	result.Format(result.c_str(), start.c_str(), end.c_str(),numChannels);
	if(numChannels == 1){
		CStdString chanid = "&StartChanId=%i";
		chanid.Format(chanid.c_str(), channelid_);
		result += chanid;
	}
	return result;
};
