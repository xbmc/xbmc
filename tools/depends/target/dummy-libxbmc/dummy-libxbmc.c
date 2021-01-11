void xbmc_dummy_function()
{
  return;
}

/*
** This function is implemented purely for darwinembedded platforms that use
** the -fembedbitcode link/flag option. We cant have undefined functions when using this
** flag. Pythonmodules need to be linked against a main function, but obviously we dont
** have a kodi main lib to link against just yet. As the pythonmodules arent statically
** linked, ios/tvos is ok with dynamically loading at runtime with the actual main() from
** libkodi
*/
int main()
{
  return 0;
}
