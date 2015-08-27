/*
 *  Copyright (C) 2010-2013 Eduard Kytmanov
 *  http://www.avmedia.su
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#ifdef HAS_DS_PLAYER
#include "DSPlayerDatabase.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "dbwrappers/dataset.h"
#include "filesystem/StackDirectory.h"
#include "video/VideoInfoTag.h "
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/StdString.h"
#include "Filters/LAVAudioSettings.h"
#include "Filters/LAVVideoSettings.h"
#include "Filters/LAVSplitterSettings.h"
#include "utils/CharsetConverter.h"

using namespace XFILE;

CEdition::CEdition()
  :editionNumber(0)
  , editionName("")
{
}

bool CEdition::IsSet() const
{
  return (!editionName.empty() && editionNumber >= 0);
}

CDSPlayerDatabase::CDSPlayerDatabase(void)
{
}


CDSPlayerDatabase::~CDSPlayerDatabase(void)
{
}

bool CDSPlayerDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseDSPlayer);
}

void CDSPlayerDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create edition table");
  m_pDS->exec("CREATE TABLE edition (idEdition integer primary key, file text, editionName text, editionNumber integer)\n");

  CLog::Log(LOGINFO, "create madvr setting table");
  m_pDS->exec("CREATE TABLE madvrSettings ( file text, Resolution integer, TvShowName txt, "
    "ChromaUpscaling integer, ChromaAntiRing bool, ChromaSuperRes bool, ChromaSuperResPasses integer, ChromaSuperResStrength float, ChromaSuperResSoftness float, ImageUpscaling integer, ImageUpAntiRing bool, ImageUpLinear bool, ImageDownscaling integer, ImageDownAntiRing bool, ImageDownLinear bool, "
    "ImageDoubleLuma integer, ImageDoubleChroma integer, ImageQuadrupleLuma integer, ImageQuadrupleChroma integer, " 
    "ImageDoubleLumaFactor integer, ImageDoubleChromaFactor integer, ImageQuadrupleLumaFactor integer, ImageQuadrupleChromaFactor integer, "
    "DeintActive integer, DeintForce interger, DeintLookPixels bool, "
    "SmoothMotion integer, Dithering integer, DitheringColoredNoise bool, DitheringEveryFrame bool, "
    "Deband bool, DebandLevel integer, DebandFadeLevel integer, "
    "FineSharp bool, FineSharpStrength float, LumaSharpen bool, LumaSharpenStrength float, LumaSharpenClamp float, LumaSharpenRadius float, AdaptiveSharpen bool, AdaptiveSharpenStrength float, "
    "UpRefFineSharp bool, UpRefFineSharpStrength float, UpRefLumaSharpen bool, UpRefLumaSharpenStrength float, UpRefLumaSharpenClamp float, UpRefLumaSharpenRadius float, UpRefAdaptiveSharpen bool, UpRefAdaptiveSharpenStrength float, SuperRes bool, SuperResStrength float, SuperResRadius float, "
    "RefineOnce bool, SuperResFirst bool"
    ")\n");

  CLog::Log(LOGINFO, "create madvr resolution for resolution table");
  m_pDS->exec("CREATE TABLE madvrDefResSettings ( Resolution integer, ResolutionInternal integer, TvShowName txt, "
    "ChromaUpscaling integer, ChromaAntiRing bool, ChromaSuperRes bool, ChromaSuperResPasses integer, ChromaSuperResStrength float, ChromaSuperResSoftness float, ImageUpscaling integer, ImageUpAntiRing bool, ImageUpLinear bool, ImageDownscaling integer, ImageDownAntiRing bool, ImageDownLinear bool, "
    "ImageDoubleLuma integer, ImageDoubleChroma integer, ImageQuadrupleLuma integer, ImageQuadrupleChroma integer, "
    "ImageDoubleLumaFactor integer, ImageDoubleChromaFactor integer, ImageQuadrupleLumaFactor integer, ImageQuadrupleChromaFactor integer, "
    "DeintActive integer, DeintForce interger, DeintLookPixels bool, "
    "SmoothMotion integer, Dithering integer, DitheringColoredNoise bool, DitheringEveryFrame bool, "
    "Deband bool, DebandLevel integer, DebandFadeLevel integer, "
    "FineSharp bool, FineSharpStrength float, LumaSharpen bool, LumaSharpenStrength float, LumaSharpenClamp float, LumaSharpenRadius float, AdaptiveSharpen bool, AdaptiveSharpenStrength float, "
    "UpRefFineSharp bool, UpRefFineSharpStrength float, UpRefLumaSharpen bool, UpRefLumaSharpenStrength float, UpRefLumaSharpenClamp float, UpRefLumaSharpenRadius float, UpRefAdaptiveSharpen bool, UpRefAdaptiveSharpenStrength float, SuperRes bool, SuperResStrength float, SuperResRadius float, "
    "RefineOnce bool, SuperResFirst bool"
    ")\n");

  CStdString strSQL = "CREATE TABLE lavvideoSettings (id integer, bTrayIcon integer, dwStreamAR integer, dwNumThreads integer, ";
  for (int i = 0; i < LAVOutPixFmt_NB; ++i)
    strSQL += StringUtils::Format("bPixFmts%i integer, ", i);
  strSQL += "dwRGBRange integer, dwHWAccel integer, ";
  for (int i = 0; i < HWCodec_NB; ++i)
    strSQL += StringUtils::Format("bHWFormats%i integer, ", i);
  strSQL += "dwHWAccelResFlags integer, dwHWDeintMode integer, dwHWDeintOutput integer, bHWDeintHQ integer, dwDeintFieldOrder integer, deintMode integer, dwSWDeintMode integer, "
    "dwSWDeintOutput integer, dwDitherMode integer"
    ")\n";

  CLog::Log(LOGINFO, "create lavvideo setting table");
  m_pDS->exec(strSQL);

  strSQL = "CREATE TABLE lavaudioSettings (id integer, bTrayIcon integer, bDRCEnabled integer, iDRCLevel integer, ";
  for (int i = 0; i < Bitstream_NB; ++i)
    strSQL += StringUtils::Format("bBitstream%i integer, ", i);
  strSQL += "bDTSHDFraming integer, bAutoAVSync integer, bExpandMono integer, bExpand61 integer, bOutputStandardLayout integer, bAllowRawSPDIF integer, ";
  for (int i = 0; i < SampleFormat_NB; ++i)
    strSQL += StringUtils::Format("bSampleFormats%i integer, ", i);
  strSQL += "bSampleConvertDither integer, bAudioDelayEnabled integer, iAudioDelay integer, bMixingEnabled integer, dwMixingLayout integer, dwMixingFlags integer, dwMixingMode integer, "
    "dwMixingCenterLevel integer, dwMixingSurroundLevel integer, dwMixingLFELevel integer"
    ")\n";

  CLog::Log(LOGINFO, "create lavaudio setting table");
  m_pDS->exec(strSQL);

  strSQL = "CREATE TABLE lavsplitterSettings (id integer, bTrayIcon integer, prefAudioLangs txt, prefSubLangs txt, subtitleAdvanced txt, subtitleMode integer, bPGSForcedStream integer, bPGSOnlyForced integer, "
    "iVC1Mode integer, bSubstreams integer, bMatroskaExternalSegments integer, bStreamSwitchRemoveAudio integer, bImpairedAudio integer, bPreferHighQualityAudio integer, dwQueueMaxSize integer, dwNetworkAnalysisDuration integer"
    ")\n";

  CLog::Log(LOGINFO, "create lavsplitter setting table");
  m_pDS->exec(strSQL);
}

void CDSPlayerDatabase::CreateAnalytics()
{
  m_pDS->exec("CREATE INDEX idxEdition ON edition (file)");
  m_pDS->exec("CREATE INDEX idxMadvrSettings ON madvrSettings (file)");
  m_pDS->exec("CREATE INDEX idxMadvrDefResSettings ON madvrDefResSettings (Resolution)");
  m_pDS->exec("CREATE INDEX idxLavVideo ON lavvideoSettings(id)");
  m_pDS->exec("CREATE INDEX idxLavAudio ON lavaudioSettings (id)");
  m_pDS->exec("CREATE INDEX idxlavSplitter ON lavsplitterSettings (id)");
}

bool CDSPlayerDatabase::GetResumeEdition(const CStdString& strFilenameAndPath, CEdition &edition)
{
  VECEDITIONS editions;
  GetEditionForFile(strFilenameAndPath, editions);
  if (editions.size() > 0)
  {
    edition = editions[0];
    return true;
  }
  return false;
}

bool CDSPlayerDatabase::GetResumeEdition(const CFileItem *item, CEdition &edition)
{
  CStdString strPath = item->GetPath();
  if ((item->IsVideoDb() || item->IsDVD()) && item->HasVideoInfoTag())
    strPath = item->GetVideoInfoTag()->m_strFileNameAndPath;

  return GetResumeEdition(strPath, edition);
}

void CDSPlayerDatabase::GetEditionForFile(const CStdString& strFilenameAndPath, VECEDITIONS& editions)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strSQL = PrepareSQL("select * from edition where file='%s' order by editionNumber", strFilenameAndPath.c_str());
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      CEdition edition;
      edition.editionName = m_pDS->fv("editionName").get_asString();
      edition.editionNumber = m_pDS->fv("editionNumber").get_asInt();

      editions.push_back(edition);
      m_pDS->next();
    }

    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CDSPlayerDatabase::AddEdition(const CStdString& strFilenameAndPath, const CEdition &edition)
{
  try
  {
    if (!edition.IsSet())		return;
    if (NULL == m_pDB.get())    return;
    if (NULL == m_pDS.get())    return;

    CStdString strSQL;
    int idEdition = -1;

    strSQL = PrepareSQL("select idEdition from edition where file='%s'", strFilenameAndPath.c_str());

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() != 0)
      idEdition = m_pDS->get_field_value("idEdition").get_asInt();
    m_pDS->close();

    if (idEdition >= 0)
      strSQL = PrepareSQL("update edition set  editionName = '%s', editionNumber = '%i' where idEdition = %i", edition.editionName.c_str(), edition.editionNumber, idEdition);
    else
      strSQL = PrepareSQL("insert into edition (idEdition, file, editionName, editionNumber) values(NULL, '%s', '%s', %i)", strFilenameAndPath.c_str(), edition.editionName.c_str(), edition.editionNumber);


    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CDSPlayerDatabase::ClearEditionOfFile(const CStdString& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strSQL = PrepareSQL("delete from edition where file='%s'", strFilenameAndPath.c_str());
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

bool CDSPlayerDatabase::GetDefResMadvrSettings(int resolution, std::string tvShowName, CMadvrSettings &settings)
{
  try
  {
    if (resolution < 0 && tvShowName == "") return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file

    CStdString strSQL = "";

    if (resolution > -1)
      strSQL = PrepareSQL("select * from madvrDefResSettings where madvrDefResSettings.Resolution=%i", resolution);

    else if (tvShowName != "")
      strSQL = PrepareSQL("select * from madvrDefResSettings where madvrDefResSettings.TvShowName='%s'", tvShowName.c_str());

    if (strSQL == "")
      return false;

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    { // get the madvr settings info

      settings.m_ChromaUpscaling = m_pDS->fv("ChromaUpscaling").get_asInt();
      settings.m_ChromaAntiRing = m_pDS->fv("ChromaAntiRing").get_asBool();
      settings.m_ChromaSuperRes = m_pDS->fv("ChromaSuperRes").get_asBool();
      settings.m_ChromaSuperResPasses = m_pDS->fv("ChromaSuperResPasses").get_asInt();
      settings.m_ChromaSuperResStrength = m_pDS->fv("ChromaSuperResStrength").get_asFloat();
      settings.m_ChromaSuperResSoftness = m_pDS->fv("ChromaSuperResSoftness").get_asFloat();

      settings.m_ImageUpscaling = m_pDS->fv("ImageUpscaling").get_asInt();
      settings.m_ImageUpAntiRing = m_pDS->fv("ImageUpAntiRing").get_asBool();
      settings.m_ImageUpLinear = m_pDS->fv("ImageUpLinear").get_asBool();

      settings.m_ImageDownscaling = m_pDS->fv("ImageDownscaling").get_asInt();
      settings.m_ImageDownAntiRing = m_pDS->fv("ImageDownAntiRing").get_asBool();
      settings.m_ImageDownLinear = m_pDS->fv("ImageDownLinear").get_asBool();

      settings.m_ImageDoubleLuma = m_pDS->fv("ImageDoubleLuma").get_asInt();
      settings.m_ImageDoubleChroma = m_pDS->fv("ImageDoubleChroma").get_asInt();
      settings.m_ImageQuadrupleLuma = m_pDS->fv("ImageQuadrupleLuma").get_asInt();
      settings.m_ImageQuadrupleChroma = m_pDS->fv("ImageQuadrupleChroma").get_asInt();

      settings.m_ImageDoubleLumaFactor = m_pDS->fv("ImageDoubleLumaFactor").get_asInt();
      settings.m_ImageDoubleChromaFactor = m_pDS->fv("ImageDoubleChromaFactor").get_asInt();
      settings.m_ImageQuadrupleLumaFactor = m_pDS->fv("ImageQuadrupleLumaFactor").get_asInt();
      settings.m_ImageQuadrupleChromaFactor = m_pDS->fv("ImageQuadrupleChromaFactor").get_asInt();

      settings.m_deintactive = m_pDS->fv("DeintActive").get_asInt();
      settings.m_deintforce = m_pDS->fv("DeintForce").get_asInt();
      settings.m_deintlookpixels = m_pDS->fv("DeintLookPixels").get_asBool();

      settings.m_smoothMotion = m_pDS->fv("SmoothMotion").get_asInt();

      settings.m_dithering = m_pDS->fv("Dithering").get_asInt();
      settings.m_ditheringColoredNoise = m_pDS->fv("DitheringColoredNoise").get_asBool();
      settings.m_ditheringEveryFrame = m_pDS->fv("DitheringEveryFrame").get_asBool();

      settings.m_deband = m_pDS->fv("Deband").get_asBool();
      settings.m_debandLevel = m_pDS->fv("DebandLevel").get_asInt();
      settings.m_debandFadeLevel = m_pDS->fv("DebandFadeLevel").get_asInt();

      settings.m_fineSharp = m_pDS->fv("FineSharp").get_asBool();
      settings.m_fineSharpStrength = m_pDS->fv("FineSharpStrength").get_asFloat();
      settings.m_lumaSharpen = m_pDS->fv("LumaSharpen").get_asBool();
      settings.m_lumaSharpenStrength = m_pDS->fv("LumaSharpenStrength").get_asFloat();
      settings.m_lumaSharpenClamp = m_pDS->fv("LumaSharpenClamp").get_asFloat();
      settings.m_lumaSharpenRadius = m_pDS->fv("LumaSharpenRadius").get_asFloat();
      settings.m_adaptiveSharpen = m_pDS->fv("AdaptiveSharpen").get_asBool();
      settings.m_adaptiveSharpenStrength = m_pDS->fv("AdaptiveSharpenStrength").get_asFloat();

      settings.m_UpRefFineSharp = m_pDS->fv("UpRefFineSharp").get_asBool();
      settings.m_UpRefFineSharpStrength = m_pDS->fv("UpRefFineSharpStrength").get_asFloat();
      settings.m_UpRefLumaSharpen = m_pDS->fv("UpRefLumaSharpen").get_asBool();
      settings.m_UpRefLumaSharpenStrength = m_pDS->fv("UpRefLumaSharpenStrength").get_asFloat();
      settings.m_UpRefLumaSharpenClamp = m_pDS->fv("UpRefLumaSharpenClamp").get_asFloat();
      settings.m_UpRefLumaSharpenRadius = m_pDS->fv("UpRefLumaSharpenRadius").get_asFloat();
      settings.m_UpRefAdaptiveSharpen = m_pDS->fv("UpRefAdaptiveSharpen").get_asBool();
      settings.m_UpRefAdaptiveSharpenStrength = m_pDS->fv("UpRefAdaptiveSharpenStrength").get_asFloat();
      settings.m_superRes = m_pDS->fv("SuperRes").get_asBool();
      settings.m_superResStrength = m_pDS->fv("SuperResStrength").get_asFloat();
      settings.m_superResRadius = m_pDS->fv("SuperResRadius").get_asFloat();

      settings.m_refineOnce = m_pDS->fv("RefineOnce").get_asBool();
      settings.m_superResFirst = m_pDS->fv("SuperResFirst").get_asBool();

      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CDSPlayerDatabase::GetVideoSettings(const CStdString &strFilenameAndPath, CMadvrSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL = PrepareSQL("select * from madvrSettings where madvrSettings.file = '%s'", strFilenameAndPath.c_str());

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    { // get the madvr settings info

      settings.m_ChromaUpscaling = m_pDS->fv("ChromaUpscaling").get_asInt();
      settings.m_ChromaAntiRing = m_pDS->fv("ChromaAntiRing").get_asBool();
      settings.m_ChromaSuperRes = m_pDS->fv("ChromaSuperRes").get_asBool();
      settings.m_ChromaSuperResPasses = m_pDS->fv("ChromaSuperResPasses").get_asInt();
      settings.m_ChromaSuperResStrength = m_pDS->fv("ChromaSuperResStrength").get_asFloat();
      settings.m_ChromaSuperResSoftness = m_pDS->fv("ChromaSuperResSoftness").get_asFloat();

      settings.m_ImageUpscaling = m_pDS->fv("ImageUpscaling").get_asInt();
      settings.m_ImageUpAntiRing = m_pDS->fv("ImageUpAntiRing").get_asBool();
      settings.m_ImageUpLinear = m_pDS->fv("ImageUpLinear").get_asBool();

      settings.m_ImageDownscaling = m_pDS->fv("ImageDownscaling").get_asInt();
      settings.m_ImageDownAntiRing = m_pDS->fv("ImageDownAntiRing").get_asBool();
      settings.m_ImageDownLinear = m_pDS->fv("ImageDownLinear").get_asBool();

      settings.m_ImageDoubleLuma = m_pDS->fv("ImageDoubleLuma").get_asInt();
      settings.m_ImageDoubleChroma = m_pDS->fv("ImageDoubleChroma").get_asInt();
      settings.m_ImageQuadrupleLuma = m_pDS->fv("ImageQuadrupleLuma").get_asInt();
      settings.m_ImageQuadrupleChroma = m_pDS->fv("ImageQuadrupleChroma").get_asInt();

      settings.m_ImageDoubleLumaFactor = m_pDS->fv("ImageDoubleLumaFactor").get_asInt();
      settings.m_ImageDoubleChromaFactor = m_pDS->fv("ImageDoubleChromaFactor").get_asInt();
      settings.m_ImageQuadrupleLumaFactor = m_pDS->fv("ImageQuadrupleLumaFactor").get_asInt();
      settings.m_ImageQuadrupleChromaFactor = m_pDS->fv("ImageQuadrupleChromaFactor").get_asInt();

      settings.m_deintactive = m_pDS->fv("DeintActive").get_asInt();
      settings.m_deintforce = m_pDS->fv("DeintForce").get_asInt();
      settings.m_deintlookpixels = m_pDS->fv("DeintLookPixels").get_asBool();

      settings.m_smoothMotion = m_pDS->fv("SmoothMotion").get_asInt();

      settings.m_dithering = m_pDS->fv("Dithering").get_asInt();
      settings.m_ditheringColoredNoise = m_pDS->fv("DitheringColoredNoise").get_asBool();
      settings.m_ditheringEveryFrame = m_pDS->fv("DitheringEveryFrame").get_asBool();

      settings.m_deband = m_pDS->fv("Deband").get_asBool();
      settings.m_debandLevel = m_pDS->fv("DebandLevel").get_asInt();
      settings.m_debandFadeLevel = m_pDS->fv("DebandFadeLevel").get_asInt();

      settings.m_fineSharp = m_pDS->fv("FineSharp").get_asBool();
      settings.m_fineSharpStrength = m_pDS->fv("FineSharpStrength").get_asFloat();
      settings.m_lumaSharpen = m_pDS->fv("LumaSharpen").get_asBool();
      settings.m_lumaSharpenStrength = m_pDS->fv("LumaSharpenStrength").get_asFloat();
      settings.m_lumaSharpenClamp = m_pDS->fv("LumaSharpenClamp").get_asFloat();
      settings.m_lumaSharpenRadius = m_pDS->fv("LumaSharpenRadius").get_asFloat();
      settings.m_adaptiveSharpen = m_pDS->fv("AdaptiveSharpen").get_asBool();
      settings.m_adaptiveSharpenStrength = m_pDS->fv("AdaptiveSharpenStrength").get_asFloat();

      settings.m_UpRefFineSharp = m_pDS->fv("UpRefFineSharp").get_asBool();
      settings.m_UpRefFineSharpStrength = m_pDS->fv("UpRefFineSharpStrength").get_asFloat();
      settings.m_UpRefLumaSharpen = m_pDS->fv("UpRefLumaSharpen").get_asBool();
      settings.m_UpRefLumaSharpenStrength = m_pDS->fv("UpRefLumaSharpenStrength").get_asFloat();
      settings.m_UpRefLumaSharpenClamp = m_pDS->fv("UpRefLumaSharpenClamp").get_asFloat();
      settings.m_UpRefLumaSharpenRadius = m_pDS->fv("UpRefLumaSharpenRadius").get_asFloat();
      settings.m_UpRefAdaptiveSharpen = m_pDS->fv("UpRefAdaptiveSharpen").get_asBool();
      settings.m_UpRefAdaptiveSharpenStrength = m_pDS->fv("UpRefAdaptiveSharpenStrength").get_asFloat();
      settings.m_superRes = m_pDS->fv("SuperRes").get_asBool();
      settings.m_superResStrength = m_pDS->fv("SuperResStrength").get_asFloat();
      settings.m_superResRadius = m_pDS->fv("SuperResRadius").get_asFloat();

      settings.m_refineOnce = m_pDS->fv("RefineOnce").get_asBool();
      settings.m_superResFirst = m_pDS->fv("SuperResFirst").get_asBool();

      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CDSPlayerDatabase::GetLAVVideoSettings(CLavSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL = PrepareSQL("select * from lavvideoSettings where id = 0");

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    { // get the lavvideo settings info

      settings.video_bTrayIcon = m_pDS->fv("bTrayIcon").get_asInt();
      settings.video_dwStreamAR = m_pDS->fv("dwStreamAR").get_asInt();
      settings.video_dwNumThreads = m_pDS->fv("dwNumThreads").get_asInt();
      for (int i = 0; i < LAVOutPixFmt_NB; ++i)
        settings.video_bPixFmts[i] = m_pDS->fv(StringUtils::Format("bPixFmts%i", i).c_str()).get_asInt();
      settings.video_dwRGBRange = m_pDS->fv("dwRGBRange").get_asInt();
      settings.video_dwHWAccel = m_pDS->fv("dwHWAccel").get_asInt();
      for (int i = 0; i < HWCodec_NB; ++i)
        settings.video_bHWFormats[i] = m_pDS->fv(StringUtils::Format("bHWFormats%i", i).c_str()).get_asInt();
      settings.video_dwHWAccelResFlags = m_pDS->fv("dwHWAccelResFlags").get_asInt();
      settings.video_dwHWDeintMode = m_pDS->fv("dwHWDeintMode").get_asInt();
      settings.video_dwHWDeintOutput = m_pDS->fv("dwHWDeintOutput").get_asInt();
      settings.video_bHWDeintHQ = m_pDS->fv("bHWDeintHQ").get_asInt();
      settings.video_dwDeintFieldOrder = m_pDS->fv("dwDeintFieldOrder").get_asInt();
      settings.video_deintMode = (LAVDeintMode)m_pDS->fv("deintMode").get_asInt();
      settings.video_dwSWDeintMode = m_pDS->fv("dwSWDeintMode").get_asInt();
      settings.video_dwSWDeintOutput = m_pDS->fv("dwSWDeintOutput").get_asInt();
      settings.video_dwDitherMode = m_pDS->fv("dwDitherMode").get_asInt();

      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CDSPlayerDatabase::GetLAVAudioSettings(CLavSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL = PrepareSQL("select * from lavaudioSettings where id = 0");

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    { // get the lavaudio settings info

      settings.audio_bTrayIcon = m_pDS->fv("bTrayIcon").get_asInt();
      settings.audio_bDRCEnabled = m_pDS->fv("bDRCEnabled").get_asInt();
      settings.audio_iDRCLevel = m_pDS->fv("iDRCLevel").get_asInt();
      for (int i = 0; i < Bitstream_NB; ++i)
        settings.audio_bBitstream[i] = m_pDS->fv(StringUtils::Format("bBitstream%i", i).c_str()).get_asInt();
      settings.audio_bDTSHDFraming = m_pDS->fv("bDTSHDFraming").get_asInt();
      settings.audio_bAutoAVSync = m_pDS->fv("bAutoAVSync").get_asInt();
      settings.audio_bExpandMono = m_pDS->fv("bExpandMono").get_asInt();
      settings.audio_bExpand61 = m_pDS->fv("bExpand61").get_asInt();
      settings.audio_bOutputStandardLayout = m_pDS->fv("bOutputStandardLayout").get_asInt();
      settings.audio_bAllowRawSPDIF = m_pDS->fv("bAllowRawSPDIF").get_asInt();
      for (int i = 0; i < SampleFormat_NB; ++i)
        settings.audio_bSampleFormats[i] = m_pDS->fv(StringUtils::Format("bSampleFormats%i", i).c_str()).get_asInt();
      settings.audio_bSampleConvertDither = m_pDS->fv("bSampleConvertDither").get_asInt();
      settings.audio_bAudioDelayEnabled = m_pDS->fv("bAudioDelayEnabled").get_asInt();
      settings.audio_iAudioDelay = m_pDS->fv("iAudioDelay").get_asInt();
      settings.audio_bMixingEnabled = m_pDS->fv("bMixingEnabled").get_asInt();
      settings.audio_dwMixingLayout = m_pDS->fv("dwMixingLayout").get_asInt();
      settings.audio_dwMixingFlags = m_pDS->fv("dwMixingFlags").get_asInt();
      settings.audio_dwMixingMode = m_pDS->fv("dwMixingMode").get_asInt();
      settings.audio_dwMixingCenterLevel = m_pDS->fv("dwMixingCenterLevel").get_asInt();
      settings.audio_dwMixingSurroundLevel = m_pDS->fv("dwMixingSurroundLevel").get_asInt();
      settings.audio_dwMixingLFELevel = m_pDS->fv("dwMixingLFELevel").get_asInt();

      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CDSPlayerDatabase::GetLAVSplitterSettings(CLavSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL = PrepareSQL("select * from lavsplitterSettings where id = 0");

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    { // get the lavsplitter settings info

      std::wstring strW;
      settings.splitter_bTrayIcon = m_pDS->fv("bTrayIcon").get_asInt();     
      g_charsetConverter.utf8ToW(m_pDS->fv("prefAudioLangs").get_asString(), strW, false);
      settings.splitter_prefAudioLangs = strW;
      g_charsetConverter.utf8ToW(m_pDS->fv("prefSubLangs").get_asString(), strW, false);
      settings.splitter_prefSubLangs = strW;
      g_charsetConverter.utf8ToW(m_pDS->fv("subtitleAdvanced").get_asString(), strW, false);
      settings.splitter_subtitleAdvanced = strW;
      settings.splitter_subtitleMode = (LAVSubtitleMode)m_pDS->fv("subtitleMode").get_asInt();
      settings.splitter_bPGSForcedStream = m_pDS->fv("bPGSForcedStream").get_asInt();
      settings.splitter_bPGSOnlyForced = m_pDS->fv("bPGSOnlyForced").get_asInt();
      settings.splitter_iVC1Mode = m_pDS->fv("iVC1Mode").get_asInt();
      settings.splitter_bSubstreams = m_pDS->fv("bSubstreams").get_asInt();
      settings.splitter_bMatroskaExternalSegments = m_pDS->fv("bMatroskaExternalSegments").get_asInt();
      settings.splitter_bStreamSwitchRemoveAudio = m_pDS->fv("bStreamSwitchRemoveAudio").get_asInt();
      settings.splitter_bImpairedAudio = m_pDS->fv("bImpairedAudio").get_asInt();
      settings.splitter_bPreferHighQualityAudio = m_pDS->fv("bPreferHighQualityAudio").get_asInt();
      settings.splitter_dwQueueMaxSize = m_pDS->fv("dwQueueMaxSize").get_asInt();
      settings.splitter_dwNetworkAnalysisDuration = m_pDS->fv("dwNetworkAnalysisDuration").get_asInt();

      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}


/// \brief Sets the settings for a particular video file
void CDSPlayerDatabase::SetLAVVideoSettings(CLavSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strSQL = StringUtils::Format("select * from lavvideoSettings where id = 0");
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();

      // update the item
      strSQL = "update lavvideoSettings set ";
      strSQL += StringUtils::Format("bTrayIcon=%i, ", settings.video_bTrayIcon);
      strSQL += StringUtils::Format("dwStreamAR=%i, ", settings.video_dwStreamAR);
      strSQL += StringUtils::Format("dwNumThreads=%i, ", settings.video_dwNumThreads);
      for (int i = 0; i < LAVOutPixFmt_NB; ++i)
        strSQL += StringUtils::Format("bPixFmts%i=%i, ", i, settings.video_bPixFmts[i]);
      strSQL += StringUtils::Format("dwRGBRange=%i, ", settings.video_dwRGBRange);
      strSQL += StringUtils::Format("dwHWAccel=%i, ", settings.video_dwHWAccel);
      for (int i = 0; i < HWCodec_NB; ++i)
        strSQL += StringUtils::Format("bHWFormats%i=%i, ", i, settings.video_bHWFormats[i]);
      strSQL += StringUtils::Format("dwHWAccelResFlags=%i, ", settings.video_dwHWAccelResFlags);
      strSQL += StringUtils::Format("dwHWDeintMode=%i, ", settings.video_dwHWDeintMode);
      strSQL += StringUtils::Format("dwHWDeintOutput=%i, ", settings.video_dwHWDeintOutput);
      strSQL += StringUtils::Format("bHWDeintHQ=%i, ", settings.video_bHWDeintHQ);
      strSQL += StringUtils::Format("dwDeintFieldOrder=%i, ", settings.video_dwDeintFieldOrder);
      strSQL += StringUtils::Format("deintMode=%i, ", settings.video_deintMode);
      strSQL += StringUtils::Format("dwSWDeintMode=%i, ", settings.video_dwSWDeintMode);
      strSQL += StringUtils::Format("dwSWDeintOutput=%i, ", settings.video_dwSWDeintOutput);
      strSQL += StringUtils::Format("dwDitherMode=%i ", settings.video_dwDitherMode);
      strSQL += "where id=0";
      m_pDS->exec(strSQL.c_str());
      return;
    }
    else
    { // add the items
      m_pDS->close();

      strSQL = "INSERT INTO lavvideoSettings (id, bTrayIcon, dwStreamAR, dwNumThreads, ";
      for (int i = 0; i < LAVOutPixFmt_NB; ++i)
        strSQL += StringUtils::Format("bPixFmts%i, ", i);
      strSQL += "dwRGBRange, dwHWAccel, ";
      for (int i = 0; i < HWCodec_NB; ++i)
        strSQL += StringUtils::Format("bHWFormats%i, ", i);
      strSQL += "dwHWAccelResFlags, dwHWDeintMode, dwHWDeintOutput, bHWDeintHQ, dwDeintFieldOrder, deintMode, dwSWDeintMode, "
        "dwSWDeintOutput, dwDitherMode"
        ") VALUES (0, ";

      strSQL += StringUtils::Format("%i, ", settings.video_bTrayIcon);
      strSQL += StringUtils::Format("%i, ", settings.video_dwStreamAR);
      strSQL += StringUtils::Format("%i, ", settings.video_dwNumThreads);
      for (int i = 0; i < LAVOutPixFmt_NB; ++i)
        strSQL += StringUtils::Format("%i, ", settings.video_bPixFmts[i]);
      strSQL += StringUtils::Format("%i, ", settings.video_dwRGBRange);
      strSQL += StringUtils::Format("%i, ", settings.video_dwHWAccel);
      for (int i = 0; i < HWCodec_NB; ++i)
        strSQL += StringUtils::Format("%i, ", settings.video_bHWFormats[i]);
      strSQL += StringUtils::Format("%i, ", settings.video_dwHWAccelResFlags);
      strSQL += StringUtils::Format("%i, ", settings.video_dwHWDeintMode);
      strSQL += StringUtils::Format("%i, ", settings.video_dwHWDeintOutput);
      strSQL += StringUtils::Format("%i, ", settings.video_bHWDeintHQ);
      strSQL += StringUtils::Format("%i, ", settings.video_dwDeintFieldOrder);
      strSQL += StringUtils::Format("%i, ", settings.video_deintMode);
      strSQL += StringUtils::Format("%i, ", settings.video_dwSWDeintMode);
      strSQL += StringUtils::Format("%i, ", settings.video_dwSWDeintOutput);
      strSQL += StringUtils::Format("%i ", settings.video_dwDitherMode);
      strSQL += ")";

      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CDSPlayerDatabase::SetLAVAudioSettings(CLavSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strSQL = StringUtils::Format("select * from lavaudioSettings where id = 0");
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();

      // update the item
      strSQL = "update lavaudioSettings set ";
      strSQL += StringUtils::Format("bTrayIcon=%i, ", settings.audio_bTrayIcon);
      strSQL += StringUtils::Format("bDRCEnabled=%i, ", settings.audio_bDRCEnabled);
      strSQL += StringUtils::Format("iDRCLevel=%i, ", settings.audio_iDRCLevel);
      for (int i = 0; i < Bitstream_NB; ++i)
        strSQL += StringUtils::Format("bBitstream%i=%i, ", i, settings.audio_bBitstream[i]);
      strSQL += StringUtils::Format("bDTSHDFraming=%i, ", settings.audio_bDTSHDFraming);
      strSQL += StringUtils::Format("bAutoAVSync=%i, ", settings.audio_bAutoAVSync);
      strSQL += StringUtils::Format("bExpandMono=%i, ", settings.audio_bExpandMono);
      strSQL += StringUtils::Format("bExpand61=%i, ", settings.audio_bExpand61);
      strSQL += StringUtils::Format("bOutputStandardLayout=%i, ", settings.audio_bOutputStandardLayout);
      strSQL += StringUtils::Format("bAllowRawSPDIF=%i, ", settings.audio_bAllowRawSPDIF);
      for (int i = 0; i < SampleFormat_NB; ++i)
        strSQL += StringUtils::Format("bSampleFormats%i=%i, ", i, settings.audio_bSampleFormats[i]);
      strSQL += StringUtils::Format("bSampleConvertDither=%i, ", settings.audio_bSampleConvertDither);
      strSQL += StringUtils::Format("bAudioDelayEnabled=%i, ", settings.audio_bAudioDelayEnabled);
      strSQL += StringUtils::Format("iAudioDelay=%i, ", settings.audio_iAudioDelay);
      strSQL += StringUtils::Format("bMixingEnabled=%i, ", settings.audio_bMixingEnabled);
      strSQL += StringUtils::Format("dwMixingLayout=%i, ", settings.audio_dwMixingLayout);
      strSQL += StringUtils::Format("dwMixingFlags=%i, ", settings.audio_dwMixingFlags);
      strSQL += StringUtils::Format("dwMixingMode=%i, ", settings.audio_dwMixingMode);
      strSQL += StringUtils::Format("dwMixingCenterLevel=%i, ", settings.audio_dwMixingCenterLevel);
      strSQL += StringUtils::Format("dwMixingSurroundLevel=%i, ", settings.audio_dwMixingSurroundLevel);
      strSQL += StringUtils::Format("dwMixingLFELevel=%i ", settings.audio_dwMixingLFELevel);
      strSQL += "where id=0";
      m_pDS->exec(strSQL.c_str());
      return;
    }
    else
    { // add the items
      m_pDS->close();

      strSQL = "INSERT INTO lavaudioSettings (id, bTrayIcon, bDRCEnabled, iDRCLevel, ";
      for (int i = 0; i < Bitstream_NB; ++i)
        strSQL += StringUtils::Format("bBitstream%i, ", i);
      strSQL += "bDTSHDFraming, bAutoAVSync, bExpandMono, bExpand61, bOutputStandardLayout, bAllowRawSPDIF, ";
      for (int i = 0; i < SampleFormat_NB; ++i)
        strSQL += StringUtils::Format("bSampleFormats%i, ", i);
      strSQL += "bSampleConvertDither, bAudioDelayEnabled, iAudioDelay, bMixingEnabled, dwMixingLayout, dwMixingFlags, dwMixingMode, "
        "dwMixingCenterLevel, dwMixingSurroundLevel, dwMixingLFELevel"
        ") VALUES (0, ";

      strSQL += StringUtils::Format("%i, ", settings.audio_bTrayIcon);
      strSQL += StringUtils::Format("%i, ", settings.audio_bDRCEnabled);
      strSQL += StringUtils::Format("%i, ", settings.audio_iDRCLevel);
      for (int i = 0; i < Bitstream_NB; ++i)
        strSQL += StringUtils::Format("%i, ", settings.audio_bBitstream[i]);
      strSQL += StringUtils::Format("%i, ", settings.audio_bDTSHDFraming);
      strSQL += StringUtils::Format("%i, ", settings.audio_bAutoAVSync);
      strSQL += StringUtils::Format("%i, ", settings.audio_bExpandMono);
      strSQL += StringUtils::Format("%i, ", settings.audio_bExpand61);
      strSQL += StringUtils::Format("%i, ", settings.audio_bOutputStandardLayout);
      strSQL += StringUtils::Format("%i, ", settings.audio_bAllowRawSPDIF);
      for (int i = 0; i < SampleFormat_NB; ++i)
        strSQL += StringUtils::Format("%i, ", i, settings.audio_bSampleFormats[i]);
      strSQL += StringUtils::Format("%i, ", settings.audio_bSampleConvertDither);
      strSQL += StringUtils::Format("%i, ", settings.audio_bAudioDelayEnabled);
      strSQL += StringUtils::Format("%i, ", settings.audio_iAudioDelay);
      strSQL += StringUtils::Format("%i, ", settings.audio_bMixingEnabled);
      strSQL += StringUtils::Format("%i, ", settings.audio_dwMixingLayout);
      strSQL += StringUtils::Format("%i, ", settings.audio_dwMixingFlags);
      strSQL += StringUtils::Format("%i, ", settings.audio_dwMixingMode);
      strSQL += StringUtils::Format("%i, ", settings.audio_dwMixingCenterLevel);
      strSQL += StringUtils::Format("%i, ", settings.audio_dwMixingSurroundLevel);
      strSQL += StringUtils::Format("%i ", settings.audio_dwMixingLFELevel);
      strSQL += ")";

      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CDSPlayerDatabase::SetLAVSplitterSettings(CLavSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    std::string str;
    CStdString strSQL = StringUtils::Format("select * from lavsplitterSettings where id = 0");
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();

      // update the item
      strSQL = "update lavsplitterSettings set ";
      strSQL += StringUtils::Format("bTrayIcon=%i, ", settings.splitter_bTrayIcon);
      g_charsetConverter.wToUTF8(settings.splitter_prefAudioLangs, str, false);
      strSQL += StringUtils::Format("prefAudioLangs='%s', ", str.c_str());
      g_charsetConverter.wToUTF8(settings.splitter_prefSubLangs, str, false);
      strSQL += StringUtils::Format("prefSubLangs='%s', ", str.c_str());
      g_charsetConverter.wToUTF8(settings.splitter_subtitleAdvanced, str, false);
      strSQL += StringUtils::Format("subtitleAdvanced='%s', ", str.c_str());
      strSQL += StringUtils::Format("subtitleMode=%i, ", settings.splitter_subtitleMode);
      strSQL += StringUtils::Format("bPGSForcedStream=%i, ", settings.splitter_bPGSForcedStream);
      strSQL += StringUtils::Format("bPGSOnlyForced=%i, ", settings.splitter_bPGSOnlyForced);
      strSQL += StringUtils::Format("iVC1Mode=%i, ", settings.splitter_iVC1Mode);
      strSQL += StringUtils::Format("bSubstreams=%i, ", settings.splitter_bSubstreams);
      strSQL += StringUtils::Format("bMatroskaExternalSegments=%i, ", settings.splitter_bMatroskaExternalSegments);
      strSQL += StringUtils::Format("bStreamSwitchRemoveAudio=%i, ", settings.splitter_bStreamSwitchRemoveAudio);
      strSQL += StringUtils::Format("bImpairedAudio=%i, ", settings.splitter_bImpairedAudio);
      strSQL += StringUtils::Format("bPreferHighQualityAudio=%i, ", settings.splitter_bPreferHighQualityAudio);
      strSQL += StringUtils::Format("dwQueueMaxSize=%i, ", settings.splitter_dwQueueMaxSize);
      strSQL += StringUtils::Format("dwNetworkAnalysisDuration=%i ", settings.splitter_dwNetworkAnalysisDuration);

      strSQL += "where id=0";
      m_pDS->exec(strSQL.c_str());
      return;
    }
    else
    { // add the items
      m_pDS->close();

      strSQL = "INSERT INTO lavsplitterSettings (id, bTrayIcon, prefAudioLangs, prefSubLangs, subtitleAdvanced, subtitleMode, bPGSForcedStream, bPGSOnlyForced, "
        "iVC1Mode, bSubstreams, bMatroskaExternalSegments, bStreamSwitchRemoveAudio, bImpairedAudio, bPreferHighQualityAudio, dwQueueMaxSize, dwNetworkAnalysisDuration"
        ") VALUES (0, ";

      strSQL += StringUtils::Format("%i, ", settings.splitter_bTrayIcon);
      g_charsetConverter.wToUTF8(settings.splitter_prefAudioLangs, str, false);
      strSQL += StringUtils::Format("'%s', ", str.c_str());
      g_charsetConverter.wToUTF8(settings.splitter_prefSubLangs, str, false);
      strSQL += StringUtils::Format("'%s', ", str.c_str());
      g_charsetConverter.wToUTF8(settings.splitter_subtitleAdvanced, str, false);
      strSQL += StringUtils::Format("'%s', ", str.c_str());
      strSQL += StringUtils::Format("%i, ", settings.splitter_subtitleMode);
      strSQL += StringUtils::Format("%i, ", settings.splitter_bPGSForcedStream);
      strSQL += StringUtils::Format("%i, ", settings.splitter_bPGSOnlyForced);
      strSQL += StringUtils::Format("%i, ", settings.splitter_iVC1Mode);
      strSQL += StringUtils::Format("%i, ", settings.splitter_bSubstreams);
      strSQL += StringUtils::Format("%i, ", settings.splitter_bMatroskaExternalSegments);
      strSQL += StringUtils::Format("%i, ", settings.splitter_bStreamSwitchRemoveAudio);
      strSQL += StringUtils::Format("%i, ", settings.splitter_bImpairedAudio);
      strSQL += StringUtils::Format("%i, ", settings.splitter_bPreferHighQualityAudio);
      strSQL += StringUtils::Format("%i, ", settings.splitter_dwQueueMaxSize);
      strSQL += StringUtils::Format("%i ", settings.splitter_dwNetworkAnalysisDuration);
      strSQL += ")";

      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

/// \brief Sets the settings for a particular video file
void CDSPlayerDatabase::SetVideoSettings(const CStdString& strFilenameAndPath, const CMadvrSettings &setting)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strSQL = StringUtils::Format("select * from madvrSettings where file='%s'", strFilenameAndPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL = PrepareSQL("update madvrSettings "
        "set Resolution=%i, set TvShowName='%s', "
        "ChromaUpscaling=%i,ChromaAntiRing=%i,ChromaSuperRes=%i, ChromaSuperResPasses=%i, ChromaSuperResStrength=%f, ChromaSuperResSoftness=%f, "
        "ImageUpscaling=%i,ImageUpAntiRing=%i,ImageUpLinear=%i, "
        "ImageDownscaling=%i,ImageDownAntiRing=%i,ImageDownLinear=%i, "
        "ImageDoubleLuma=%i, ImageDoubleChroma=%i, ImageQuadrupleLuma=%i, ImageQuadrupleChroma=%i, "
        "ImageDoubleLumaFactor=%i, ImageDoubleChromaFactor=%i, ImageQuadrupleLumaFactor=%i, ImageQuadrupleChromaFactor=%i, "
        "DeintActive=%i, DeintForce=%i, DeintLookPixels=%i, "
        "SmoothMotion=%i, Dithering=%i, DitheringColoredNoise=%i, DitheringEveryFrame=%i, "
        "Deband=%i, DebandLevel=%i, DebandFadeLevel=%i, "
        "FineSharp=%i, FineSharpStrength=%f, LumaSharpen=%i, LumaSharpenStrength=%f, LumaSharpenClamp=%f, LumaSharpenRadius=%f, AdaptiveSharpen=%i, AdaptiveSharpenStrength=%f, "
        "UpRefFineSharp=%i, UpRefFineSharpStrength=%f, UpRefLumaSharpen=%i, UpRefLumaSharpenStrength=%f, UpRefLumaSharpenClamp=%f, UpRefLumaSharpenRadius=%f, UpRefAdaptiveSharpen=%i, UpRefAdaptiveSharpenStrength=%f, SuperRes=%i, SuperResStrength=%f, SuperResRadius=%f, "
        "RefineOnce=%i, SuperResFirst=%i "
        "where file='%s'",
        setting.m_Resolution, setting.m_TvShowName.c_str(),
        setting.m_ChromaUpscaling,setting.m_ChromaAntiRing,setting.m_ChromaSuperRes,setting.m_ChromaSuperResPasses,setting.m_ChromaSuperResStrength,setting.m_ChromaSuperResSoftness,
        setting.m_ImageUpscaling,setting.m_ImageUpAntiRing,setting.m_ImageUpLinear,
        setting.m_ImageDownscaling, setting.m_ImageDownAntiRing, setting.m_ImageDownLinear, 
        setting.m_ImageDoubleLuma, setting.m_ImageDoubleChroma, setting.m_ImageQuadrupleLuma, setting.m_ImageQuadrupleChroma,
        setting.m_ImageDoubleLumaFactor, setting.m_ImageDoubleChromaFactor, setting.m_ImageQuadrupleLumaFactor, setting.m_ImageQuadrupleChromaFactor,
        setting.m_deintactive, setting.m_deintforce, setting.m_deintlookpixels,
        setting.m_smoothMotion, setting.m_dithering, setting.m_ditheringColoredNoise, setting.m_ditheringEveryFrame,
        setting.m_deband, setting.m_debandLevel, setting.m_debandFadeLevel, 
        setting.m_fineSharp, setting.m_fineSharpStrength, setting.m_lumaSharpen, setting.m_lumaSharpenStrength, setting.m_lumaSharpenClamp, setting.m_lumaSharpenRadius, setting.m_adaptiveSharpen, setting.m_adaptiveSharpenStrength,
        setting.m_UpRefFineSharp, setting.m_UpRefFineSharpStrength, setting.m_UpRefLumaSharpen, setting.m_UpRefLumaSharpenStrength, setting.m_UpRefLumaSharpenClamp, setting.m_UpRefLumaSharpenRadius, setting.m_UpRefAdaptiveSharpen, setting.m_UpRefAdaptiveSharpenStrength, setting.m_superRes, setting.m_superResStrength, setting.m_superResRadius,
        setting.m_refineOnce, setting.m_superResFirst,
        strFilenameAndPath.c_str());
      m_pDS->exec(strSQL.c_str());
      return;   
    }
    else
    { // add the items
      m_pDS->close();
      strSQL = "INSERT INTO madvrSettings (file, Resolution, TvShowName, "
        "ChromaUpscaling, ChromaAntiRing, ChromaSuperRes, ChromaSuperResPasses, ChromaSuperResStrength, ChromaSuperResSoftness, "
        "ImageUpscaling, ImageUpAntiRing, ImageUpLinear, "
        "ImageDownscaling, ImageDownAntiRing, ImageDownLinear, "
        "ImageDoubleLuma, ImageDoubleChroma, ImageQuadrupleLuma, ImageQuadrupleChroma, "
        "ImageDoubleLumaFactor, ImageDoubleChromaFactor, ImageQuadrupleLumaFactor, ImageQuadrupleChromaFactor, "
        "DeintActive, DeintForce, DeintLookPixels, "
        "SmoothMotion, Dithering, DitheringColoredNoise, DitheringEveryFrame, "
        "Deband, DebandLevel, DebandFadeLevel, "
        "FineSharp, FineSharpStrength, LumaSharpen, LumaSharpenStrength, LumaSharpenClamp, LumaSharpenRadius, AdaptiveSharpen, AdaptiveSharpenStrength, "
        "UpRefFineSharp, UpRefFineSharpStrength, UpRefLumaSharpen, UpRefLumaSharpenStrength, UpRefLumaSharpenClamp, UpRefLumaSharpenRadius, UpRefAdaptiveSharpen, UpRefAdaptiveSharpenStrength, SuperRes, SuperResStrength, SuperResRadius, "
        "RefineOnce, SuperResFirst"
        ") VALUES ";
      strSQL += PrepareSQL("('%s',%i,'%s',%i,%i,%i,%i,%f,%f,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%f,%i,%f,%f,%f,%i,%f,%i,%f,%i,%f,%f,%f,%i,%f,%i,%f,%f,%i,%i)",
        strFilenameAndPath.c_str(), setting.m_Resolution, setting.m_TvShowName.c_str(), setting.m_ChromaUpscaling, setting.m_ChromaAntiRing, setting.m_ChromaSuperRes, setting.m_ChromaSuperResPasses, setting.m_ChromaSuperResStrength, setting.m_ChromaSuperResSoftness,
        setting.m_ImageUpscaling, setting.m_ImageUpAntiRing, setting.m_ImageUpLinear,
        setting.m_ImageDownscaling, setting.m_ImageDownAntiRing, setting.m_ImageDownLinear,
        setting.m_ImageDoubleLuma,setting.m_ImageDoubleChroma,setting.m_ImageQuadrupleLuma,setting.m_ImageQuadrupleChroma,
        setting.m_ImageDoubleLumaFactor, setting.m_ImageDoubleChromaFactor, setting.m_ImageQuadrupleLumaFactor, setting.m_ImageQuadrupleChromaFactor,
        setting.m_deintactive, setting.m_deintforce, setting.m_deintlookpixels,
        setting.m_smoothMotion, setting.m_dithering, setting.m_ditheringColoredNoise, setting.m_ditheringEveryFrame,
        setting.m_deband, setting.m_debandLevel, setting.m_debandFadeLevel,
        setting.m_fineSharp, setting.m_fineSharpStrength, setting.m_lumaSharpen, setting.m_lumaSharpenStrength, setting.m_lumaSharpenClamp, setting.m_lumaSharpenRadius, setting.m_adaptiveSharpen, setting.m_adaptiveSharpenStrength,
        setting.m_UpRefFineSharp, setting.m_UpRefFineSharpStrength, setting.m_UpRefLumaSharpen, setting.m_UpRefLumaSharpenStrength, setting.m_UpRefLumaSharpenClamp, setting.m_UpRefLumaSharpenRadius, setting.m_UpRefAdaptiveSharpen, setting.m_UpRefAdaptiveSharpenStrength, setting.m_superRes, setting.m_superResStrength, setting.m_superResRadius,
        setting.m_refineOnce, setting.m_superResFirst
        );
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CDSPlayerDatabase::EraseLAVVideo()
{
  try
  {
    std::string sql = "DELETE FROM lavvideoSettings";
    m_pDS->exec(sql);
    CLog::Log(LOGINFO, "Deleting LAVVideo settings");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}
void CDSPlayerDatabase::EraseLAVAudio()
{
  try
  {
    std::string sql = "DELETE FROM lavaudioSettings";
    m_pDS->exec(sql);
    CLog::Log(LOGINFO, "Deleting LAVAudio settings");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}
void CDSPlayerDatabase::EraseLAVSplitter()
{
  try
  {
    std::string sql = "DELETE FROM lavsplitterSettings";
    m_pDS->exec(sql);
    CLog::Log(LOGINFO, "Deleting LAVSplitter settings");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CDSPlayerDatabase::EraseVideoSettings(const std::string &path /* = ""*/)
{
  try
  {
    std::string sql = "DELETE FROM madvrSettings";
    m_pDS->exec(sql);

    sql = "DELETE FROM madvrDefResSettings";
    m_pDS->exec(sql);
    CLog::Log(LOGINFO, "Deleting madvr settings information for all files");

    
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CDSPlayerDatabase::EraseVideoSettings(int resolution, int resolutionInternal, std::string tvShowName)
{
  try
  {
    if (resolution < 0 && tvShowName == "") return;
    CStdString strSQL = "";

    if (resolution > -1)
    {
      strSQL = StringUtils::Format("DELETE FROM madvrSettings where Resolution=%i", resolution);
      CLog::Log(LOGINFO, "Deleting madvr settings information for %i files", resolution);
      m_pDS->exec(strSQL);

      strSQL = StringUtils::Format("DELETE FROM madvrDefResSettings where ResolutionInternal=%i", resolutionInternal);
      CLog::Log(LOGINFO, "Deleting madvr default settings information for %i files", resolutionInternal);
      m_pDS->exec(strSQL);
    }
    else
    {
      strSQL = StringUtils::Format("DELETE FROM madvrSettings where TvShowName='%s'", tvShowName.c_str());
      CLog::Log(LOGINFO, "Deleting madvr settings information for %s files", tvShowName.c_str());
      m_pDS->exec(strSQL);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CDSPlayerDatabase::CreateVideoSettings(int resolution, int resolutionInternal, std::string tvShowName, const CMadvrSettings &setting)
{
  try
  {
    if (resolution < 0 && tvShowName == "") return;
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strSQL = "";
    CStdString strWhere = "";

    if (resolution > -1)
    {
      strSQL = StringUtils::Format("select * from madvrDefResSettings where Resolution='%i'", resolution);
      strWhere = StringUtils::Format("where Resolution=%i", resolution);
      tvShowName = "NOTVSHOW_NULL";
    }
    else if (tvShowName != "")
    {
      strSQL = StringUtils::Format("select * from madvrDefResSettings where TvShowName='%s'", tvShowName.c_str());
      strWhere = StringUtils::Format("where TvShowNamen='%s'", tvShowName.c_str());
    }
    if (strSQL == "") 
      return;

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL = PrepareSQL("update madvrDefResSettings "
        "ChromaUpscaling=%i,ChromaAntiRing=%i,ChromaSuperRes=%i, ChromaSuperResPasses=%i, ChromaSuperResStrength=%f, ChromaSuperResSoftness=%f, "
        "ImageUpscaling=%i,ImageUpAntiRing=%i,ImageUpLinear=%i, "
        "ImageDownscaling=%i,ImageDownAntiRing=%i,ImageDownLinear=%i, "
        "ImageDoubleLuma=%i, ImageDoubleChroma=%i, ImageQuadrupleLuma=%i, ImageQuadrupleChroma=%i, "
        "ImageDoubleLumaFactor=%i, ImageDoubleChromaFactor=%i, ImageQuadrupleLumaFactor=%i, ImageQuadrupleChromaFactor=%i, "
        "DeintActive=%i, DeintForce=%i, DeintLookPixels=%i, "
        "SmoothMotion=%i, Dithering=%i, DitheringColoredNoise=%i, DitheringEveryFrame=%i, "
        "Deband=%i, DebandLevel=%i, DebandFadeLevel=%i, "
        "FineSharp=%i, FineSharpStrength=%f, LumaSharpen=%i, LumaSharpenStrength=%f, LumaSharpenClamp=%f, LumaSharpenRadius=%f, AdaptiveSharpen=%i, AdaptiveSharpenStrength=%f, "
        "UpRefFineSharp=%i, UpRefFineSharpStrength=%f, UpRefLumaSharpen=%i, UpRefLumaSharpenStrength=%f, UpRefLumaSharpenClamp=%f, UpRefLumaSharpenRadius=%f, UpRefAdaptiveSharpen=%i, UpRefAdaptiveSharpenStrength=%f, SuperRes=%i, SuperResStrength=%f, SuperResRadius=%f, "
        "RefineOnce=%i, SuperResFirst=%i ",
        setting.m_ChromaUpscaling, setting.m_ChromaAntiRing, setting.m_ChromaSuperRes, setting.m_ChromaSuperResPasses, setting.m_ChromaSuperResStrength, setting.m_ChromaSuperResSoftness,
        setting.m_ImageUpscaling, setting.m_ImageUpAntiRing, setting.m_ImageUpLinear,
        setting.m_ImageDownscaling, setting.m_ImageDownAntiRing, setting.m_ImageDownLinear,
        setting.m_ImageDoubleLuma, setting.m_ImageDoubleChroma, setting.m_ImageQuadrupleLuma, setting.m_ImageQuadrupleChroma,
        setting.m_ImageDoubleLumaFactor, setting.m_ImageDoubleChromaFactor, setting.m_ImageQuadrupleLumaFactor, setting.m_ImageQuadrupleChromaFactor,
        setting.m_deintactive, setting.m_deintforce, setting.m_deintlookpixels,
        setting.m_smoothMotion, setting.m_dithering, setting.m_ditheringColoredNoise, setting.m_ditheringEveryFrame,
        setting.m_deband, setting.m_debandLevel, setting.m_debandFadeLevel,
        setting.m_fineSharp, setting.m_fineSharpStrength, setting.m_lumaSharpen, setting.m_lumaSharpenStrength, setting.m_lumaSharpenClamp, setting.m_lumaSharpenRadius, setting.m_adaptiveSharpen, setting.m_adaptiveSharpenStrength,
        setting.m_UpRefFineSharp, setting.m_UpRefFineSharpStrength, setting.m_UpRefLumaSharpen, setting.m_UpRefLumaSharpenStrength, setting.m_UpRefLumaSharpenClamp, setting.m_UpRefLumaSharpenRadius, setting.m_UpRefAdaptiveSharpen, setting.m_UpRefAdaptiveSharpenStrength, setting.m_superRes, setting.m_superResStrength, setting.m_superResRadius,
        setting.m_refineOnce, setting.m_superResFirst
        );
      
      strSQL += strSQL + strWhere;
      m_pDS->exec(strSQL.c_str());
      return;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL = "INSERT INTO madvrDefResSettings (Resolution, ResolutionInternal, TvShowName,"
        "ChromaUpscaling, ChromaAntiRing, ChromaSuperRes, ChromaSuperResPasses, ChromaSuperResStrength, ChromaSuperResSoftness, "
        "ImageUpscaling, ImageUpAntiRing, ImageUpLinear, "
        "ImageDownscaling, ImageDownAntiRing, ImageDownLinear, "
        "ImageDoubleLuma, ImageDoubleChroma, ImageQuadrupleLuma, ImageQuadrupleChroma, "
        "ImageDoubleLumaFactor, ImageDoubleChromaFactor, ImageQuadrupleLumaFactor, ImageQuadrupleChromaFactor, "
        "DeintActive, DeintForce, DeintLookPixels, "
        "SmoothMotion, Dithering, DitheringColoredNoise, DitheringEveryFrame, "
        "Deband, DebandLevel, DebandFadeLevel, "
        "FineSharp, FineSharpStrength, LumaSharpen, LumaSharpenStrength, LumaSharpenClamp, LumaSharpenRadius, AdaptiveSharpen, AdaptiveSharpenStrength, "
        "UpRefFineSharp, UpRefFineSharpStrength, UpRefLumaSharpen, UpRefLumaSharpenStrength, UpRefLumaSharpenClamp, UpRefLumaSharpenRadius, UpRefAdaptiveSharpen, UpRefAdaptiveSharpenStrength, SuperRes, SuperResStrength, SuperResRadius, "
        "RefineOnce, SuperResFirst"
        ") VALUES ";
      strSQL += PrepareSQL("(%i,%i,'%s',%i,%i,%i,%i,%f,%f,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%f,%i,%f,%f,%f,%i,%f,%i,%f,%i,%f,%f,%f,%i,%f,%i,%f,%f,%i,%i)",
        resolution, resolutionInternal, tvShowName.c_str(), setting.m_ChromaUpscaling, setting.m_ChromaAntiRing, setting.m_ChromaSuperRes, setting.m_ChromaSuperResPasses, setting.m_ChromaSuperResStrength, setting.m_ChromaSuperResSoftness,
        setting.m_ImageUpscaling, setting.m_ImageUpAntiRing, setting.m_ImageUpLinear,
        setting.m_ImageDownscaling, setting.m_ImageDownAntiRing, setting.m_ImageDownLinear,
        setting.m_ImageDoubleLuma, setting.m_ImageDoubleChroma, setting.m_ImageQuadrupleLuma, setting.m_ImageQuadrupleChroma,
        setting.m_ImageDoubleLumaFactor, setting.m_ImageDoubleChromaFactor, setting.m_ImageQuadrupleLumaFactor, setting.m_ImageQuadrupleChromaFactor,
        setting.m_deintactive, setting.m_deintforce, setting.m_deintlookpixels,
        setting.m_smoothMotion, setting.m_dithering, setting.m_ditheringColoredNoise, setting.m_ditheringEveryFrame,
        setting.m_deband, setting.m_debandLevel, setting.m_debandFadeLevel,
        setting.m_fineSharp, setting.m_fineSharpStrength, setting.m_lumaSharpen, setting.m_lumaSharpenStrength, setting.m_lumaSharpenClamp, setting.m_lumaSharpenRadius, setting.m_adaptiveSharpen, setting.m_adaptiveSharpenStrength,
        setting.m_UpRefFineSharp, setting.m_UpRefFineSharpStrength, setting.m_UpRefLumaSharpen, setting.m_UpRefLumaSharpenStrength, setting.m_UpRefLumaSharpenClamp, setting.m_UpRefLumaSharpenRadius, setting.m_UpRefAdaptiveSharpen, setting.m_UpRefAdaptiveSharpenStrength, setting.m_superRes, setting.m_superResStrength, setting.m_superResRadius,
        setting.m_refineOnce, setting.m_superResFirst
        );
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%ip) failed", __FUNCTION__, resolution);
  }
}

#endif