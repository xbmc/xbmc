cd /xbmc
files=$(find system -iname *.dll)
tar -czf /xbmc/upload/windows-i386-xbmc-deps.tar.gz $files project/BuildDependencies/lib project/BuildDependencies/include