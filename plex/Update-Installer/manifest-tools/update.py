import dexml
from dexml import fields

class PatchElement(dexml.Model):
  class meta:
    tagname = "patch"
    order_sensitive = False

  name = fields.String(tagname="name")
  patchName = fields.String(tagname="patchName")
  patchHash = fields.String(tagname="patchHash")

  targetHash = fields.String(tagname="targetHash")
  targetSize = fields.String(tagname="targetSize")
  targetPerm = fields.String(tagname="targetPerm")

  sourceHash = fields.String(tagname="sourceHash")
  
  package = fields.String(tagname="package")

class FileElement(dexml.Model):
  class meta:
    tagname = "file"
    order_sensitive = False

  name = fields.String(tagname="name")
  package = fields.String(tagname="package")
  included = fields.String(tagname="included")
  fileHash = fields.String(tagname="hash", required=False)
  size = fields.String(tagname="size", required=False)
  permissions = fields.String(tagname="permissions", required=False)
  is_main_binary = fields.String(tagname="is-main-binary", required=False)
  targetLink = fields.String(tagname="target", required=False)

  def __eq__(self, other):
    return ((self.name == other.name) and 
            (self.fileHash == other.fileHash) and
            (self.size == other.size) and 
            (self.permissions == other.permissions))

  def __str__(self):
    return "%s [hash: %s, size: %s, permissions: %s]" % (self.name, self.fileHash, self.size, self.permissions)

class PackageElement(dexml.Model):
  class meta:
    tagname = "package"
    order_sensitive = False

  name = fields.String(tagname="name")
  fileHash = fields.String(tagname="hash")
  size = fields.String(tagname="size")

class Update(dexml.Model):
  class meta:
    tagname = "update"
    order_sensitive = False

  version = fields.Integer()
  targetVersion = fields.String(tagname="targetVersion")
  platform = fields.String(tagname="platform")
  dependencies = fields.List(fields.String(tagname="file"), tagname="dependencies")
  pathprefix = fields.String(tagname="pathprefix", required=False)

  install = fields.List(FileElement, tagname="install", required=False)
  patches = fields.List(PatchElement, tagname="patches", required=False)
  manifest = fields.List(FileElement, tagname="manifest")
  packages = fields.List(PackageElement, tagname="packages")

  def get_filemap(self):
    return {a.name: a for a in self.manifest}
