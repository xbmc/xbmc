#include "GetChannelListResult.h"
#include <stdlib.h>

#include "tinyXML/tinyxml.h"
#include "utils/log.h"

GetChannelListResult::GetChannelListResult() {
}

GetChannelListResult::~GetChannelListResult() {
}

void GetChannelListResult::parseData(const CStdString& xmlData) {
	TiXmlDocument xml;
	xml.Parse(xmlData.c_str(), 0, TIXML_ENCODING_LEGACY);

	TiXmlElement* rootXmlNode = xml.RootElement();

	if (!rootXmlNode) {
		errors_.push_back(" No root node parsed");
		CLog::Log(LOGDEBUG, "MythXML GetChannelListResult - No root node parsed");
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

	TiXmlElement* programGuideNode = programGuideResponseNode->FirstChildElement("ProgramGuide");
	TiXmlElement* channelsNode = programGuideNode->FirstChildElement("Channels");
	TiXmlElement* child = NULL;
	for( child = channelsNode->FirstChildElement("Channel"); child; child = child->NextSiblingElement("Channel")){
	  SChannel channel;
      channel.id = atoi(child->Attribute("chanId"));
	  channel.name = child->Attribute("channelName");
	  channel.callsign = child->Attribute("callSign");
	  channel.number = child->Attribute("chanNum");
	  channels_.push_back(channel);
	}
}
