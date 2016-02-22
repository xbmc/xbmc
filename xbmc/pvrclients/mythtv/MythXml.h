/*
 * MythXml.h
 *
 *  Created on: Oct 7, 2010
 *      Author: tafypz
 */

#ifndef XBMC_PVRCLIENTS_MYTHTV_MYTHXML_H_
#define XBMC_PVRCLIENTS_MYTHTV_MYTHXML_H_

#include "client.h"

/*! \class MythXml
	\brief Acts as the glue between the PVR Addon world and the mythXML world.
 */
class MythXml {
public:
	MythXml();
	virtual ~MythXml();
	void init();
	void cleanup();
	bool open(CStdString hostname, int port, CStdString user, CStdString pass, int pin, long timeout);
	int getNumChannels();
	PVR_ERROR requestChannelList(PVR_HANDLE handle, bool bRadio);
	PVR_ERROR requestEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
private:
	bool checkConnection();
	CStdString hostname_;
	int port_;
	int timeout_;
	int pin_;
};

#endif /* XBMC_PVRCLIENTS_MYTHTV_MYTHXML_H_ */
