/*
 * MythXmlCommand.cpp
 *
 *  Created on: Oct 7, 2010
 *      Author: mythtv
 */

#include "MythXmlCommand.h"

#include "FileSystem/FileCurl.h"

#include "utils/log.h"

using namespace XFILE;

MythXmlCommand::MythXmlCommand() {
}

MythXmlCommand::~MythXmlCommand() {
}

void MythXmlCommand::execute(const CStdString& hostname, int port, const MythXmlCommandParameters& params, MythXmlCommandResult& result, int timeout){
	CStdString strUrl = createRequestUrl(hostname, port, params);
	CStdString strXML;
	CFileCurl http;
	http.SetTimeout(timeout);
	if (http.Get(strUrl, strXML))
	{
		CLog::Log(LOGDEBUG, "Got response from mythtv backend: %s", strUrl.c_str());
	}
	http.Cancel();
	result.parseData(strXML);
}

CStdString MythXmlCommand::createRequestUrl(const CStdString& hostname, int port, const MythXmlCommandParameters& params){
	CStdString requestURL;
	requestURL.Format("http://%s:%i/Myth/%s", hostname.c_str(), port, getCommand().c_str());
	if(params.hasParameters()){
		requestURL += params.createParameterString();
	}
	return requestURL;
}
