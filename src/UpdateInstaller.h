#pragma once

#include "Platform.h"
#include "FileUtils.h"
#include "UpdateScript.h"

#include <list>
#include <string>
#include <map>

class UpdateObserver;

/** Central class responsible for installing updates,
  * launching an elevated copy of the updater if required
  * and restarting the main application once the update
  * is installed.
  */
class UpdateInstaller
{
public:
  enum Mode
  {
    Setup,
    Main
  };

  UpdateInstaller();
  void setInstallDir(const std::string& path);
  void setTargetDir(const std::string& path);
  void setPackageDir(const std::string& path);
  void setMode(Mode mode);
  void setScript(UpdateScript* script);
  void setWaitPid(PLATFORM_PID pid);
  void setForceElevated(bool elevated);
  void setAutoClose(bool autoClose);

  void setObserver(UpdateObserver* observer);

  void run() throw();

  void restartMainApp();

private:
  bool checkAccess();

  void installFiles();
  void patchFiles();
  void uninstallFiles();
  void installFile(const UpdateScriptFile& file);
  void patchFile(const UpdateScriptPatch& file);
  void reportError(const std::string& error);
  void postInstallUpdate();
  void updateProgress();
  void verifyAgainstManifest();
  void findFiles(const std::string& path, std::vector<std::string>& list);
  void copyBundle();

  std::list<std::string> updaterArgs() const;
  std::string friendlyErrorForError(const FileUtils::IOException& ex) const;

  Mode m_mode;
  std::string m_installDir;
  std::string m_targetDir;
  std::string m_packageDir;
  std::string m_tempDir;
  PLATFORM_PID m_waitPid;
  UpdateScript* m_script;
  UpdateObserver* m_observer;
  bool m_forceElevated;
  bool m_autoClose;

  int m_installed;
};
