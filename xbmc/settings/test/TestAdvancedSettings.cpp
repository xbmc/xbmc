/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "network/DNSNameCache.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#include <array>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace
{
class CAdvancedSettingsInit : public CAdvancedSettings
{
public:
  CAdvancedSettingsInit() { Initialize(); }

  void DoRedact(CXBMCTinyXML& doc) { Redact(doc); }
};

struct RegExpT
{
  enum class Action
  {
    APPEND,
    APPEND_OLD,
    OVERWRITE,
    PREPEND,
  };
  std::string_view suite;
  std::string_view tag;
  std::vector<std::string> CAdvancedSettings::*member;
};

class RegExpTest : public testing::WithParamInterface<RegExpT>, public testing::Test
{
};

std::string GenREXML(std::string_view suite, std::string_view tag, RegExpT::Action action)
{
  auto getAction = [action]() -> std::string_view
  {
    switch (action)
    {
      case RegExpT::Action::APPEND:
        return R"(action="append")";
      case RegExpT::Action::APPEND_OLD:
        return R"(append="yes")";
      case RegExpT::Action::OVERWRITE:
        return "";
      case RegExpT::Action::PREPEND:
        return R"(action="prepend")";
    }
    std::exit(1);
  };

  if (suite.empty())
    return StringUtils::Format(R"(<advancedsettings>
                                  <{0} {1}>
                                    <regexp>1</regexp>
                                    <regexp>2</regexp>
                                  </{0}>
                                </advancedsettings>)",
                               tag, getAction());
  else
    return StringUtils::Format(R"(<advancedsettings>
                                  <{0}>
                                    <{1} {2}>
                                      <regexp>1</regexp>
                                      <regexp>2</regexp>
                                    </{1}>
                                  </{0}>
                                </advancedsettings>)",
                               suite, tag, getAction());
}

template<RegExpT::Action action>
void RunRETest(const RegExpT& param)
{
  CXBMCTinyXML doc;
  doc.Parse(GenREXML(param.suite, param.tag, action));
  CAdvancedSettingsInit settings;
  const auto orig_size = std::invoke(param.member, settings).size();
  settings.ParseSettingsXML(doc.RootElement());
  const auto& array = std::invoke(param.member, settings);
  if constexpr (action == RegExpT::Action::OVERWRITE)
  {
    EXPECT_EQ(array.size(), 2);
    EXPECT_EQ(array[0], "1");
    EXPECT_EQ(array[1], "2");
  }
  else if constexpr (action == RegExpT::Action::APPEND || action == RegExpT::Action::APPEND_OLD)
  {
    EXPECT_EQ(array.size(), orig_size + 2);
    EXPECT_EQ(array[orig_size], "1");
    EXPECT_EQ(array[orig_size + 1], "2");
  }
  else if constexpr (action == RegExpT::Action::PREPEND)
  {
    EXPECT_EQ(array.size(), orig_size + 2);
    EXPECT_EQ(array[0], "1");
    EXPECT_EQ(array[1], "2");
  }
}

struct ExtT
{
  std::string_view tag;
  std::string_view add;
  std::string_view remove;
  std::string CAdvancedSettings::*member;
};

class ExtensionTest : public testing::WithParamInterface<ExtT>, public testing::Test
{
};

std::string GenExtXML(const ExtT& param)
{
  return StringUtils::Format(R"(<advancedsettings>
                                <{0}>
                                  <add>{1}</add>
                                  <remove>{2}</remove>
                                </{0}>
                              </advancedsettings>)",
                             param.tag, param.add, param.remove);
}

struct DbTestT
{
  std::string_view tag;
  DatabaseSettings CAdvancedSettings::*member;
};

class DatabaseTest : public testing::WithParamInterface<DbTestT>, public testing::Test
{
};

std::string GenDbXML(std::string_view tag, const DatabaseSettings& param)
{
  return StringUtils::Format(R"(<advancedsettings>
                                <{0}>
                                 <type>{1}</type>
                                 <host>{2}</host>
                                 <port>{3}</port>
                                 <user>{4}</user>
                                 <pass>{5}</pass>
                                 <name>{6}</name>
                                 <key>{7}</key>
                                 <cert>{8}</cert>
                                 <ca>{9}</ca>
                                 <capath>{10}</capath>
                                 <ciphers>{11}</ciphers>
                                 <connecttimeout>{12}</connecttimeout>
                                 <compression>{13}</compression>
                                </{0}>
                              </advancedsettings>)",
                             tag, param.type, param.host, param.port, param.user, param.pass,
                             param.name, param.key, param.cert, param.ca, param.capath,
                             param.ciphers, param.connecttimeout, param.compression);
}

} // namespace

TEST(TestAdvancedSettings, Audio)
{
  const std::string xml =
      R"(<advancedsettings>
           <audio>
             <defaultplayer>1</defaultplayer>
             <playcountminimumpercent>101</playcountminimumpercent>
             <usetimeseeking>false</usetimeseeking>
             <timeseekforward>20</timeseekforward>
             <timeseekforwardbig>200</timeseekforwardbig>
             <timeseekbackward>-30</timeseekbackward>
             <timeseekbackwardbig>-300</timeseekbackwardbig>
             <percentseekforward>5</percentseekforward>
             <percentseekforwardbig>50</percentseekforwardbig>
             <percentseekbackward>-3</percentseekbackward>
             <percentseekbackwardbig>-7</percentseekbackwardbig>
             <applydrc>7.3</applydrc>
             <limiterhold>2.5</limiterhold>
             <limiterrelease>8.5</limiterrelease>
             <maxpassthroughoffsyncduration>40</maxpassthroughoffsyncduration>
             <allowmultichannelfloat>true</allowmultichannelfloat>
             <superviseaudiodelay>true</superviseaudiodelay>
           </audio>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_audioDefaultPlayer, "1");
  EXPECT_FLOAT_EQ(settings.m_audioPlayCountMinimumPercent, 101.f);
  EXPECT_FALSE(settings.m_musicUseTimeSeeking);
  EXPECT_EQ(settings.m_musicTimeSeekForward, 20);
  EXPECT_EQ(settings.m_musicTimeSeekForwardBig, 200);
  EXPECT_EQ(settings.m_musicTimeSeekBackward, -30);
  EXPECT_EQ(settings.m_musicTimeSeekBackwardBig, -300);
  EXPECT_EQ(settings.m_musicPercentSeekForward, 5);
  EXPECT_EQ(settings.m_musicPercentSeekForwardBig, 50);
  EXPECT_EQ(settings.m_musicPercentSeekBackward, -3);
  EXPECT_EQ(settings.m_musicPercentSeekBackwardBig, -7);
  EXPECT_FLOAT_EQ(settings.m_audioApplyDrc, 7.3f);
  EXPECT_FLOAT_EQ(settings.m_limiterHold, 2.5f);
  EXPECT_FLOAT_EQ(settings.m_limiterRelease, 8.5f);
  EXPECT_EQ(settings.m_maxPassthroughOffSyncDuration, 40);
  EXPECT_TRUE(settings.m_AllowMultiChannelFloat);
  EXPECT_TRUE(settings.m_superviseAudioDelay);
}

TEST(TestAdvancedSettings, X11)
{
  const std::string xml =
      R"(<advancedsettings>
           <x11>
             <omlsync>false</omlsync>
           </x11>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_FALSE(settings.m_omlSync);
}

TEST(TestAdvancedSettings, Video)
{
  const std::string xml =
      R"(<advancedsettings>
           <video>
             <stereoscopicregex3d>1</stereoscopicregex3d>
             <stereoscopicregexsbs>2</stereoscopicregexsbs>
             <stereoscopicregextab>3</stereoscopicregextab>
             <subsdelayrange>20</subsdelayrange>
             <subsdelaystep>0.5</subsdelaystep>
             <audiodelayrange>30</audiodelayrange>
             <audiodelaystep>0.4</audiodelaystep>
             <defaultplayer>4</defaultplayer>
             <fullscreenonmoviestart>false</fullscreenonmoviestart>
             <playcountminimumpercent>101</playcountminimumpercent>
             <ignoresecondsatstart>123</ignoresecondsatstart>
             <usetimeseeking>false</usetimeseeking>
             <timeseekforward>10</timeseekforward>
             <timeseekforwardbig>100</timeseekforwardbig>
             <timeseekbackward>-20</timeseekbackward>
             <timeseekbackwardbig>-200</timeseekbackwardbig>
             <percentseekforward>7</percentseekforward>
             <percentseekforwardbig>70</percentseekforwardbig>
             <percentseekbackward>-4</percentseekbackward>
             <percentseekbackwardbig>-6</percentseekbackwardbig>
             <filenameidentifier>5</filenameidentifier>
             <cleandatetime>6</cleandatetime>
             <ppffmpegpostprocessing>7</ppffmpegpostprocessing>
             <vdpauscaling>1</vdpauscaling>
             <nonlinearstretchratio>0.2</nonlinearstretchratio>
             <autoscalemaxfps>50</autoscalemaxfps>
             <useocclusionquery>1</useocclusionquery>
             <vdpauInvTelecine>true</vdpauInvTelecine>
             <vdpauHDdeintSkipChroma>true</vdpauHDdeintSkipChroma>
             <bypasscodecprofile>true</bypasscodecprofile>
             <checkdxvacompatibility>true</checkdxvacompatibility>
             <fpsdetect>2</fpsdetect>
             <maxtempo>2.0</maxtempo>
             <preferstereostream>true</preferstereostream>
           </video>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_stereoscopicregex_3d, "1");
  EXPECT_EQ(settings.m_stereoscopicregex_sbs, "2");
  EXPECT_EQ(settings.m_stereoscopicregex_tab, "3");
  EXPECT_EQ(settings.m_videoSubsDelayRange, 20.f);
  EXPECT_EQ(settings.m_videoSubsDelayStep, 0.5f);
  EXPECT_EQ(settings.m_videoAudioDelayRange, 30.f);
  EXPECT_EQ(settings.m_videoAudioDelayStep, 0.4f);
  EXPECT_EQ(settings.m_videoDefaultPlayer, "4");
  EXPECT_FALSE(settings.m_fullScreenOnMovieStart);
  EXPECT_EQ(settings.m_videoPlayCountMinimumPercent, 101.f);
  EXPECT_EQ(settings.m_videoIgnoreSecondsAtStart, 123);
  EXPECT_FALSE(settings.m_videoUseTimeSeeking);
  EXPECT_EQ(settings.m_videoTimeSeekForward, 10);
  EXPECT_EQ(settings.m_videoTimeSeekForwardBig, 100);
  EXPECT_EQ(settings.m_videoTimeSeekBackward, -20);
  EXPECT_EQ(settings.m_videoTimeSeekBackwardBig, -200);
  EXPECT_EQ(settings.m_videoPercentSeekForward, 7);
  EXPECT_EQ(settings.m_videoPercentSeekForwardBig, 70);
  EXPECT_EQ(settings.m_videoPercentSeekBackward, -4);
  EXPECT_EQ(settings.m_videoPercentSeekBackwardBig, -6);
  EXPECT_EQ(settings.m_videoFilenameIdentifierRegExp, "5");
  EXPECT_EQ(settings.m_videoCleanDateTimeRegExp, "6");
  EXPECT_EQ(settings.m_videoPPFFmpegPostProc, "7");
  EXPECT_EQ(settings.m_videoVDPAUScaling, 1);
  EXPECT_EQ(settings.m_videoNonLinStretchRatio, 0.2f);
  EXPECT_EQ(settings.m_videoAutoScaleMaxFps, 50.f);
  EXPECT_EQ(settings.m_videoCaptureUseOcclusionQuery, 1);
  EXPECT_TRUE(settings.m_videoVDPAUtelecine);
  EXPECT_TRUE(settings.m_videoVDPAUdeintSkipChromaHD);
  EXPECT_TRUE(settings.m_videoBypassCodecProfile);
  EXPECT_TRUE(settings.m_DXVACheckCompatibility);
  EXPECT_EQ(settings.m_videoFpsDetect, 2);
  EXPECT_EQ(settings.m_maxTempo, 2.f);
  EXPECT_TRUE(settings.m_videoPreferStereoStream);

  // TODO: Refresh stuff
}

TEST(TestAdvancedSettings, MusicLibrary)
{
  const std::string xml =
      R"(<advancedsettings>
           <musiclibrary>
             <recentlyaddeditems>1</recentlyaddeditems>
             <prioritiseapetags>true</prioritiseapetags>
             <allitemsonbottom>true</allitemsonbottom>
             <cleanonupdate>true</cleanonupdate>
             <artistsortonupdate>true</artistsortonupdate>
             <albumformat>1</albumformat>
             <itemseparator>2</itemseparator>
             <dateadded>0</dateadded>
             <useisodates>true</useisodates>
             <artistnavigatestosongs>true</artistnavigatestosongs>
             <artistseparators>
              <separator>3</separator>
              <separator>4</separator>
             </artistseparators>
           </musiclibrary>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_iMusicLibraryRecentlyAddedItems, 1);
  EXPECT_TRUE(settings.m_prioritiseAPEv2tags);
  EXPECT_TRUE(settings.m_bMusicLibraryAllItemsOnBottom);
  EXPECT_TRUE(settings.m_bMusicLibraryCleanOnUpdate);
  EXPECT_TRUE(settings.m_bMusicLibraryArtistSortOnUpdate);
  EXPECT_EQ(settings.m_strMusicLibraryAlbumFormat, "1");
  EXPECT_EQ(settings.m_musicItemSeparator, "2");
  EXPECT_EQ(settings.m_iMusicLibraryDateAdded, 0);
  EXPECT_TRUE(settings.m_bMusicLibraryUseISODates);
  EXPECT_TRUE(settings.m_bMusicLibraryArtistNavigatesToSongs);
  EXPECT_EQ(settings.m_musicArtistSeparators.size(), 2U);
  EXPECT_EQ(settings.m_musicArtistSeparators[0], "3");
  EXPECT_EQ(settings.m_musicArtistSeparators[1], "4");
}

TEST(TestAdvancedSettings, VideoLibrary)
{
  const std::string xml =
      R"(<advancedsettings>
           <videolibrary>
             <allitemsonbottom>true</allitemsonbottom>
             <recentlyaddeditems>2</recentlyaddeditems>
             <cleanonupdate>true</cleanonupdate>
             <usefasthash>false</usefasthash>
             <itemseparator>1</itemseparator>
             <importwatchedstate>false</importwatchedstate>
             <importresumepoint>false</importresumepoint>
             <dateadded>2</dateadded>
             <casesensitivelocalartmatch>false</casesensitivelocalartmatch>
             <minimumepisodeplaylistduration>3</minimumepisodeplaylistduration>
             <disableepisoderanges>true</disableepisoderanges>
             <noremoteartwithlocalscraper>true</noremoteartwithlocalscraper>
           </videolibrary>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_TRUE(settings.m_bVideoLibraryAllItemsOnBottom);
  EXPECT_EQ(settings.m_iVideoLibraryRecentlyAddedItems, 2);
  EXPECT_TRUE(settings.m_bVideoLibraryCleanOnUpdate);
  EXPECT_FALSE(settings.m_bVideoLibraryUseFastHash);
  EXPECT_EQ(settings.m_videoItemSeparator, "1");
  EXPECT_FALSE(settings.m_bVideoLibraryImportWatchedState);
  EXPECT_FALSE(settings.m_bVideoLibraryImportResumePoint);
  EXPECT_EQ(settings.m_iVideoLibraryDateAdded, 2);
  EXPECT_FALSE(settings.m_caseSensitiveLocalArtMatch);
  EXPECT_EQ(settings.m_minimumEpisodePlaylistDuration, 3);
  EXPECT_TRUE(settings.m_disableEpisodeRanges);
  EXPECT_TRUE(settings.m_bNoRemoteArtWithLocalScraper);
}

TEST(TestAdvancedSettings, VideoScanner)
{
  const std::string xml =
      R"(<advancedsettings>
           <videoscanner>
             <ignoreerrors>true</ignoreerrors>
           </videoscanner>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_TRUE(settings.m_bVideoScannerIgnoreErrors);
}

TEST(TestAdvancedSettings, Slideshow)
{
  const std::string xml =
      R"(<advancedsettings>
           <slideshow>
             <panamount>1.0</panamount>
             <zoomamount>2.0</zoomamount>
             <blackbarcompensation>3.0</blackbarcompensation>
           </slideshow>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_slideshowPanAmount, 1.0);
  EXPECT_EQ(settings.m_slideshowZoomAmount, 2.0);
  EXPECT_EQ(settings.m_slideshowBlackBarCompensation, 3.0);
}

TEST(TestAdvancedSettings, Network)
{
  const std::string xml =
      R"(<advancedsettings>
           <network>
             <curlclienttimeout>1</curlclienttimeout>
             <curllowspeedtime>2</curllowspeedtime>
             <curlretries>3</curlretries>
             <curlkeepaliveinterval>4</curlkeepaliveinterval>
             <disableipv6>true</disableipv6>
             <disablehttp2>true</disablehttp2>
             <catrustfile>1</catrustfile>
             <nfstimeout>5</nfstimeout>
             <nfsretries>6</nfsretries>
           </network>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_curlconnecttimeout, 1);
  EXPECT_EQ(settings.m_curllowspeedtime, 2);
  EXPECT_EQ(settings.m_curlretries, 3);
  EXPECT_EQ(settings.m_curlKeepAliveInterval, 4);
  EXPECT_TRUE(settings.m_curlDisableIPV6);
  EXPECT_TRUE(settings.m_curlDisableHTTP2);
  EXPECT_EQ(settings.m_caTrustFile, "1");
  EXPECT_EQ(settings.m_nfsTimeout, 5);
  EXPECT_EQ(settings.m_nfsRetries, 6);
}

TEST(TestAdvancedSettings, JsonRPC)
{
  const std::string xml =
      R"(<advancedsettings>
           <jsonrpc>
             <compactoutput>false</compactoutput>
             <tcpport>1</tcpport>
           </jsonrpc>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_FALSE(settings.m_jsonOutputCompact);
  EXPECT_EQ(settings.m_jsonTcpPort, 1);
}

TEST(TestAdvancedSettings, Samba)
{
  const std::string xml =
      R"(<advancedsettings>
           <samba>
             <doscodepage>1</doscodepage>
             <clienttimeout>5</clienttimeout>
             <statfiles>false</statfiles>
           </samba>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_sambadoscodepage, "1");
  EXPECT_EQ(settings.m_sambaclienttimeout, 5);
  EXPECT_FALSE(settings.m_sambastatfiles);
}

TEST(TestAdvancedSettings, HTTPDirectory)
{
  const std::string xml =
      R"(<advancedsettings>
           <httpdirectory>
             <statfilesize>true</statfilesize>
           </httpdirectory>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_TRUE(settings.m_bHTTPDirectoryStatFilesize);
}

TEST(TestAdvancedSettings, FTP)
{
  const std::string xml =
      R"(<advancedsettings>
           <ftp>
             <remotethumbs>true</remotethumbs>
           </ftp>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_TRUE(settings.m_bFTPThumbs);
}

TEST(TestAdvancedSettings, TopLevel)
{
  const std::string xml =
      R"(<advancedsettings>
           <cddbaddress>1</cddbaddress>
           <addsourceontop>true</addsourceontop>
           <airtunesport>2</airtunesport>
           <airplayport>3</airplayport>
           <handlemounting>false</handlemounting>
           <automountopticalmedia>false</automountopticalmedia>
           <minimizetotray>true</minimizetotray>
           <fullscreen>true</fullscreen>
           <splash>false</splash>
           <showexitbutton>false</showexitbutton>
           <canwindowed>false</canwindowed>
           <songinfoduration>4</songinfoduration>
           <playlistretries>5</playlistretries>
           <playlisttimeout>6</playlisttimeout>
           <glrectanglehack>true</glrectanglehack>
           <skiploopfilter>7</skiploopfilter>
           <virtualshares>false</virtualshares>
           <packagefoldersize>8</packagefoldersize>
           <cachepath>https://some.where/foo</cachepath>
           <tvmultipartmatching>9</tvmultipartmatching>
           <remotedelay>10</remotedelay>
           <scanirserver>false</scanirserver>
           <fanartres>11</fanartres>
           <imageres>12</imageres>
           <imagescalingalgorithm>bicubic</imagescalingalgorithm>
           <imagequalityjpeg>13</imagequalityjpeg>
           <playlistasfolders>false</playlistasfolders>
           <uselocalecollation>false</uselocalecollation>
           <detectasudf>true</detectasudf>
           <shoutcastart>false</shoutcastart>
           <cputempcommand>14</cputempcommand>
           <gputempcommand>15</gputempcommand>
           <alwaysontop>true</alwaysontop>
           <enablemultimediakeys>true</enablemultimediakeys>
           <seeksteps>16,17</seeksteps>
           <opengldebugging>true</opengldebugging>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_cddbAddress, "1");
  EXPECT_TRUE(settings.m_addSourceOnTop);
  EXPECT_EQ(settings.m_airTunesPort, 2);
  EXPECT_EQ(settings.m_airPlayPort, 3);
  EXPECT_FALSE(settings.m_handleMounting);
  EXPECT_FALSE(settings.m_autoMountOpticalMedia);
#if defined(TARGET_WINDOWS_DESKTOP)
  EXPECT_TRUE(settings.m_minimizeToTray);
#endif
#if defined(TARGET_DARWIN_OSX) || defined(TARGET_WINDOWS)
  EXPECT_TRUE(settings.m_startFullScreen);
#endif
  EXPECT_FALSE(settings.m_splashImage);
  EXPECT_FALSE(settings.m_showExitButton);
  EXPECT_FALSE(settings.m_canWindowed);
  EXPECT_EQ(settings.m_songInfoDuration, 4);
  EXPECT_EQ(settings.m_playlistRetries, 5);
  EXPECT_EQ(settings.m_playlistTimeout, 6);
  EXPECT_TRUE(settings.m_GLRectangleHack);
  EXPECT_EQ(settings.m_iSkipLoopFilter, 7);
  EXPECT_FALSE(settings.m_bVirtualShares);
  EXPECT_EQ(settings.m_addonPackageFolderSize, 8);
  EXPECT_EQ(settings.m_cachePath, "https://some.where/foo/");
  EXPECT_EQ(settings.m_tvshowMultiPartEnumRegExp, "9");
  EXPECT_EQ(settings.m_remoteDelay, 10);
  EXPECT_FALSE(settings.m_bScanIRServer);
  EXPECT_EQ(settings.m_fanartRes, 11);
  EXPECT_EQ(settings.m_imageRes, 12);
  EXPECT_EQ(settings.m_imageScalingAlgorithm, CPictureScalingAlgorithm::Bicubic);
  EXPECT_EQ(settings.m_imageQualityJpeg, 13);
  EXPECT_FALSE(settings.m_playlistAsFolders);
  EXPECT_FALSE(settings.m_useLocaleCollation);
  EXPECT_TRUE(settings.m_detectAsUdf);
  EXPECT_FALSE(settings.m_bShoutcastArt);
  EXPECT_EQ(settings.m_cpuTempCmd, "14");
  EXPECT_EQ(settings.m_gpuTempCmd, "15");
  EXPECT_TRUE(settings.m_alwaysOnTop);
  EXPECT_TRUE(settings.m_enableMultimediaKeys);
  EXPECT_EQ(settings.m_seekSteps.size(), 2);
  EXPECT_EQ(settings.m_seekSteps[0], 16);
  EXPECT_EQ(settings.m_seekSteps[1], 17);
  EXPECT_TRUE(settings.m_openGlDebugging);
}

TEST(TestAdvancedSettings, EPG)
{
  const std::string xml =
      R"(<advancedsettings>
           <epg>
             <updatecheckinterval>1</updatecheckinterval>
             <cleanupinterval>2</cleanupinterval>
             <activetagcheckinterval>3</activetagcheckinterval>
             <retryinterruptedupdateinterval>4</retryinterruptedupdateinterval>
             <updateemptytagsinterval>5</updateemptytagsinterval>
             <displayupdatepopup>false</displayupdatepopup>
             <displayincrementalupdatepopup>true</displayincrementalupdatepopup>
           </epg>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_iEpgUpdateCheckInterval, 1);
  EXPECT_EQ(settings.m_iEpgCleanupInterval, 2);
  EXPECT_EQ(settings.m_iEpgActiveTagCheckInterval, 3);
  EXPECT_EQ(settings.m_iEpgRetryInterruptedUpdateInterval, 4);
  EXPECT_EQ(settings.m_iEpgUpdateEmptyTagsInterval, 5);
  EXPECT_FALSE(settings.m_bEpgDisplayUpdatePopup);
  EXPECT_TRUE(settings.m_bEpgDisplayIncrementalUpdatePopup);
}

TEST(TestAdvancedSettings, EDL)
{
  const std::string xml =
      R"(<advancedsettings>
           <edl>
             <mergeshortcommbreaks>true</mergeshortcommbreaks>
             <displaycommbreaknotifications>false</displaycommbreaknotifications>
             <maxcommbreaklength>1</maxcommbreaklength>
             <mincommbreaklength>2</mincommbreaklength>
             <maxcommbreakgap>3</maxcommbreakgap>
             <maxstartgap>4</maxstartgap>
             <commbreakautowait>5</commbreakautowait>
             <commbreakautowind>6</commbreakautowind>
           </edl>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_TRUE(settings.m_bEdlMergeShortCommBreaks);
  EXPECT_FALSE(settings.m_EdlDisplayCommbreakNotifications);
  EXPECT_EQ(settings.m_iEdlMaxCommBreakLength, 1);
  EXPECT_EQ(settings.m_iEdlMinCommBreakLength, 2);
  EXPECT_EQ(settings.m_iEdlMaxCommBreakGap, 3);
  EXPECT_EQ(settings.m_iEdlMaxStartGap, 4);
  EXPECT_EQ(settings.m_iEdlCommBreakAutowait, 5);
  EXPECT_EQ(settings.m_iEdlCommBreakAutowind, 6);
}

TEST(TestAdvancedSettings, SortTokens)
{
  const std::string xml =
      R"(<advancedsettings>
           <sorttokens>
             <token>foo</token>
             <token separators=".">bar</token>
             <token separators="">foobar</token>
           </sorttokens>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  const auto orig_size = settings.m_vecTokens.size();
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_vecTokens.size(), orig_size + 5);
  EXPECT_TRUE(settings.m_vecTokens.find("foo ") != settings.m_vecTokens.end());
  EXPECT_TRUE(settings.m_vecTokens.find("foo.") != settings.m_vecTokens.end());
  EXPECT_TRUE(settings.m_vecTokens.find("foo_") != settings.m_vecTokens.end());
  EXPECT_TRUE(settings.m_vecTokens.find("bar.") != settings.m_vecTokens.end());
  EXPECT_TRUE(settings.m_vecTokens.find("foobar") != settings.m_vecTokens.end());
}

TEST(TestAdvancedSettings, PathSubstitution)
{
  const std::string xml =
      R"(<advancedsettings>
           <pathsubstitution>
             <substitute>
               <from>/foo</from>
                <to>/bar</to>
              </substitute>
             <substitute>
               <from>/foobar</from>
                <to>/mama</to>
              </substitute>
           </pathsubstitution>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  using namespace std::string_literals;
  const auto ref = std::vector{
      std::pair{"/foo"s, "/bar"s},
      std::pair{"/foobar"s, "/mama"s},
  };
  EXPECT_EQ(settings.m_pathSubstitutions, ref);
}

TEST(TestAdvancedSettings, MusicFileNameFilters)
{
  const std::string xml =
      R"(<advancedsettings>
           <musicfilenamefilters>
             <filter>foo</filter>
             <filter>bar</filter>
           </musicfilenamefilters>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  using namespace std::string_literals;
  const auto ref = std::vector{
      "foo"s,
      "bar"s,
  };
  EXPECT_EQ(settings.m_musicTagsFromFileFilters, ref);
}

TEST(TestAdvancedSettings, Hosts)
{
  const std::string xml =
      R"(<advancedsettings>
           <hosts>
             <entry name="foo">1.2.3.4</entry>
             <entry name="bar">5.6.7.8</entry>
           </hosts>
         </advancedsettings>)";

  auto cache = std::make_shared<CDNSNameCache>();
  CServiceBroker::RegisterDNSNameCache(cache);
  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  std::string ip;
  EXPECT_TRUE(CServiceBroker::GetDNSNameCache()->GetCached("foo", ip));
  EXPECT_EQ(ip, "1.2.3.4");
  EXPECT_TRUE(CServiceBroker::GetDNSNameCache()->GetCached("bar", ip));
  EXPECT_EQ(ip, "5.6.7.8");
  CServiceBroker::UnregisterDNSNameCache();
}

TEST(TestAdvancedSettings, PowerManagement)
{
  const std::string xml =
      R"(<advancedsettings>
           <powermanagement>
             <powerdown>1</powerdown>
             <reboot>2</reboot>
             <suspend>3</suspend>
             <hibernate>4</hibernate>
           </powermanagement>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_powerdownCommand, "1");
  EXPECT_EQ(settings.m_rebootCommand, "2");
  EXPECT_EQ(settings.m_suspendCommand, "3");
  EXPECT_EQ(settings.m_hibernateCommand, "4");
}

TEST(TestAdvancedSettings, PVR)
{
  const std::string xml =
      R"(<advancedsettings>
           <pvr>
             <timecorrection>1</timecorrection>
             <infotoggleinterval>2</infotoggleinterval>
             <channeliconsautoscan>false</channeliconsautoscan>
             <autoscaniconsuserset>true</autoscaniconsuserset>
             <numericchannelswitchtimeout>53</numericchannelswitchtimeout>
             <timeshiftthreshold>4</timeshiftthreshold>
             <timeshiftsimpleosd>false</timeshiftsimpleosd>
             <pvrrecordings>
              <sortmethod>episode</sortmethod>
              <sortorder>ascending</sortorder>
             </pvrrecordings>
           </pvr>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_EQ(settings.m_iPVRTimeCorrection, 1);
  EXPECT_EQ(settings.m_iPVRInfoToggleInterval, 2);
  EXPECT_FALSE(settings.m_bPVRChannelIconsAutoScan);
  EXPECT_TRUE(settings.m_bPVRAutoScanIconsUserSet);
  EXPECT_EQ(settings.m_iPVRNumericChannelSwitchTimeout, 53);
  EXPECT_EQ(settings.m_iPVRTimeshiftThreshold, 4);
  EXPECT_FALSE(settings.m_bPVRTimeshiftSimpleOSD);
  EXPECT_EQ(settings.m_PVRDefaultSortOrder.sortBy, SortBy::EPISODE_NUMBER);
  EXPECT_EQ(settings.m_PVRDefaultSortOrder.sortOrder, SortOrder::ASCENDING);
}

TEST(TestAdvancedSettings, GUI)
{
  const std::string xml =
      R"(<advancedsettings>
           <gui>
             <visualizedirtyregions>true</visualizedirtyregions>
             <algorithmdirtyregions>1</algorithmdirtyregions>
             <smartredraw>true</smartredraw>
             <anisotropicfiltering>2</anisotropicfiltering>
             <fronttobackrendering>true</fronttobackrendering>
             <geometryclear>false</geometryclear>
             <asynctextureupload>true</asynctextureupload>
             <transparentvideolayout>true</transparentvideolayout>
           </gui>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  EXPECT_TRUE(settings.m_guiVisualizeDirtyRegions);
  EXPECT_EQ(settings.m_guiAlgorithmDirtyRegions, 1);
  EXPECT_TRUE(settings.m_guiSmartRedraw);
  EXPECT_EQ(settings.m_guiAnisotropicFiltering, 2);
  EXPECT_TRUE(settings.m_guiFrontToBackRendering);
  EXPECT_FALSE(settings.m_guiGeometryClear);
  EXPECT_TRUE(settings.m_guiAsyncTextureUpload);
  EXPECT_TRUE(settings.m_guiVideoLayoutTransparent);
}

TEST(TestAdvancedSettings, VideoAdjustRefreshRate)
{
  const std::string xml =
      R"(<advancedsettings>
           <video>
             <adjustrefreshrate>
               <override>
                 <fps>25.0</fps>
                 <refresh>25.0</refresh>
               </override>
               <override>
                 <fpsmin>25.8</fpsmin>
                 <fpsmax>26.2</fpsmax>
                 <refresh>47.0</refresh>
               </override>
               <override>
                 <fps>27.0</fps>
                 <refreshmin>46.8</refreshmin>
                 <refreshmax>47.2</refreshmax>
               </override>
               <override>
                 <fpsmin>25.8</fpsmin>
                 <fpsmax>26.2</fpsmax>
                 <refreshmin>46.8</refreshmin>
                 <refreshmax>47.2</refreshmax>
               </override>
               <fallback>
                 <refresh>25.0</refresh>
               </fallback>
               <fallback>
                 <refreshmin>24.8</refreshmin>
                 <refreshmax>25.8</refreshmax>
               </fallback>
             </adjustrefreshrate>
           </video>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  const auto ref = std::vector{
      RefreshOverride{24.99f, 25.01f, 24.99f, 25.01f, false},
      RefreshOverride{25.8f, 26.2f, 46.99f, 47.01f, false},
      RefreshOverride{26.99f, 27.01f, 46.8f, 47.2f, false},
      RefreshOverride{25.8f, 26.2f, 46.8f, 47.2f, false},
      RefreshOverride{0.f, 0.f, 24.99f, 25.01f, true},
      RefreshOverride{0.f, 0.f, 24.8f, 25.8f, true},
  };

  ASSERT_EQ(settings.m_videoAdjustRefreshOverrides.size(), ref.size());
  for (size_t i = 0; i < ref.size(); ++i)
  {
    const auto& r = ref[i];
    const auto& p = settings.m_videoAdjustRefreshOverrides[i];
    EXPECT_FLOAT_EQ(p.refreshmax, r.refreshmax);
    EXPECT_FLOAT_EQ(p.refreshmin, r.refreshmin);
    EXPECT_FLOAT_EQ(p.fpsmax, r.fpsmax);
    EXPECT_FLOAT_EQ(p.fpsmin, r.fpsmin);
    EXPECT_EQ(p.fallback, r.fallback);
  }
}

TEST(TestAdvancedSettings, VideoLatency)
{
  const std::string xml =
      R"(<advancedsettings>
           <video>
             <latency>
               <refresh>
                 <rate>25.0</rate>
                 <delay>100</delay>
                 <hdrextradelay>200</hdrextradelay>
               </refresh>
               <refresh>
                 <rate>25.0</rate>
                 <delay>100</delay>
               </refresh>
               <refresh>
                 <rate>25.0</rate>
                 <hdrextradelay>200</hdrextradelay>
               </refresh>
               <refresh>
                 <min>24.8</min>
                 <max>25.2</max>
                 <delay>100</delay>
                 <hdrextradelay>200</hdrextradelay>
               </refresh>
             </latency>
           </video>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());
  const auto ref = std::vector{
      RefreshVideoLatency{24.99f, 25.01f, 100.f, 200.f},
      RefreshVideoLatency{24.99f, 25.01f, 100.f, 0.f},
      RefreshVideoLatency{24.99f, 25.01f, 0.f, 200.f},
      RefreshVideoLatency{24.8f, 25.2f, 100.f, 200.f},
  };

  ASSERT_EQ(settings.m_videoRefreshLatency.size(), ref.size());
  for (size_t i = 0; i < ref.size(); ++i)
  {
    const auto& r = ref[i];
    const auto& p = settings.m_videoRefreshLatency[i];
    EXPECT_FLOAT_EQ(p.refreshmax, r.refreshmax);
    EXPECT_FLOAT_EQ(p.refreshmin, r.refreshmin);
    EXPECT_FLOAT_EQ(p.delay, r.delay);
    EXPECT_FLOAT_EQ(p.hdrextradelay, r.hdrextradelay);
  }
}

TEST_P(ExtensionTest, AddAndRemove)
{
  CAdvancedSettingsInit settings;
  const auto orig_array = StringUtils::Split(std::invoke(GetParam().member, settings), "|");
  const auto add_array = StringUtils::Split(GetParam().add, "|");
  const auto remove_array = StringUtils::Split(GetParam().remove, "|");

  auto is_in_orig = [&orig_array](const auto& r)
  { return std::ranges::find(orig_array, r) != orig_array.end(); };

  EXPECT_TRUE(std::ranges::all_of(remove_array, is_in_orig));
  EXPECT_TRUE(std::ranges::none_of(add_array, is_in_orig));

  CXBMCTinyXML doc;
  doc.Parse(GenExtXML(GetParam()));
  settings.ParseSettingsXML(doc.RootElement());

  const auto new_array = StringUtils::Split(std::invoke(GetParam().member, settings), "|");
  auto is_in_new = [&new_array](const auto& r)
  { return std::ranges::find(new_array, r) != new_array.end(); };

  EXPECT_TRUE(std::ranges::none_of(remove_array, is_in_new));
  EXPECT_TRUE(std::ranges::all_of(add_array, is_in_new));
}

TEST_P(RegExpTest, Append)
{
  RunRETest<RegExpT::Action::APPEND>(GetParam());
}

TEST_P(RegExpTest, AppendOld)
{
  RunRETest<RegExpT::Action::APPEND_OLD>(GetParam());
}

TEST_P(RegExpTest, Overwrite)
{
  RunRETest<RegExpT::Action::OVERWRITE>(GetParam());
}

TEST_P(RegExpTest, Prepend)
{
  RunRETest<RegExpT::Action::PREPEND>(GetParam());
}

TEST_P(DatabaseTest, Parse)
{
  CXBMCTinyXML doc;
  const DatabaseSettings ref{
      .type = "1",
      .host = "2",
      .port = "3",
      .user = "4",
      .pass = "5",
      .key = "6",
      .cert = "7",
      .ca = "8",
      .capath = "9",
      .ciphers = "10",
      .connecttimeout = 11,
      .compression = true,
  };
  doc.Parse(GenDbXML(GetParam().tag, ref));
  CAdvancedSettingsInit settings;
  settings.ParseSettingsXML(doc.RootElement());

  const auto& p = std::invoke(GetParam().member, settings);
  EXPECT_EQ(p.type, ref.type);
  EXPECT_EQ(p.host, ref.host);
  EXPECT_EQ(p.port, ref.port);
  EXPECT_EQ(p.user, ref.user);
  EXPECT_EQ(p.pass, ref.pass);
  EXPECT_EQ(p.name, ref.name);
  EXPECT_EQ(p.key, ref.key);
  EXPECT_EQ(p.cert, ref.cert);
  EXPECT_EQ(p.ca, ref.ca);
  EXPECT_EQ(p.capath, ref.capath);
  EXPECT_EQ(p.ciphers, ref.ciphers);
  EXPECT_EQ(p.connecttimeout, ref.connecttimeout);
  EXPECT_EQ(p.compression, ref.compression);
}

TEST_P(DatabaseTest, Redact)
{
  CXBMCTinyXML doc;
  const DatabaseSettings ref{
      .type = "1",
      .host = "2",
      .port = "3",
      .user = "4",
      .pass = "5",
      .key = "6",
      .cert = "7",
      .ca = "8",
      .capath = "9",
      .ciphers = "10",
      .connecttimeout = 11,
      .compression = true,
  };
  doc.Parse(GenDbXML(GetParam().tag, ref));
  CAdvancedSettingsInit settings;
  settings.DoRedact(doc);
  settings.ParseSettingsXML(doc.RootElement());

  const auto& p = std::invoke(GetParam().member, settings);
  EXPECT_EQ(p.type, ref.type);
  EXPECT_EQ(p.host, ref.host);
  EXPECT_EQ(p.port, ref.port);
  EXPECT_EQ(p.user, ref.user);
  EXPECT_EQ(p.pass, "*****");
  EXPECT_EQ(p.name, ref.name);
  EXPECT_EQ(p.key, ref.key);
  EXPECT_EQ(p.cert, ref.cert);
  EXPECT_EQ(p.ca, ref.ca);
  EXPECT_EQ(p.capath, ref.capath);
  EXPECT_EQ(p.ciphers, ref.ciphers);
  EXPECT_EQ(p.connecttimeout, ref.connecttimeout);
  EXPECT_EQ(p.compression, ref.compression);
}

namespace
{

// clang-format off
const auto regexp_tests = std::array{
  RegExpT{"audio", "excludefromlisting", &CAdvancedSettings::m_audioExcludeFromListingRegExps},
  RegExpT{"audio", "excludefromscan", &CAdvancedSettings::m_audioExcludeFromScanRegExps},
  RegExpT{"video", "excludefromlisting", &CAdvancedSettings::m_videoExcludeFromListingRegExps},
  RegExpT{"video", "excludefromscan", &CAdvancedSettings::m_moviesExcludeFromScanRegExps},
  RegExpT{"video", "excludetvshowsfromscan", &CAdvancedSettings::m_tvshowExcludeFromScanRegExps},
  RegExpT{"video", "cleanstrings", &CAdvancedSettings::m_videoCleanStringRegExps},
  RegExpT{"", "pictureexcludes", &CAdvancedSettings::m_pictureExcludeFromListingRegExps},
  RegExpT{"", "trailermatching", &CAdvancedSettings::m_trailerMatchRegExps},
};

const auto extension_tests = std::array{
  ExtT{"pictureextensions",  ".foo|.bar",       ".gif|.jpg", &CAdvancedSettings::m_pictureExtensions},
  ExtT{"musicextensions",    ".foo|.bar",       ".mp3|.m4a", &CAdvancedSettings::m_musicExtensions},
  ExtT{"videoextensions",    ".foo|.bar",       ".mov|.mkv", &CAdvancedSettings::m_videoExtensions},
  ExtT{"discstubextensions", ".foo|.bar",       ".disc",     &CAdvancedSettings::m_discStubExtensions},
  ExtT{"musicthumbs",        "foo.jpg|bar.png", "folder.jpg|Cover.jpg", &CAdvancedSettings::m_musicThumbs},
};

const auto db_tests = std::array{
  DbTestT{"videodatabase", &CAdvancedSettings::m_databaseVideo},
  DbTestT{"musicdatabase", &CAdvancedSettings::m_databaseMusic},
  DbTestT{"tvdatabase",    &CAdvancedSettings::m_databaseTV},
  DbTestT{"epgdatabase",   &CAdvancedSettings::m_databaseEpg},
};
// clang-format on

} // namespace

INSTANTIATE_TEST_SUITE_P(TestAdvancedSettings, RegExpTest, testing::ValuesIn(regexp_tests));
INSTANTIATE_TEST_SUITE_P(TestAdvancedSettings, ExtensionTest, testing::ValuesIn(extension_tests));
INSTANTIATE_TEST_SUITE_P(TestAdvancedSettings, DatabaseTest, testing::ValuesIn(db_tests));
