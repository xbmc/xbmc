#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include "threads/Thread.h"

class CURL;
class TiXmlElement;
class CFileItem;
class CFileItemList;

struct STREAMINFO
{
  std::string frontend;
  std::string service_name;
  std::string service_reference;
  std::string provider;
  std::string vpid;
  std::string apid;
  std::string pcrpid;
  std::string tpid;
  std::string tsid;
  std::string onid;
  std::string sid;
  std::string pmt;
  std::string video_format;
  std::string supported_crypt_systems;
  std::string used_crypt_systems;
  std::string satellite;
  std::string frequency;
  std::string symbol_rate;
  std::string polarisation;
  std::string inversion;
  std::string fec;
  std::string snr;
  std::string agc;
  std::string ber;
  std::string lock;
  std::string sync;
};
struct VIDEOSUBCHANNEL
{
  std::vector<std::string> reference;
  std::vector<std::string> name;
  std::vector<std::string> selected;
  std::string current_name;
  bool mode;
};
typedef struct AUDIOCHANNEL
{
  std::string pid;
  std::string selected;
  std::string name;
} sAudioChannel;
struct CURRENTSERVICEDATA
{
  std::string service_name;
  std::string service_reference;
  std::vector<AUDIOCHANNEL> audio_channels;
  int requested_audio_channel;
  std::string audio_track;
  std::string current_event_date;
  std::string current_event_time;
  std::string current_event_start;
  std::string current_event_duration;
  std::string current_event_description;
  std::string current_event_details;
  std::string next_event_date;
  std::string next_event_time;
  std::string next_event_start;
  std::string next_event_duration;
  std::string next_event_description;
  std::string next_event_details;
};
struct BOXSTATUS
{
  std::string current_time;
  std::string standby;
  std::string recording;
  std::string mode;
  std::string ip;
};
struct BOXSINFO
{
  std::string image_version;
  std::string image_url;
  std::string image_comment;
  std::string image_catalog;
  std::string firmware;
  std::string fpfirmware;
  std::string webinterface;
  std::string model;
  std::string manufacturer;
  std::string processor;
  std::string usbstick;
  std::string disk;
};
struct SERVICE_EPG
{
  std::string service_reference;
  std::string service_name;
  std::string image_comment;
  std::string event;
  std::string date;
  std::string time;
  std::string duration;
  std::string descritption;
  std::string genre;
  std::string genrecategory;
  std::string start;
  std::string details;
};
struct ZAPSTREAM
{
  bool initialized;
  bool available;
};
class CTuxBoxUtil
{
  public:
    STREAMINFO sStrmInfo;
    CURRENTSERVICEDATA sCurSrvData;
    BOXSTATUS sBoxStatus;
    BOXSINFO sBoxInfo;
    SERVICE_EPG sServiceEPG;
    VIDEOSUBCHANNEL vVideoSubChannel;
    ZAPSTREAM sZapstream;

    CTuxBoxUtil(void);
    virtual ~CTuxBoxUtil(void);

    bool GetZapUrl(const std::string& strPath, CFileItem &items);
    static bool ParseBouquets(TiXmlElement *root, CFileItemList &items, CURL &url, std::string strFilter, std::string strChild);
    static bool ParseBouquetsEnigma2(TiXmlElement *root, CFileItemList &items, CURL &url, std::string& strFilter, std::string& strChild);
    static bool ParseChannels(TiXmlElement *root, CFileItemList &items, CURL &url, std::string strFilter, std::string strChild);
    static bool ParseChannelsEnigma2(TiXmlElement *root, CFileItemList &items, CURL &url, std::string& strFilter, std::string& strChild);
    bool ZapToUrl(CURL url, const std::string &pathOption);
    bool StreamInformations(TiXmlElement *pRootElement);
    bool CurrentServiceData(TiXmlElement *pRootElement);
    bool BoxStatus(TiXmlElement *pRootElement);
    bool BoxInfo(TiXmlElement *pRootElement);
    bool ServiceEPG(TiXmlElement *pRootElement);
    bool GetHttpXML(CURL url,std::string strRequestType);
    bool GetGUIRequestedAudioChannel(AUDIOCHANNEL& sRequestedAC);
    bool GetRequestedAudioChannel(AUDIOCHANNEL& sRequestedAC) const;
    bool GetVideoSubChannels(std::string& strVideoSubChannelName, std::string& strVideoSubChannelPid);
    bool GetVideoChannels(TiXmlElement *pRootElement);
    bool CreateNewItem(const CFileItem& item, CFileItem& item_new);
    static bool InitZapstream(const std::string& strPath);
    static bool SetAudioChannel(const std::string& strPath, const AUDIOCHANNEL& sAC);

    static std::string GetPicon(std::string strServiceName);
    static std::string GetSubMode(int iMode, std::string& strXMLRootString, std::string& strXMLChildString);
    static std::string DetectSubMode(std::string strSubMode, std::string& strXMLRootString, std::string& strXMLChildString);
};
extern CTuxBoxUtil g_tuxbox;

class CTuxBoxService : public CThread
{
public:
  CTuxBoxService();
  ~CTuxBoxService();

  bool Start();
  void Stop();
  bool IsRunning();

  virtual void OnExit();
  virtual void OnStartup();
  virtual void Process();
};
extern CTuxBoxService g_tuxboxService;
