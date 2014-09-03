#pragma once

#include <string>
#include <vector>
#include <map>

namespace tinyxml2
{
  class XMLElement;
}

/** Represents a package containing one or more
  * files for an update.
  */
class UpdateScriptPackage
{
public:
  UpdateScriptPackage() : size(0)
  {
  }

  std::string name;
  std::string sha1;
  std::string source;
  int size;

  bool operator==(const UpdateScriptPackage& other) const
  {
    return name == other.name && sha1 == other.sha1 && source == other.source && size == other.size;
  }
};

class UpdateScriptPatch
{
public:
  UpdateScriptPatch() : targetPerm(0)
  {
  }

  std::string path;
  std::string patchPath;
  std::string patchHash;
  std::string targetHash;
  int targetSize;
  int targetPerm;
  std::string sourceHash;
  std::string package;
};

/** Represents a file to be installed as part of an update. */
class UpdateScriptFile
{
public:
  UpdateScriptFile() : permissions(0), isMainBinary(false)
  {
  }

  std::string path;
  std::string package;
  std::string linkTarget;
  std::string pathPrefix;
  std::string hash;

  /** The permissions for this file, specified
      * using the standard Unix mode_t values.
      */
  int permissions;

  bool isMainBinary;

  bool operator==(const UpdateScriptFile& other) const
  {
    return hash == other.hash && path == other.path && package == other.package &&
           permissions == other.permissions && linkTarget == other.linkTarget &&
           isMainBinary == other.isMainBinary;
  }
};

/** Stores information about the packages and files included
  * in an update, parsed from an XML file.
  */
class UpdateScript
{
public:
  UpdateScript();

  /** Initialize this UpdateScript with the script stored
      * in the XML file at @p path.
      */
  void parse(const std::string& path);

  bool isValid() const;
  const std::string path() const;
  const std::vector<std::string>& dependencies() const;
  const std::vector<UpdateScriptPackage>& packages() const;
  const std::vector<UpdateScriptFile>& filesToInstall() const;
  const std::vector<std::string>& filesToUninstall() const;
  const std::vector<UpdateScriptPatch>& patches() const;
  const std::map<std::string, UpdateScriptFile>& filesManifest() const;

private:
  void parseUpdate(const tinyxml2::XMLElement* element);
  UpdateScriptPatch parsePatch(const tinyxml2::XMLElement* element);
  UpdateScriptFile parseFile(const tinyxml2::XMLElement* element);
  UpdateScriptPackage parsePackage(const tinyxml2::XMLElement* element);

  std::string m_path;
  std::vector<std::string> m_dependencies;
  std::vector<UpdateScriptPackage> m_packages;
  std::vector<UpdateScriptFile> m_filesToInstall;
  std::vector<UpdateScriptPatch> m_patchesToInstall;
  std::map<std::string, UpdateScriptFile> m_filesManifest;
  std::vector<std::string> m_filesToUninstall;

  std::string m_pathPrefix;
};
