/*
 * GetNumChannelsResult.cpp
 *
 *  Created on: Oct 7, 2010
 *      Author: tafypz
 */

#include "GetNumChannelsResult.h"
#include <stdlib.h>

#include "tinyXML/tinyxml.h"
#include "utils/log.h"

GetNumChannelsResult::GetNumChannelsResult() {
	numberOfChannels_ = 0;
}

GetNumChannelsResult::~GetNumChannelsResult() {
}

void GetNumChannelsResult::parseData(const CStdString& xmlData) {
	TiXmlDocument xml;
	xml.Parse(xmlData.c_str(), 0, TIXML_ENCODING_LEGACY);

	TiXmlElement* rootXmlNode = xml.RootElement();

	if (!rootXmlNode) {
		errors_.push_back(" No root node parsed");
		CLog::Log(LOGDEBUG, "MythXML GetNumChannelsResult - No root node parsed");
	    return;
	}

	TiXmlElement* programGuideResponseNode = NULL;
	CStdString strValue = rootXmlNode->Value();
	if (strValue.Find("GetProgramGuideResponse") >= 0 ) {
		programGuideResponseNode = rootXmlNode;
	}
	else if (strValue.Find("detail") >= 0 ) {
		// process the error.
		TiXmlElement* errorCodeXmlNode = rootXmlNode->FirstChildElement("errorCode");
		TiXmlElement* errorDescXmlNode = rootXmlNode->FirstChildElement("errorDescription");
		CStdString error;
		error.Format("ErrorCode [%i] - %s", errorCodeXmlNode->GetText(), errorDescXmlNode->GetText());
		errors_.push_back(error);
		return;
	 }
	else
		return;

	TiXmlElement* numOfChannelsXmlNode = programGuideResponseNode->FirstChildElement("NumOfChannels");
	CStdString val = numOfChannelsXmlNode->GetText();
	int numberOfChannels = atoi(val.c_str());
	numberOfChannels_ = numberOfChannels;
}
