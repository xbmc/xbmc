Get-ChildItem -Recurse -Include *.pdb -Exclude vc140.pdb,'kodi-test.pdb',gtest.pdb,CMakeCCompilerId.pdb,CMakeCXXCompilerId.pdb,example.pdb ..\.. |
foreach {
  if (-not $_.DirectoryName.EndsWith('PDB')) {
    Copy-Item $_.FullName (join-path PDB $_.Name)
  }
}