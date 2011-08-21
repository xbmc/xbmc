#pragma once

#include <string>
#include <vector>

class TiXmlElement;

class UpdateScriptPackage
{
	public:
		UpdateScriptPackage()
		: size(0)
		{}

		std::string name;
		std::string sha1;
		std::string source;
		int size;
};

class UpdateScriptFile
{
	public:
		UpdateScriptFile()
		: permissions(0)
		{}

		std::string path;
		std::string package;
		int permissions;
		std::string linkTarget;
};

class UpdateScript
{
	public:
		UpdateScript();

		void parse(const std::string& path);

		const std::string path() const;
		const std::vector<std::string>& dependencies() const;
		const std::vector<UpdateScriptPackage>& packages() const;
		const std::vector<UpdateScriptFile>& filesToInstall() const;
		const std::vector<std::string>& filesToUninstall() const;

	private:
		UpdateScriptFile parseFile(const TiXmlElement* element);
		UpdateScriptPackage parsePackage(const TiXmlElement* element);

		std::string m_path;
		std::vector<std::string> m_dependencies;
		std::vector<UpdateScriptPackage> m_packages;
		std::vector<UpdateScriptFile> m_filesToInstall;
		std::vector<std::string> m_filesToUninstall;
};

