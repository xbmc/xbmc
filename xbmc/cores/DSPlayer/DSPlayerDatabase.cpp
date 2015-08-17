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
  m_pDS->exec("CREATE TABLE madvrSettings ( file text, Resolution integer,"
    "ChromaUpscaling integer, ChromaAntiRing bool, ChromaSuperRes bool, ImageUpscaling integer, ImageUpAntiRing bool, ImageUpLinear bool, ImageDownscaling integer, ImageDownAntiRing bool, ImageDownLinear bool, "
    "ImageDoubleLuma integer, ImageDoubleChroma integer, ImageQuadrupleLuma integer, ImageQuadrupleChroma integer, " 
    "ImageDoubleLumaFactor integer, ImageDoubleChromaFactor integer, ImageQuadrupleLumaFactor integer, ImageQuadrupleChromaFactor integer, "
    "DeintActive integer, DeintForce interger, DeintLookPixels bool, "
    "SmoothMotion integer, Dithering integer, DitheringColoredNoise bool, DitheringEveryFrame bool, "
    "Deband bool, DebandLevel integer, DebandFadeLevel integer"
    ")\n");

  CLog::Log(LOGINFO, "create madvr resolution for resolution table");
  m_pDS->exec("CREATE TABLE madvrDefResSettings ( Resolution integer,"
    "ChromaUpscaling integer, ChromaAntiRing bool, ChromaSuperRes bool, ImageUpscaling integer, ImageUpAntiRing bool, ImageUpLinear bool, ImageDownscaling integer, ImageDownAntiRing bool, ImageDownLinear bool, "
    "ImageDoubleLuma integer, ImageDoubleChroma integer, ImageQuadrupleLuma integer, ImageQuadrupleChroma integer, "
    "ImageDoubleLumaFactor integer, ImageDoubleChromaFactor integer, ImageQuadrupleLumaFactor integer, ImageQuadrupleChromaFactor integer, "
    "DeintActive integer, DeintForce interger, DeintLookPixels bool, "
    "SmoothMotion integer, Dithering integer, DitheringColoredNoise bool, DitheringEveryFrame bool, "
    "Deband bool, DebandLevel integer, DebandFadeLevel integer"
    ")\n");
}

void CDSPlayerDatabase::CreateAnalytics()
{
  m_pDS->exec("CREATE INDEX idxEdition ON edition (file)");
  m_pDS->exec("CREATE INDEX idxMadvrSettings ON madvrSettings (file)");
  m_pDS->exec("CREATE INDEX idxMadvrDefResSettings ON madvrDefResSettings (Resolution)");
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

bool CDSPlayerDatabase::GetDefResMadvrSettings(int resolution, CMadvrSettings &settings)
{
  try
  {
    if (resolution < 0) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    // ok, now obtain the settings for this file
    CStdString strSQL = PrepareSQL("select * from madvrDefResSettings where madvrDefResSettings.Resolution=%i", resolution);

    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    { // get the madvr settings info

      settings.m_ChromaUpscaling = m_pDS->fv("ChromaUpscaling").get_asInt();
      settings.m_ChromaAntiRing = m_pDS->fv("ChromaAntiRing").get_asBool();
      settings.m_ChromaSuperRes = m_pDS->fv("ChromaSuperRes").get_asBool();

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
        "set Resolution=%i, "
        "ChromaUpscaling=%i,ChromaAntiRing=%i,ChromaSuperRes=%i, "
        "ImageUpscaling=%i,ImageUpAntiRing=%i,ImageUpLinear=%i, "
        "ImageDownscaling=%i,ImageDownAntiRing=%i,ImageDownLinear=%i, "
        "ImageDoubleLuma=%i, ImageDoubleChroma=%i, ImageQuadrupleLuma=%i, ImageQuadrupleChroma=%i, "
        "ImageDoubleLumaFactor=%i, ImageDoubleChromaFactor=%i, ImageQuadrupleLumaFactor=%i, ImageQuadrupleChromaFactor=%i, "
        "DeintActive=%i, DeintForce=%i, DeintLookPixels=%i, "
        "SmoothMotion=%i, Dithering=%i, DitheringColoredNoise=%i, DitheringEveryFrame=%i, "
        "Deband=%i, DebandLevel=%i, DebandFadeLevel=%i "
        "where file='%s'",
        setting.m_Resolution,
        setting.m_ChromaUpscaling,setting.m_ChromaAntiRing,setting.m_ChromaSuperRes,
        setting.m_ImageUpscaling,setting.m_ImageUpAntiRing,setting.m_ImageUpLinear,
        setting.m_ImageDownscaling, setting.m_ImageDownAntiRing, setting.m_ImageDownLinear, 
        setting.m_ImageDoubleLuma, setting.m_ImageDoubleChroma, setting.m_ImageQuadrupleLuma, setting.m_ImageQuadrupleChroma,
        setting.m_ImageDoubleLumaFactor, setting.m_ImageDoubleChromaFactor, setting.m_ImageQuadrupleLumaFactor, setting.m_ImageQuadrupleChromaFactor,
        setting.m_deintactive, setting.m_deintforce, setting.m_deintlookpixels,
        setting.m_smoothMotion, setting.m_dithering, setting.m_ditheringColoredNoise, setting.m_ditheringEveryFrame,
        setting.m_deband, setting.m_debandLevel, setting.m_debandFadeLevel, 
        strFilenameAndPath.c_str());
      m_pDS->exec(strSQL.c_str());
      return;   
    }
    else
    { // add the items
      m_pDS->close();
      strSQL = "INSERT INTO madvrSettings (file, Resolution, "
        "ChromaUpscaling, ChromaAntiRing, ChromaSuperRes, "
        "ImageUpscaling, ImageUpAntiRing, ImageUpLinear, "
        "ImageDownscaling, ImageDownAntiRing, ImageDownLinear, "
        "ImageDoubleLuma, ImageDoubleChroma, ImageQuadrupleLuma, ImageQuadrupleChroma, "
        "ImageDoubleLumaFactor, ImageDoubleChromaFactor, ImageQuadrupleLumaFactor, ImageQuadrupleChromaFactor, "
        "DeintActive, DeintForce, DeintLookPixels, "
        "SmoothMotion, Dithering, DitheringColoredNoise, DitheringEveryFrame, "
        "Deband, DebandLevel, DebandFadeLevel"
        ") VALUES ";
      strSQL += PrepareSQL("('%s',%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i)",
        strFilenameAndPath.c_str(), setting.m_Resolution, setting.m_ChromaUpscaling, setting.m_ChromaAntiRing, setting.m_ChromaSuperRes,
        setting.m_ImageUpscaling, setting.m_ImageUpAntiRing, setting.m_ImageUpLinear,
        setting.m_ImageDownscaling, setting.m_ImageDownAntiRing, setting.m_ImageDownLinear,
        setting.m_ImageDoubleLuma,setting.m_ImageDoubleChroma,setting.m_ImageQuadrupleLuma,setting.m_ImageQuadrupleChroma,
        setting.m_ImageDoubleLumaFactor, setting.m_ImageDoubleChromaFactor, setting.m_ImageQuadrupleLumaFactor, setting.m_ImageQuadrupleChromaFactor,
        setting.m_deintactive, setting.m_deintforce, setting.m_deintlookpixels,
        setting.m_smoothMotion, setting.m_dithering, setting.m_ditheringColoredNoise, setting.m_ditheringEveryFrame,
        setting.m_deband, setting.m_debandLevel, setting.m_debandFadeLevel
        );
      m_pDS->exec(strSQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
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

void CDSPlayerDatabase::EraseVideoSettings(int resolution)
{
  try
  {
    CStdString strSQL = StringUtils::Format("DELETE FROM madvrSettings where Resolution=%i", resolution);

    CLog::Log(LOGINFO, "Deleting madvr settings information for %ip files",resolution);

    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CDSPlayerDatabase::CreateVideoSettings(int resolution, const CMadvrSettings &setting)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString strSQL = StringUtils::Format("select * from madvrDefResSettings where Resolution='%i'", resolution);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL = PrepareSQL("update madvrDefResSettings "
        "set ChromaUpscaling=%i,ChromaAntiRing=%i,ChromaSuperRes=%i, "
        "ImageUpscaling=%i,ImageUpAntiRing=%i,ImageUpLinear=%i, "
        "ImageDownscaling=%i,ImageDownAntiRing=%i,ImageDownLinear=%i, "
        "ImageDoubleLuma=%i, ImageDoubleChroma=%i, ImageQuadrupleLuma=%i, ImageQuadrupleChroma=%i, "
        "ImageDoubleLumaFactor=%i, ImageDoubleChromaFactor=%i, ImageQuadrupleLumaFactor=%i, ImageQuadrupleChromaFactor=%i, "
        "DeintActive=%i, DeintForce=%i, DeintLookPixels=%i, "
        "SmoothMotion=%i, Dithering=%i, DitheringColoredNoise=%i, DitheringEveryFrame=%i, "
        "Deband=%i, DebandLevel=%i, DebandFadeLevel=%i "
        "where Resolution=%i",
        setting.m_ChromaUpscaling, setting.m_ChromaAntiRing, setting.m_ChromaSuperRes,
        setting.m_ImageUpscaling, setting.m_ImageUpAntiRing, setting.m_ImageUpLinear,
        setting.m_ImageDownscaling, setting.m_ImageDownAntiRing, setting.m_ImageDownLinear,
        setting.m_ImageDoubleLuma, setting.m_ImageDoubleChroma, setting.m_ImageQuadrupleLuma, setting.m_ImageQuadrupleChroma,
        setting.m_ImageDoubleLumaFactor, setting.m_ImageDoubleChromaFactor, setting.m_ImageQuadrupleLumaFactor, setting.m_ImageQuadrupleChromaFactor,
        setting.m_deintactive, setting.m_deintforce, setting.m_deintlookpixels,
        setting.m_smoothMotion, setting.m_dithering, setting.m_ditheringColoredNoise, setting.m_ditheringEveryFrame,
        setting.m_deband, setting.m_debandLevel, setting.m_debandFadeLevel,
        resolution);
      m_pDS->exec(strSQL.c_str());
      return;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL = "INSERT INTO madvrDefResSettings (Resolution, "
        "ChromaUpscaling, ChromaAntiRing, ChromaSuperRes, "
        "ImageUpscaling, ImageUpAntiRing, ImageUpLinear, "
        "ImageDownscaling, ImageDownAntiRing, ImageDownLinear, "
        "ImageDoubleLuma, ImageDoubleChroma, ImageQuadrupleLuma, ImageQuadrupleChroma, "
        "ImageDoubleLumaFactor, ImageDoubleChromaFactor, ImageQuadrupleLumaFactor, ImageQuadrupleChromaFactor, "
        "DeintActive, DeintForce, DeintLookPixels, "
        "SmoothMotion, Dithering, DitheringColoredNoise, DitheringEveryFrame, "
        "Deband, DebandLevel, DebandFadeLevel"
        ") VALUES ";
      strSQL += PrepareSQL("(%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i)",
        resolution, setting.m_ChromaUpscaling, setting.m_ChromaAntiRing, setting.m_ChromaSuperRes,
        setting.m_ImageUpscaling, setting.m_ImageUpAntiRing, setting.m_ImageUpLinear,
        setting.m_ImageDownscaling, setting.m_ImageDownAntiRing, setting.m_ImageDownLinear,
        setting.m_ImageDoubleLuma, setting.m_ImageDoubleChroma, setting.m_ImageQuadrupleLuma, setting.m_ImageQuadrupleChroma,
        setting.m_ImageDoubleLumaFactor, setting.m_ImageDoubleChromaFactor, setting.m_ImageQuadrupleLumaFactor, setting.m_ImageQuadrupleChromaFactor,
        setting.m_deintactive, setting.m_deintforce, setting.m_deintlookpixels,
        setting.m_smoothMotion, setting.m_dithering, setting.m_ditheringColoredNoise, setting.m_ditheringEveryFrame,
        setting.m_deband, setting.m_debandLevel, setting.m_debandFadeLevel
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