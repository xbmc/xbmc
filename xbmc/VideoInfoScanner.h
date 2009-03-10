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
#include "utils/Thread.h"
#include "VideoDatabase.h"
#include "ScraperSettings.h"
#include "NfoFile.h"
#include "utils/IMDB.h"

class CIMDB;

namespace VIDEO
{
  typedef struct SScanSettings
  {
    bool parent_name;       /* use the parent dirname as name of lookup */
    bool parent_name_root;  /* use the name of directory where scan started as name for files in that dir */
    int  recurse;           /* recurse into sub folders (indicate levels) */
    bool noupdate;          /* exclude from update library function */
  } SScanSettings;

  typedef struct SEpisode
  {
    CStdString strPath;
    int iSeason;
    int iEpisode;
  } SEpisode;

  typedef std::vector<SEpisode> EPISODES;

  enum SCAN_STATE { PREPARING = 0, REMOVING_OLD, CLEANING_UP_DATABASE, FETCHING_MOVIE_INFO, FETCHING_MUSICVIDEO_INFO, FETCHING_TVSHOW_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

  class IVideoInfoScannerObserver
  {
  public:
    virtual ~IVideoInfoScannerObserver() { }
    virtual void OnStateChanged(SCAN_STATE state) = 0;
    virtual void OnDirectoryChanged(const CStdString& strDirectory) = 0;
    virtual void OnDirectoryScanned(const CStdString& strDirectory) = 0;
    virtual void OnSetProgress(int currentItem, int itemCount)=0;
    virtual void OnSetCurrentProgress(int currentItem, int itemCount)=0;
    virtual void OnSetTitle(const CStdString& strTitle) = 0;
    virtual void OnFinished() = 0;
  };

  class CVideoInfoScanner : CThread, public IRunnable
  {
  public:
    CVideoInfoScanner();
    virtual ~CVideoInfoScanner();
    void Start(const CStdString& strDirectory, const SScraperInfo& info, const SScanSettings& settings, bool bUpdateAll);
    bool IsScanning();
    void Stop();
    void SetObserver(IVideoInfoScannerObserver* pObserver);

    void EnumerateSeriesFolder(CFileItem* item, EPISODES& episodeList);
    long AddMovieAndGetThumb(CFileItem *pItem, const CStdString &content, CVideoInfoTag &movieDetails, long idShow, bool bApplyToDir=false, CGUIDialogProgress* pDialog = NULL);
    bool OnProcessSeriesFolder(IMDB_EPISODELIST& episodes, EPISODES& files, long lShowId, const CStdString& strShowTitle, CGUIDialogProgress* pDlgProgress = NULL);
    static CStdString GetnfoFile(CFileItem *item, bool bGrabAny=false);
    long GetIMDBDetails(CFileItem *pItem, CScraperUrl &url, const SScraperInfo& info, bool bUseDirNames=false, CGUIDialogProgress* pDialog=NULL, bool combined=false);
    bool RetrieveVideoInfo(CFileItemList& items, bool bDirNames, const SScraperInfo& info, bool bRefresh=false, CScraperUrl *pURL=NULL, CGUIDialogProgress* pDlgProgress  = NULL);
    static void ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb);
    CNfoFile::NFOResult CheckForNFOFile(CFileItem* pItem, bool bGrabAny, SScraperInfo& info, CScraperUrl& scrUrl);
    CIMDB m_IMDB;
  protected:
    virtual void Process();
    bool DoScan(const CStdString& strDirectory, SScanSettings settings);

    virtual void Run();
    int CountFiles(const CStdString& strPath);
    void FetchSeasonThumbs(long lTvShowId);
    void FetchActorThumbs(const std::vector<SActorInfo>& actors);
    static int GetPathHash(const CFileItemList &items, CStdString &hash);

  protected:
    IVideoInfoScannerObserver* m_pObserver;
    int m_currentItem;
    int m_itemCount;
    bool m_bRunning;
    bool m_bUpdateAll;
    bool m_bCanInterrupt;
    bool m_bClean;
    CStdString m_strStartDir;
    CVideoDatabase m_database;
    SScraperInfo m_info;
    std::map<CStdString,SScanSettings> m_pathsToScan;
    std::set<CStdString> m_pathsToCount;
    std::vector<long> m_pathsToClean;
    CNfoFile m_nfoReader;
  };
}

