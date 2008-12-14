
To test the SimpleHandler you need to add the following line to the 
configuration file of your appweb server. Change the extension to 
be the URL extension you wish to use when invoking the handler.

LoadModule simpleHandler pathToSimpleHandlerModule
AddHandler simpleHandler  .extension

