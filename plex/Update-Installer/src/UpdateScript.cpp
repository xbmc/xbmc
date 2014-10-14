#include "UpdateScript.h"

#include "Log.h"
#include "StringUtils.h"
#include "FileUtils.h"

#include "tinyxml2/tinyxml2.h"
#include "bzip2/bzlib.h"

using namespace tinyxml2;

std::string elementText(const XMLElement* element)
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

  tinyxml2::XMLDocument document;
  XMLError error = XML_ERROR_EMPTY_DOCUMENT;

  if (endsWith(path, ".bz2"))
  {
    std::string finalData = "";

    // Bziped script, let's start by unpacking it.
    BZFILE* bfp = BZ2_bzopen(path.c_str(), "r");
    if (bfp)
    {
      size_t allocated = 1024 * 1024; // start with one MB
      char* uncompressedData = (char*)malloc(allocated);
      int err = BZ_OK;

      while (err == BZ_OK)
      {
        int read = BZ2_bzRead(&err, bfp, uncompressedData, (int)allocated);
        if (read > 0)
          finalData.append(uncompressedData, (size_t)read);
      }

      free(uncompressedData);
    }

    if (finalData.size() > 0)
    {
      error = document.Parse(finalData.c_str());
    }
    else
    {
      LOG(Warn, "Failed to uncompress bz2 script");
    }
  }
  else
  {
    error = document.LoadFile(path.c_str());
  }

  if (error == XML_NO_ERROR)
  {
    m_path = path;

    LOG(Info, "Loaded script from " + path);

    const XMLElement* updateNode = document.RootElement();
    parseUpdate(updateNode);
  }
  else
  {
    LOG(Error, "Unable to load script " + path + ", error code: " + intToStr(error));
  }
}

bool UpdateScript::isValid() const
{
  return !m_path.empty();
}

void UpdateScript::parseUpdate(const XMLElement* updateNode)
{
  bool isV2Compatible = strToBool(notNullString(updateNode->Attribute("v2-compatible")));

  const XMLElement* depsNode = updateNode->FirstChildElement("dependencies");
  const XMLElement* depFileNode = depsNode->FirstChildElement("file");
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

  const XMLElement* prefixNode = updateNode->FirstChildElement("pathprefix");
  if (prefixNode)
    m_pathPrefix = notNullString(prefixNode->GetText());

  const XMLElement* installNode = updateNode->FirstChildElement(installNodeName);
  if (installNode)
  {
    const XMLElement* installFileNode = installNode->FirstChildElement("file");
    while (installFileNode)
    {
      m_filesToInstall.push_back(parseFile(installFileNode));
      installFileNode = installFileNode->NextSiblingElement("file");
    }
  }

  const XMLElement* manifestNode = updateNode->FirstChildElement("manifest");
  if (manifestNode)
  {
    const XMLElement* manifestFileNode = manifestNode->FirstChildElement("file");
    while (manifestFileNode)
    {
      UpdateScriptFile file = parseFile(manifestFileNode);
      m_filesManifest[file.path] = file;
      manifestFileNode = manifestFileNode->NextSiblingElement("file");
    }
  }

  const XMLElement* patchesNode = updateNode->FirstChildElement("patches");
  if (patchesNode)
  {
    const XMLElement* patchesFileNode = patchesNode->FirstChildElement("patch");
    while (patchesFileNode)
    {
      m_patchesToInstall.push_back(parsePatch(patchesFileNode));
      patchesFileNode = patchesFileNode->NextSiblingElement("patch");
    }
  }

  const XMLElement* uninstallNode = updateNode->FirstChildElement("uninstall");
  if (uninstallNode)
  {
    const XMLElement* uninstallFileNode = uninstallNode->FirstChildElement("file");
    while (uninstallFileNode)
    {
      m_filesToUninstall.push_back(uninstallFileNode->GetText());
      uninstallFileNode = uninstallFileNode->NextSiblingElement("file");
    }
  }

  const XMLElement* packagesNode = updateNode->FirstChildElement("packages");
  if (packagesNode)
  {
    const XMLElement* packageNode = packagesNode->FirstChildElement("package");
    while (packageNode)
    {
      m_packages.push_back(parsePackage(packageNode));
      packageNode = packageNode->NextSiblingElement("package");
    }
  }
}

UpdateScriptPatch UpdateScript::parsePatch(const XMLElement* element)
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

UpdateScriptFile UpdateScript::parseFile(const XMLElement* element)
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

UpdateScriptPackage UpdateScript::parsePackage(const XMLElement* element)
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

const std::vector<UpdateScriptPatch>& UpdateScript::patches() const
{
  return m_patchesToInstall;
}

const std::map<std::string, UpdateScriptFile>& UpdateScript::filesManifest() const
{
  return m_filesManifest;
}

const std::string UpdateScript::path() const
{
  return m_path;
}
