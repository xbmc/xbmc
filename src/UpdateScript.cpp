#include "UpdateScript.h"

#include "Log.h"
#include "StringUtils.h"

#include "tinyxml/tinyxml.h"

UpdateScript::UpdateScript()
{
}

void UpdateScript::parse(const std::string& path)
{
	m_path.clear();

	TiXmlDocument document(path);
	if (document.LoadFile())
	{
		m_path = path;

		LOG(Info,"Loaded script from " + path);

		const TiXmlElement* updateNode = document.RootElement();
		const TiXmlElement* v3UpdateNode = updateNode->FirstChildElement("update-v3");
		if (v3UpdateNode)
		{
			// this XML file is structured for backwards compatibility
			// with Mendeley Desktop <= 1.0 clients.  The normal update XML contents
			// are wrapped in an <update-v3> node.
			parseUpdate(v3UpdateNode);
		}
		else
		{
			parseUpdate(updateNode);
		}
	}
	else
	{
		LOG(Error,"Unable to load script " + path);
	}
}

bool UpdateScript::isValid() const
{
	return !m_path.empty();
}

void UpdateScript::parseUpdate(const TiXmlElement* updateNode)
{
	const TiXmlElement* depsNode = updateNode->FirstChildElement("dependencies");
	const TiXmlElement* depFileNode = depsNode->FirstChildElement("file");
	while (depFileNode)
	{
		m_dependencies.push_back(std::string(depFileNode->GetText()));
		depFileNode = depFileNode->NextSiblingElement("file");
	}

	const TiXmlElement* installNode = updateNode->FirstChildElement("install");
	if (installNode)
	{
		const TiXmlElement* installFileNode = installNode->FirstChildElement("file");
		while (installFileNode)
		{
			m_filesToInstall.push_back(parseFile(installFileNode));
			installFileNode = installFileNode->NextSiblingElement("file");
		}
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
	file.isMainBinary = strToBool(elementText(element->FirstChildElement("is-main-binary")));
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

const std::string UpdateScript::path() const
{
	return m_path;
}

