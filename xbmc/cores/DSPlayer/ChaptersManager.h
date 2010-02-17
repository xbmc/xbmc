#pragma once

#include "dshowutil/dshowutil.h"

// IAMExtendedSeeking
#include <qnetwork.h>

#include "DSGraph.h"
#include "log.h"
#include "CharsetConverter.h"

/// \brief Contains informations about a chapter
struct SChapterInfos
{
  CStdString name; ///< Chapter's name
  double time; ///< Chapter's start time (in ms)
};

/** DSPlayer Chapters Manager.

  Singleton class handling chapters management
 */
class CChaptersManager
{
public:
  /// Store singleton instance
  static CChaptersManager *m_pSingleton;
  /// Retrieve singleton instance
  static CChaptersManager *getSingleton();
  /// Destroy the singleton instance
  static void Destroy();

  /** Retrieve the chapters count.
  @return Number of chapters in the media file
  */
  int GetChapterCount(void);
  /** Retrive the current chapter.
   * @return ID of the current chapter
   */
  int GetChapter(void);
  /** Retrive current chapter's name
   * @param[out] strChapterName The chapter's name
   */
  void GetChapterName(CStdString& strChapterName);
  /** Sync the current chapter with the media file */
  void UpdateChapters(void);
  /** Seek to the specified chapter
   * @param iChapter Chapter to seek to
   * @return Always 0
   */
  int SeekChapter(int iChapter);
  /** Load the chapters from the media file
   * @return True if succeeded, false else
   */
  bool LoadChapters(void);
  /** Initialize the chapter's manager
   * @param[in] Splitter Pointer to the splitter interface
   * @param[in] Graph Pointer to a CDSGraph instance
   * @return True if the manager is initialized, false else
   */
  bool InitManager(IBaseFilter *Splitter, CDSGraph *Graph);

private:
  /// Constructor
  CChaptersManager(void);
  /// Destructor
  ~CChaptersManager(void);

  /// Store SChapterInfos structs
  std::map<long, SChapterInfos *> m_chapters;
  /// ID of the current chapter
  long m_currentChapter;
  /// Pointer to the IAMExtendedSeeking interface
  IAMExtendedSeeking* m_pIAMExtendedSeeking;
  /// Splitter
  IBaseFilter* m_pSplitter;
  /// CDSGraph instance
  CDSGraph *m_pGraph;

  /// Is the manager initialized?
  bool m_init;
};

