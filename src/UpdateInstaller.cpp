#include "UpdateInstaller.h"

#include "FileUtils.h"
#include "Log.h"
#include "ProcessUtils.h"
#include "UpdateObserver.h"

UpdateInstaller::UpdateInstaller()
: m_mode(Setup)
, m_waitPid(0)
, m_script(0)
, m_observer(0)
{
}

void UpdateInstaller::setWaitPid(PLATFORM_PID pid)
{
	m_waitPid = pid;
}

void UpdateInstaller::setInstallDir(const std::string& path)
{
	m_installDir = path;
}

void UpdateInstaller::setPackageDir(const std::string& path)
{
	m_packageDir = path;
}

void UpdateInstaller::setBackupDir(const std::string& path)
{
	m_backupDir = path;
}

void UpdateInstaller::setMode(Mode mode)
{
	m_mode = mode;
}

void UpdateInstaller::setScript(UpdateScript* script)
{
	m_script = script;
}

std::list<std::string> UpdateInstaller::updaterArgs() const
{
	std::list<std::string> args;
	args.push_back("--install-dir");
	args.push_back(m_installDir);
	args.push_back("--package-dir");
	args.push_back(m_packageDir);
	args.push_back("--script");
	args.push_back(m_script->path());
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

void UpdateInstaller::run() throw ()
{
	if (!m_script || !m_script->isValid())
	{
		reportError("Unable to read update script");
		return;
	}
	if (m_installDir.empty())
	{
		reportError("No installation directory specified");
		return;
	}

	std::string updaterPath;
	try
	{
		updaterPath = ProcessUtils::currentProcessPath();
	}
	catch (const FileUtils::IOException& ex)
	{
		LOG(Error,"error reading process path with mode " + intToStr(m_mode));
		reportError("Unable to determine path of updater");
		return;
	}

	if (m_mode == Setup)
	{
		if (m_waitPid != 0)
		{
			LOG(Info,"Waiting for main app process to finish");
			ProcessUtils::waitForProcess(m_waitPid);
		}

		std::list<std::string> args = updaterArgs();
		args.push_back("--mode");
		args.push_back("main");
		args.push_back("--wait");
		args.push_back(intToStr(ProcessUtils::currentProcessId()));

		int installStatus = 0;
		if (!checkAccess())
		{
			LOG(Info,"Insufficient rights to install app to " + m_installDir + " requesting elevation");

			// start a copy of the updater with admin rights
			installStatus = ProcessUtils::runElevated(updaterPath,args);
		}
		else
		{
			LOG(Info,"Sufficient rights to install app - restarting with same permissions");
			installStatus = ProcessUtils::runSync(updaterPath,args);
		}

		if (installStatus == 0)
		{
			LOG(Info,"Update install completed");
		}
		else
		{
			LOG(Error,"Update install failed with status " + intToStr(installStatus));
		}

		// restart the main application - this is currently done
		// regardless of whether the installation succeeds or not
		restartMainApp();

		// clean up files created by the updater
		cleanup();
	}
	else if (m_mode == Main)
	{
		LOG(Info,"Starting update installation");

		std::string error;
		try
		{
			LOG(Info,"Installing new and updated files");
			installFiles();

			LOG(Info,"Uninstalling removed files");
			uninstallFiles();

			LOG(Info,"Removing backups");
			removeBackups();

			postInstallUpdate();
		}
		catch (const FileUtils::IOException& exception)
		{
			error = exception.what();
		}
		catch (const std::string& genericError)
		{
			error = genericError;
		}

		if (!error.empty())
		{
			LOG(Error,std::string("Error installing update ") + error);
			revert();
			if (m_observer)
			{
				m_observer->updateError(error);
			}
		}

		if (m_observer)
		{
			m_observer->updateFinished();
		}
	}
}

void UpdateInstaller::cleanup()
{
	try
	{
		FileUtils::rmdirRecursive(m_packageDir.c_str());
	}
	catch (const FileUtils::IOException& ex)
	{
		LOG(Error,"Error cleaning up updater " + std::string(ex.what()));
	}
	LOG(Info,"Updater files removed");
}

void UpdateInstaller::revert()
{
	std::map<std::string,std::string>::const_iterator iter = m_backups.begin();
	for (;iter != m_backups.end();iter++)
	{
		const std::string& installedFile = iter->first;
		const std::string& backupFile = iter->second;

		if (FileUtils::fileExists(installedFile.c_str()))
		{
			FileUtils::removeFile(installedFile.c_str());
		}
		FileUtils::moveFile(backupFile.c_str(),installedFile.c_str());
	}
}

void UpdateInstaller::installFile(const UpdateScriptFile& file)
{
	std::string destPath = m_installDir + '/' + file.path;
	std::string target = file.linkTarget;

	// backup the existing file if any
	backupFile(destPath);

	// create the target directory if it does not exist
	std::string destDir = FileUtils::dirname(destPath.c_str());
	if (!FileUtils::fileExists(destDir.c_str()))
	{
		FileUtils::mkdir(destDir.c_str());
	}

	if (target.empty())
	{
		// locate the package containing the file
		std::string packageFile = m_packageDir + '/' + file.package + ".zip";
		if (!FileUtils::fileExists(packageFile.c_str()))
		{
			throw "Package file does not exist: " + packageFile;
		}

		// extract the file from the package and copy it to
		// the destination
		FileUtils::extractFromZip(packageFile.c_str(),file.path.c_str(),destPath.c_str());

		// set the permissions on the newly extracted file
		FileUtils::setQtPermissions(destPath.c_str(),file.permissions);
	}
	else
	{
		// create the symlink
		FileUtils::createSymLink(destPath.c_str(),target.c_str());
	}
}

void UpdateInstaller::installFiles()
{
	std::vector<UpdateScriptFile>::const_iterator iter = m_script->filesToInstall().begin();
	int filesInstalled = 0;
	for (;iter != m_script->filesToInstall().end();iter++)
	{
		installFile(*iter);
		++filesInstalled;
		if (m_observer)
		{
			double percentage = (1.0 * filesInstalled / m_script->filesToInstall().size()) * 100.0;
			m_observer->updateProgress(static_cast<int>(percentage));
		}
	}
}

void UpdateInstaller::uninstallFiles()
{
	std::vector<std::string>::const_iterator iter = m_script->filesToUninstall().begin();
	for (;iter != m_script->filesToUninstall().end();iter++)
	{
		std::string path = m_installDir + '/' + iter->c_str();
		if (FileUtils::fileExists(path.c_str()))
		{
			FileUtils::removeFile(path.c_str());
		}
		else
		{
			LOG(Warn,"Unable to uninstall file " + path + " because it does not exist.");
		}
	}
}

void UpdateInstaller::backupFile(const std::string& path)
{
	if (!FileUtils::fileExists(path.c_str()))
	{
		// no existing file to backup
		return;
	}
	
	std::string backupPath = path + ".bak";
	FileUtils::removeFile(backupPath.c_str());
	FileUtils::moveFile(path.c_str(), backupPath.c_str());
	m_backups[path] = backupPath;
}

void UpdateInstaller::removeBackups()
{
	std::map<std::string,std::string>::const_iterator iter = m_backups.begin();
	for (;iter != m_backups.end();iter++)
	{
		const std::string& backupFile = iter->second;
		FileUtils::removeFile(backupFile.c_str());
	}
}

bool UpdateInstaller::checkAccess()
{
	std::string testFile = m_installDir + "/update-installer-test-file";

	try
	{
		FileUtils::removeFile(testFile.c_str());
	}
	catch (const FileUtils::IOException& error)
	{
		LOG(Info,"Removing existing access check file failed " + std::string(error.what()));
	}

	try
	{
		FileUtils::touch(testFile.c_str());
		FileUtils::removeFile(testFile.c_str());
		return true;
	}
	catch (const FileUtils::IOException& error)
	{
		LOG(Info,"checkAccess() failed " + std::string(error.what()));
		return false;
	}
}

void UpdateInstaller::setObserver(UpdateObserver* observer)
{
	m_observer = observer;
}

void UpdateInstaller::restartMainApp()
{
	std::string command;
	std::list<std::string> args;

	for (std::vector<UpdateScriptFile>::const_iterator iter = m_script->filesToInstall().begin();
	     iter != m_script->filesToInstall().end();
	     iter++)
	{
		if (iter->isMainBinary)
		{
			command = m_installDir + '/' + iter->path;
		}
	}

	if (!command.empty())
	{
		LOG(Info,"Starting main application " + command);
		ProcessUtils::runAsync(command,args);
	}
	else
	{
		LOG(Error,"No main binary specified in update script");
	}
}

void UpdateInstaller::postInstallUpdate()
{
	// perform post-install actions

#ifdef PLATFORM_MAC
	// touch the application's bundle directory so that
	// OS X' Launch Services notices any changes in the application's
	// Info.plist file.
	FileUtils::touch(m_installDir.c_str());
#endif
}

