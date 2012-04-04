#pragma once 

#include "../../../lib/platform/util/StdString.h"
#include "xmlParser.h"
#include "client.h"
#include "../../../lib/platform/threads/threads.h"
    
struct VuWebResponse {
  char *response;
  int iSize;
};

typedef enum VU_UPDATE_STATE
{
    VU_UPDATE_STATE_NONE,
    VU_UPDATE_STATE_FOUND,
    VU_UPDATE_STATE_UPDATED,
    VU_UPDATE_STATE_NEW
} VU_UPDATE_STATE;

struct VuChannelGroup {
  std::string strServiceReference;
  std::string strGroupName;
  int iGroupState;

  VuChannelGroup() 
  { 
    iGroupState = VU_UPDATE_STATE_NEW;
  }

  bool operator==(const VuChannelGroup &right) const
  {
    return (! strServiceReference.compare(right.strServiceReference)) && (! strGroupName.compare(right.strGroupName));
  }

};

struct VuChannel
{
  bool bRadio;
  int iUniqueId;
  int iChannelNumber;
  std::string strGroupName;
  std::string strChannelName;
  std::string strServiceReference;
  std::string strStreamURL;
  std::string strIconPath;
  int iChannelState;

  VuChannel()
  {
    iChannelState = VU_UPDATE_STATE_NEW;
  }
  
  bool operator==(const VuChannel &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (bRadio == right.bRadio); 
    bChanged = bChanged && (iUniqueId == right.iUniqueId); 
    bChanged = bChanged && (iChannelNumber == right.iChannelNumber); 
    bChanged = bChanged && (! strGroupName.compare(right.strGroupName));
    bChanged = bChanged && (! strChannelName.compare(right.strChannelName));
    bChanged = bChanged && (! strServiceReference.compare(right.strServiceReference));
    bChanged = bChanged && (! strStreamURL.compare(right.strStreamURL));
    bChanged = bChanged && (! strIconPath.compare(right.strIconPath));

    return bChanged;
  }

};



struct VuEPGEntry 
{
  int iEventId;
  std::string strServiceReference;
  std::string strTitle;
  int iChannelId;
  time_t startTime;
  time_t endTime;
  std::string strPlotOutline;
  std::string strPlot;
};

struct VuTimer
{
  std::string strTitle;
  std::string strPlot;
  int iChannelId;
  time_t startTime;
  time_t endTime;
  bool bRepeating; 
  int iWeekdays;
  int iEpgID;
  PVR_TIMER_STATE state; 
};

struct VuRecording
{
  std::string strRecordingId;
  time_t startTime;
  int iDuration;
  std::string strTitle;
  std::string strStreamURL;
  std::string strPlot;
  std::string strPlotOutline;
  std::string strChannelName;
};
 
class Vu  : public PLATFORM::CThread
{
private:

  // members
  bool  m_bIsConnected;
  std::string m_strServerName;
  std::string m_strURL;
  int m_iNumChannels;
  int m_iNumTimers;
  int m_iNumRecordings;
  int m_iNumChannelGroups;
  int m_iCurrentChannel;
  std::vector<VuChannel> m_channels;
  std::vector<VuTimer> m_timers;
  std::vector<VuRecording> m_recordings;
  std::vector<VuChannelGroup> m_groups;
  bool m_bInitial;

  PLATFORM::CMutex m_mutex;
  PLATFORM::CCondition<bool> m_started;
 

  // functions

  void StoreChannelData();
  void LoadChannelData();
  CStdString GetHttpXML(CStdString& url);
  int GetChannelNumber(CStdString strServiceReference);
  CStdString URLEncodeInline(const CStdString& strData);
  bool SendSimpleCommand(const CStdString& strCommandURL, CStdString& strResult);
  static int VuWebResponseCallback(void *contents, int iLength, int iSize, void *memPtr); 
  CStdString GetGroupServiceReference(CStdString strGroupName);
  bool LoadChannels(CStdString strServerReference, CStdString strGroupName);
  bool LoadChannels();
  bool LoadChannelGroups();

  // helper functions
  static bool GetInt(XMLNode xRootNode, const char* strTag, int& iIntValue);
  static bool GetBoolean(XMLNode xRootNode, const char* strTag, bool& bBoolValue);
  static bool GetString(XMLNode xRootNode, const char* strTag, CStdString& strStringValue);
  static long TimeStringToSeconds(const CStdString &timeString);
  static int SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results, unsigned int iMaxStrings = 0);
  bool CheckForGroupUpdate();
  bool CheckForChannelUpdate();
  std::string& Escape(std::string &s, std::string from, std::string to);


protected:
  virtual void *Process(void);

public:
  Vu(void);
  ~Vu();

  const char * GetServerName();
  bool IsConnected(); 
  int GetChannelsAmount(void);
  PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio);
  PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  int GetCurrentClientChannel(void);
  int GetTimersAmount(void);
  PVR_ERROR GetTimers(PVR_HANDLE handle);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer);
  unsigned int GetRecordingsAmount();
  PVR_ERROR    GetRecordings(PVR_HANDLE handle);
  PVR_ERROR    DeleteRecording(const PVR_RECORDING &recinfo);
  unsigned int GetNumChannelGroups(void);
  PVR_ERROR    GetChannelGroups(PVR_HANDLE handle);
  PVR_ERROR    GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  bool SwitchChannel(const PVR_CHANNEL &channel);


  bool Open();
  void Action();
  void Close();


};

