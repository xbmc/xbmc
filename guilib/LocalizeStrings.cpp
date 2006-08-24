#include "include.h"
#include "LocalizeStrings.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "XMLUtils.h"

// can be removed after hardcoded strings are in strings.xml
CStdString ToUTF8(const CStdString& strEncoding, const CStdString& str)
{
  if (strEncoding.IsEmpty())
    return str;

  CStdString ret;
  g_charsetConverter.stringCharsetToUtf8(strEncoding, str, ret);
  return ret;
}

CLocalizeStrings g_localizeStrings;
extern CStdString g_LoadErrorStr;

CLocalizeStrings::CLocalizeStrings(void)
{

}

CLocalizeStrings::~CLocalizeStrings(void)
{

}

bool CLocalizeStrings::Load(const CStdString& strFileName)
{
  m_vecStrings.erase(m_vecStrings.begin(), m_vecStrings.end());
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strFileName.c_str()))
  {
    CLog::Log(LOGERROR, "unable to load %s: %s at line %d", strFileName.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    g_LoadErrorStr.Format("%s, Line %d\n%s", strFileName.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }
  
  CStdString strEncoding;
  XMLUtils::GetEncoding(&xmlDoc, strEncoding);

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || 
       pRootElement->Value()!=CStdString("strings"))
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <strings>", strFileName.c_str());
    g_LoadErrorStr.Format("%s\nDoesnt start with <strings>", strFileName.c_str());
    return false;
  }

  const TiXmlElement *pChild = pRootElement->FirstChildElement("string");
  while (pChild)
  {
    const TiXmlNode *pChildID = pChild->FirstChild("id");
    const TiXmlNode *pChildText = pChild->FirstChild("value");
    if (pChildID && !pChildID->NoChildren() &&
        pChildText && !pChildText->NoChildren())
    { // Load old style language file with nodes for id and value
      DWORD dwID = atoi(pChildID->FirstChild()->Value());

      CStdString utf8String;
      if (strEncoding.IsEmpty()) // Is language file utf8?
        utf8String=pChildText->FirstChild()->Value();
      else
        g_charsetConverter.stringCharsetToUtf8(strEncoding, pChildText->FirstChild()->Value(), utf8String);

      m_vecStrings[dwID] = utf8String;
    }
    else
    { // Load new style language file with id as attribute
      const char* attrId=pChild->Attribute("id");
      if (attrId && !pChild->NoChildren())
      {
        DWORD dwID = atoi(attrId);
        CStdString utf8String;
        if (strEncoding.IsEmpty()) // Is language file utf8?
          utf8String=pChild->FirstChild()->Value();
        else
          g_charsetConverter.stringCharsetToUtf8(strEncoding, pChild->FirstChild()->Value(), utf8String);

        m_vecStrings[dwID] = utf8String;
      }
    }
    pChild = pChild->NextSiblingElement("string");
  }

  if (!strFileName.Equals("Q:\\language\\english\\strings.xml"))
  {
    // load the original english file
    // and copy any missing texts
    TiXmlDocument xmlDoc;
    if ( !xmlDoc.LoadFile("Q:\\language\\english\\strings.xml") )
    {
      return true;
    }

    XMLUtils::GetEncoding(&xmlDoc, strEncoding);

    TiXmlElement* pRootElement = xmlDoc.RootElement();
    if (!pRootElement || pRootElement->NoChildren() || 
         pRootElement->Value()!=CStdString("strings")) return false;

    const TiXmlElement *pChild = pRootElement->FirstChildElement("string");
    while (pChild)
    {
      const TiXmlNode *pChildID = pChild->FirstChild("id");
      const TiXmlNode *pChildText = pChild->FirstChild("value");
      if (pChildID && !pChildID->NoChildren() &&
          pChildText && !pChildText->NoChildren())
      { // Load old style language file with nodes for id and value
        DWORD dwID = atoi(pChildID->FirstChild()->Value());

        ivecStrings i = m_vecStrings.find(dwID);
        if (i == m_vecStrings.end())
        {
          CStdString utf8String;
          if (strEncoding.IsEmpty()) // Is language file utf8?
            utf8String=pChildText->FirstChild()->Value();
          else
            g_charsetConverter.stringCharsetToUtf8(strEncoding, pChildText->FirstChild()->Value(), utf8String);

          m_vecStrings[dwID] = utf8String;
        }
      }
      else
      { // Load new style language file with id as attribute
        const char* attrId=pChild->Attribute("id");
        if (attrId && !pChild->NoChildren())
        {
          DWORD dwID = atoi(attrId);
          ivecStrings i = m_vecStrings.find(dwID);
          if (i == m_vecStrings.end())
          {
            CStdString utf8String;
            if (strEncoding.IsEmpty()) // Is language file utf8?
              utf8String=pChild->FirstChild()->Value();
            else
              g_charsetConverter.stringCharsetToUtf8(strEncoding, pChild->FirstChild()->Value(), utf8String);

            m_vecStrings[dwID] = utf8String;
          }
        }
      }
      pChild = pChild->NextSiblingElement("string");
    }

  }

  // TODO: localize 2.0
  // Hardcoded strings (starting at 20000)
  m_vecStrings[20000] = "CDDA Rip Folder";
  m_vecStrings[20001] = "Use external DVD Player";
  m_vecStrings[20002] = "External DVD Player";
  m_vecStrings[20003] = "Trainers Folder";
  m_vecStrings[20004] = "Screenshot Folder";
  m_vecStrings[20005] = "Recordings Folder";
  m_vecStrings[20006] = "Playlists Folder";
  m_vecStrings[20007] = "Recordings";
  m_vecStrings[20008] = "Screenshots";
  m_vecStrings[20009] = "Use XBMC";
  m_vecStrings[20010] = "Artist Image";
  m_vecStrings[20011] = "Music Playlists";
  m_vecStrings[20012] = "Video Playlists";
  m_vecStrings[20013] = "Do you wish to launch the game?";
  m_vecStrings[20014] = "Sort by: Playlist";
  m_vecStrings[20015] = "IMDb Thumb";
  m_vecStrings[20016] = "Current Thumb";
  m_vecStrings[20017] = "Local Thumb";
  m_vecStrings[20018] = "No Thumb";
  m_vecStrings[20019] = "Thumbnail";
  m_vecStrings[20020] = "Cannot use both KAI and trainer";
  m_vecStrings[20021] = "Choose which you want to use";
  m_vecStrings[20022] = "";
  m_vecStrings[20023] = "Conflict";
  m_vecStrings[20024] = "Scan new";
  m_vecStrings[20025] = "Scan all"; 
  m_vecStrings[20026] = "Region"; 
  m_vecStrings[20027] = ToUTF8(strEncoding, "°F");
  m_vecStrings[20028] = ToUTF8(strEncoding, "K");
  m_vecStrings[20029] = ToUTF8(strEncoding, "°C");
  m_vecStrings[20030] = ToUTF8(strEncoding, "°Ré");
  m_vecStrings[20031] = ToUTF8(strEncoding, "°Ra"); 
  m_vecStrings[20032] = ToUTF8(strEncoding, "°Rø"); 
  m_vecStrings[20033] = ToUTF8(strEncoding, "°De"); 
  m_vecStrings[20034] = ToUTF8(strEncoding, "°N");
  m_vecStrings[20038] = "Lock music section";
  m_vecStrings[20039] = "Lock video section";
  m_vecStrings[20040] = "Lock pictures section";
  m_vecStrings[20041] = "Lock programs and scripts sections";
  m_vecStrings[20042] = "Lock filemanager";
  m_vecStrings[20043] = "Lock settings";
  m_vecStrings[20044] = "Start fresh";
  m_vecStrings[20045] = "Enter master mode";
  m_vecStrings[20046] = "Leave master mode";
  m_vecStrings[20047] = "Create profile '%s' ?";
  m_vecStrings[20048] = "Start with fresh settings";
  m_vecStrings[20049] = "Best Available";
  m_vecStrings[20050] = "Autoswitch between 16x9 and 4x3";
  m_vecStrings[20051] = "Treat stacked files as single file";
  m_vecStrings[20052] = "Caution";
  m_vecStrings[20053] = "Left master mode";
  m_vecStrings[20054] = "Entered master mode";
  m_vecStrings[20055] = "Allmusic.com Thumb";
  m_vecStrings[20056] = "Source Thumbnail";
  m_vecStrings[20057] = "Remove Thumbnail";
  m_vecStrings[20058] = "Add Profile...";
  m_vecStrings[20059] = "Query Info For All Albums";
  m_vecStrings[20060] = "Media info";
  m_vecStrings[20061] = "Separate";
  m_vecStrings[20062] = "Shares with default";
  m_vecStrings[20063] = "Shares with default (read only)";
  m_vecStrings[20064] = "Copy default";
  m_vecStrings[20065] = "Profile picture";
  m_vecStrings[20066] = "Lock preferences";
  m_vecStrings[20067] = "Edit profile";
  m_vecStrings[20068] = "Profile lock";
  m_vecStrings[20069] = "Could not create folder";
  m_vecStrings[20070] = "Profile directory";
  m_vecStrings[20071] = "Start with fresh media sources";
  m_vecStrings[20072] = "Make sure that the selected folder is writable";
  m_vecStrings[20073] = "and that the new folder name is valid.";
  m_vecStrings[20074] = "MPAA Rating:";
  m_vecStrings[20075] = "Enter master lock code";
  m_vecStrings[20076] = "Ask for master lock code on startup";
  m_vecStrings[20077] = "Skin Settings";
  m_vecStrings[20078] = "- no link set -";
  m_vecStrings[20079] = "Enable Animations";
  m_vecStrings[20080] = "Disable RSS during Music";
  m_vecStrings[20081] = "Enable Bookmarks";
  m_vecStrings[20082] = "Show XLink Kai info";
  m_vecStrings[20083] = "Show Music Info";
  m_vecStrings[20084] = "Show Weather Info";
  m_vecStrings[20085] = "Show System Info";
  m_vecStrings[20086] = "Show Available Disc Space C: E: F:";
  m_vecStrings[20087] = "Show Available Disc Space E: F: G:";
  m_vecStrings[20088] = "Weather Info";
  m_vecStrings[20089] = "Drive Space Free";
  m_vecStrings[20090] = "Enter the name of an existing Share";
  m_vecStrings[20091] = "Lock Code";
  m_vecStrings[20092] = "Load profile";
  m_vecStrings[20093] = "Profile name";
  m_vecStrings[20094] = "Media sources";
  m_vecStrings[20095] = "Enter profile lock code";
  m_vecStrings[20096] = "Login screen";
  m_vecStrings[20097] = "Fetching album info";
  m_vecStrings[20098] = "Fetching info for album";
  m_vecStrings[20099] = "Can't rip CD or Track while playing from CD";
  m_vecStrings[20100] = "Master code and Locks";
  m_vecStrings[20101] = "Entering master code always enables master mode";
  m_vecStrings[20102] = "or copy from default?";
  m_vecStrings[20103] = "Save changes to profile?";
  m_vecStrings[20104] = "Old settings found.";
  m_vecStrings[20105] = "Do you want to use them?";
  m_vecStrings[20106] = "Old media sources found.";
  m_vecStrings[20107] = "Separate (locked)";
  m_vecStrings[20108] = "Root";
  m_vecStrings[20109] = "Skin Zoom";
  m_vecStrings[20110] = "UPnP Client";
  m_vecStrings[20111] = "Autostart";
  m_vecStrings[20112] = "Last login: %s";
  m_vecStrings[20113] = "Never logged on";
  m_vecStrings[20114] = "Profile %i / %i";
  m_vecStrings[20115] = "User Login / Select a Profile";
  m_vecStrings[20116] = "Use locks on login screen";
  m_vecStrings[20117] = "Invalid lock code.";
  m_vecStrings[20118] = "Requires the master lock to be set.";
  m_vecStrings[20119] = "Would you like to set it now?";
  m_vecStrings[20120] = "Loading program information";
  m_vecStrings[20121] = "Party on!";
  m_vecStrings[20122] = "True";
  m_vecStrings[20123] = "Filtering songs";
  m_vecStrings[20124] = "Adding songs";
  m_vecStrings[20125] = "Logged on as";
  m_vecStrings[20126] = "Log off";
  m_vecStrings[20127] = "Hide 'all' items in library views";
  m_vecStrings[20128] = "Go to Root";

  m_vecStrings[20200] = ToUTF8(strEncoding, "km/h");
  m_vecStrings[20201] = ToUTF8(strEncoding, "m/min");
  m_vecStrings[20202] = ToUTF8(strEncoding, "m/s");
  m_vecStrings[20203] = ToUTF8(strEncoding, "ft/h");
  m_vecStrings[20204] = ToUTF8(strEncoding, "ft/min");
  m_vecStrings[20205] = ToUTF8(strEncoding, "ft/s");
  m_vecStrings[20206] = ToUTF8(strEncoding, "mph");
  m_vecStrings[20207] = ToUTF8(strEncoding, "kts");
  m_vecStrings[20208] = ToUTF8(strEncoding, "Beaufort");
  m_vecStrings[20209] = ToUTF8(strEncoding, "inch/s");
  m_vecStrings[20210] = ToUTF8(strEncoding, "yard/s");
  m_vecStrings[20211] = ToUTF8(strEncoding, "Furlong/Fortnight");

  // new strings for weather localization
  m_vecStrings[1411] = "with";
  m_vecStrings[1412] = "windy";

  m_vecStrings[16020] = "Deinterlace";    //OVERRIDE
  m_vecStrings[16021] = "Bob";            //OVERRIDE
  m_vecStrings[16022] = "Bob (Inverted)"; //OVERRIDE

  m_vecStrings[20129] = "Weave";            //NEW
  m_vecStrings[20130] = "Weave (Inverted)"; //NEW
  m_vecStrings[20131] = "Blend";            //NEW

  m_vecStrings[20132] = "Restart Video";
  m_vecStrings[20133] = "Edit Network Location";
  m_vecStrings[20134] = "Remove Network Location";
  m_vecStrings[20135] = "Do you want to scan the folder?";
  m_vecStrings[20136] = "Memory Unit";
  m_vecStrings[20137] = "UnMounted from Drive:";
  m_vecStrings[20138] = "Mounted to Drive:";
  m_vecStrings[20139] = "Unable to mount!";
  m_vecStrings[20140] = "Lock Screensaver";

  return true;
}

static CStdString szEmptyString = "";

const CStdString& CLocalizeStrings::Get(DWORD dwCode) const
{
  ivecStrings i;
  if (dwCode == 20045)
  {
    CLog::DebugLog("fisemannen!");
  }
  i = m_vecStrings.find(dwCode);
  if (i == m_vecStrings.end())
  {
    return szEmptyString;
  }
  return i->second;
}

void CLocalizeStrings::Clear()
{
  m_vecStrings.erase(m_vecStrings.begin(), m_vecStrings.end());
}
