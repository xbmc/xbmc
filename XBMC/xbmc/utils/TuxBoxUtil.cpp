//
// GeminiServer
//
#include "stdafx.h"
#include "TuxBoxUtil.h"
#include "../util.h"
#include "../FileSystem/FileCurl.h"
#include "../utils/HttpHeader.h"
#include "../utils/Http.h"
#include "../util.h"
#include "../GUIDialogContextMenu.h"
#include "../application.h"
#include "../applicationmessenger.h"
#include "GUIInfoManager.h"

using namespace XFILE;

CTuxBoxUtil g_tuxbox;
CTuxBoxService g_tuxboxService;

CTuxBoxService::CTuxBoxService()
{
}
CTuxBoxService::~CTuxBoxService()
{
}
CTuxBoxUtil::CTuxBoxUtil(void)
{
}
CTuxBoxUtil::~CTuxBoxUtil(void)
{
}
bool CTuxBoxService::Start()
{
  if(g_advancedSettings.m_iTuxBoxEpgRequestTime != 0)
  {
    StopThread();
    Create(false);
    return true;
  }
  else
    return false;
}
void CTuxBoxService::Stop()
{
  CLog::Log(LOGDEBUG, __FUNCTION__" - Stopping CTuxBoxService thread");
  StopThread();
}
void CTuxBoxService::OnStartup()
{
  CLog::Log(LOGDEBUG, __FUNCTION__" - Starting CTuxBoxService thread");
  SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST);
}
void CTuxBoxService::OnExit()
{
  CThread::m_bStop = true;
}
bool CTuxBoxService::IsRunning()
{
  return !CThread::m_bStop;
}
void CTuxBoxService::Process()
{
  CStdString strCurrentServiceName = g_tuxbox.sCurSrvData.service_name;
  CStdString strURL;

  while(!CThread::m_bStop && g_application.IsPlaying())
  {
    strURL = g_application.CurrentFileItem().m_strPath;
    if(!CUtil::IsTuxBox(strURL))
      break;

    int iRequestTimer = g_advancedSettings.m_iTuxBoxEpgRequestTime *1000; //seconds
    Sleep(iRequestTimer);

    CURL url(strURL);
    if(g_tuxbox.GetHttpXML(url,"currentservicedata"))
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - receive current service data was successful");
      if(!strCurrentServiceName.IsEmpty()&& 
        !strCurrentServiceName.Equals("NULL") &&
        !g_tuxbox.sCurSrvData.service_name.IsEmpty() &&
        !g_tuxbox.sCurSrvData.service_name.Equals("-") &&
        !g_tuxbox.vVideoSubChannel.mode)
      {
        //Detect Channel Change
        //We need to detect the channel on the TuxBox Device! 
        //On changing the channel on the device we will loose the stream and mplayer seems not able to detect it to stop
        if (strCurrentServiceName != g_tuxbox.sCurSrvData.service_name && g_application.IsPlaying())
        {
          CLog::Log(LOGDEBUG," - ERROR: Non controlled channel change detected! Stopping current playing stream!");
          g_applicationMessenger.MediaStop();
          break;
        }
      }
      //Update infomanager from tuxbox client
      g_infoManager.UpdateFromTuxBox();
    }
    else
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Could not receive current service data");
    }
  }
  return;
}
bool CTuxBoxUtil::CreateNewItem(const CFileItem& item, CFileItem& item_new)
{
  //Build new Item
  item_new.SetLabel(item.GetLabel());
  item_new.m_strPath = item.m_strPath;
  item_new.SetThumbnailImage(item.GetThumbnailImage());
  
  if(g_tuxbox.GetZapUrl(item.m_strPath, item_new))
  {
    if(vVideoSubChannel.mode)
      vVideoSubChannel.current_name = item_new.GetLabel()+" ("+vVideoSubChannel.current_name+")";
    return true;
  }
  else
  {
    if(!sBoxStatus.recording.Equals("1")) //Don't Show this Dialog, if the Box is in Recording mode! A previos YN Dialog was send to user!
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" ---------------------------------------------------------");
      CLog::Log(LOGDEBUG, __FUNCTION__" - WARNING: Zaping Failed no Zap Point found!");
      CLog::Log(LOGDEBUG, __FUNCTION__" ---------------------------------------------------------");
      CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dialog)
      {
        CStdString strText;
        strText.Format(g_localizeStrings.Get(21334).c_str(), item.GetLabel());
        dialog->SetHeading( 21331 );
        dialog->SetLine( 0, strText);
        dialog->SetLine( 1, 21333 );
        dialog->SetLine( 2, "" );
        dialog->DoModal();
      }
    }
  }
  return false;
}
bool CTuxBoxUtil::ParseBouquets(TiXmlElement *root, CFileItemList &items, CURL &url, CStdString strFilter, CStdString strChild)
{
  //
  //strChild.Format("bouquet");
  //

  CStdString strItemName, strItemPath, strOptions, strPort;
  TiXmlElement *pRootElement =root;
  TiXmlNode *pNode = NULL;
  TiXmlNode *pIt = NULL;
  items.m_idepth =1;
  // Get Options
  strOptions = url.GetOptions();
  // Detect Port
  if (url.GetPort()!=0 && url.GetPort()!=80)
  {
    strPort.Format(":%i",url.GetPort());
  }
  else 
  {
    strPort = "";
  }

  if (!pRootElement)
  {    
    CLog::Log(LOGWARNING, __FUNCTION__" - No %s found",strChild.c_str());
    return false;
  }
  if (strFilter.IsEmpty())
  {
    pNode = pRootElement->FirstChild(strChild.c_str());
    if (!pNode)
    {
      CLog::Log(LOGWARNING, __FUNCTION__" - No %s found",strChild.c_str());
      return false;
    }
    while(pNode)
    {
        pIt = pNode->FirstChild("name");
        if (pIt)
        {
          strItemName = pIt->FirstChild()->Value();
          
          pIt = pNode->FirstChild("reference");
          if (pIt)
          {
            strItemPath = pIt->FirstChild()->Value();
            // add. bouquets to item list! 
            CFileItem* pItem = new CFileItem;
            pItem->m_bIsFolder = true;
            pItem->SetLabel(strItemName);
            url.SetOptions("/"+strOptions+"&reference="+strItemPath);
            pItem->m_strPath = "tuxbox://"+url.GetUserName()+":"+url.GetPassWord()+"@"+url.GetHostName()+strPort+url.GetOptions();  
            items.Add(pItem);
            //DEBUG Log
            CLog::Log(LOGDEBUG, __FUNCTION__" - Name:    %s",strItemName.c_str());
            CLog::Log(LOGDEBUG, __FUNCTION__" - Adress:  %s",pItem->m_strPath.c_str());
          }
        }
        pNode = pNode->NextSibling(strChild.c_str());
    }
  }
  return true;
}

bool CTuxBoxUtil::ParseChannels(TiXmlElement *root, CFileItemList &items, CURL &url, CStdString strFilter, CStdString strChild)
{
  //
  //strChild.Format("bouquet");
  //

  CStdString strItemName, strItemPath,strPort;
  TiXmlElement *pRootElement =root;
  TiXmlNode *pNode = NULL;
  TiXmlNode *pIt = NULL;
  TiXmlNode *pIta = NULL;
  items.m_idepth =2;

  //Detect Port
  if (url.GetPort()!=0 && url.GetPort()!=80)
  {
    strPort.Format(":%i",url.GetPort());
  }
  else 
  {
    strPort = "";
  }

  if (!pRootElement)
  {    
    CLog::Log(LOGWARNING, __FUNCTION__" - No %ss found",strChild.c_str());
    return false;
  }
  if(!strFilter.IsEmpty())
  {
    pNode = pRootElement->FirstChild(strChild.c_str());
    if (!pNode)
    {
      CLog::Log(LOGWARNING, __FUNCTION__" - No %s found",strChild.c_str());
      return false;
    }
    while(pNode)
    {
        pIt = pNode->FirstChild("name");
        if (pIt)
        {
          strItemName = pIt->FirstChild()->Value();
          
          pIt = pNode->FirstChild("reference");
          if (strFilter.Equals(pIt->FirstChild()->Value()))
          {
            pIt = pNode->FirstChild("service");
            if (!pIt)
            {
              CLog::Log(LOGWARNING, __FUNCTION__" - No service found");
              return false;
            }
            while(pIt)
            {
                pIta = pIt->FirstChild("name");
                if (pIta)
                {
                  strItemName = pIta->FirstChild()->Value();
                  
                  pIta = pIt->FirstChild("reference");
                  if (pIta)
                  {
                    strItemPath = pIta->FirstChild()->Value();
                    // channel listing add. to item list!
                    CFileItem* pbItem = new CFileItem;
                    pbItem->m_bIsFolder = false;
                    pbItem->SetLabel(strItemName);
                    pbItem->m_strPath = "tuxbox://"+url.GetUserName()+":"+url.GetPassWord()+"@"+url.GetHostName()+strPort+"/cgi-bin/zapTo?path="+strItemPath+".ts";  
                    pbItem->SetContentType("video/x-ms-asf");
                    //pbItem->SetThumbnailImage(GetPicon(strItemName)); //Set Picon Image

                    //DEBUG Log
                    CLog::Log(LOGDEBUG, __FUNCTION__" - Name:    %s",strItemName.c_str());
                    CLog::Log(LOGDEBUG, __FUNCTION__" - Adress:  %s",pbItem->m_strPath.c_str());
                    
                    //add to the list
                    items.Add(pbItem);
                  }
                }
                pIt = pIt->NextSibling("service");
            }
          }
        }
        pNode = pNode->NextSibling(strChild.c_str());
    }
    return true;
  }
  return false;
}
bool CTuxBoxUtil::ZapToUrl(CURL url, CStdString strOptions, int ipoint) 
{
  // send Zap 
  CStdString strZapUrl, strPostUrl, strZapName, strFilter;
  //Extract the ZAP to Service String
  strFilter = strOptions.Right((strOptions.size()-(ipoint+6)));
  //Remove the ".ts"
  strFilter = strFilter.Left(strFilter.size()-3);
  //Get the Service Name
  strZapName = url.GetFileNameWithoutPath();

  // Detect Port and Create ZAP URL
  if (url.GetPort()!=0 && url.GetPort()!=80)
    strZapUrl.Format("http://%s:%s@%s:%i",url.GetUserName().c_str(),url.GetPassWord().c_str(),url.GetHostName().c_str(),url.GetPort());
  else
    strZapUrl.Format("http://%s:%s@%s",url.GetUserName().c_str(),url.GetPassWord().c_str(),url.GetHostName().c_str());
  strPostUrl.Format("/cgi-bin/zapTo?path=%s",strFilter.c_str());

  //Set Zap URL
  CURL urlx(strZapUrl);

  //Check Recording State!
  if(GetHttpXML(urlx,"boxstatus"))
  {
    if(sBoxStatus.recording.Equals("1"))
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" ---------------------------------------------------------");
      CLog::Log(LOGDEBUG, __FUNCTION__" - WARNING: Device is Recording! Record Mode is: %s",sBoxStatus.recording.c_str());
      CLog::Log(LOGDEBUG, __FUNCTION__" ---------------------------------------------------------");
      CGUIDialogYesNo* dialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      if (dialog)
      {
        //Target TuxBox is in Recording mode! Are you sure to stream ?YN
        dialog->SetHeading(21331);
        dialog->SetLine( 0, 21332);
        dialog->SetLine( 1, 21335);
        dialog->SetLine( 2, "" );
        dialog->DoModal();
      }
      if (!dialog->IsConfirmed())
      {
        //DialogYN = NO -> Return false!
        return false;
      }
    }
  }
    
  //Send ZAP Command
  CFileCurl http;
  if(http.Open(strZapUrl+strPostUrl, false))
  {
    //DEBUG LOG
    CLog::Log(LOGDEBUG, __FUNCTION__" - Zapped to: %s (%s)",strZapName.c_str(),strZapUrl.c_str());
    CLog::Log(LOGDEBUG, __FUNCTION__" - Zap String: %s",strPostUrl.c_str());

    //Request StreamInfo
    GetHttpXML(urlx,"streaminfo");
    
    //Extract StreamInformations
    int iRetry=0;
    //PMT must be a valid value to be sure that the ZAP is OK and we can stream!
    while(sStrmInfo.pmt.Equals("ffffffffh") && iRetry!=10) //try 10 Times
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Requesting STREAMINFO! TryCount: %i!",iRetry);
      GetHttpXML(urlx,"streaminfo");
      iRetry=iRetry+1;
    }
    
    // PMT Not Valid? Try Time 10 reached, checking for advancedSettings m_iTuxBoxZapWaitTime
    if(sStrmInfo.pmt.Equals("ffffffffh") && g_advancedSettings.m_iTuxBoxZapWaitTime > 0 )
    {
      iRetry = 0;
      CLog::Log(LOGDEBUG, __FUNCTION__" - Starting TuxBox ZapWaitTimer!");
      while(sStrmInfo.pmt.Equals("ffffffffh") && iRetry!=10) //try 10 Times
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" - Requesting STREAMINFO! TryCount: %i!",iRetry);
        GetHttpXML(urlx,"streaminfo");
        iRetry=iRetry+1;
        if(sStrmInfo.pmt.Equals("ffffffffh"))
        {
          CLog::Log(LOGERROR, __FUNCTION__" - STREAMINFO ERROR! Could not receive all data, TryCount: %i!",iRetry);
          CLog::Log(LOGERROR, __FUNCTION__" - PMT is: %s (not a Valid Value)! Waiting %i sec.",sStrmInfo.pmt.c_str(), g_advancedSettings.m_iTuxBoxZapWaitTime);
          Sleep(g_advancedSettings.m_iTuxBoxZapWaitTime*1000);
        }
      }
    }
    
    //PMT Failed! No StreamInformations availible.. closing stream 
    if (sStrmInfo.pmt.Equals("ffffffffh"))
    {
      CLog::Log(LOGERROR, __FUNCTION__"-------------------------------------------------------------");
      CLog::Log(LOGERROR, __FUNCTION__" - STREAMINFO ERROR! Could not receive all data, TryCount: %i!",iRetry);
      CLog::Log(LOGERROR, __FUNCTION__" - PMT is: %s (not a Valid Value)! There is nothing to Stream!",sStrmInfo.pmt.c_str());
      CLog::Log(LOGERROR, __FUNCTION__" - The Stream will stopped!");
      CLog::Log(LOGERROR, __FUNCTION__"-------------------------------------------------------------");
      return false;
    }
    //Currentservicedata
    GetHttpXML(urlx,"currentservicedata");
    //boxstatus
    GetHttpXML(urlx,"boxstatus");
    //boxinfo
    GetHttpXML(urlx,"boxinfo");
    //serviceepg
    GetHttpXML(urlx,"serviceepg");
    return true;
  }
  return false;
}
bool CTuxBoxUtil::GetZapUrl(const CStdString& strPath, CFileItem &items )
{
  CURL url(strPath);
  CStdString strOptions = url.GetOptions();
  if (strOptions.IsEmpty())
    return false;

  int ipoint = strOptions.Find("?path=");
  if (ipoint >=0)
  {
    if(ZapToUrl(url, strOptions, ipoint))
    {
      //Check VideoSubChannels
      if(GetHttpXML(url,"currentservicedata")) //Update Currentservicedata
      {
        //Detect VideoSubChannels
        CStdString strVideoSubChannelName, strVideoSubChannelPID;
        if(GetVideoSubChannels(strVideoSubChannelName,strVideoSubChannelPID ))
        {
          // new videosubchannel selected! settings options to new video zap id
          strOptions = "?path="+strVideoSubChannelPID+".ts";
          // zap again now to new videosubchannel
          if(ZapToUrl(url, strOptions, ipoint))
          {
            vVideoSubChannel.mode = true;
            vVideoSubChannel.current_name = strVideoSubChannelName;
          }
        }
        else 
          vVideoSubChannel.mode= false;
      }

      CStdString strStreamURL, strVideoStream;
      CStdString strLabel, strLabel2;
      CStdString strAudioChannelName, strAudioChannelPid;
      
      if (GetAudioChannels(strAudioChannelName, strAudioChannelPid))
      {
        if(strAudioChannelPid.Left(2).Equals("0x"))
          strAudioChannelPid.Replace("0x","");

        strVideoStream.Format("0,%s,%s,%s,,,%s",sStrmInfo.pmt.Left(4).c_str(), sStrmInfo.vpid.Left(4).c_str(), strAudioChannelPid.Left(4).c_str(),sStrmInfo.pcrpid.Left(4).c_str());
      }
      else
      {
        if(sStrmInfo.apid.Left(2).Equals("0x")) 
          sStrmInfo.apid.Replace("0x","");
        if(sCurSrvData.audio_channel_1_pid.Left(2).Equals("0x"))
          sCurSrvData.audio_channel_1_pid.Replace("0x","");
        if(sCurSrvData.audio_channel_2_pid.Left(2).Equals("0x"))
          sCurSrvData.audio_channel_2_pid.Replace("0x","");

        if(g_application.m_eForcedNextPlayer == EPC_DVDPLAYER || g_advancedSettings.m_bTuxBoxSendAllAPids)
          strVideoStream.Format("0,%s,%s,%s,%s,%s,%s",sStrmInfo.pmt.Left(4).c_str(), sStrmInfo.vpid.Left(4).c_str(), sStrmInfo.apid.Left(4).c_str(), sCurSrvData.audio_channel_1_pid.Left(4).c_str(), sCurSrvData.audio_channel_2_pid.Left(4).c_str(), sStrmInfo.pcrpid.Left(4).c_str());
        else 
          strVideoStream.Format("0,%s,%s,%s,,,%s",sStrmInfo.pmt.Left(4).c_str(), sStrmInfo.vpid.Left(4).c_str(), sStrmInfo.apid.Left(4).c_str(), sStrmInfo.pcrpid.Left(4).c_str());
      }
      
      //strStreamURL.Format("http://%s:%i/%s",url.GetHostName().c_str(),TS_STREAM_PORT,strVideoStream.c_str());
      strStreamURL.Format("http://%s:%s@%s:%i/%s",url.GetUserName().c_str(),url.GetPassWord().c_str(), url.GetHostName().c_str(),TS_STREAM_PORT,strVideoStream.c_str());
      strLabel.Format("%s: %s %s-%s",items.GetLabel().c_str(), sCurSrvData.current_event_date.c_str(),sCurSrvData.current_event_start.c_str(), sCurSrvData.current_event_start.c_str());
      strLabel2.Format("%s", sCurSrvData.current_event_description.c_str());
      
      // Set Event details
      CStdString strGenre, strTitle;
      strGenre.Format("%s %s  -  (%s: %s)",g_localizeStrings.Get(143),sCurSrvData.current_event_description, g_localizeStrings.Get(209),sCurSrvData.next_event_description);
      strTitle.Format("%s",sCurSrvData.current_event_details);
      int iDuration = atoi(sCurSrvData.current_event_duration.c_str());
      
      items.GetVideoInfoTag()->m_strGenre = strGenre;  // VIDEOPLAYER_GENRE: current_event_description (Film Name)
      items.GetVideoInfoTag()->m_strTitle = strTitle; // VIDEOPLAYER_TITLE: current_event_details     (Film beschreibung)
      StringUtils::SecondsToTimeString(iDuration,items.GetVideoInfoTag()->m_strRuntime); //VIDEOPLAYER_DURATION: current_event_duration (laufzeit in sec.)
      
      //
      items.m_strPath = strStreamURL;
      items.m_iDriveType = url.GetPort(); // Dirty Hack! But i need to hold the Port ;)
      items.SetLabel(items.GetLabel()); // VIDEOPLAYER_DIRECTOR: service_name (Program Name)
      items.SetLabel2(sCurSrvData.current_event_description); // current_event_description (Film Name)
      items.m_bIsFolder = false;
      items.SetContentType("video/x-ms-asf");
      
      return true;
    }
  }
  return false;
}

bool CTuxBoxUtil::GetHttpXML(CURL url,CStdString strRequestType)
{
  // Check and Set URL Request Option
  if(!strRequestType.IsEmpty())
  {
    if(strRequestType.Equals("streaminfo"))
    {
      url.SetOptions("xml/streaminfo");
    }
    else if(strRequestType.Equals("currentservicedata"))
    {
      url.SetOptions("xml/currentservicedata");
    }
    else if(strRequestType.Equals("boxstatus"))
    {
      url.SetOptions("xml/boxstatus");
    }
    else if(strRequestType.Equals("boxinfo"))
    {
      url.SetOptions("xml/boxinfo");
    }
    else if(strRequestType.Equals("serviceepg"))
    {
      url.SetOptions("xml/serviceepg");
    }
    else 
    {
      CLog::Log(LOGERROR, __FUNCTION__" - Request Type is not defined! You requested: %s",strRequestType.c_str());
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, __FUNCTION__" - strRequestType Request Type is Empty!");
    return false;
  }
  
  // Clean Up the URL, so we have a clean request!
  url.SetFileName("");
  
  //Open 
  CFileCurl http;
  http.SetTimeout(20);
  if(http.Open(url, false)) 
  {
    CStdString strTmp;
    int size_read = 0;  
    int data_size = 0;
    int size_total = (int)http.GetLength();
        
    if(size_total >0)
    {
      // read response from server into string buffer
      strTmp.reserve(size_total);
      char buffer[16384];
      while( (size_read = http.Read( buffer, sizeof(buffer)-1) ) > 0 )
      {
        buffer[size_read] = 0;
        strTmp += buffer;
        data_size += size_read;
      }

      // parse returned xml
      TiXmlDocument doc;
      TiXmlElement *XMLRoot=NULL;
      strTmp.Replace("></",">-</"); //FILL EMPTY ELEMENTS WITH "-"!
      doc.Parse(strTmp.c_str());
      strTmp.Empty();

      XMLRoot = doc.RootElement();
      CStdString strRoot = XMLRoot->Value();
      if( strRoot.Equals("streaminfo"))
      {
        return StreamInformations(XMLRoot);
      }
      else if(strRoot.Equals("currentservicedata"))
      {
        return CurrentServiceData(XMLRoot);
      }
      else if(strRoot.Equals("boxstatus"))
      {
        return BoxStatus(XMLRoot);
      }
      else if(strRoot.Equals("boxinfo"))
      {
        return BoxInfo(XMLRoot);
      }
      else if(strRoot.Equals("serviceepg") || strRoot.Equals("service_epg"))
      {
        return ServiceEPG(XMLRoot);
      }
      else
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Unable to parse xml");
        CLog::Log(LOGERROR, __FUNCTION__" - Request String: %s",strRoot.c_str());
      }
      return false;
    }
    else
    {
      CLog::Log(LOGERROR, __FUNCTION__" - http length is invalid!");
      return false;
    }
  }
  CLog::Log(LOGERROR, __FUNCTION__" - Open URL Failed! Unable to get XML structure");
  return false;
}
bool CTuxBoxUtil::StreamInformations(TiXmlElement *pRootElement)
{
  /*
  Sample:
  http://192.168.0.110:31339/0,0065,01ff,0200,0201,0203,01ff

  http://getIP:31339/0,pmtpid,vpid,apid,apids,apids,pcrpid;

  vpid,pmtpid,pcrpid,apid  --> xml/streaminfo
  apids --> /xml/currentservicedata
  
  apid: is the defined audio stream!
  Normal Stereo: http://192.168.0.110:31339/0,0065,01ff,0200,0201,0203,01ff
  Normal English: http://192.168.0.110:31339/0,0065,01ff,0201,,,01ff
  Normal DD5.1/AC3: http://192.168.0.110:31339/0,0065,01ff,0203,,,01ff
  */

  TiXmlNode *pNode = NULL;
  TiXmlNode *pIt = NULL;
  CStdString strRoot = pRootElement->Value();
  if(pRootElement !=NULL)
  {
    pNode = pRootElement->FirstChild("frontend");
    if (pNode)
    {
      sStrmInfo.frontend = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Frontend: %s", sStrmInfo.frontend.c_str());
    }
    pNode = pRootElement->FirstChild("service");
    if (pNode)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Service");
      pIt = pNode->FirstChild("name");
      if (pIt)
      {
        sStrmInfo.service_name = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Name: %s", sStrmInfo.service_name.c_str());
      }
      pIt = pNode->FirstChild("reference");
      if (pIt)
      {
        sStrmInfo.service_reference = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Reference: %s", sStrmInfo.service_reference.c_str());
      }
    }

    pNode = pRootElement->FirstChild("provider");
    if(pNode && pNode->FirstChild())
    {
      sStrmInfo.provider= pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Provider: %s", sStrmInfo.provider.c_str());
    }
    pNode = pRootElement->FirstChild("vpid");
    if (pNode)
    {
      sStrmInfo.vpid = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Vpid: %s", sStrmInfo.vpid.c_str());
    }
    pNode = pRootElement->FirstChild("apid");
    if (pNode)
    {
      sStrmInfo.apid = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Apid: %s", sStrmInfo.apid.c_str());
    }
    pNode = pRootElement->FirstChild("pcrpid");
    if (pNode)
    {
      sStrmInfo.pcrpid = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - PcrPid: %s", sStrmInfo.pcrpid.c_str());
    }
    pNode = pRootElement->FirstChild("tpid");
    if (pNode)
    {
      sStrmInfo.tpid = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Tpid: %s", sStrmInfo.tpid.c_str());
    }
    pNode = pRootElement->FirstChild("tsid");
    if (pNode)
    {
      sStrmInfo.tsid = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Tsid: %s", sStrmInfo.tsid.c_str());
    }
    pNode = pRootElement->FirstChild("onid");
    if (pNode)
    {
      sStrmInfo.onid = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Onid: %s", sStrmInfo.onid.c_str());
    }
    pNode = pRootElement->FirstChild("sid");
    if (pNode)
    {
      sStrmInfo.sid = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Sid: %s", sStrmInfo.sid.c_str());
    }
    pNode = pRootElement->FirstChild("pmt");
    if (pNode)
    {
      sStrmInfo.pmt = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Pmt: %s", sStrmInfo.pmt.c_str());
    }
    pNode = pRootElement->FirstChild("video_format");
    if (pNode)
    {
      sStrmInfo.video_format = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Video Format: %s", sStrmInfo.video_format.c_str());
    }
    pNode = pRootElement->FirstChild("supported_crypt_systems");
    if (pNode)
    {
      sStrmInfo.supported_crypt_systems = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Supported Crypt Systems: %s", sStrmInfo.supported_crypt_systems.c_str());
    }
    pNode = pRootElement->FirstChild("used_crypt_systems");
    if (pNode)
    {
      sStrmInfo.used_crypt_systems = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Used Crypt Systems: %s", sStrmInfo.used_crypt_systems.c_str());
    }
    pNode = pRootElement->FirstChild("satellite");
    if (pNode)
    {
      sStrmInfo.satellite = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Satellite: %s", sStrmInfo.satellite.c_str());
    }
    pNode = pRootElement->FirstChild("frequency");
    if (pNode)
    {
      sStrmInfo.frequency = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Frequency: %s", sStrmInfo.frequency.c_str());
    }
    pNode = pRootElement->FirstChild("symbol_rate");
    if (pNode)
    {
      sStrmInfo.symbol_rate = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Symbol Rate: %s", sStrmInfo.symbol_rate.c_str());
    }
    pNode = pRootElement->FirstChild("polarisation");
    if (pNode)
    {
      sStrmInfo.polarisation = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Polarisation: %s", sStrmInfo.polarisation.c_str());
    }
    pNode = pRootElement->FirstChild("inversion");
    if (pNode)
    {
      sStrmInfo.inversion = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Inversion: %s", sStrmInfo.inversion.c_str());
    }
    pNode = pRootElement->FirstChild("fec");
    if (pNode)
    {
      sStrmInfo.fec = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Fec: %s", sStrmInfo.fec.c_str());
    }
    pNode = pRootElement->FirstChild("snr");
    if (pNode)
    {
      sStrmInfo.snr = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Snr: %s", sStrmInfo.snr.c_str());
    }
    pNode = pRootElement->FirstChild("agc");
    if (pNode)
    {
      sStrmInfo.agc = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Agc: %s",  sStrmInfo.agc.c_str());
    }
    pNode = pRootElement->FirstChild("ber");
    if (pNode)
    {
      sStrmInfo.ber = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - ber: %s", sStrmInfo.ber.c_str());
    }
    pNode = pRootElement->FirstChild("lock");
    if (pNode)
    {
      sStrmInfo.lock = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Lock: %s", sStrmInfo.lock.c_str());
    }
    pNode = pRootElement->FirstChild("sync");
    if (pNode)
    {
      sStrmInfo.sync = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Sync: %s", sStrmInfo.sync.c_str());
    }
    return true;
  }
  return false;
}
bool CTuxBoxUtil::CurrentServiceData(TiXmlElement *pRootElement)
{
  TiXmlNode *pNode = NULL;
  TiXmlNode *pIt = NULL;
  TiXmlNode *pVal = NULL;
  if(pRootElement)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - Current Service Data");
    pNode = pRootElement->FirstChild("service");
    if (pNode)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Service");
      pIt = pNode->FirstChild("name");
      if (pIt)
      {
        sCurSrvData.service_name = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" Service Name: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("reference");
      if (pIt)
      {
        sCurSrvData.service_reference = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" Service Reference: %s", pIt->FirstChild()->Value());
      }
    }

    pNode = pRootElement->FirstChild("audio_channels");
    if (pNode)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Audio Channels");
      int i = 0;
      pIt = pNode->FirstChild("channel");
      while(pIt)
      {
        pVal = pIt->FirstChild("pid");
        if(pVal)
        {
          if(i==0) sCurSrvData.audio_channel_0_pid = pVal->FirstChild()->Value();
          if(i==1) sCurSrvData.audio_channel_1_pid = pVal->FirstChild()->Value();
          if(i==2) sCurSrvData.audio_channel_2_pid = pVal->FirstChild()->Value();
        }
        
        pVal = pIt->FirstChild("selected");
        if(pVal)
        {
          if(i==0) sCurSrvData.audio_channel_0_selected = pVal->FirstChild()->Value();
          if(i==1) sCurSrvData.audio_channel_1_selected = pVal->FirstChild()->Value();
          if(i==2) sCurSrvData.audio_channel_2_selected = pVal->FirstChild()->Value();
        }
        
        pVal = pIt->FirstChild("name");
        if(pVal)
        {
          if(i==0) sCurSrvData.audio_channel_0_name = pVal->FirstChild()->Value();
          if(i==1) sCurSrvData.audio_channel_1_name = pVal->FirstChild()->Value();
          if(i==2) sCurSrvData.audio_channel_2_name = pVal->FirstChild()->Value();
        }

        if(i==0) CLog::Log(LOGDEBUG, __FUNCTION__" - Audio Channels: Channel %i -> PID: %s Selected: %s Name: %s", i, sCurSrvData.audio_channel_0_pid.c_str(), sCurSrvData.audio_channel_0_selected.c_str(), sCurSrvData.audio_channel_0_name.c_str() );
        if(i==1) CLog::Log(LOGDEBUG, __FUNCTION__" - Audio Channels: Channel %i -> PID: %s Selected: %s Name: %s", i, sCurSrvData.audio_channel_1_pid.c_str(), sCurSrvData.audio_channel_1_selected.c_str(), sCurSrvData.audio_channel_1_name.c_str() );
        if(i==2) CLog::Log(LOGDEBUG, __FUNCTION__" - Audio Channels: Channel %i -> PID: %s Selected: %s Name: %s", i, sCurSrvData.audio_channel_2_pid.c_str(), sCurSrvData.audio_channel_2_selected.c_str(), sCurSrvData.audio_channel_2_name.c_str() );

        i=i+1;
        pIt = pIt->NextSibling("channel");
      }
    }
    pNode = pRootElement->FirstChild("audio_track");
    if (pNode)
    {
      sCurSrvData.audio_track = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Audio Track: %s", pNode->FirstChild()->Value() );
    }
    pNode = pRootElement->FirstChild("video_channels");
    if (pNode)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Video Channels");
      pIt = pNode->FirstChild("service");
      if (pIt)
      {
        vVideoSubChannel.name.clear();
        vVideoSubChannel.reference.clear();
        vVideoSubChannel.selected.clear();
        int i = 0;
        while(pIt)
        {
          pVal = pIt->FirstChild("name");
          if(pVal)
          {
            vVideoSubChannel.name.push_back(pVal->FirstChild()->Value());
            CLog::Log(LOGDEBUG, __FUNCTION__" - Video Sub Channel %i:      Name: %s", i,pVal->FirstChild()->Value());
          }
          pVal = pIt->FirstChild("reference");
          if(pVal)
          {
            vVideoSubChannel.reference.push_back(pVal->FirstChild()->Value());
            CLog::Log(LOGDEBUG, __FUNCTION__" - Video Sub Channel %i: Reference: %s", i,pVal->FirstChild()->Value());
          }
          pVal = pIt->FirstChild("selected");
          if(pVal)
          {
            vVideoSubChannel.selected.push_back(pVal->FirstChild()->Value());
            CLog::Log(LOGDEBUG, __FUNCTION__" - Video Sub Channel %i: Selected: %s", i,pVal->FirstChild()->Value());
          }
          pIt = pIt->NextSibling("service");
          i++;
        }
      }
      else
      {
        vVideoSubChannel.name.clear();
        vVideoSubChannel.reference.clear();
        vVideoSubChannel.selected.clear();
      }
    }
    pNode = pRootElement->FirstChild("current_event");
    if (pNode)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Current Event");
      pIt = pNode->FirstChild("date");
      if (pIt)
      {
        sCurSrvData.current_event_date = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Date: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("time");
      if (pIt)
      {
        sCurSrvData.current_event_time = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Time: %s", pIt->FirstChild()->Value());
      }

      pIt = pNode->FirstChild("start");
      if (pIt)
      {
        sCurSrvData.current_event_start = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Start: %s", pIt->FirstChild()->Value());
      }

      pIt = pNode->FirstChild("duration");
      if (pIt)
      {
        sCurSrvData.current_event_duration = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Duration: %s", pIt->FirstChild()->Value());
      }

      pIt = pNode->FirstChild("description");
      if (pIt)
      {
        sCurSrvData.current_event_description = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Description: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("details");
      if (pIt)
      {
        sCurSrvData.current_event_details = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Details: %s", pIt->FirstChild()->Value());
      }
    }
    pNode = pRootElement->FirstChild("next_event");
    if (pNode)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Next Event");
      pIt = pNode->FirstChild("date");
      if (pIt)
      {
        sCurSrvData.next_event_date = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Date: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("time");
      if (pIt)
      {
        sCurSrvData.next_event_time = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Time: %s", pIt->FirstChild()->Value());
      }

      pIt = pNode->FirstChild("start");
      if (pIt)
      {
        sCurSrvData.next_event_start = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Start: %s", pIt->FirstChild()->Value());
      }

      pIt = pNode->FirstChild("duration");
      if (pIt)
      {
        sCurSrvData.next_event_duration = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Duration: %s", pIt->FirstChild()->Value());
      }

      pIt = pNode->FirstChild("description");
      if (pIt)
      {
        sCurSrvData.next_event_description = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Description: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("details");
      if (pIt)
      {
        sCurSrvData.next_event_details = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Details: %s", pIt->FirstChild()->Value());
      }
    }
    return true;
  }
  return false;

}
bool CTuxBoxUtil::BoxStatus(TiXmlElement *pRootElement)
{
  //Tuxbox Controll Commands
  /*
    /cgi-bin/admin?command=wakeup
    /cgi-bin/admin?command=standby
    /cgi-bin/admin?command=shutdown
    /cgi-bin/admin?command=reboot
    /cgi-bin/admin?command=restart
  */

  TiXmlNode *pNode = NULL;
  TiXmlNode *pIt = NULL;
  if(pRootElement)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - BoxStatus");
    pNode = pRootElement->FirstChild("current_time");
    if (pNode)
    {
      sBoxStatus.current_time = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Current Time: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("standby");
    if (pNode)
    {
      sBoxStatus.standby = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Standby: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("recording");
    if (pNode)
    {
      sBoxStatus.recording = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Recording: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("mode");
    if (pNode)
    {
      sBoxStatus.mode = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Mode: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("ip");
    if (pNode)
    {
      sBoxStatus.ip = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Ip: %s", pNode->FirstChild()->Value());
    }
    return true;
  }
  return false;
}
bool CTuxBoxUtil::BoxInfo(TiXmlElement *pRootElement)
{
  TiXmlNode *pNode = NULL;
  TiXmlNode *pIt = NULL;
  TiXmlNode *pVal = NULL;
  if(pRootElement)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - BoxInfo");
    pNode = pRootElement->FirstChild("image");
    if (pNode)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Image");
      pIt = pNode->FirstChild("version");
      if (pIt)
      {
        sBoxInfo.image_version = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" Image Version: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("url");
      if (pIt)
      {
        sBoxInfo.image_url = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" Image Url: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("comment");
      if (pIt)
      {
        sBoxInfo.image_comment = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" Image Comment: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("catalog");
      if (pIt)
      {
        sBoxInfo.image_catalog = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" Image Catalog: %s", pIt->FirstChild()->Value());
      }
    }
    pNode = pRootElement->FirstChild("firmware");
    if (pNode)
    {
      sBoxInfo.firmware = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Firmware: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("fpfirmware");
    if (pNode)
    {
      sBoxInfo.fpfirmware = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - FP Firmware: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("webinterface");
    if (pNode)
    {
      sBoxInfo.webinterface = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Web Interface: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("model");
    if (pNode)
    {
      sBoxInfo.model = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Model: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("manufacturer");
    if (pNode)
    {
      sBoxInfo.manufacturer = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Manufacturer: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("processor");
    if (pNode)
    {
      sBoxInfo.processor = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Processor: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("usbstick");
    if (pNode)
    {
      sBoxInfo.usbstick = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - USB Stick: %s", pNode->FirstChild()->Value());
    }
    pNode = pRootElement->FirstChild("disk");
    if (pNode)
    {
      sBoxInfo.disk = pNode->FirstChild()->Value();
      CLog::Log(LOGDEBUG, __FUNCTION__" - Disk: %s", pNode->FirstChild()->Value());
    }
    return true;
  }
  return false;
}
bool CTuxBoxUtil::ServiceEPG(TiXmlElement *pRootElement)
{
  TiXmlNode *pNode = NULL;
  TiXmlNode *pIt = NULL;
  TiXmlNode *pVal = NULL;
  if(pRootElement)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - Service EPG");
    pNode = pRootElement->FirstChild("service");
    if (pNode)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Service");
      pIt = pNode->FirstChild("reference");
      if (pIt)
      {
        sServiceEPG.service_reference = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Service Reference: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("name");
      if (pIt)
      {
        sServiceEPG.service_name = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Service Name: %s", pIt->FirstChild()->Value());
      }
    }
    //Todo there is more then 1 event! Create a Event List!
    pNode = pRootElement->FirstChild("event");
    if (pNode)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" - Event");
      pIt = pNode->FirstChild("date");
      if (pIt)
      {
        sServiceEPG.date = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Date: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("time");
      if (pIt)
      {
        sServiceEPG.time = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Time: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("duration");
      if (pIt)
      {
        sServiceEPG.duration = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Duration: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("descritption");
      if (pIt)
      {
        sServiceEPG.descritption = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Descritption: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("genre");
      if (pIt)
      {
        sServiceEPG.genre = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Genre: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("genrecategory");
      if (pIt)
      {
        sServiceEPG.genrecategory = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Genrecategory: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("start");
      if (pIt)
      {
        sServiceEPG.start = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Start: %s", pIt->FirstChild()->Value());
      }
      pIt = pNode->FirstChild("details");
      if (pIt)
      {
        sServiceEPG.details = pIt->FirstChild()->Value();
        CLog::Log(LOGDEBUG, __FUNCTION__" - Details: %s", pIt->FirstChild()->Value());
      }
    }
    return true;
  }
  return false;
}
//PopUp and request the AudioChannel
//No PopUp: On 1x detected AudioChannel
bool CTuxBoxUtil::GetAudioChannels(CStdString& strAudioChannelName, CStdString& strAudioChannelPid)
{
  //DVDPlayer Can play all AudioStreams! No need to popup the AudioChannel selector!
  if(g_application.m_eForcedNextPlayer == EPC_DVDPLAYER)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - DVDPlayer is used to play the Stream! Disabling Audio Channel Selection! Returning False to use All Possible Audio channels!");
    return false;
  }
  // Audio Selection is Disabled! Return false to use default values!
  if(!g_advancedSettings.m_bTuxBoxAudioChannelSelection)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - Audio Channel Selection is Disabled! Returning False to use the default values!");
    return false;
  }
  // We have only one Audio Channel return false to use default values!
  if(sCurSrvData.audio_channel_1_name.IsEmpty() && sCurSrvData.audio_channel_2_name.IsEmpty())
    return false;

  // popup the context menu
  CGUIDialogContextMenu *pMenu;
  pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    // load Audio context menu
    pMenu->Initialize();
    // add the needed Audio buttons
    int btn_AudioChannel_0=-1;
    if(!sCurSrvData.audio_channel_0_name.IsEmpty())
      btn_AudioChannel_0 = pMenu->AddButton(sCurSrvData.audio_channel_0_name); // A0

    int btn_AudioChannel_1=-1;
    if(!sCurSrvData.audio_channel_1_name.IsEmpty())
      btn_AudioChannel_1 = pMenu->AddButton(sCurSrvData.audio_channel_1_name); // A1

    int btn_AudioChannel_2=-1;
    if(!sCurSrvData.audio_channel_2_name.IsEmpty())
      btn_AudioChannel_2 = pMenu->AddButton(sCurSrvData.audio_channel_2_name); // A2

    pMenu->CenterWindow();
    pMenu->DoModal();
    int btnid = pMenu->GetButton();
    if (btnid == btn_AudioChannel_0)
    {
      strAudioChannelName = sCurSrvData.audio_channel_0_name;
      strAudioChannelPid = sCurSrvData.audio_channel_0_pid;
      return true;
    }
    if (btnid == btn_AudioChannel_1)
    {
      strAudioChannelName = sCurSrvData.audio_channel_1_name;
      strAudioChannelPid = sCurSrvData.audio_channel_1_pid;
      return true;
    }
    if (btnid == btn_AudioChannel_2)
    {
      strAudioChannelName = sCurSrvData.audio_channel_2_name;
      strAudioChannelPid = sCurSrvData.audio_channel_2_pid;
      return true;
    }
  }
  return false;
}
bool CTuxBoxUtil::GetVideoSubChannels(CStdString& strVideoSubChannelName, CStdString& strVideoSubChannelPid)
{
  // no video sub channel return false!
  if(vVideoSubChannel.name.size() <= 0 || vVideoSubChannel.reference.size() <= 0)
    return false;

  // IsPlaying, Stop it..
  if(g_application.IsPlaying())
    g_applicationMessenger.MediaStop();

  // popup the context menu
  CGUIDialogContextMenu *pMenu;
  pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    pMenu->Initialize();
    // load Video Sub Channels to context menu
    std::vector<int> btn;
    for (unsigned int i=0; vVideoSubChannel.name.size() > i; ++i)
      btn.push_back(pMenu->AddButton(vVideoSubChannel.name[i]));
    
    pMenu->CenterWindow();
    pMenu->DoModal();
    // get selected Video Sub Channel name and reference zap
    int btnid = pMenu->GetButton();
    for(int i=0; btn[i] >0; i++)
    {
      if(btnid == btn[i])
      {
        strVideoSubChannelName = vVideoSubChannel.name[i];
        strVideoSubChannelPid = vVideoSubChannel.reference[i];
        btn.clear();
        vVideoSubChannel.name.clear();
        vVideoSubChannel.reference.clear();
        vVideoSubChannel.selected.clear();
        return true;
      }
    }
  }
  return false;
}
//Input: Service Name (Channel Namne)
//Output: picon url (on ERROR the default icon path will be returned)
CStdString CTuxBoxUtil::GetPicon(CStdString strServiceName)
{
  if(!g_advancedSettings.m_bTuxBoxPictureIcon)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" PictureIcon Detection is Disabled! Using default icon");
    return "";
  }
  if (strServiceName.IsEmpty())
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" Service Name is Empty! Can not detect a PictureIcon. Using default icon!");
    return "";
  }
  else
  {
    CStdString piconXML, piconPath, defaultPng;
    CStdString strName, strPng;
    piconPath = "Q:\\userdata\\pictureicon\\Picon\\";
    defaultPng = piconPath+"default.png";
    piconXML = "Q:\\userdata\\pictureicon\\picon.xml";
    TiXmlDocument piconDoc;
    bool bPngError = false;
    if (!CFile::Exists(piconXML))
    { 
      return defaultPng;
    }
    if (!piconDoc.LoadFile(piconXML))
    {
      CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", piconXML.c_str(), piconDoc.ErrorRow(), piconDoc.ErrorDesc());
      return defaultPng;
    }

    TiXmlElement *pRootElement = piconDoc.RootElement();
    if (!pRootElement || strcmpi(pRootElement->Value(),"picon") != 0)
    {
      CLog::Log(LOGERROR, "Error loading %s, no <picon> node", piconXML.c_str());
      return defaultPng;
    }

    TiXmlElement* pServices = pRootElement->FirstChildElement("services");
    TiXmlElement* pService;
    pService = pServices->FirstChildElement("service");
    while(pService)
    {
      if(pService->Attribute("name"))
      {
        strName.Format("%s",pService->Attribute("name"));
      }
      if(pService->Attribute("png"))
      {
        strPng.Format("%s",pService->Attribute("png"));
      }
      
      if(strName.Equals(strServiceName))
      {
        strPng.Format("%s%s",piconPath.c_str(), strPng.c_str());
        strPng.ToLower();
        CLog::Log(LOGDEBUG, __FUNCTION__" %s: Path is: %s",strServiceName.c_str(), strPng.c_str());
        return strPng;
      }
      pService = pService->NextSiblingElement("service");
    }
    return defaultPng;
  }
}

// iMODE: 0 = TV, 1 = Radio, 2 = Data, 3 = Movies, 4 = Root
// SUBMODE: 0 = n/a, 1 = All, 2 = Satellites, 2 = Providers, 4 = Bouquets
CStdString CTuxBoxUtil::GetSubMode(int iMode, CStdString& strXMLRootString, CStdString& strXMLChildString)
{
  //Todo: add a setting: "Don't Use Request mode" to advanced.xml
  
  // MODE: 0 = TV, 1 = Radio, 2 = Data, 3 = Movies, 4 = Root
	// SUBMODE: 0 = n/a, 1 = All, 2 = Satellites, 2 = Providers, 4 = Bouquets
  // Default Submode
  int iSubmode = 4;
  CStdString strSubMode;

  if(iMode <0 || iMode >4)
  {
    strSubMode.Format("xml/services?mode=0&submode=4");
    strXMLRootString.Format("bouquets");
    strXMLChildString.Format("bouquet");
    return strSubMode;
  }
  
  // popup the context menu
  CGUIDialogContextMenu *pMenu;
  pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (pMenu)
  {
    // load Audio context menu
    pMenu->Initialize();
    // add the needed Audio buttons
    int iSubmode_1 = pMenu->AddButton("All"); //;
    int iSubmode_2 = pMenu->AddButton("Satellites"); //;
    int iSubmode_3 = pMenu->AddButton("Providers");
    int iSubmode_4 = pMenu->AddButton("Bouquets");
    
    pMenu->CenterWindow();
    pMenu->DoModal();
    int btnid = pMenu->GetButton();
    if(btnid == iSubmode_1)
    {
      iSubmode = 1;
      strXMLRootString.Format("services");
      strXMLChildString.Format("service");
    }
    else if(btnid == iSubmode_2)
    {
      iSubmode = 2;
      strXMLRootString.Format("satellites");
      strXMLChildString.Format("satellite");

    }
    else if(btnid == iSubmode_3)
    {
      iSubmode = 3;
      strXMLRootString.Format("providers");
      strXMLChildString.Format("provider");
    }
    else if(btnid == iSubmode_4)
    {
      iSubmode = 4;
      strXMLRootString.Format("bouquets");
      strXMLChildString.Format("bouquet");
    }
  }
  strSubMode.Format("xml/services?mode=%i&submode=%i",iMode,iSubmode);
  return strSubMode;
}
//Input: url/path of share/item file/folder
//Output: the detected submode root and child string
CStdString CTuxBoxUtil::DetectSubMode(CStdString strSubMode, CStdString& strXMLRootString, CStdString& strXMLChildString)
{
  //strSubMode = "xml/services?mode=0&submode=1"
  CStdString strFilter;
  int ipointMode = strSubMode.Find("?mode=");
  int ipointSubMode = strSubMode.Find("&submode=");
  if (ipointMode >=0)
  {
    strFilter = strSubMode.GetAt(ipointMode+6);
  }
  if (ipointSubMode >=0)
  {
    CStdString strTemp;
    strTemp = strSubMode.GetAt(ipointSubMode+9);
    if(strTemp.Equals("1"))
    {
      strXMLRootString.Format("unknowns");
      strXMLChildString.Format("unknown");
    }
    else if(strTemp.Equals("2"))
    {
      strXMLRootString.Format("satellites");
      strXMLChildString.Format("satellite");
    }
    else if(strTemp.Equals("3"))
    {
      strXMLRootString.Format("providers");
      strXMLChildString.Format("provider");
    }
    else if(strTemp.Equals("4"))
    {
      strXMLRootString.Format("bouquets");
      strXMLChildString.Format("bouquet");
    }

  }
  return strFilter;
}
