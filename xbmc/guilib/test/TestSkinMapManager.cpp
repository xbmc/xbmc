/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/SkinMapManager.h"

#include <gtest/gtest.h>
#include <tinyxml.h>

using namespace KODI::GUILIB;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------
class TestSkinMapManager : public ::testing::Test
{
protected:
  void SetUp() override { m_mgr.Clear(); }

  // Convenience: load maps from an inline XML string whose root is <includes>
  void Load(const std::string& xml)
  {
    TiXmlDocument doc;
    doc.Parse(xml.c_str());
    m_mgr.LoadMaps(doc.RootElement());
  }

  CSkinMapManager m_mgr;
};

// ---------------------------------------------------------------------------
// Basic lookup
// ---------------------------------------------------------------------------

TEST_F(TestSkinMapManager, LookupHit)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="ac3">Dolby Digital</entry>
        <entry key="eac3">Dolby Digital+</entry>
        <entry key="flac">FLAC</entry>
      </map>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "ac3"), "Dolby Digital");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "eac3"), "Dolby Digital+");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "flac"), "FLAC");
}

TEST_F(TestSkinMapManager, LookupMissReturnsRawKey)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="ac3">Dolby Digital</entry>
      </map>
    </includes>
  )");

  // Key not in map — raw value returned
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "unknown_codec"), "unknown_codec");
}

TEST_F(TestSkinMapManager, LookupUnknownMapReturnsRawKey)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="ac3">Dolby Digital</entry>
      </map>
    </includes>
  )");

  // Map doesn't exist at all — raw value returned
  EXPECT_EQ(m_mgr.Lookup("NonExistentMap", "ac3"), "ac3");
}

TEST_F(TestSkinMapManager, LookupEmptyKeyReturnsEmptyString)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="ac3">Dolby Digital</entry>
      </map>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", ""), "");
}

// ---------------------------------------------------------------------------
// ref — pure alias (no overrides)
// ---------------------------------------------------------------------------

TEST_F(TestSkinMapManager, RefPureAliasInheritsAllEntries)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="ac3">Dolby Digital</entry>
        <entry key="flac">FLAC</entry>
      </map>
      <map name="DefaultAltAudioMap" ref="DefaultCodecMap"/>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "ac3"), "Dolby Digital");
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "flac"), "FLAC");
}

// ---------------------------------------------------------------------------
// ref — alias with overrides
// ---------------------------------------------------------------------------

TEST_F(TestSkinMapManager, RefOverrideTakesPriorityOverBase)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="dtshd_ma_x">DTS-HD MA &#8226; DTS:X</entry>
        <entry key="truehd_atmos">TrueHD &#8226; Atmos</entry>
        <entry key="flac">FLAC</entry>
      </map>
      <map name="DefaultAltAudioMap" ref="DefaultCodecMap">
        <entry key="dtshd_ma_x">DTS-HD MA / DTS:X</entry>
        <entry key="truehd_atmos">Dolby TrueHD / Atmos</entry>
      </map>
    </includes>
  )");

  // Overridden keys use the override value
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "dtshd_ma_x"), "DTS-HD MA / DTS:X");
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "truehd_atmos"), "Dolby TrueHD / Atmos");

  // Non-overridden key falls through to base map
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "flac"), "FLAC");
}

TEST_F(TestSkinMapManager, RefOverrideMissReturnsRawKey)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="ac3">Dolby Digital</entry>
      </map>
      <map name="DefaultAltAudioMap" ref="DefaultCodecMap">
        <entry key="dtshd_ma_x">DTS-HD MA / DTS:X</entry>
      </map>
    </includes>
  )");

  // Key not in override or base — raw value returned
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "unknown_codec"), "unknown_codec");
}

// ---------------------------------------------------------------------------
// Multi-level ref chain
// ---------------------------------------------------------------------------

TEST_F(TestSkinMapManager, MultiLevelRefChain)
{
  Load(R"(
    <includes>
      <map name="BaseMap">
        <entry key="ac3">Dolby Digital</entry>
        <entry key="flac">FLAC</entry>
      </map>
      <map name="MidMap" ref="BaseMap">
        <entry key="eac3">Dolby Digital+</entry>
      </map>
      <map name="TopMap" ref="MidMap">
        <entry key="dtshd_ma">DTS-HD MA</entry>
      </map>
    </includes>
  )");

  // Key defined at each level
  EXPECT_EQ(m_mgr.Lookup("TopMap", "dtshd_ma"), "DTS-HD MA"); // own entry
  EXPECT_EQ(m_mgr.Lookup("TopMap", "eac3"), "Dolby Digital+"); // from MidMap
  EXPECT_EQ(m_mgr.Lookup("TopMap", "ac3"), "Dolby Digital"); // from BaseMap
  EXPECT_EQ(m_mgr.Lookup("TopMap", "flac"), "FLAC"); // from BaseMap
}

// ---------------------------------------------------------------------------
// Circular ref detection
// ---------------------------------------------------------------------------

TEST_F(TestSkinMapManager, CircularRefReturnsRawKey)
{
  // Set up a circular ref chain in XML to exercise the Lookup cycle guard.
  Load(R"(
    <includes>
      <map name="MapA" ref="MapB">
        <entry key="ac3">Dolby Digital</entry>
      </map>
      <map name="MapB" ref="MapA">
        <entry key="flac">FLAC</entry>
      </map>
    </includes>
  )");

  // Should not infinite-loop; raw value returned
  EXPECT_EQ(m_mgr.Lookup("MapA", "eac3"), "eac3");
  EXPECT_EQ(m_mgr.Lookup("MapB", "eac3"), "eac3");
}

// ---------------------------------------------------------------------------
// Duplicate keys
// ---------------------------------------------------------------------------

TEST_F(TestSkinMapManager, DuplicateKeyFirstValueWins)
{
  // The XML parser will hand us both entries; first one loaded wins via try_emplace
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="ac3">Dolby Digital</entry>
        <entry key="ac3">DUPLICATE</entry>
      </map>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "ac3"), "Dolby Digital");
}

TEST_F(TestSkinMapManager, DuplicateMapEmptyDefinitionReplacesEntries)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="ac3">Dolby Digital</entry>
      </map>
      <map name="DefaultCodecMap"/>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "ac3"), "ac3");
}

TEST_F(TestSkinMapManager, DuplicateMapDefinitionClearsOldRef)
{
  Load(R"(
    <includes>
      <map name="BaseMap">
        <entry key="ac3">Dolby Digital</entry>
      </map>
      <map name="DefaultCodecMap" ref="BaseMap"/>
      <map name="DefaultCodecMap">
        <entry key="flac">FLAC</entry>
      </map>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "ac3"), "ac3");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "flac"), "FLAC");
}

// ---------------------------------------------------------------------------
// Empty map skipped
// ---------------------------------------------------------------------------

TEST_F(TestSkinMapManager, EmptyMapProducesNoEntries)
{
  Load(R"(
    <includes>
      <map name="EmptyMap">
      </map>
    </includes>
  )");

  // Map has no entries — raw value returned
  EXPECT_EQ(m_mgr.Lookup("EmptyMap", "ac3"), "ac3");
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST_F(TestSkinMapManager, ClearRemovesAllMapsAndRefs)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="ac3">Dolby Digital</entry>
      </map>
      <map name="DefaultAltAudioMap" ref="DefaultCodecMap"/>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "ac3"), "Dolby Digital");

  m_mgr.Clear();

  // After clear everything returns raw key
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "ac3"), "ac3");
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "ac3"), "ac3");
}

// ---------------------------------------------------------------------------
// Estuary map spot-checks (based on Includes_Maps.xml)
// ---------------------------------------------------------------------------

TEST_F(TestSkinMapManager, EstuaryDefaultCodecMapAudioSpotCheck)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="aac">AAC</entry>
        <entry key="ac3">Dolby Digital</entry>
        <entry key="eac3">Dolby Digital+</entry>
        <entry key="eac3_ddp_atmos">Dolby Digital+ &#8226; Atmos</entry>
        <entry key="dtshd_ma">DTS-HD MA</entry>
        <entry key="dtshd_ma_x">DTS-HD MA &#8226; DTS:X</entry>
        <entry key="truehd">TrueHD</entry>
        <entry key="truehd_atmos">TrueHD &#8226; Atmos</entry>
        <entry key="flac">FLAC</entry>
      </map>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "aac"), "AAC");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "ac3"), "Dolby Digital");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "eac3"), "Dolby Digital+");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "dtshd_ma"), "DTS-HD MA");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "truehd"), "TrueHD");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "flac"), "FLAC");
}

TEST_F(TestSkinMapManager, EstuaryDefaultCodecMapVideoSpotCheck)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="h264">H.264</entry>
        <entry key="hevc">H.265</entry>
        <entry key="hev1">H.265</entry>
        <entry key="av1">AV1</entry>
        <entry key="vp9">VP9</entry>
      </map>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "h264"), "H.264");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "hevc"), "H.265");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "hev1"), "H.265");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "av1"), "AV1");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "vp9"), "VP9");
}

TEST_F(TestSkinMapManager, EstuaryDefaultAltAudioMapOverrides)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="dtshd_ma_x">DTS-HD MA &#8226; DTS:X</entry>
        <entry key="dtshd_ma_x_imax">DTS-HD MA &#8226; DTS:X IMAX</entry>
        <entry key="eac3_ddp_atmos">Dolby Digital+ &#8226; Atmos</entry>
        <entry key="truehd_atmos">TrueHD &#8226; Atmos</entry>
        <entry key="flac">FLAC</entry>
      </map>
      <map name="DefaultAltAudioMap" ref="DefaultCodecMap">
        <entry key="dtshd_ma_x">DTS-HD MA / DTS:X</entry>
        <entry key="dtshd_ma_x_imax">DTS-HD MA / DTS:X IMAX</entry>
        <entry key="eac3_ddp_atmos">Dolby Digital+ / Atmos</entry>
        <entry key="truehd_atmos">Dolby TrueHD / Atmos</entry>
      </map>
    </includes>
  )");

  // Overrides use slash separators
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "dtshd_ma_x"), "DTS-HD MA / DTS:X");
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "dtshd_ma_x_imax"), "DTS-HD MA / DTS:X IMAX");
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "eac3_ddp_atmos"), "Dolby Digital+ / Atmos");
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "truehd_atmos"), "Dolby TrueHD / Atmos");

  // Non-overridden key still falls through to base
  EXPECT_EQ(m_mgr.Lookup("DefaultAltAudioMap", "flac"), "FLAC");

  // Base map's non-overridden key is unaffected by the override map — verify it still
  // contains the bullet-separated form by checking it's NOT the slash form
  EXPECT_NE(m_mgr.Lookup("DefaultCodecMap", "dtshd_ma_x"), "DTS-HD MA / DTS:X");
}

TEST_F(TestSkinMapManager, EstuaryDefaultAudioTrackMapOverrides)
{
  Load(R"(
    <includes>
      <map name="DefaultCodecMap">
        <entry key="aac_latm">AAC</entry>
        <entry key="mp3float">MP3</entry>
        <entry key="pcm_bluray">PCM</entry>
        <entry key="pcm_s16le">PCM</entry>
        <entry key="pcm_s24le">PCM</entry>
        <entry key="wavpack">WAVP</entry>
        <entry key="wmav2">WMA</entry>
        <entry key="flac">FLAC</entry>
      </map>
      <map name="DefaultAudioTrackMap" ref="DefaultCodecMap">
        <entry key="aac_latm">AAC LATM</entry>
        <entry key="mp3float">MP3 Float</entry>
        <entry key="pcm_bluray">PCM Bluray</entry>
        <entry key="pcm_s16le">PCM S16LE</entry>
        <entry key="pcm_s24le">PCM S24LE</entry>
        <entry key="wavpack">WAVPACK</entry>
        <entry key="wmav2">WMAv2</entry>
      </map>
    </includes>
  )");

  // All seven keys use verbose forms in DefaultAudioTrackMap
  EXPECT_EQ(m_mgr.Lookup("DefaultAudioTrackMap", "aac_latm"), "AAC LATM");
  EXPECT_EQ(m_mgr.Lookup("DefaultAudioTrackMap", "mp3float"), "MP3 Float");
  EXPECT_EQ(m_mgr.Lookup("DefaultAudioTrackMap", "pcm_bluray"), "PCM Bluray");
  EXPECT_EQ(m_mgr.Lookup("DefaultAudioTrackMap", "pcm_s16le"), "PCM S16LE");
  EXPECT_EQ(m_mgr.Lookup("DefaultAudioTrackMap", "pcm_s24le"), "PCM S24LE");
  EXPECT_EQ(m_mgr.Lookup("DefaultAudioTrackMap", "wavpack"), "WAVPACK");
  EXPECT_EQ(m_mgr.Lookup("DefaultAudioTrackMap", "wmav2"), "WMAv2");

  // Non-overridden key falls through to base
  EXPECT_EQ(m_mgr.Lookup("DefaultAudioTrackMap", "flac"), "FLAC");

  // Base map uses short forms
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "aac_latm"), "AAC");
  EXPECT_EQ(m_mgr.Lookup("DefaultCodecMap", "wavpack"), "WAVP");
}

TEST_F(TestSkinMapManager, EstuaryDefaultResolutionMap)
{
  Load(R"(
    <includes>
      <map name="DefaultResolutionMap">
        <entry key="480">SD</entry>
        <entry key="540">SD</entry>
        <entry key="576">SD</entry>
        <entry key="720">HD</entry>
        <entry key="1080">HD</entry>
        <entry key="4K">UHD</entry>
        <entry key="8K">UHD</entry>
      </map>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultResolutionMap", "480"), "SD");
  EXPECT_EQ(m_mgr.Lookup("DefaultResolutionMap", "720"), "HD");
  EXPECT_EQ(m_mgr.Lookup("DefaultResolutionMap", "1080"), "HD");
  EXPECT_EQ(m_mgr.Lookup("DefaultResolutionMap", "4K"), "UHD");
  EXPECT_EQ(m_mgr.Lookup("DefaultResolutionMap", "8K"), "UHD");
  EXPECT_EQ(m_mgr.Lookup("DefaultResolutionMap", "360"), "360"); // unknown — raw
}

TEST_F(TestSkinMapManager, EstuaryDefaultHdrMap)
{
  Load(R"(
    <includes>
      <map name="DefaultHdrMap">
        <entry key="dolbyvision">Dolby Vision</entry>
        <entry key="hdr10plus">HDR10+</entry>
        <entry key="hdr10">HDR10</entry>
        <entry key="hlg">HLG</entry>
      </map>
    </includes>
  )");

  EXPECT_EQ(m_mgr.Lookup("DefaultHdrMap", "dolbyvision"), "Dolby Vision");
  EXPECT_EQ(m_mgr.Lookup("DefaultHdrMap", "hdr10plus"), "HDR10+");
  EXPECT_EQ(m_mgr.Lookup("DefaultHdrMap", "hdr10"), "HDR10");
  EXPECT_EQ(m_mgr.Lookup("DefaultHdrMap", "hlg"), "HLG");
  EXPECT_EQ(m_mgr.Lookup("DefaultHdrMap", "sdr"), "sdr"); // unknown — raw
}
