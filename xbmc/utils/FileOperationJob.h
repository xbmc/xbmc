#include "system.h"
#include "FileItem.h"
#include "Job.h"
#include "../FileSystem/File.h"

class CFileOperationJob : public CJob
{
public:
  enum FileAction
  {
    ActionCopy = 1,
    ActionMove,
    ActionDelete,
    ActionCreateFolder,
    ActionDeleteFolder,
  };

  CFileOperationJob(FileAction action, CFileItemList & items, const CStdString& strDestFile);

  virtual bool DoWork();
  const CStdString &GetAverageSpeed()     { return m_avgSpeed; }
  const CStdString &GetCurrentOperation() { return m_currentOperation; }
  const CStdString &GetCurrentFile()      { return m_currentFile; }
private:
  class CFileOperation : public XFILE::IFileCallback
  {
  public:
    CFileOperation(FileAction action, const CStdString &strFileA, const CStdString &strFileB, int64_t time);
    bool ExecuteOperation(CFileOperationJob *base, double &current, double opWeight);
    void Debug();
    virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed);
  private:
    FileAction m_action;
    CStdString m_strFileA, m_strFileB;
    int64_t m_time;
  };
  friend class CFileOperation;
  typedef std::vector<CFileOperation> FileOperationList;
  bool DoProcess(FileAction action, CFileItemList & items, const CStdString& strDestFile, FileOperationList &fileOperations, double &totalTime);
  bool DoProcessFolder(FileAction action, const CStdString& strPath, const CStdString& strDestFile, FileOperationList &fileOperations, double &totalTime);
  bool DoProcessFile(FileAction action, const CStdString& strFileA, const CStdString& strFileB, FileOperationList &fileOperations, double &totalTime);

  static inline bool CanBeRenamed(const CStdString &strFileA, const CStdString &strFileB);

  FileAction m_action;
  CFileItemList m_items;
  CStdString m_strDestFile;
  CStdString m_avgSpeed, m_currentOperation, m_currentFile;
};
