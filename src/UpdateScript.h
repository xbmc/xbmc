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

		bool operator==(const UpdateScriptPackage& other) const
		{
			return name == other.name &&
			       sha1 == other.sha1 &&
			       source == other.source &&
			       size == other.size;
		}
};

class UpdateScriptFile
{
	public:
		UpdateScriptFile()
		: permissions(0)
		, isMainBinary(false)
		{}

		std::string path;
		std::string package;
		int permissions;
		std::string linkTarget;
		bool isMainBinary;

		bool operator==(const UpdateScriptFile& other) const
		{
			return path == other.path &&
			       package == other.package &&
			       permissions == other.permissions &&
			       linkTarget == other.linkTarget &&
			       isMainBinary == other.isMainBinary;
		}
};

class UpdateScript
{
	public:
		UpdateScript();

		void parse(const std::string& path);

		bool isValid() const;
		const std::string path() const;
		const std::vector<std::string>& dependencies() const;
		const std::vector<UpdateScriptPackage>& packages() const;
		const std::vector<UpdateScriptFile>& filesToInstall() const;
		const std::vector<std::string>& filesToUninstall() const;

	private:
		void parseUpdate(const TiXmlElement* element);
		UpdateScriptFile parseFile(const TiXmlElement* element);
		UpdateScriptPackage parsePackage(const TiXmlElement* element);

		std::string m_path;
		std::vector<std::string> m_dependencies;
		std::vector<UpdateScriptPackage> m_packages;
		std::vector<UpdateScriptFile> m_filesToInstall;
		std::vector<std::string> m_filesToUninstall;
};

