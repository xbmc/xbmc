#include "UpdateInstaller.h"

#include "AppInfo.h"
#include "FileUtils.h"
#include "Log.h"
#include "ProcessUtils.h"
#include "UpdateObserver.h"
#include "bspatch.h"
#include "DirIterator.h"

UpdateInstaller::UpdateInstaller()
  : m_mode(Setup),
    m_waitPid(0),
    m_script(0),
    m_observer(0),
    m_forceElevated(false),
    m_autoClose(false),
    m_installed(0)
{
  m_tempDir = FileUtils::tempPath();
  LOG(Info, "Using tmpdir: " + m_tempDir);
}

void UpdateInstaller::setWaitPid(PLATFORM_PID pid)
{
  m_waitPid = pid;
}

void UpdateInstaller::setInstallDir(const std::string& path)
{
  m_installDir = path;
}

void UpdateInstaller::setTargetDir(const std::string& path)
{
  m_targetDir = path;
}

void UpdateInstaller::setPackageDir(const std::string& path)
{
  m_packageDir = path;
}

void UpdateInstaller::setMode(Mode mode)
{
  m_mode = mode;
}

void UpdateInstaller::setScript(UpdateScript* script)
{
  m_script = script;
}

void UpdateInstaller::setForceElevated(bool elevated)
{
  m_forceElevated = elevated;
}

std::list<std::string> UpdateInstaller::updaterArgs() const
{
  std::list<std::string> args;
  args.push_back("--install-dir");
  args.push_back(m_targetDir);
  args.push_back("--package-dir");
  args.push_back(m_packageDir);
  args.push_back("--script");
  args.push_back(m_script->path());
  if (m_autoClose)
  {
    args.push_back("--auto-close");
  }
  return args;
}

void UpdateInstaller::reportError(const std::string& error)
{
  if (m_observer)
  {
    m_observer->updateError(error);
    m_observer->updateFinished();
  }
}

std::string UpdateInstaller::friendlyErrorForError(const FileUtils::IOException& exception) const
{
  std::string friendlyError;

  switch (exception.type())
  {
    case FileUtils::IOException::ReadOnlyFileSystem:
#ifdef PLATFORM_MAC
      friendlyError = AppInfo::appName() + " was started from a read-only location.  "
                                           "Copy it to the Applications folder on your Mac and run "
                                           "it from there.";
#else
      friendlyError = AppInfo::appName() +
                      " was started from a read-only location.  "
                      "Re-install it to a location that can be updated and run it from there.";
#endif
      break;
    case FileUtils::IOException::DiskFull:
      friendlyError = "The disk is full.  Please free up some space and try again.";
      break;
    default:
      break;
  }

  return friendlyError;
}

#define DID_CANCEL() { if (m_observer && m_observer->didCancel()) { throw std::string("Update canceled"); } }

void UpdateInstaller::run() throw()
{
  if (!m_script || !m_script->isValid())
  {
    reportError("Unable to read update script");
    return;
  }
  if (m_targetDir.empty())
  {
    reportError("No installation directory specified");
    return;
  }

  std::string updaterPath;
  try
  {
    updaterPath = ProcessUtils::currentProcessPath();
  }
  catch (const FileUtils::IOException&)
  {
    LOG(Error, "error reading process path with mode " + intToStr(m_mode));
    reportError("Unable to determine path of updater");
    return;
  }

  if (m_mode == Setup)
  {
    if (m_waitPid != 0)
    {
      LOG(Info, "Waiting for main app process to finish");
      ProcessUtils::waitForProcess(m_waitPid);
    }

    std::list<std::string> args = updaterArgs();
    args.push_back("--mode");
    args.push_back("main");
    args.push_back("--wait");
    args.push_back(intToStr(ProcessUtils::currentProcessId()));

    int installStatus = 0;
    if (m_forceElevated || !checkAccess())
    {
      LOG(Info, "Insufficient rights to install app to " + m_targetDir + " requesting elevation");

      // start a copy of the updater with admin rights
      installStatus = ProcessUtils::runElevated(updaterPath, args, AppInfo::name());
    }
    else
    {
      LOG(Info, "Sufficient rights to install app - restarting with same permissions");
      installStatus = ProcessUtils::runSync(updaterPath, args);
    }

    if (installStatus == 0)
    {
      LOG(Info, "Update install completed");
    }
    else
    {
      LOG(Error, "Update install failed with status " + intToStr(installStatus));
    }

    // restart the main application - this is currently done
    // regardless of whether the installation succeeds or not
    restartMainApp();
  }
  else if (m_mode == Main)
  {
    LOG(Info, "Starting update installation");

    // the detailed error string returned by the OS
    std::string error;
    // the message to present to the user.  This may be the same
    // as 'error' or may be different if a more helpful suggestion
    // can be made for a particular problem
    std::string friendlyError;

    std::string backupDir = m_targetDir;
    if (endsWith(backupDir, "/"))
      backupDir = backupDir.substr(0, backupDir.size() - 1);

    backupDir += ".bak";

    try
    {
      LOG(Info, "Copy bundle");
      copyBundle();

      DID_CANCEL();

      LOG(Info, "Patching files");
      patchFiles();

      LOG(Info, "Installing new and updated files");
      installFiles();

      LOG(Info, "Uninstalling removed files");
      uninstallFiles();

      DID_CANCEL();

      LOG(Info, "Verifying files against manifest");
      verifyAgainstManifest();

      // last chance
      DID_CANCEL();

      LOG(Info, "Moving bundle inplace");
      FileUtils::moveFile(m_targetDir.c_str(), backupDir.c_str());
      FileUtils::moveFile(m_installDir.c_str(), m_targetDir.c_str());

      postInstallUpdate();
    }
    catch (const FileUtils::IOException& exception)
    {
      error = exception.what();
      friendlyError = friendlyErrorForError(exception);
    }
    catch (const std::string& genericError)
    {
      error = genericError;
    }

    if (!error.empty())
    {
      LOG(Error, std::string("Error installing update ") + error);

      if (m_observer)
      {
        if (friendlyError.empty())
        {
          friendlyError = error;
        }
        m_observer->updateError(friendlyError);
      }
    }
    else if (m_observer)
    {
      m_observer->updateMessage("Finishing up...");
    }

    try
    {
      LOG(Info, "Removing backups and other cruft...");
      if (FileUtils::fileExists(backupDir.c_str()))
        FileUtils::rmdirRecursive(backupDir.c_str());

      if (FileUtils::fileExists(m_installDir.c_str()) && m_installDir != m_targetDir)
        FileUtils::rmdirRecursive(m_installDir.c_str());

      FileUtils::rmdir(m_tempDir.c_str());
    }
    catch (const FileUtils::IOException& exception)
    {
      error = exception.what();
      // Log about it, but since we are done no need to do anything else
      LOG(Error, std::string("Failed to cleanup: " + error));
    }

    if (m_observer)
    {
      m_observer->updateFinished();
    }
  }
}

void UpdateInstaller::installFile(const UpdateScriptFile& file)
{
  std::string destPath = m_installDir + '/' + file.path;
  std::string target = file.linkTarget;

  // create the target directory if it does not exist
  std::string destDir = FileUtils::dirname(destPath.c_str());
  if (!FileUtils::fileExists(destDir.c_str()))
  {
    FileUtils::mkpath(destDir.c_str());
  }

  if (target.empty())
  {
    // locate the package containing the file
    if (!file.package.empty())
    {
      std::string packageFile = m_packageDir + '/' + file.package + ".zip";
      if (!FileUtils::fileExists(packageFile.c_str()))
      {
        throw "Package file does not exist: " + packageFile;
      }

      std::string filePath = file.path;
      if (!file.pathPrefix.empty())
        filePath = file.pathPrefix + '/' + file.path;
      
      if (FileUtils::fileExists(destPath.c_str()) && FileUtils::fileIsLink(destPath.c_str()))
      {
        LOG(Info, "New file is not link, removing: " + destPath);
        if (FileUtils::isDirectory(destPath.c_str()))
          FileUtils::rmdirRecursive(destPath.c_str());
        else
          FileUtils::removeFile(destPath.c_str());
      }

      // extract the file from the package and copy it to
      // the destination
      FileUtils::extractFromZip(packageFile.c_str(), filePath.c_str(), destPath.c_str());
    }
    else
    {
      // if no package is specified, look for an uncompressed file in the
      // root of the package directory
      std::string sourceFile = m_packageDir + '/' + FileUtils::fileName(file.path.c_str());
      if (!FileUtils::fileExists(sourceFile.c_str()))
      {
        throw "Source file does not exist: " + sourceFile;
      }
      FileUtils::copyFile(sourceFile.c_str(), destPath.c_str());
    }

    // set the permissions on the newly extracted file
    FileUtils::chmod(destPath.c_str(), file.permissions);
  }
  else
  {
    // create the symlink
    if (FileUtils::fileExists(destPath.c_str()))
    {
      bool recreate = false;
      if (FileUtils::fileIsLink(destPath.c_str()))
      {
        std::string symTarget = FileUtils::getSymlinkTarget(destPath.c_str());
        if (symTarget != target)
        {
          LOG(Info, "need new symlink " + target + " != " + symTarget);
          recreate = true;
        }
      }
      else
        recreate = true;
      
      if (recreate)
      {
        LOG(Info, "Recreating symlink " + destPath + "->" + target);
        if (FileUtils::isDirectory(destPath.c_str()))
          FileUtils::rmdirRecursive(destPath.c_str());
        else
          FileUtils::removeFile(destPath.c_str());
        FileUtils::createSymLink(destPath.c_str(), target.c_str());
      }
    }
  }
}

void UpdateInstaller::updateProgress()
{
  if (m_observer)
  {
    int toInstall =
    static_cast<int>(m_script->filesToInstall().size() + m_script->patches().size());
    double percentage = ((1.0 * m_installed) / toInstall) * 100.0;
    m_observer->updateProgress(static_cast<int>(percentage));
  }
}

void UpdateInstaller::patchFile(const UpdateScriptPatch& patch)
{
  std::string oldFile = m_installDir + '/' + patch.path;

  if (!FileUtils::fileExists(oldFile.c_str()))
  {
    throw "Can't find file to patch: " + oldFile;
  }

  const std::string oldFileHash = FileUtils::sha1FromFile(oldFile.c_str());
  if (oldFileHash != patch.sourceHash)
  {
    throw "File sha1 mismatch, can't patch: " + oldFile;
  }

  if (!patch.package.empty())
  {
    std::string packageFile = m_packageDir + '/' + patch.package + ".zip";
    if (!FileUtils::fileExists(packageFile.c_str()))
    {
      throw "Package file not found: " + packageFile;
    }

    std::string patchPath = patch.patchPath;
    std::string patchDir = m_tempDir + "/__patches/" + patchPath;
    if (!FileUtils::fileExists(FileUtils::dirname(patchDir.c_str()).c_str()))
    {
      FileUtils::mkpath(FileUtils::dirname(patchDir.c_str()).c_str());
    }

    FileUtils::extractFromZip(packageFile.c_str(), patchPath.c_str(), patchDir.c_str());

    std::string newFilePath = oldFile + ".new";

    if (!FileUtils::patchFile(oldFile.c_str(), newFilePath.c_str(), patchDir.c_str()))
    {
      throw "bspatch() failed!";
    }

    std::string newFileHash = FileUtils::sha1FromFile(newFilePath.c_str());
    if (newFileHash != patch.targetHash)
    {
      throw "After patching the hash was all wrong!";
    }

    FileUtils::removeFile(oldFile.c_str());
    FileUtils::moveFile(newFilePath.c_str(), oldFile.c_str());
    FileUtils::chmod(oldFile.c_str(), patch.targetPerm);
  }
}

void UpdateInstaller::copyBundle()
{
  if (m_observer)
  {
    m_observer->updateMessage("Creating Backup...");
  }

  m_installDir = m_tempDir + '/' + FileUtils::fileName(m_targetDir.c_str());

  if (FileUtils::fileExists(m_installDir.c_str()))
  {
    LOG(Warn, "Backup directory " + m_installDir + " - removing it.");
    FileUtils::rmdirRecursive(m_installDir.c_str());
  }

  FileUtils::copyTree(m_targetDir, m_installDir);
}

void UpdateInstaller::patchFiles()
{
  std::vector<UpdateScriptPatch>::const_iterator iter = m_script->patches().begin();
  if (m_observer)
  {
    m_observer->updateMessage("Patching Files...");
  }

  std::string patchDir = m_tempDir + "/__patches/";
  if (!FileUtils::fileExists(patchDir.c_str()))
  {
    FileUtils::mkpath(patchDir.c_str());
  }

  for (; iter != m_script->patches().end(); iter++)
  {
    patchFile(*iter);
    ++m_installed;
    updateProgress();
    DID_CANCEL();
  }

  FileUtils::rmdirRecursive(patchDir.c_str());
}

void UpdateInstaller::installFiles()
{
  if (m_observer)
  {
    m_observer->updateMessage("Installing Files...");
  }
  std::vector<UpdateScriptFile>::const_iterator iter = m_script->filesToInstall().begin();
  for (; iter != m_script->filesToInstall().end(); iter++)
  {
    installFile(*iter);
    ++m_installed;
    updateProgress();

    DID_CANCEL();
  }
}

void UpdateInstaller::findFiles(const std::string& path, std::vector<std::string>& list)
{
  DirIterator iter(path.c_str());
  while (iter.next())
  {
    if (iter.isDir())
    {
      if (iter.fileName() == "." || iter.fileName() == "..")
      {
        continue;
      }
      findFiles(iter.filePath(), list);
    }
    else
    {
      if (!endsWith(iter.fileName(), ".bak"))
        list.push_back(iter.filePath());
    }
  }
}

void UpdateInstaller::verifyAgainstManifest()
{
  std::vector<std::string> files;
  findFiles(m_installDir, files);
  std::map<std::string, UpdateScriptFile> fileMap = m_script->filesManifest();

  if (m_observer)
  {
    m_observer->updateMessage("Verifying Installation...");
  }

  for (std::vector<std::string>::const_iterator it = files.begin(); it != files.end(); ++it)
  {
    std::string filePath = *it;
    filePath = filePath.substr(m_installDir.size() + 1);

    if (fileMap.find(filePath) == fileMap.end())
    {
      FileUtils::removeFile((m_installDir + '/' + filePath).c_str());
    }
    else
    {
      UpdateScriptFile scriptFile = fileMap[filePath];
      
      // verify links
      if (!scriptFile.linkTarget.empty())
      {
        if (!FileUtils::fileIsLink((m_installDir + '/' + filePath).c_str()))
        {
          LOG(Error, "Path: " + filePath + " is supposed to be a link!");
          throw "Expected link but got regular file: " + filePath;
        }
        
        if (scriptFile.linkTarget != FileUtils::getSymlinkTarget((m_installDir + '/' + filePath).c_str()))
        {
          LOG(Error, "Path " + filePath + " does not link to " + scriptFile.linkTarget);
          throw filePath + " is not linked to " + scriptFile.linkTarget;
        }
      }
      else if (!scriptFile.hash.empty())
      {
        std::string hash = FileUtils::sha1FromFile((m_installDir + '/' + filePath).c_str());
        if (hash != scriptFile.hash)
        {
          LOG(Error, "File " + filePath + " had the wrong hash! Expected: " + scriptFile.hash +
                     ", got: " + hash);
          throw "Wrong hash on file: " + filePath;
        }
      }
      
    }
  }
}

void UpdateInstaller::uninstallFiles()
{
  if (m_observer)
  {
    m_observer->updateMessage("Removing Files...");
  }

  std::vector<std::string>::const_iterator iter = m_script->filesToUninstall().begin();
  for (; iter != m_script->filesToUninstall().end(); iter++)
  {
    std::string path = m_installDir + '/' + iter->c_str();
    if (FileUtils::fileExists(path.c_str()))
    {
      FileUtils::removeFile(path.c_str());
    }
    else
    {
      LOG(Warn, "Unable to uninstall file " + path + " because it does not exist.");
    }
  }
}

bool UpdateInstaller::checkAccess()
{
  std::string testFile = m_targetDir + "/update-installer-test-file";

  try
  {
    FileUtils::removeFile(testFile.c_str());
  }
  catch (const FileUtils::IOException& error)
  {
    LOG(Info, "Removing existing access check file failed " + std::string(error.what()));
  }

  try
  {
    FileUtils::touch(testFile.c_str());
    FileUtils::removeFile(testFile.c_str());

    // we need to make sure that we can rename the current binary dir as well
    // since that directory might not have the same rights.
    //
    FileUtils::moveFile(m_targetDir.c_str(), (m_targetDir + std::string(".bak")).c_str());
    FileUtils::moveFile((m_targetDir + std::string(".bak")).c_str(), m_targetDir.c_str());
    return true;
  }
  catch (const FileUtils::IOException& error)
  {
    LOG(Info, "checkAccess() failed " + std::string(error.what()));
    return false;
  }
}

void UpdateInstaller::setObserver(UpdateObserver* observer)
{
  m_observer = observer;
}

void UpdateInstaller::restartMainApp()
{
  try
  {
    std::string command;
    std::list<std::string> args;

    args.push_back("--from-auto-update");

    for (std::map<std::string, UpdateScriptFile>::const_iterator iter =
         m_script->filesManifest().begin();
         iter != m_script->filesManifest().end(); iter++)
    {
      if (iter->second.isMainBinary)
      {
        command = m_targetDir + '/' + iter->second.path;
      }
    }

    if (!command.empty())
    {
      LOG(Info, "Starting main application " + command);
      ProcessUtils::runAsync(command, args);
    }
    else
    {
      LOG(Error, "No main binary specified in update script");
    }
  }
  catch (const std::exception& ex)
  {
    LOG(Error, "Unable to restart main app " + std::string(ex.what()));
  }
}

void UpdateInstaller::postInstallUpdate()
{
// perform post-install actions

#ifdef PLATFORM_MAC
  // touch the application's bundle directory so that
  // OS X' Launch Services notices any changes in the application's
  // Info.plist file.
  FileUtils::touch(m_targetDir.c_str());
#endif
}

void UpdateInstaller::setAutoClose(bool autoClose)
{
  m_autoClose = autoClose;
}
