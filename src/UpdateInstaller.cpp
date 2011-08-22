#include "UpdateInstaller.h"

#include "FileOps.h"
#include "Log.h"
#include "ProcessUtils.h"
#include "UpdateObserver.h"

UpdateInstaller::UpdateInstaller()
: m_mode(Setup)
, m_script(0)
, m_waitPid(0)
, m_observer(0)
{
}

void UpdateInstaller::setWaitPid(long long pid)
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

void UpdateInstaller::run() throw ()
{
	std::string updaterPath;
	try
	{
		updaterPath = ProcessUtils::currentProcessPath();
	}
	catch (const FileOps::IOException& ex)
	{
		LOG(Error,"error reading process path with mode " + intToStr(m_mode));
		return;
	}

	if (m_mode == Setup)
	{
		LOG(Info,"Preparing update installation");
		LOG(Info,"Waiting for main app process to finish");
		ProcessUtils::waitForProcess(m_waitPid);

		std::list<std::string> args = updaterArgs();
		args.push_back("--mode");
		args.push_back("main");
		args.push_back("--wait");
		args.push_back(intToStr(ProcessUtils::currentProcessId()));

		if (!checkAccess())
		{
			LOG(Info,"Insufficient rights to install app to " + m_installDir + " requesting elevation");

			// start a copy of the updater with admin rights
			ProcessUtils::runElevated(updaterPath,args);
		}
		else
		{
			LOG(Info,"Sufficient rights to install app - restarting with same permissions");

			// TODO - Change this to run synchronously
			ProcessUtils::runAsync(updaterPath,args);
		}
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
		}
		catch (const FileOps::IOException& exception)
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

		std::list<std::string> args = updaterArgs();
		args.push_back("--mode");
		args.push_back("cleanup");
		args.push_back("--wait");
		args.push_back(intToStr(ProcessUtils::currentProcessId()));
		ProcessUtils::runAsync(updaterPath,args);
	}
	else if (m_mode == Cleanup)
	{
		LOG(Info,"Cleaning up temporary updater files");

		ProcessUtils::waitForProcess(m_waitPid);

		try
		{
			cleanup();
		}
		catch (const FileOps::IOException& ex)
		{
			LOG(Error,"Error cleaning up updater " + std::string(ex.what()));
		}
		LOG(Info,"Updater files removed");
	}
}

void UpdateInstaller::cleanup()
{
	FileOps::rmdirRecursive(m_packageDir.c_str());
}

void UpdateInstaller::revert()
{
	std::map<std::string,std::string>::const_iterator iter = m_backups.begin();
	for (;iter != m_backups.end();iter++)
	{
		const std::string& installedFile = iter->first;
		const std::string& backupFile = iter->second;

		if (FileOps::fileExists(installedFile.c_str()))
		{
			FileOps::removeFile(installedFile.c_str());
		}
		FileOps::moveFile(backupFile.c_str(),installedFile.c_str());
	}
}

void UpdateInstaller::installFile(const UpdateScriptFile& file)
{
	std::string destPath = m_installDir + '/' + file.path;
	std::string target = file.linkTarget;

	// backup the existing file if any
	backupFile(destPath);

	// create the target directory if it does not exist
	std::string destDir = FileOps::dirname(destPath.c_str());
	if (!FileOps::fileExists(destDir.c_str()))
	{
		FileOps::mkdir(destDir.c_str());
	}

	if (target.empty())
	{
		// locate the package containing the file
		std::string packageFile = m_packageDir + '/' + file.package + ".zip";
		if (!FileOps::fileExists(packageFile.c_str()))
		{
			throw "Package file does not exist: " + packageFile;
		}

		// extract the file from the package and copy it to
		// the destination
		FileOps::extractFromZip(packageFile.c_str(),file.path.c_str(),destPath.c_str());

		// set the permissions on the newly extracted file
		FileOps::setQtPermissions(destPath.c_str(),file.permissions);
	}
	else
	{
		// create the symlink
		FileOps::createSymLink(destPath.c_str(),target.c_str());
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
			m_observer->updateProgress(static_cast<float>(filesInstalled) / m_script->filesToInstall().size() * 100);
		}
	}
}

void UpdateInstaller::uninstallFiles()
{
	std::vector<std::string>::const_iterator iter = m_script->filesToUninstall().begin();
	for (;iter != m_script->filesToUninstall().end();iter++)
	{
		std::string path = m_installDir + '/' + iter->c_str();
		if (FileOps::fileExists(path.c_str()))
		{
			FileOps::removeFile(path.c_str());
		}
		else
		{
			LOG(Warn,"Unable to uninstall file " + path + " because it does not exist.");
		}
	}
}

void UpdateInstaller::backupFile(const std::string& path)
{
	if (!FileOps::fileExists(path.c_str()))
	{
		// no existing file to backup
		return;
	}
	
	std::string backupPath = path + ".bak";
	FileOps::removeFile(backupPath.c_str());
	FileOps::moveFile(path.c_str(), backupPath.c_str());
	m_backups[path] = backupPath;
}

void UpdateInstaller::removeBackups()
{
	std::map<std::string,std::string>::const_iterator iter = m_backups.begin();
	for (;iter != m_backups.end();iter++)
	{
		const std::string& installedFile = iter->first;
		const std::string& backupFile = iter->second;
		FileOps::removeFile(backupFile.c_str());
	}
}

bool UpdateInstaller::checkAccess()
{
	std::string testFile = m_installDir + "/update-installer-test-file";

	try
	{
		FileOps::removeFile(testFile.c_str());
	}
	catch (const FileOps::IOException& error)
	{
		LOG(Info,"Removing existing access check file failed " + std::string(error.what()));
	}

	try
	{
		FileOps::touch(testFile.c_str());
		FileOps::removeFile(testFile.c_str());
		return true;
	}
	catch (const FileOps::IOException& error)
	{
		LOG(Info,"checkAccess() failed " + std::string(error.what()));
		return false;
	}
}

