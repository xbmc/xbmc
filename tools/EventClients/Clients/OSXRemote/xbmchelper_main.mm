#include "Carbon/Carbon.h"
#import "XBMCHelper.h"
#include <getopt.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iterator>

using namespace std;

//instantiate XBMCHelper which registers itself to IR handling stuff
XBMCHelper* gp_xbmchelper;
eRemoteMode g_mode = DEFAULT_MODE;
std::string g_server_address="localhost";
int         g_server_port = 9777;
std::string g_app_path = "";
std::string g_app_home = "";
double g_universal_timeout = 0.500;
bool g_verbose_mode = false;

//
const char* PROGNAME="XBMCHelper";
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
  printf("   Sends Apple Remote events to Kodi.\n\n");
  printf("Usage: %s [OPTIONS...]\n\nOptions:\n", PROGNAME);
  printf("  -h, --help           print this help message and exit.\n");
  printf("  -s, --server <addr>  send events to the specified IP.\n");
  printf("  -p, --port <port>    send events to the specified port.\n");
  printf("  -u, --universal      runs in Universal Remote mode.\n");
  printf("  -t, --timeout <ms>   timeout length for sequences (default: 500ms).\n");
  printf("  -m, --multiremote    runs in Multi-Remote mode (adds remote identifier as additional idenfier to buttons)\n");
  printf("  -a, --appPath        path to Kodi.app (MenuPress launch support).\n");
  printf("  -z, --appHome        path to Kodi.app/Content/Resources \n");
  printf("  -v, --verbose        prints lots of debugging information.\n");
}

//----------------------------------------------------------------------------
void ReadConfig()
{
	// Compute filename.
  std::string strFile = getenv("HOME");
  strFile += "/Library/Application Support/Kodi/XBMCHelper.conf";
  
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
	istringstream is(strData);
	vector<string> args = vector<string>(istream_iterator<string>(is), istream_iterator<string>());
  
	// Convert to char**.
	int argc = args.size() + 1;
	char** argv = new char*[argc + 1];
	int i = 0;
	argv[i++] = (char*)"XBMCHelper";
  
	for (vector<string>::iterator it = args.begin(); it != args.end(); ){
    //fixup the arguments, here: remove '"' like bash would normally do
    std::string::size_type j = 0;
    while ((j = it->find("\"", j)) != std::string::npos )
      it->replace(j, 1, "");
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
  g_server_port = 9777;
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
