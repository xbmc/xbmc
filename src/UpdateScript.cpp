#include "UpdateScript.h"

#include "Log.h"
#include "StringUtils.h"

#include "tinyxml/tinyxml.h"

std::string elementText(const TiXmlElement* element)
{
  if (!element)
  {
    return std::string();
  }
  return element->GetText();
}

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

    LOG(Info, "Loaded script from " + path);

    const TiXmlElement* updateNode = document.RootElement();
    parseUpdate(updateNode);
  }
  else
  {
    LOG(Error, "Unable to load script " + path);
  }
}

bool UpdateScript::isValid() const
{
  return !m_path.empty();
}

void UpdateScript::parseUpdate(const TiXmlElement* updateNode)
{
  bool isV2Compatible = strToBool(notNullString(updateNode->Attribute("v2-compatible")));

  const TiXmlElement* depsNode = updateNode->FirstChildElement("dependencies");
  const TiXmlElement* depFileNode = depsNode->FirstChildElement("file");
  while (depFileNode)
  {
    m_dependencies.push_back(std::string(depFileNode->GetText()));
    depFileNode = depFileNode->NextSiblingElement("file");
  }

  const char* installNodeName;
  if (isV2Compatible)
  {
    // this update script has been generated for backwards compatibility with
    // Mendeley Desktop 1.0 which downloads files specified in the <install>
    // section instead of the <packages> section.  The <install> section
    // in this case lists the packages and the real list of files to install
    // is in the <install-v3> section
    installNodeName = "install-v3";
  }
  else
  {
    installNodeName = "install";
  }

  const TiXmlElement* prefixNode = updateNode->FirstChildElement("pathprefix");
  if (prefixNode)
    m_pathPrefix = notNullString(prefixNode->GetText());

  const TiXmlElement* installNode = updateNode->FirstChildElement(installNodeName);
  if (installNode)
  {
    const TiXmlElement* installFileNode = installNode->FirstChildElement("file");
    while (installFileNode)
    {
      m_filesToInstall.push_back(parseFile(installFileNode));
      installFileNode = installFileNode->NextSiblingElement("file");
    }
  }

  const TiXmlElement* manifestNode = updateNode->FirstChildElement("manifest");
  if (manifestNode)
  {
    const TiXmlElement* manifestFileNode = manifestNode->FirstChildElement("file");
    while (manifestFileNode)
    {
      UpdateScriptFile file = parseFile(manifestFileNode);
      m_filesManifest[file.path] = file;
      manifestFileNode = manifestFileNode->NextSiblingElement("file");
    }
  }

  const TiXmlElement* patchesNode = updateNode->FirstChildElement("patches");
  if (patchesNode)
  {
    const TiXmlElement* patchesFileNode = patchesNode->FirstChildElement("patch");
    while (patchesFileNode)
    {
      m_patchesToInstall.push_back(parsePatch(patchesFileNode));
      patchesFileNode = patchesFileNode->NextSiblingElement("patch");
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

UpdateScriptPatch UpdateScript::parsePatch(const TiXmlElement *element)
{
  UpdateScriptPatch patch;
  patch.patchHash = elementText(element->FirstChildElement("patchHash"));
  patch.path = elementText(element->FirstChildElement("name"));
  patch.patchPath = elementText(element->FirstChildElement("patchName"));
  patch.targetHash = elementText(element->FirstChildElement("targetHash"));
  patch.targetSize = atoi(elementText(element->FirstChildElement("targetSize")).c_str());
  patch.sourceHash = elementText(element->FirstChildElement("sourceHash"));
  patch.package = elementText(element->FirstChildElement("package"));

  std::string modeString = elementText(element->FirstChildElement("targetPerm"));
  sscanf(modeString.c_str(), "%i", &patch.targetPerm);

  return patch;
}

UpdateScriptFile UpdateScript::parseFile(const TiXmlElement* element)
{
  UpdateScriptFile file;
  file.path = elementText(element->FirstChildElement("name"));
  file.hash = elementText(element->FirstChildElement("hash"));
  file.package = elementText(element->FirstChildElement("package"));

  std::string modeString = elementText(element->FirstChildElement("permissions"));
  sscanf(modeString.c_str(), "%i", &file.permissions);

  file.linkTarget = elementText(element->FirstChildElement("target"));
  file.isMainBinary = strToBool(elementText(element->FirstChildElement("is-main-binary")));

  if (!m_pathPrefix.empty())
    file.pathPrefix = m_pathPrefix;

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

const std::vector<UpdateScriptPatch> &UpdateScript::patches() const
{
  return m_patchesToInstall;
}

const std::map<std::string, UpdateScriptFile> &UpdateScript::filesManifest() const
{
  return m_filesManifest;
}

const std::string UpdateScript::path() const
{
  return m_path;
}
