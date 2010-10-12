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
	for( it = channellist.begin(); it != channellist.end(); ++it){
	  const SChannel& channel = *it;
	  
	  PVR_CHANNEL tag;
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
	return PVR_ERROR_NOT_IMPLEMENTED;
}

bool MythXml::checkConnection(){
	return true;
}
