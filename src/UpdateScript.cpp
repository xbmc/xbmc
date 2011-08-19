#include "UpdateScript.h"

#include "Log.h"

#include "tinyxml/tinyxml.h"

std::string intToStr(int i)
{
	std::stringstream stream;
	stream << i;
	return stream.str();
}

UpdateScript::UpdateScript()
{
}

void UpdateScript::parse(const std::string& path)
{
	TiXmlDocument document(path);
	if (document.LoadFile())
	{
		LOG(Info,"Loaded script from " + path);

		const TiXmlElement* updateNode = document.RootElement();

		const TiXmlElement* depsNode = updateNode->FirstChildElement("dependencies");
		const TiXmlElement* depFileNode = depsNode->FirstChildElement("file");
		while (depFileNode)
		{
			m_dependencies.push_back(std::string(depFileNode->GetText()));
			depFileNode = depFileNode->NextSiblingElement("file");
		}
		LOG(Info,"Dependency count " + intToStr(m_dependencies.size()));

		const TiXmlElement* installNode = updateNode->FirstChildElement("install");
		if (installNode)
		{
			const TiXmlElement* installFileNode = installNode->FirstChildElement("file");
			while (installFileNode)
			{
				m_filesToInstall.push_back(parseFile(installFileNode));
				installFileNode = installFileNode->NextSiblingElement("file");
			}
			LOG(Info,"Files to install count " + intToStr(m_filesToInstall.size()));
		}

		const TiXmlElement* uninstallNode = updateNode->FirstChildElement("uninstall");
		if (uninstallNode)
		{
			const TiXmlElement* uninstallFileNode = uninstallNode->FirstChildElement("file");
			while (uninstallFileNode)
			{
				m_filesToUninstall.push_back(uninstallFileNode->GetText());
				uninstallFileNode = uninstallFileNode->NextSiblingElement("file");
			}
			LOG(Info,"Files to uninstall count " + intToStr(m_filesToUninstall.size()));
		}

		const TiXmlElement* packagesNode = updateNode->FirstChildElement("packages");
		if (packagesNode)
		{
			const TiXmlElement* packageNode = packagesNode->FirstChildElement("package");
			while (packageNode)
			{
				m_packages.push_back(parsePackage(packageNode));
				packageNode = packageNode->NextSiblingElement("package");
			}
			LOG(Info,"Package count " + intToStr(m_packages.size()));
		}
	}
	else
	{
		LOG(Error,"Unable to load script " + path);
	}
}

std::string elementText(const TiXmlElement* element)
{
	if (!element)
	{
		return std::string();
	}
	return element->GetText();
}

UpdateScriptFile UpdateScript::parseFile(const TiXmlElement* element)
{
	UpdateScriptFile file;
	file.path = elementText(element->FirstChildElement("name"));
	file.package = elementText(element->FirstChildElement("package"));
	file.permissions = atoi(elementText(element->FirstChildElement("permissions")).c_str());
	file.linkTarget = elementText(element->FirstChildElement("target"));
	return file;
}

UpdateScriptPackage UpdateScript::parsePackage(const TiXmlElement* element)
{
	UpdateScriptPackage package;
	package.name = elementText(element->FirstChildElement("name"));
	package.sha1 = elementText(element->FirstChildElement("hash"));
	package.source = elementText(element->FirstChildElement("source"));
	package.size = atoi(elementText(element->FirstChildElement("size")).c_str());
	return package;
}

const std::vector<std::string>& UpdateScript::dependencies() const
{
	return m_dependencies;
}

const std::vector<UpdateScriptPackage>& UpdateScript::packages() const
{
	return m_packages;
}

const std::vector<UpdateScriptFile>& UpdateScript::filesToInstall() const
{
	return m_filesToInstall;
}

const std::vector<std::string>& UpdateScript::filesToUninstall() const
{
	return m_filesToUninstall;
}


