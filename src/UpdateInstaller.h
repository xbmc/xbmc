#pragma once

#include <string>

class UpdateScript;

class UpdateInstaller
{
	public:
		enum Mode
		{
			Setup,
			Main,
			Cleanup
		};

		UpdateInstaller();
		void setInstallDir(const std::string& path);
		void setPackageDir(const std::string& path);
		void setBackupDir(const std::string& path);
		void setMode(Mode mode);
		void setScript(UpdateScript* script);

		void run();

	private:
		Mode m_mode;
		std::string m_installDir;
		std::string m_packageDir;
		std::string m_backupDir;
		UpdateScript* m_script;
};

