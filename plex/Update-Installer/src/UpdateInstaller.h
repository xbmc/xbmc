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
		void setPackageDir(const std::string& path);
		void setBackupDir(const std::string& path);
		void setMode(Mode mode);
		void setScript(UpdateScript* script);
		void setWaitPid(PLATFORM_PID pid);
		void setForceElevated(bool elevated);
		void setAutoClose(bool autoClose);

		void setObserver(UpdateObserver* observer);

		void run() throw ();

		void restartMainApp();

	private:
		void cleanup();
		void revert();
		void removeBackups();
		bool checkAccess();

		void installFiles();
		void uninstallFiles();
		void installFile(const UpdateScriptFile& file);
		void backupFile(const std::string& path);
		void reportError(const std::string& error);
		void postInstallUpdate();

		std::list<std::string> updaterArgs() const;
		std::string friendlyErrorForError(const FileUtils::IOException& ex) const;

		Mode m_mode;
		std::string m_installDir;
		std::string m_packageDir;
		std::string m_backupDir;
		PLATFORM_PID m_waitPid;
		UpdateScript* m_script;
		UpdateObserver* m_observer;
		std::map<std::string,std::string> m_backups;
		bool m_forceElevated;
		bool m_autoClose;
};

