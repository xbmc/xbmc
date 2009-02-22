#ifndef XBMC_IPVRCLIENT_H
#define XBMC_IPVRCLIENT_H
#pragma once

/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include <vector>

#include "../utils/TVChannel.h"
#include "../../pvrclients/PVRClientTypes.h"

/**
* IPVRClientCallback Class
*/
class IPVRClientCallback
{
public:
  virtual void OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg)=0;
};


/**
* IPVRClient PVR Client control class
*/
class IPVRClient
{
public:
/***************************************/
/**_CLASS INTERFACE___________________**/

  /**
  * Constructor
  * \param long clientID    = Individual ID for the generated Client Class
  * \param *callback         = IPVRClientCallback callback to the PVRManager
  */
  IPVRClient(long clientID, IPVRClientCallback *callback){};

  /**
  * Destructor
  */
  virtual ~IPVRClient(){};

/***************************************/
/**_SERVER INTERFACE__________________**/

  /**
  * Get the current client ID registered at object initalization by PVRManager
  * \return long                = Client ID
  */
  virtual long GetID()=0;

  /**
  * Get the default connection properties
  * \return PVR_SERVERPROPS      = pointer to client properties struct
  */
  virtual PVR_ERROR GetProperties(PVR_SERVERPROPS *props)=0;

  /**
  * Connect to the PVR backend
  * \return PVR_ERROR            = Error code
  */
  virtual PVR_ERROR Connect()=0;

  /**
  * Disconnect from the PVR backend
  */
  virtual void Disconnect()=0;

  /**
  * Check if XBMC is connected to the PVR backend
  * \return bool                 = true = connected, false = disconnected
  */
  virtual bool IsUp()=0;

/****************************************/
/**_GENERAL INTERFACE__________________**/
  
  /**
  * Get a string with the backend name
  * \return std::string          = Backend name
  */
  virtual const std::string GetBackendName()=0;
  
  /**
  * Get a string with the backend version
  * \return std::string          = Backend version
  */
  virtual const std::string GetBackendVersion()=0;

  /**
  * Get the used and total diskspace for recordings available on the backend
  * \param long long ptr *total  = total available bytes
  * \param long long ptr *used   = number of bytes already used
  * \return PVR_ERROR            = Error code
  */
  virtual PVR_ERROR GetDriveSpace(long long *total, long long *used)=0;

/****************************************/
/**_BOUQUET INTERFACE__________________**/

  /**
  * Get number of bouquets available
  * \return int                  = number of bouquets (-1 if fails)
  */
  virtual int GetNumBouquets()=0;

  /**
  * Get bouquet information
  * \param unsigned int number   = bouquet number
  * \param PVR_BOUQUET info      = bouquet information
  * \return PVR_ERROR            = Error code
  */
  virtual PVR_ERROR GetBouquetInfo(const unsigned int number, PVR_BOUQUET& info)=0;

/****************************************/
/**_CHANNEL INTERFACE__________________**/

  /**
  * Get number of channels available
  * \return int                  = number of channels (-1 if fails)
  */
  virtual int GetNumChannels()=0;

  /**
  * Get all channels
  * \param VECCHANNELS *channels = list of channels available
  * \return PVR_ERROR            = Error code
  */
  virtual PVR_ERROR GetChannelList(VECCHANNELS &channels)=0;

/****************************************/
/**_ EPG INTERFACE ____________________**/

  /**
  * Get EPG information for specified channel
  * \param unsigned int number    = Channel number
  * \param PVR_CHANDATA *epg      = pointer to a new epg information structure
  * \param time_t start           = start of EPG range
  * \param time_t end             = end of EPG range
  * \return PVR_ERROR             = Error code
  */
  virtual PVR_ERROR GetEPGForChannel(const unsigned int number, PVR_PROGLIST *epg, time_t start = NULL, time_t end = NULL)=0;

  /**
  * Get 'Now' programme information
  * \param unsigned int number    = Channel number
  * \param CTVEPGInfoTag *result  = pointer to a new epg information structure
  * \return PVR_ERROR             = Error code
  */
  virtual PVR_ERROR GetEPGNowInfo(const unsigned int number, PVR_PROGINFO *result)=0;

  /**
  * Get 'Next' information
  * \param unsigned int number    = Channel number
  * \param CTVEPGInfoTag *result  = pointer to a new epg information structure
  * \return PVR_ERROR             = Error code
  */
  virtual PVR_ERROR GetEPGNextInfo(const unsigned int number, PVR_PROGINFO *result)=0;

  /**
  * Get end of available EPG 
  * \param unsigned int number    = Channel number
  * \param CFileItemList *results = pointer to a new epg information structure
  * \return PVR_ERROR             = Error code
  */
  virtual PVR_ERROR GetEPGDataEnd(time_t end)=0;

  /****************************************/
  /**_ TIMER INTERFACE __________________**/

  ///**
  //* Get number of active timers
  //* \return int                   = number of timers active (-1 if fails)
  //*/
  //virtual int GetNumTimers(void)=0;

  ///**
  //* Get a list of any timers that are available, including ones not yet finished
  //* \param VECTVTIMERS            = results VECTVTIMERS to be populated with list of timers
  //* \return bool                  = true if any timers found
  //*/
  //virtual PVR_ERROR GetAllTimers(VECTVTIMERS *results)=0;

  ///**
  //* Add a timer
  //* \param CTVTimerInfoTag        = pointer to a timer information with new data
  //* \return PVR_ERROR              = Error code
  //*/
  //virtual PVR_ERROR AddTimer(const CTVTimerInfoTag &timerinfo)=0;

  ///**
  //* Delete a timer
  //* \param unsigned int number    = timer number (equal to ID returned by GetAllTimers)
  //* \return PVR_ERROR              = Error code
  //*/
  //virtual PVR_ERROR DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force = false)=0;

  ///**
  //* Rename a timer
  //* \param unsigned int number    = timer number
  //* \param CStdString newname     = pointer to the new timer name
  //* \return PVR_ERROR              = Error code
  //*/
  //virtual PVR_ERROR RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname)=0;

  ///**
  //* Update a timer
  //* \param CTVTimerInfoTag        = pointer to a timer information with updated data
  //* \return PVR_ERROR              = Error code
  //*/
  //virtual PVR_ERROR UpdateTimer(const CTVTimerInfoTag &timerinfo)=0;

};

#endif /* XBMC_IPVRCLIENT_H */
