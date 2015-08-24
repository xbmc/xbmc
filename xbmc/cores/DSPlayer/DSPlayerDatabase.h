#pragma once
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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "Filters/MadvrSettings.h"
#include "Filters/LavSettings.h"
#include "dbwrappers/Database.h"
#include "FileItem.h"
#include "utils/StdString.h"

class CEdition
{
public:
  CEdition();

  CStdString editionName;
  int editionNumber;
  bool IsSet() const;
};

typedef std::vector<CEdition> VECEDITIONS;

class CDSPlayerDatabase : public CDatabase
{
public:
  CDSPlayerDatabase(void);
  ~CDSPlayerDatabase(void);

  virtual bool Open();
  bool GetResumeEdition(const CFileItem *item, CEdition &edition);
  bool GetResumeEdition(const CStdString& strFilenameAndPath, CEdition &edition);
  void GetEditionForFile(const CStdString& strFilenameAndPath, VECEDITIONS &ditions);
  void AddEdition(const CStdString& strFilenameAndPath, const CEdition &edition);
  void ClearEditionOfFile(const CStdString& strFilenameAndPath);

  bool GetVideoSettings(const CStdString &strFilenameAndPath, CMadvrSettings &settings);
  void SetVideoSettings(const CStdString &strFilenameAndPath, const CMadvrSettings &settings);
  void EraseVideoSettings(const std::string &path = "");
  void EraseVideoSettings(const int resolution);
  bool GetDefResMadvrSettings(int resolution, CMadvrSettings &settings);
  void CreateVideoSettings(int resolution, const CMadvrSettings &setting);
  bool GetLAVVideoSettings(CLavSettings &settings);
  bool GetLAVAudioSettings(CLavSettings &settings);
  bool GetLAVSplitterSettings(CLavSettings &settings);
  void SetLAVVideoSettings(CLavSettings &settings);
  void SetLAVAudioSettings(CLavSettings &settings);
  void SetLAVSplitterSettings(CLavSettings &settings);

protected:
  virtual void CreateTables();
  virtual void CreateAnalytics();

  virtual int GetMinSchemaVersion() const { return 4; };
  virtual int GetExportVersion() const { return 1; };
  virtual int GetSchemaVersion() const { return 4; };
  const char *GetBaseDBName() const { return "DSPlayer"; };

};
