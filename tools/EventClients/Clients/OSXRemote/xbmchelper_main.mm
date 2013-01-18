#include "Carbon/Carbon.h"
#import "XBMCHelper.h"
#include <getopt.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iterator>

/* PLEX */
#include <boost/tokenizer.hpp>
/* END PLEX */

using namespace std;

//instantiate XBMCHelper which registers itself to IR handling stuff
XBMCHelper* gp_xbmchelper;
eRemoteMode g_mode = DEFAULT_MODE;
std::string g_server_address="localhost";
#ifndef __PLEX__
int         g_server_port = 9777;
#else
int         g_server_port = 9778;
#endif
std::string g_app_path = "";
std::string g_app_home = "";
double g_universal_timeout = 0.500;
bool g_verbose_mode = false;

//
#ifndef __PLEX__
const char* PROGNAME="XBMCHelper";
#else
const char* PROGNAME="PlexHTHelper";
#endif
const char* PROGVERS="0.7";

void ParseOptions(int argc, char** argv);
void ReadConfig();

static struct option long_options[] = {
{ "help",       no_argument,       0, 'h' },
{ "server",     required_argument, 0, 's' },
{ "port",       required_argument, 0, 'p' },
{ "universal",  no_argument,       0, 'u' },
{ "multiremote",no_argument,       0, 'm' },
{ "timeout",    required_argument, 0, 't' },
{ "verbose",    no_argument,       0, 'v' },
{ "externalConfig", no_argument,   0, 'x' },
{ "appPath",    required_argument, 0, 'a' },
{ "appHome",    required_argument, 0, 'z' }, 
{ 0, 0, 0, 0 },
};

static const char *options = "hs:umt:vxa:z:";

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void usage(void)
{
  printf("%s (version %s)\n", PROGNAME, PROGVERS);
  printf("   Sends Apple Remote events to XBMC.\n\n");
  printf("Usage: %s [OPTIONS...]\n\nOptions:\n", PROGNAME);
  printf("  -h, --help           print this help message and exit.\n");
  printf("  -s, --server <addr>  send events to the specified IP.\n");
  printf("  -p, --port <port>    send events to the specified port.\n");
  printf("  -u, --universal      runs in Universal Remote mode.\n");
  printf("  -t, --timeout <ms>   timeout length for sequences (default: 500ms).\n");
  printf("  -m, --multiremote    runs in Multi-Remote mode (adds remote identifier as additional idenfier to buttons)\n");
  printf("  -a, --appPath        path to XBMC.app (MenuPress launch support).\n");
  printf("  -z, --appHome        path to XBMC.app/Content/Resources/XBMX \n");
  printf("  -v, --verbose        prints lots of debugging information.\n");
}

//----------------------------------------------------------------------------
void ReadConfig()
{
	// Compute filename.
  std::string strFile = getenv("HOME");
#ifndef __PLEX__
  strFile += "/Library/Application Support/XBMC/XBMCHelper.conf";
#else
  strFile += "/Library/Application Support/"+ std::string(PLEX_TARGET_NAME) +"/PlexHTHelper.conf";
#endif
  
	// Open file.
  std::ifstream ifs(strFile.c_str());
	if (!ifs)
		return;
  
	// Read file.
	stringstream oss;
	oss << ifs.rdbuf();
  
	if (!ifs && !ifs.eof())
		return;
  
	// Tokenize.
	string strData(oss.str());
#ifndef __PLEX__
  istringstream is(strData);
  vector<string> args = vector<string>(istream_iterator<string>(is), istream_iterator<string>());
#else
  string separator1("");//dont let quoted arguments escape themselves
  string separator2(" ");//split on spaces
  string separator3("\"\'");//let it have quoted arguments

  boost::escaped_list_separator<char> els(separator1,separator2,separator3);
  boost::tokenizer< boost::escaped_list_separator<char> > tok(strData, els);
  vector<string> args;

  for(boost::tokenizer< boost::escaped_list_separator<char> >::iterator beg=tok.begin(); beg!=tok.end();++beg)
  {
    args.push_back(*beg);
  }
#endif
  
	// Convert to char**.
	int argc = args.size() + 1;
	char** argv = new char*[argc + 1];
	int i = 0;
#ifndef __PLEX__
	argv[i++] = (char*)"XBMCHelper";
#else
  argv[i++] = (char*)"PlexHTHelper";
#endif

	for (vector<string>::iterator it = args.begin(); it != args.end(); ){
#ifndef __PLEX__
    //fixup the arguments, here: remove '"' like bash would normally do
    std::string::size_type j = 0;
    while ((j = it->find("\"", j)) != std::string::npos )
      it->replace(j, 1, "");
#endif
    argv[i++] = (char* )(*it++).c_str();
  }

	argv[i] = 0;
  
	// Parse the arguments.
	ParseOptions(argc, argv);
  
	delete[] argv;
}

//----------------------------------------------------------------------------
void ParseOptions(int argc, char** argv)
{
  int c, option_index = 0;
  //set the defaults
	bool readExternal = false;
  g_server_address = "localhost";
#ifndef __PLEX__
  g_server_port = 9777;
#else
  g_server_port = 9778;
#endif
  g_mode = DEFAULT_MODE;
  g_app_path = "";
  g_app_home = "";
  g_universal_timeout = 0.5;
  g_verbose_mode = false;
  
  while ((c = getopt_long(argc, argv, options, long_options, &option_index)) != -1) 
	{
    switch (c) {
      case 'h':
        usage();
        exit(0);
        break;
      case 'v':
        g_verbose_mode = true;
        break;
      case 's':
        g_server_address = optarg;
        break;
      case 'p':
        g_server_port = atoi(optarg);
        break;
      case 'u':
        g_mode = UNIVERSAL_MODE;
        break;
      case 'm':
        g_mode = MULTIREMOTE_MODE;
        break;        
      case 't':
        g_universal_timeout = atof(optarg) * 0.001;
        break;
      case 'x':
        readExternal = true;
        break;
      case 'a':
        g_app_path = optarg;
        break;
      case 'z':
        g_app_home = optarg;
        break;
      default:
        usage();
        exit(1);
        break;
    }
  }
  //reset getopts state
  optreset = 1;
  optind = 0;
  
	if (readExternal == true)
		ReadConfig();	
    
}

//----------------------------------------------------------------------------
void ConfigureHelper(){
  [gp_xbmchelper enableVerboseMode:g_verbose_mode];
  
  //set apppath to startup when pressing Menu
  [gp_xbmchelper setApplicationPath:[NSString stringWithUTF8String:g_app_path.c_str()]];    
  //set apppath to startup when pressing Menu
  [gp_xbmchelper setApplicationHome:[NSString stringWithUTF8String:g_app_home.c_str()]];
  //connect to specified server
  [gp_xbmchelper connectToServer:[NSString stringWithUTF8String:g_server_address.c_str()] onPort:g_server_port withMode:g_mode withTimeout: g_universal_timeout];  
}

//----------------------------------------------------------------------------
void Reconfigure(int nSignal)
{
	if (nSignal == SIGHUP){
		ReadConfig();
    ConfigureHelper();
  }
	else {
    QuitEventLoop(GetMainEventLoop());
  }
}

//----------------------------------------------------------------------------
int main (int argc,  char * argv[]) {
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  
  ParseOptions(argc,argv);

  NSLog(@"%s %s starting up...", PROGNAME, PROGVERS);
  gp_xbmchelper = [[XBMCHelper alloc] init];  
  if(gp_xbmchelper){
    signal(SIGHUP, Reconfigure);
    signal(SIGINT, Reconfigure);
    signal(SIGTERM, Reconfigure);
    
    ConfigureHelper();
    
    //run event loop in this thread
    RunCurrentEventLoop(kEventDurationForever);
    NSLog(@"%s %s exiting...", PROGNAME, PROGVERS);
    //cleanup
    [gp_xbmchelper release];    
  } else {
    NSLog(@"%s %s failed to initialize remote.", PROGNAME, PROGVERS);  
    return -1;
  }
  [pool drain];
  return 0;
}
