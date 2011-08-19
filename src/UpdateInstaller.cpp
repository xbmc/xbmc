#include "UpdateInstaller.h"

#include "Log.h"

UpdateInstaller::UpdateInstaller()
: m_mode(Setup)
, m_script(0)
{
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

void UpdateInstaller::run()
{
	LOG(Info,"Starting update installation");

	if (m_mode == Setup)
	{
	}
	else if (m_mode == Main)
	{
	}
	else if (m_mode == Cleanup)
	{
	}
}

