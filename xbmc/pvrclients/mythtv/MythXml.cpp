/*
 * MythXml.cpp
 *
 *  Created on: Oct 7, 2010
 *      Author: tafypz
 */

#include "MythXml.h"

#include "FileSystem/FileCurl.h"
#include "utils/log.h"

#include "libmythxml/GetNumChannelsParameters.h"
#include "libmythxml/GetNumChannelsResult.h"
#include "libmythxml/GetNumChannelsCommand.h"
#include "libmythxml/GetChannelListCommand.h"
#include "libmythxml/GetChannelListParameters.h"
#include "libmythxml/GetChannelListResult.h"
#include "libmythxml/GetProgramGuideParameters.h"
#include "libmythxml/GetProgramGuideResult.h"
#include "libmythxml/GetProgramGuideCommand.h"


using namespace XFILE;

MythXml::MythXml() {
	hostname_ = "";
	port_ = -1;
	pin_ = -1;
	timeout_ = -1;
}

MythXml::~MythXml() {
}

void MythXml::init(){
}

void MythXml::cleanup(){
}

bool MythXml::open(CStdString hostname, int port, CStdString user, CStdString pass, int pin, long timeout){
	hostname_ = hostname;
	port_ = port;
	timeout_ = timeout;
	pin_ = pin;
	CStdString strUrl;
	strUrl.Format("http://%s:%i/Myth/GetConnectionInfo?Pin=%i", hostname.c_str(), port, pin);
	CStdString strXML;

	CFileCurl http;

	http.SetTimeout(timeout);
	if(!http.Get(strUrl, strXML)){
		CLog::Log(LOGDEBUG, "MythXml - Could not open connection to mythtv backend.");
		http.Cancel();
		return false;
	}
	http.Cancel();
	return true;
}

int MythXml::getNumChannels(){
	if(!checkConnection())
		return 0;
	GetNumChannelsCommand cmd;
	GetNumChannelsParameters params;
	GetNumChannelsResult result;
	cmd.execute(hostname_, port_, params, result, timeout_);
	return result.getNumberOfChannels();
}

PVR_ERROR MythXml::requestChannelList(PVRHANDLE handle, int radio){
  if(!checkConnection())
		return PVR_ERROR_SERVER_ERROR;
	GetChannelListCommand cmd;
	GetChannelListParameters params;
	GetChannelListResult result;
	cmd.execute(hostname_, port_, params, result, timeout_);
  
	if(!result.isSuccess())
	  return PVR_ERROR_UNKOWN;
	
	const vector<SChannel>& channellist = result.getChannels();
	vector<SChannel>::const_iterator it;
	PVR_CHANNEL tag;
	for( it = channellist.begin(); it != channellist.end(); ++it){
	  const SChannel& channel = *it; 
	  memset(&tag, 0 , sizeof(tag));
	  tag.uid           = channel.id;
	  tag.number        = channel.id;
	  tag.name          = channel.name.c_str();
	  tag.callsign      = channel.callsign.c_str();;
	  tag.radio         = false;
	  tag.input_format  = "";
	  tag.stream_url    = "";
	  tag.bouquet       = 0;

	  PVR->TransferChannelEntry(handle, &tag);
	}
	return PVR_ERROR_NO_ERROR;
}

PVR_ERROR MythXml::requestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end){
  if(!checkConnection())
		return PVR_ERROR_SERVER_ERROR;
	GetProgramGuideCommand cmd;
	GetProgramGuideParameters params(channel.uid, CDateTime(start), CDateTime(end), true);
	GetProgramGuideResult result;
	
	cmd.execute(hostname_, port_, params, result, timeout_);
  
	if(!result.isSuccess())
	  return PVR_ERROR_UNKOWN;
	
	PVR_PROGINFO guideItem;
	const vector<SEpg>& epgInfo = result.getEpg();
	vector<SEpg>::const_iterator it;
	for( it = epgInfo.begin(); it != epgInfo.end(); ++it)
	{
	  const SEpg& epg = *it;
	  time_t itemStart;
	  time_t itemEnd;
	  epg.start_time.GetAsTime(itemStart);
	  epg.end_time.GetAsTime(itemEnd);
	  
	  guideItem.channum         = epg.chan_num;
	  guideItem.uid             = epg.id;
	  guideItem.title           = epg.title;
	  guideItem.subtitle        = epg.subtitle;
	  guideItem.description     = epg.description;
	  guideItem.genre_type      = epg.genre_type;
	  guideItem.genre_sub_type  = epg.genre_subtype;
	  guideItem.parental_rating = epg.parental_rating;
	  guideItem.starttime       = itemStart;
	  guideItem.endtime         = itemEnd;
	  PVR->TransferEpgEntry(handle, &guideItem);
	}
	return PVR_ERROR_NO_ERROR;
}

bool MythXml::checkConnection(){
	return true;
}
