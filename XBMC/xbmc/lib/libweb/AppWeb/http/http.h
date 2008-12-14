///
///	@file 	http.h
/// @brief 	Primary header for the Mbedthis Http Web Server
///	@overview See http.dox for additional documentation.
///		The Str type implies that the string has been dynamically allocated.
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////
#ifndef _h_HTTP
#define _h_HTTP 1

#include	"mpr.h"
#include	"var.h"
#include	"shared.h"
#include	"httpEnv.h"

/////////////////////////////// Forward Declarations ///////////////////////////

class	MaAlias;
class	MaAuth;
class	MaApplet;
class	MaDataStream;
class	MaHandler;
class	MaHandlerService;
class	MaHeader;
class	MaHost;
class	MaHostAddress;
class	MaHttp;
class	MaHttpError;
class	MaHttpMime;
class	MaListen;
class	MaLocation;
class	MaMimeHashEntry;
class	MaModule;
class	MaRequest;
#if BLD_FEATURE_IF_MODIFIED
class	MaRequestMatch;
class	MaRequestModified;
#endif
#if BLD_FEATURE_ROMFS
class	MaRomFileSystem;
class	MaRomInode;
class	MaRomFile;
class	MaRomHashEntry;
#endif
class	MaServer;
#if BLD_FEATURE_SESSION
class	MaSession;
#endif
class	MaVhost;
class	MaDir;
class	MaStats;

#if BLD_FEATURE_SSL_MODULE
//
//	Special import for SslConfig
//
class	MaSslConfig;
#endif

/////////////////////////////////// Defines ////////////////////////////////////

//	MOB -- change all these to MA_

#if BLD_FEATURE_SQUEEZE || DOXYGEN
/// Default HTTP buffer size
#define MPR_HTTP_BUFSIZE			(512)
#define MPR_HTTP_DOC_BUFSIZE		(512)
#define MPR_HTTP_IN_BUFSIZE			(512)
#else
#define MPR_HTTP_BUFSIZE			(2048)
#define MPR_HTTP_DOC_BUFSIZE		(1024)
#define MPR_HTTP_IN_BUFSIZE			(4096)
#endif

#define MPR_HTTP_SERVER_NAME		"Mbedthis-Appweb/" BLD_VERSION
#define MPR_HTTP_SERVER_TIMEOUT		(300 * 1000)	// Overridden in http.conf
#define MPR_HTTP_SESSION_TIMEOUT	(1800)			// 30 mins "  "  "
#define MA_SERVER_DEFAULT_PORT_NUM 	80

// 
//	Primary state machine states for the web server
// 
#define MPR_HTTP_START			0x1			// Starting state 
#define MPR_HTTP_HEADER			0x2			// Ready to read first line 
#define MPR_HTTP_RUN_HANDLERS	0x4			// Start running handlers
#define MPR_HTTP_RUNNING		0x8			// Processing request
#define MPR_HTTP_DONE			0x10		// Processing complete

// 
//	Server request (MaRequst->flags) and Client flags
//
#define MPR_HTTP_KEEP_ALIVE		0x1			// Keep connection alive after req
#define MPR_HTTP_COOKIE			0x2			// Cookie supplied
#define MPR_HTTP_IF_MODIFIED	0x4			// If-[un]modified-since supplied
#define MPR_HTTP_POST_REQUEST	0x8			// Post method
#define MPR_HTTP_LOCAL			0x10		// Request originate on local system
#define MPR_HTTP_SOCKET_EVENT	0x20		// Request came through socketEvent
#define MPR_HTTP_LENGTH			0x40		// Request specified content length 
#define MPR_HTTP_CONTENT_DATA	0x80		// Has content data
#define MPR_HTTP_CONN_CLOSED	0x100		// Connection closed
#define MPR_HTTP_INCOMPLETE		0x200		// Request prematurely terminated
#define MPR_HTTP_BLOCKING		0x400		// Block waiting for data
#define MPR_HTTP_REUSE			0x800		// Connection used keep-alive
#define MPR_HTTP_DONT_CACHE		0x1000		// Add no-cache to the response
#define MPR_HTTP_HEADER_WRITTEN	0x2000		// Headers have been output
#define MPR_HTTP_CREATE_ENV		0x4000		// Must create env for this request
#define MPR_HTTP_FLUSHED		0x8000		// Already flushed some data
#define MPR_HTTP_OPENED_DOC		0x10000		// Document has been opened
#define MPR_HTTP_CUSTOM_HEADERS	0x20000		// Handler is using custom headers
#define MPR_HTTP_DELETE_REQUEST	0x40000		// DELETE method 
#define MPR_HTTP_GET_REQUEST	0x80000		// HEAD method 
#define MPR_HTTP_HEAD_REQUEST	0x100000	// HEAD method 
#define MPR_HTTP_OPTIONS_REQUEST 0x200000	// OPTIONS method 
#define MPR_HTTP_PUT_REQUEST	0x400000	// PUT method 
#define MPR_HTTP_TRACE_REQUEST	0x800000	// TRACE method 
#define MPR_HTTP_PULL_POST		0x1000000	// Pull post data (server only)
#define MPR_HTTP_DONT_FINISH 	0x2000000	// Don't auto finish the request
#define MPR_HTTP_CHUNKED 		0x4000000	// Output is chunked
#define MPR_HTTP_NO_CHUNKING 	0x8000000	// Client requested no chunking
#define MPR_HTTP_OWN_GLOBAL 	0x10000000	// TerminalHandler creates own global obj

#if BLD_FEATURE_RANGES
#define MPR_HTTP_OUTPUT_MULTI 	0x10000000	// Using output multipart ranges
#define MPR_HTTP_INPUT_RANGE 	0x20000000	// Using input ranges
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaLimits ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaLimits { 
  public:
	int				maxBody;		///< Max size of an incoming request
	int				maxResponseBody;///< Max size of generated response content
	int				maxScriptSize;	///< Max size of a script
	int				maxFirstLine;	///< Max size of the first request line
	int				maxUrl;			///< Max size of a URL
	int				maxHeader;		///< Max size of the total header
	int				maxNumHeader;	///< Max number of lines of header
	int				maxThreads;		///< Max number of pool threads
	int				minThreads;		///< Min number of pool threads
	int				maxUploadSize;	///< Max size of an uploaded file
	int				sendBufferSize;	///< TCP/IP send buffer size
	int				threadStackSize;///< Stack size for each pool thread
  public:
					MaLimits();
					~MaLimits();
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MaHttp ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	There is one instance of the Http class per application. It holds a list
//	of HTTP servers running inside the application.
//

class MaHttp {
  private:
	int				gid;
	char			*group;
	MprList			handlerServices;		// List of loaded handler services
	MaLimits		limits;
	MprList			modules;
	MprList			servers;				// List of web servers
	int				uid;
	char			*user;

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;					// Multi-thread sync
#endif

  public:
	//
	//	Published API
	//
					MaHttp();
					~MaHttp();				///< Destructor
	int				start();
	int				stop();

	//
	//	Unpublished API
	//
	int				changeGroup();
	int				changeUser();
	MaModule		*findModule(char *name);
	MaServer		*findServer(char *name);
	char			*getGroup();
	MaLimits		*getLimits() { return &limits; };
	int				getHandlerServicesCount() { 
						return handlerServices.getNumItems(); 
					};
	MprList			*getModules() { return &modules; };				
	char			*getUser();
	void			insertHandlerService(MaHandlerService *hs);
	void			insertModule(MaModule *mp);
	void			insertServer(MaServer *sp);
	MaHandlerService* lookupHandlerService(char *name);
	void			removeHandlerService(MaHandlerService *hs);
	void			removeModule(MaModule *mp);
	int				setGroup(char *s);
	int				setUser(char *s);
	void			startHandlers();
	void			startModules();
	void			stopHandlers();
	void			stopModules();

	void			unloadModules();

#if BLD_FEATURE_MULTITHREAD
	inline void		lock() { mutex->lock(); };
	inline void		unlock() { mutex->unlock(); };
#else
	inline void		lock() {};
	inline void		unlock() {};
#endif
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaListen ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define MA_LISTEN_DEFAULT_PORT	0x1		// Use default port 80
#define MA_LISTEN_WILD_PORT		0x2		// Port spec missing
#define MA_LISTEN_WILD_IP		0x4		// IP spec missing (first endpoint)	
#define MA_LISTEN_WILD_IP2		0x8		// IP spec missing (second+ endpoint)

class MaListen : public MprLink {
  private:
	int				flags;
	char			*ipAddr;
	int				port;
	MprSocket		*sock;
#if BLD_FEATURE_SSL_MODULE
	bool			secure;
	MaSslConfig		*sslConfig;
#endif

  public:
					MaListen(char *ipAddr, int port, int flags = 0);
					~MaListen();
	int				close();
	char			*getIpAddr() { return ipAddr; };
	int				getFlags() { return flags; };
	int				getPort() { return port; };
	int				open(MaServer *sp);
#if BLD_FEATURE_SSL_MODULE
	bool			isSecure() { return secure; };
	void			setSslConfig(MaSslConfig *config);
#endif
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaHandlerService ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaHandlerService : public MprLink {
  private:
	MprStr			handlerName;

  public:
					MaHandlerService(const char *name);
	virtual			~MaHandlerService();
	char			*getName() { return handlerName; };
	virtual	MaHandler 
					*newHandler(MaServer *server, MaHost *host, char *ex);
	virtual int		start();
	virtual int		stop();
};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaHostAddress ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Flags
//
#define MPR_HTTP_IPADDR_VHOST	0x1

class MaHostAddress : public MprHashEntry {
  private:
	MprStr			ipAddr;
	int				port;
	int				flags;
	MprList			vhosts;					// Vhosts using this address
  public:
					MaHostAddress(char *ipAddrPort);
					~MaHostAddress();
	MaHost			*findHost(char *hostStr);
	char			*getIpAddr();
	int				getPort();
	void			insertVhost(MaVhost *vhost);
	bool			isNamedVhost();
	void			setNamedVhost();

	friend class	MaServer;
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaModule ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define	MPR_MODULE_DLL			0x1			// Module is a DLL vs static link
#define	MPR_MODULE_LOADED		0x2			// Loaded 

class MaModule : public MprLink {
  public:
	int				flags;
	void			*handle;				// If a DLL
	char			*name;

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;					// Multi-thread sync
#endif

  public:
					MaModule(char *name, void *handle);
	virtual			~MaModule();
	char			*getName();
	void			*getHandle();
	void			setHandle(void *handle);

	virtual int		parseConfig(char *key, char *value, MaServer *server, 
						MaHost *host, MaAuth *auth, MaDir* dir, 
						MaLocation *location);
	virtual int		start();
	virtual void	stop();

  private:
#if BLD_FEATURE_MULTITHREAD
	inline void		lock() { mutex->lock(); };
	inline void		unlock() { mutex->unlock(); };
#else
	inline void		lock() {};
	inline void		unlock() {};
#endif
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MaVhost ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaVhost : public MprLink {
  private:
	MaHost		*host;

  public:
					MaVhost(MaHost *host);
					~MaVhost();
	MaHost			*getHost();
	friend class	MaHostAddress;
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaServer ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	An application may have any number of HTTP servers, each managed
//	by an instance of the Server class. Typically there will be only one 
//	server in an application. There may be multiple virtual hosts and one 
//	default host for each server class. A server will typically be configured 
//	by calling the configure method for each server which parses a file to 
//	define the server and virtual host configuration.
//

class MaServer : public MprLink {
  public:
	MprVar			appObj;
	MaHttp			*http;		
	MprLogModule	*tMod;
	MprFileSystem	*fileSystem;
	static MaServer	*defaultServer;

  private:
	MprFileSystem	*defaultFileSystem;
	MprList			hosts;					// List of host objects
	MaHost			*defaultHost;			// Primary host
	MprHashTable	*hostAddresses;			// List of HostAddress objects
	int				lineNumber;				// Line in http.conf
	MprList			listens;				// List of listening sockets
	int				maxConcurrentRequests;	// Maximum number of clients
	MprStr			name;					// Unique name for this server
	MprStr			serverRoot;				// Server root
	bool			alreadyLogging;			// Already logging
#if BLD_FEATURE_LICENSE
	uint64			license;				// License key & activation
	uint			installTime;			// Time of first installation
	int				evalDays;				// Days into evaluation
	int				evalExtension;			// Extended evaluation
	bool			expired;				// Software is licensed or expired
	MprTimer		*expiryTimer;			// Session timeout timer
#endif

  public:
	//
	//	Published API
	//
					MaServer(MaHttp* hs, char *name, char *root = 0,
						char *ipAddr = 0, int port = 0);
					~MaServer();
#if BLD_FEATURE_CONFIG_PARSE
	int				configure(char *configFile);
#endif
	int				getLine() { return lineNumber; };
	int				loadModule(char *name, char *libname = 0);
	MaHost 			*newHost(char *docRoot, char *ipSpec = 0);
#if BLD_FEATURE_CONFIG_SAVE
	int				saveConfig(char *configFile);
#endif
	int				start();
	int				stop();

	//
	//	Unpublished API
	//
	int				createHostAddresses(MaHost *host, char *value);
	void			displayConfig();
	int 			featureSupported(char *key);
	MaHost			*findHost(char *name);
	static MaServer	*getDefaultServer();
	MaHost			*getDefaultHost();
	MprFileSystem	*getFileSystem() { return fileSystem; };
	int				getValue(char **value, char *buf, char **nextToken, 
						int quotes);
	MprHashTable	*getHostAddresses();
	MprList			*getListens() { return &listens; };
	char			*getName();
	char			*getServerRoot();
	int				processSetting(char *key, char *val, MaHost *host, 
						MaDir* dir, MaLocation *location);
	void			setDefaultHost(MaHost *host);
	static void		setDefaultServer(MaServer *server);
	void 			setDefaultPage(char *path, char *fileName);
	void			setFileSystem(MprFileSystem *fs);
	void			setServerRoot(char *path);
	int				splitValue(char **s1, char **s2, char *buf, int quotes);

#if BLD_FEATURE_SSL_MODULE
	int				setSslListeners(MaHost *host, MaSslConfig *config);
#endif
#if BLD_FEATURE_LICENSE
	void			calcExpiryDays();
	bool			isExpired() { return expired != 0; }
#endif
};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// MaAuth ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//	Flags
//
#define MPR_HTTP_AUTH_USER_HASH		0x1		// User hash created
#define MPR_HTTP_AUTH_GROUP_HASH	0x2		// Group hash created
#define MPR_HTTP_AUTH_REQUIRED		0x4		// Dir/Location requires auth

enum MaAuthOrder {
	MPR_HTTP_ALLOW_DENY,
	MPR_HTTP_DENY_ALLOW
};

enum MaAuthType {
	MPR_HTTP_AUTH_UNKNOWN,
	MPR_HTTP_AUTH_BASIC,
	MPR_HTTP_AUTH_DIGEST
};

#define MPR_HTTP_ACL_ALL	(-1)			// All bits set

typedef long MaAcl;							// Access control mask


class MaUser : public MprHashEntry {
  private:
	bool			enabled;
	MaAcl			acl;					// Union (or) of all group Acls
	MprStr			password;
	MprStr			realm;
	MprStr			userName;
	MprStr			userRealmKey;
  public:
					MaUser(char *user, char *realm, char *password, 
						bool enabled);
					~MaUser();
	MaAcl			getAcl() { return acl; };
	bool			getEnabled() { return enabled; };
	char			*getKey() { return userRealmKey; };
	char			*getName() { return userName; };
	char			*getRealm() { return realm; };
	char			*getPassword() { return password; };
	void			setEnabled(bool e) { enabled = e; };
	void			setAcl(MaAcl a) { acl = a; };
};


class MaGroup : public MprHashEntry {
  private:
	MaAcl			acl;
	bool			enabled;
	MprStr			groupName;
	MprStringList	*userList;							// List of users
  public:
					MaGroup(char *group, MaAcl acl, bool enabled);
					~MaGroup();
	int				addUser(char *user);
	MaAcl			getAcl() { return acl; };
	bool			getEnabled() { return enabled; };
	char			*getName() { return groupName; };
	MprStringList	*getUsers() { return userList; };
	int				removeUser(char *user);
	void			setAcl(MaAcl acl) { this->acl = acl; };
	void			setEnabled(bool e) { enabled = e; };
};


//
//	The MaAuth class is the foundation authorization class and is used as
//	base class by MaDirectory and MaLocation. It stores the authorization
// configuration information required to determine if a client request
//	should be permitted to the resource controlled by this object. 
//
class MaAuth : public MprLink {
  private:
	MprStr			allowSpec;
	bool			anyValidUser;
	bool			authInherited;
	MaAuthType		authType;
	MprStr			denySpec;
	int				flags;
	MaAuthOrder		order;
	MprStr			qop;
	MaAcl			requiredAcl;
	MprStr			requiredRealm;
	MprStr			requiredGroups;
	MprStr			requiredUsers;

	MprHashTable	*userHash;
	MprHashTable	*groupHash;

  public:
	//
	//	Published API
	//
					MaAuth();
					~MaAuth();
	int				addGroup(char *group, MaAcl acl, bool enable);
	int				addUsersToGroup(char *group, char *users);
	int				addUser(char *user, char *realm, char *passwd, bool enable);
	int				disableGroup(char *group);
	int				disableUser(char *user, char *realm);
	int				enableGroup(char *group);
	int				enableUser(char *user, char *realm);
	MaAcl			getRequiredAcl();
	MaAcl			getGroupAcl(char *group);
	void			getGroups(MprStringList *list);
	void			getUsers(MprStringList *list);
	bool			isGroupEnabled(char *group);
	bool			isUserEnabled(char *user, char *realm);
	MaAcl			parseAcl(char *aclStr);
	int				removeGroup(char *group);
	int				removeUser(char *user, char *realm);
	int				removeUsersFromGroup(char *group, char *users);
	void			reset();
	int				setGroupAcl(char *group, MaAcl acl);

	//
	//	Unpublished API
	//
	MaAuth			*getAuth() { return this; }
	int				getPassword(char *passBuf, int passLen, char *user, 
						char *realm);
	char			*getAllowSpec();
	bool			getAnyValidUser();
	char			*getDenySpec();
	MprHashTable	*getGroupHash() { return groupHash; };
	char			*getQop();
	char			*getRealm();
	char			*getRequiredGroups();
	char			*getRequiredUsers();
	MaAuthOrder		getOrder();
	MaAuthType		getType();
	MprHashTable	*getUserHash() { return userHash; };
	void			inheritAuth(MaAuth *auth);
	void			inheritUserGroup(MaAuth *auth);
	inline int		isAuthInherited() { return authInherited; };
	bool			isUserValid(char *user, char *realm);
	bool			isAuthRequired();	
	void			release();
	void			setAllowSpec(char *spec);
	void			setAnyValidUser();
	void			setDenySpec(char *spec);
	void			setOrder(MaAuthOrder order);
	void			setQop(char *qop);
	void			setRealm(char *realm);
	int				setRequiredAcl(MaAcl acl);
	void			setRequiredGroups(char *groups);
	void			setRequiredUsers(char *users);
	void			setType(MaAuthType typ);
	void			updateUserAcls();

};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// MaDir ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaDir : public MaAuth {
  private:
	MaHost			*host;
	MprStr			indexName;
	MprStr			path;
	int				pathLen;
	bool			inherited;

  public:
					MaDir(MaHost *host);
					MaDir(MaHost *host, char *path);
					MaDir(MaHost *host, MaDir *dp);
					~MaDir();
	inline MaAuth 	*getAuth() { return (MaAuth*) this; };
	inline char		*getIndex() { return indexName; };
	inline char		*getPath() { return path; };
	inline int		getPathLen() { return pathLen; };
	inline int		isInherited() { return inherited; };
	void			setIndex(char *index);
	void			setPath(char *path);
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaLocation /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Flags
//
#define MPR_HTTP_LOC_EXTRA_PATH	0x1				// Do extra path processing

class MaLocation : public MaAuth {
  private:
	int				flags;
	MprStr			prefix;
	int				prefixLen;
	MprStr			handlerName;
	bool			inherited;

  public:
					MaLocation();
					MaLocation(MaAuth *master);
					MaLocation(MaLocation *lp);
					~MaLocation();
	inline MaAuth 	*getAuth() { return (MaAuth*) this; };
	inline int		getFlags() { return flags; };
	inline char		*getHandlerName() { return handlerName; };
	inline char		*getPrefix() { return prefix; };
	inline int		getPrefixLen() { return prefixLen; };
	inline int		isInherited() { return inherited; };
	void			setFlags(int flags) { this->flags = flags; };
	void			setHandler(char *handlerName);
	void			setPrefix(char *prefix);
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MaStats ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Count of handler hits kept in Handler
//

class MaStats {
  public:
	long			accessErrors;			// Access violations 
	long			activeRequests;			// Currently active requests
	long			maxActiveRequests;		// Currently active requests
	long			errors;					// General errors 
	long			keptAlive;				// Requests service on keep-alive
	long			requests;				// Total requests
	long			redirects;				// Redirections 
	long			timeouts;				// Request timeouts
	long			copyDown;				// Times buffer had to copy down
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaAlias ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaAlias : public MprLink {
  private:
	char			*prefix;				// URI prefix
	int				prefixLen;
	char			*aliasName;				// Physical path name
	int				redirectCode;
	bool			inherited;

  public:
					MaAlias(char *prefix, char *dirName, int code = 0);
					MaAlias(MaAlias *ap);
					~MaAlias();
	inline char		*getName() { return aliasName; };
	inline char		*getPrefix() { return prefix; };
	inline int		getPrefixLen() { return prefixLen; };
	inline int		getRedirectCode() { return redirectCode; };
	inline int		isInherited() { return inherited; };
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaMimeHashEntry ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Mime Type hash table entry (the URL extension is the key)
//
class MaMimeHashEntry : public MprHashEntry {
  private:
	MprStr			mimeType;
	MprStr			actionProgram;
  public:
					MaMimeHashEntry(char *ext, char *mimeType);
	virtual			~MaMimeHashEntry();
	char			*getMimeType() { return mimeType; };
	void			setActionProgram(char *actionProgram);
	char			*getActionProgram() { return actionProgram; };
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaSession //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_SESSION

//	MOB -- should use product name to allow myAppweb on same system.
#define MA_HTTP_SESSION_PREFIX "_appwebSessionId_"

//
//	The MaSession class provides persistent data storage for user's state data
//	accross multiple requests. The MaSession is a subclass of the MprHashEntry
//	class which allows MaSession instances to be stored in a MprHashTable for
//	quick access by their session ID values.
//
class MaSession : public MprHashEntry {
  private:
	MaHost			*host;					// Host containing session
	MprTimer		*expiryTimer;			// Session timeout timer
	int				lastActivity;			// Time of last session activity
#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;					// Multi-thread sync
#endif
	int				timeout;				// Timeout period
	MprVar			sessionData;			// Actual session[] data object
	void			(*expiryCallback)(void *arg);
	void			*expiryArg;	

  public:
					MaSession(MaHost *host, char *sessionId, int timeout);
					~MaSession();
	char			*getId() { return getKey(); };
	MprVar			*getSessionData() { return &sessionData; };
	MaHost			*getHost() { return host; };
	int				getLastActivity() { return lastActivity; };
	int				getTimeout() { return timeout; };
	void			setLastActivity(int t) { lastActivity = t; };
	void			setExpiryTimer(MprTimer *tp) { expiryTimer = tp; };
	void			setExpiryCallback(void (*callback)(void *arg), void *arg) { 
						expiryCallback = callback; 
						expiryArg = arg; 
					};

#if BLD_FEATURE_MULTITHREAD
	inline void		lock() { mutex->lock(); };
	inline void		unlock() { mutex->unlock(); };
#else
	inline void		lock() { };
	inline void		unlock() { };
#endif
};

#endif
////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MaHost ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Flags
//
#define MPR_HTTP_HOST_REUSE_MIME	0x1		// Reuse another hosts mime types
#define MPR_HTTP_VHOST				0x2		// Is a virtual host
#define MPR_HTTP_NAMED_VHOST		0x4		// Named virtual host
#define MPR_HTTP_RESET_HANDLERS		0x8		// Handlers reset from default
#define MPR_HTTP_ADD_HANDLER		0x10	// Added at least one handler
#define MPR_HTTP_USE_CHUNKING		0x20	// Use output chunking if required
#define MPR_HTTP_NO_TRACE			0x40	// Prevent use of TRACE

//
//	Flags for mapToStorage
//
#define MPR_HTTP_REDIRECT			0x1
#define MPR_HTTP_REMATCH			0x2

//
//	A Host object represents a single HTTP connection endpoint. This may be
//	a default server or a virtual server. Multiple Hosts may be contained 
//	within a single Server.
//
class MaHost : public MprLink {
  public:
	MaServer		*server;
	MaStats			stats;

	//	FUTURE -- OPT. Convert as many pointers as possible to put data inline
  private:
	bool			authEnabled;
	MprList			aliases;				// List of Alias definitions
	MprList			dirs;					// List of Directory definitions
	MprStr			documentRoot;
	MprHashTable	*errorDocuments;
	int				flags;
	MprList			handlers;				// List of handlers for this host 
	int				httpVersion;			// HTTP/1.X
	MprStr			ipSpec;					// IP:PORT address (with wildcards)
	MaLimits		*limits;				// Pointer to http->limits
	MprList			locations;				// List of Location defintions
	MprHashTable	*mimeTypes;
	char			*mimeFile;				// Name of the mime types file
#if BLD_FEATURE_DLL
	MprStr			moduleDirs;				// Directories for modules
#endif
	MprStr			name;					// ServerName directive
	MprList			requests;
	MprStr			secret;					// Random bytes for authentication
	int				timeout;				// Timeout for requests
	MprLogModule	*tMod;

#if BLD_FEATURE_SESSION
	MprHashTable	*sessions;				// Session tables
	int				sessionTimeout;			// Default session timeout
	bool			sessionAutoCreate;		// Auto creation of sessions
#endif

#if BLD_FEATURE_ACCESS_LOG
	int				logFd;
	char			*logFormat;
	MaHost			*logHost;				// If set, use this hosts logs
	char			*logPath;
#endif

#if BLD_FEATURE_KEEP_ALIVE
	int				keepAlive;				// Keep alive supported
	int				keepAliveTimeout;		// Timeout for keep-alive
	int				maxKeepAlive;			// Max keep-alive requests
#endif

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;
#endif

#if BLD_FEATURE_SSL_MODULE
	bool			secure;
#endif

  public:
	///	Constructor
					MaHost(MaServer *sp, char *ipSpec);
	///	Destructor
					~MaHost();

	int				addHandler(char *module, char *extensions);
	void			addErrorDocument(char *code, char *url);
	void 			addMimeType(char *mimeType, char *ext);
	void			deleteHandlers();
	void			disableAuth() { authEnabled = 0; };
	void			enableAuth() { authEnabled = 1; };
	MaDir			*findBestDir(char *path);
	MaDir			*findDir(char *path);
	MaLocation 		*findLocation(char *prefix);
	MprList			*getAliases();
	MprList			*getDirs();
	char			*getDocumentRoot() { return documentRoot; };
	int				getFlags() { return flags; };
	MprList			*getHandlers();
	int				getHttpVersion() { return httpVersion; };
	char			*getIpSpec() { return ipSpec; };
	MprList			*getLocations();
	MaLimits		*getLimits() { return limits; };
#if BLD_FEATURE_ACCESS_LOG
	char			*getLogFormat() { return logFormat; };
	int				getLogFd() { return logFd; };
	MaHost			*getLogHost() { return logHost; };
	char			*getLogPath() { return logPath; };
#endif
	MprHashTable	*getMimeTypes();
	char			*getMimeActionProgram(char *mimeType);
	char			*getMimeFile() { return mimeFile; };
#if BLD_FEATURE_DLL
	char			*getModuleDirs() { return moduleDirs; };
#endif
	char			*getName() { return name; };
	char			*getSecret() { return secret; };
	MaServer		*getServer() { return server; };
	int				getTimeout() { return timeout; };
	void			inheritHost(MaHost *hp);
	int				insertAlias(MaAlias* ap);
	void			insertDir(MaDir* dp);
	int				insertLocation(MaLocation *loc);
	void			insertHandler(MaHandler *hp);
	void			insertRequest(MaRequest *rq);
	bool			isAuthEnabled() { return authEnabled; };
	bool			isVhost();
	bool			isNamedVhost();
	char			*lookupErrorDocument(int code);
	char			*lookupMimeType(char *ext);
	MaHandler		*lookupHandler(char *name);
	char			*makePath(char *buf, int buflen, char *file, 
						bool validate = 1);
	int				mapToStorage(MaRequest *rq, char *path, int len, char *uri,
						int flags);
	MaHandler		*matchHandlers(MaRequest *rq);
	int				openMimeTypes(char *path);
	void			removeRequest(MaRequest *rq);
	char			*replaceReferences(char *buf, int buflen, char *str);
	void			setChunking(bool on);
	void			setDocumentRoot(char *path);
	void			setHttpVersion(int v);
	void			setIpSpec(char *ipSpec);
	void			setLog(char *path, char *format);
	void			setLogHost(MaHost *host);
	void			setMimeTypes(MprHashTable *table);
	void 			setMimeActionProgram(char *mimeType, char *actionProgram);
	void			setName(char *ipAddrPort);
	void			setModuleDirs(char *path);
	void			setNamedVhost();
	void			setTimeout(int t) { timeout = t; };
	void			setTraceMethod(bool on);
	void			setVhost();
	int				start();
	int				stop();
	bool			useChunking();
	void			writeLog(char *buf, int len);

#if BLD_FEATURE_KEEP_ALIVE
	int				getKeepAlive() { return keepAlive; };
	int				getKeepAliveTimeout() { return keepAliveTimeout; };
	int				getMaxKeepAlive() { return maxKeepAlive; };
	void			setKeepAlive(int f) { keepAlive = f; };
	void			setKeepAliveTimeout(int t) { keepAliveTimeout = t; };
	void			setMaxKeepAlive(int m) { maxKeepAlive = m; };
#endif
#if BLD_FEATURE_MULTITHREAD
	inline void		lock() { mutex->lock(); };
	inline void		unlock() { mutex->unlock(); };
#else
	inline void		lock() { };
	inline void		unlock() { };
#endif
#if BLD_FEATURE_SESSION
	MaSession		*createSession(int timeout);
	void			destroySession(MaSession *session);
	int				getSessionTimeout() { return sessionTimeout; };
	bool			getSessionAutoCreate() { return sessionAutoCreate; };
	MprHashTable	*getSessions() { return sessions; };
	MaSession		*lookupSession(char *sessionId);
	void			setSessionTimeout(int t) { sessionTimeout = t; };
	void			setSessionAutoCreate(bool on) { sessionAutoCreate = on; };
#endif

#if BLD_FEATURE_SSL_MODULE
	bool			isSecure() { return secure; };
	void			setSecure(bool on);
#endif
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaDataStream ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//
//	Request handlers use DataStreams to buffer and manage the output of data
//	back to the client. Data streams are used for returning static file 
//	content and for dynamically generated data.
//

class MaDataStream : public MprLink {
  public:
	char			*name;
	MprBuf			buf;
	int				size;
	
  public:
					MaDataStream(char *name, int initial, int max);
					~MaDataStream();
	void			flush() { buf.flush(); size = 0; };
	char			*getName() { return name; };
	int				getSize() { return size; };
	void			setSize(int s) { size = s; };
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaHeader ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	The header as received from the client
//

class MaHeader {
  public:
	MprStr			firstLine;

	//
	//	These char *pointers point into various parts of buf
	//
	MprStr			buf;				// Tokenized with '\0'
	char			*method;
	char			*proto;
	char			*uri;

	//
	//	These are set from the headers
	//
	MprStr			authDetails;
	MprStr			authType;
	MprStr			host;
	MprStr			contentMimeType;		// Mime type of the request payload
	MprStr			userAgent;
#if BLD_FEATURE_COOKIE || BLD_FEATURE_SESSION
	MprStr			cookie;
#endif

  public:
					MaHeader();
					~MaHeader();
	void			reset();
};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaRequestMatch ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_IF_MODIFIED

//
//	MaRequestMatch stores "If-Match" and "If-None-Match" request headers.
//	Referenced only by MaRequest.
//
class MaRequestMatch {
  private:
	MprStringList	etags;
	bool			ifMatch;

 public:
					MaRequestMatch();
	void 			addEtag(char *newEtag);
	void			setMatch(bool match);

	// returns true if any of the etags match.
	bool 			matches(char *responseEtag);
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaRequestModified //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	MaRequestModified stores "If-Modified-Since" and "If-Unmodified-Since"
//	request headers. Referenced only by MaRequest.
//
class MaRequestModified {
  private:
	uint			since;
	bool			ifModified;

  public:
					MaRequestModified();

	//	returns true if the if-modified header matches and thus the request 
	//	should be not be performed. Otherwise a 304 not modified will be 
	//	returned. The parameter 'time' is the last modified time of the 
	//	resource in the client's cache. 
	bool 			matches(uint time);
	void			setDate(uint when, bool ifMod);
};

#endif

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaRequest //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Flags used by finishRequest
//
#define	MPR_HTTP_CLOSE				0x1
#define	MPR_HTTP_DONT_CLOSE			0x0

//
//	Flags used by flushOutput(background, finishRequest)
//
#define	MPR_HTTP_BACKGROUND_FLUSH	0x1
#define	MPR_HTTP_FOREGROUND_FLUSH	0x0
#define MPR_HTTP_FINISH_REQUEST		0x1

//
//	Flags for formatAltResponse
//
#define MPR_HTTP_DONT_ESCAPE		0x1

//
//	The Request class manages a HTTP client request to the server. If TCP/IP
//	Keep-Alive is used, the Request object may be reused for subsequent 
//	requests from the client on the same TCP/IP socket connection.
//
class MaRequest : public MprLink {
  public:
	MaHost			*host;				// Pointer to host class
	MaStats			stats;
	MprFileSystem	*fileSystem;		// File system (normal or ROM)

  private:
	MaHostAddress 	*address;
	MprStringList	*responseHeaders;
	int				responseCode;
	int				contentLength;
	char			contentLengthStr[12];

	MprList			outputStreams;
	MaDataStream	*hdrBuf;			// Buffer for headers
	MaDataStream	*docBuf;			// Document to send 
	MaDataStream	*dynBuf;			// Buffer for dynamic (generated) data
	MaDataStream	*writeBuf;			// Just a pointer into the other streams

	int				bytesWritten;
	MaDir*			dir;				// Best matching dir (PTR only)
	MaHandler		*currentHandler;
	MprStr			decodedQuery;
	MprStr			etag;				// Unique identifier tag
	MprStr			extraPath;			// Extra path information
	MprFile			*file;				// File to be served
	MprFileInfo		fileInfo;			// File information for the URL
	MprStr			fileName;
	int				flags;
	MprStr			group;				// Supplied via basic / digest auth 
	MprList			handlers;
	MaHeader		header;
	MprBuf*			inBuf;
	int				inUse;				// In use reference count
	MaLimits		*limits;			// Pointer to http->limits
	MprSocket		*listenSock;
	MaLocation		*location;			// Best matching location (PTR only)
	int				methodFlags;
	MprVar			variables[MA_HTTP_OBJ_MAX];
	MprStr			password;
	char			localPort[8];
	int				outputCurrentPos;	// Current position in output
	int				remotePort;
	int				remainingChunk;		// Remaining response chunk to output
	int				remainingContent;	// Remaining content data to read
	int				remainingKeepAlive;
	MprStr			remoteIpAddr;
	MprStr			remoteHostName;
	char			*requestMimeType;	// Mime type of the URL document
	int				responseLength;		// Response content length
	MprStr			responseMimeType;	// Mime type of the response payload

	MprStr			scriptName;
#if BLD_FEATURE_LEGACY_API
	void			*scriptHandle;		// Deprecated in 2.0
#endif
	MprSocket		*sock;
	int				socketEventMask;
	int				state;
	MaHandler		*terminalHandler;	// Actual handler doing the processing
	int				timeout;
	MprTimer		*timer;
	int				timestamp;
	MprLogModule	*tMod;
	MprStr			user;				// Supplied via basic / digest auth 
	MprStr			uri;
	MaUrl			url;				// Request url

#if BLD_FEATURE_SESSION
	MaSession		*session;			// Pointer to session object
	//	FUTURE -- not needed. Can use session->
	MprStr			sessionId;			// Unique session identifier
#endif
#if BLD_FEATURE_IF_MODIFIED
	MaRequestMatch	requestMatch;
	MaRequestModified requestModified;
#endif
#if BLD_FEATURE_RANGES
	int				inputEnd;			// For ranged input requests
	int				inputStart;			// For ranged input requests
	int				inputTotalSize;		// Total size of input content
	int				outputEnd;			// For ranged content responses
	int				outputStart;		// For ranged content responses

	/*
	 *	Output ranges
	 */
	MprStr			range;		
	MprStr			rangeTok;			// For tokenizing		
	MprStr			rangeBoundary;
	int				sumOfRanges;		// Sum of ranges
	char			*nextRange;		
#endif
#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;
#endif

  public:
	//
	//	Published API
	//
					MaRequest(MaHostAddress *ap, MaHost *hp);
					~MaRequest();
#if BLD_FEATURE_COOKIE || BLD_FEATURE_SESSION
	char			*getCookie();	/* Use getCookies() now */
	char			*getCookies();
	int 			getCrackedCookie(char *cookie, char **name, char **value, 
						char **path);
#endif
	char			*getFileName() { return fileName; };
	int 			getOutputStreamLength(bool countHeaders);
#if BLD_FEATURE_LEGACY_API
	char			*getVar(char *var, char *defaultValue);
#endif
	char			*getVar(MaEnvType objType, char *var, char *defaultValue);
	int				getIntVar(MaEnvType objType, char *var, int defaultValue);
	int				readPostData(char *buf, int bufsize);
	void			redirect(int code, char *targetUrl);
	void			requestError(int code, char *fmt, ...);
#if BLD_FEATURE_COOKIE || BLD_FEATURE_SESSION
	void 			setCookie(char *name, char *value, int lifetime, 
						char *path, bool secure);
#endif
	void 			setHeader(char *value, bool allowMultiple = 0);
	void			setHeaderFlags(int setFlags, int clearFlags);
	int				setFileName(char *newPath);
	void			setPullPost();
#if BLD_FEATURE_LEGACY_API
	void			setVar(char *var, char *value);
#endif
	void			setVar(MaEnvType objType, char *var, char *value);
	void			setIntVar(MaEnvType objType, char *var, int value);
	int				write(char *buf, int size);
	int				write(char *s);
	int				writeFmt(char *fmt, ...);

#if BLD_FEATURE_SESSION
	void			createSession(int timeout);
	void			destroySession();
	MaSession		*getSession() { return session; };
	char			*getSessionId() { return sessionId; };
#endif
#if BLD_FEATURE_IF_MODIFIED
	///
	///	@synopsis Returns whether conditional request should be performed
	///	@overview This call compares ETag-fields sent with request with
	///		the ETag supplied by the client, and returns true or
	///		false depending on was a match requested or not.
	///	@param realEtag ETag regenerated from the file
	///	@returns true if the etag matches, otherwise returns false.
	inline bool		matchEtag(char *realEtag) {
						return requestMatch.matches(realEtag);
					}
	///
	///	@synopsis Returns whether conditional request should be performed
	///	@overview This call compares value from last modification time sent
	///		with request towards the real last modification time of the
	///		file, and returns true or false depending on was a modification
	///		requested or not.
	///	@param realModTime Last modified time of the resource in the client's
	///		cache.
	///	@returns true if the modified header supplied by the client has been
	///		satisfied and no full response is required.
	inline bool		matchModified(uint realModTime) {
						return requestModified.matches(realModTime);
					}
#endif

	//
	//	Unpublished API
	//
	int				acceptEvent(void *data, MprSocket *sock, char *ipaddr, 
						int port, MprSocket *lp, int isPoolThread); 
	void			resetEnvObj();
	void			incAccessError() { stats.accessErrors++; };
	int				backgroundFlush();
	int				blockingWrite(char *buf, int len);
	void			cancelOutput();
	void			cancelRequest();
	void			cancelTimeout();
	void			closeSocket();
	void			deleteHandlers();
	void			closeDoc();
#if BLD_FEATURE_LEGACY_API
	int				compareVar(char *var, char *value);
#endif
	int				compareVar(MaEnvType type, char *var, char *value);
	void			createEnvironment();
	void			createEnvironmentStore();
	void			createQueryVars(char *buf, int len);
	void			enableReadEvents(bool on);
	void			enableWriteEvents(bool on);
	void			finishRequest(bool closeSocket = 0);
	void			finishRequest(int code, bool closeSocket);
	int				flushOutput(bool background, bool completeRequired);
	int				foregroundFlush();
	void			formatAltResponse(int code, char *msg, int flags);
	int				getBytesWritten();
	char			*getAuthDetails();
	char			*getAuthType();
	MaAuth			*getAuth();
	int				getContentLength() { return contentLength; }
	MaHandler		*getCurrentHandler() { return currentHandler; };
	MaDataStream	*getDocBuf() { return docBuf; };
	MaDataStream	*getDynBuf() { return dynBuf; };
	char			*getEtag() { return etag; };
	char			*getErrorMsg(int code);
	char			*getExtraPath() { return extraPath; };
	inline int		getFd() { return (sock) ? sock->getFd() : -1; };
	MprFileInfo		*getFileInfo() { return &fileInfo; };
	int				getFlags();
	char			*getGroup() { return group; };
	MaHeader		*getHeader() { return &header; };
	char			*getIpAddr() { return listenSock->getIpAddr(); };
	char			*getRemoteIpAddr() { return remoteIpAddr; };
	MaLimits		*getLimits() { return limits; };
	MaLocation		*getLocation() { return location; };
	char			*getMethod() { return header.method; };
	int				getMethodFlags() { return methodFlags; };
	int 			getNumEnvProperties();
	char			*getPassword() { return password; };
	int				getPort() { return listenSock->getPort(); };
	char			*getOriginalUri();
	char			*getQueryString();
	int				getRemainingContent() { return remainingContent; }
	char			*getRequestMimeType() { return requestMimeType; };
	char			*getRequestContentMimeType() { 
						return header.contentMimeType; 
					};
	int				getRemainingKeepAlive() { return remainingKeepAlive; };
	int				getResponseCode() { return responseCode; };
	char			*getScriptName() { return scriptName; };
#if BLD_FEATURE_LEGACY_API
	void			*getScriptHandle() { return scriptHandle; };
#endif
	MprSocket		*getSocket() { return sock; };
	int				getState() { return state; };
	MaHandler		*getHandler() { return terminalHandler; };
	int				getTimeSinceLastActivity();
	char			*getUri();
	MaUrl			*getUrl() { return &url; };
	char			*getUriExt() { return url.ext; };
	char			*getUser() { return user; };
	char			*getUserAgent() { return header.userAgent; };
	MprVar			*getVariables() { return variables; };
	void			insertHandler(MaHandler *hp);
	void			insertDataStream(MaDataStream *dp);
	bool			isDir(char *path);
	void			logRequest();
	int				matchHandlers();
	int				outputBuffer(MaDataStream *dp, int len);
	int				openDoc(char *path);
	void			outputHeader(char *fmt, ...);
	int				parseFirstLine(char *line);
	int				parseHeader(char *line);
	int				parseUri();
	void			processRequest();
	int				readDoc(char *buf, int len);
	int				readEvent();
	void			reRunHandlers();
	void			reset();
	void			runHandlers();
	void			seekDoc(long offset, int origin);
	void			setBytesWritten(int written);
	void			setDataStream(bool buffered, bool document);
	void			setDir(MaDir *dp) { dir = dp; };
	int				setExtraPath(char *prefix, int prefixLen);
	void			setFlags(int orFlags, int andFlags);
	void			setGroup(char *group);
	void			setLocation(MaLocation *lp) { location = lp; };
	void			setPassword(char *password);
	void			setResponseCode(int code) { responseCode = code; };
	void			setResponseMimeType(char *mimeType);
#if BLD_FEATURE_LEGACY_API
	void			setScriptHandle(void *s) { scriptHandle = s; };
#endif
	void			setState(int s) { state = s; };
	void			setTimeMark();
	void			setUri(char *path);
	void			setUser(char *user);
	int				statDoc(MprFileInfo *info);
	int				setupHandlers();
#if BLD_FEATURE_LEGACY_API
	int				testVar(char *var);
#endif
	int				testVar(MaEnvType objType, char *var);
	int				timeoutCheck();
	int				writeBlk(MaDataStream *dp, char *buf, int len);
	int				writeEvent(bool completeRequired);
	void			writeHeaders();
#if BLD_FEATURE_LEGACY_API
	void			unsetVar(char *var);
#endif
	void			unsetVar(MaEnvType objType, char *var);

#if BLD_FEATURE_RANGES
	void			deRangeOutput();
	bool			isRangedInput() { 
						return (flags & MPR_HTTP_INPUT_RANGE) ? 1 : 0;
					};
	bool			isRangedOutput() { return (range) ? 1 : 0; };
	int				getNextRange(char **nextRange);
	int				getInputStart() { return inputStart;};
	int				getInputEnd() { return inputEnd;};
#else
	bool			isRangedInput() { return 0; };
#endif

#if BLD_FEATURE_MULTITHREAD
	inline void		lock() { 
		mutex->lock(); 
		inUse++;
	};
	inline void		unlock() { 
		if (--inUse <= 0 && flags & MPR_HTTP_CONN_CLOSED) {
			delete this;
		} else {
			mutex->unlock(); 
		}
	};
#else
	inline void		lock() { inUse++; };
	inline void		unlock() { 
		if (--inUse <= 0 && flags & MPR_HTTP_CONN_CLOSED) {
			delete this;
		}
	};
#endif
#if BLD_FEATURE_KEEP_ALIVE
	void			setNoKeepAlive();
#endif

	friend class	Rom;
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaHandler //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Handler Flags
//	FUTURE -- change to MA_HANDLER (LEGACY -- define to MPR also)
//
#define	MPR_HANDLER_DELETE		0x1			// Support DELETE requests
#define	MPR_HANDLER_GET			0x2			// Support GET requests
#define	MPR_HANDLER_HEAD		0x4			// Support HEAD requests
#define	MPR_HANDLER_OPTIONS		0x8			// Support OPTIONS requests
#define	MPR_HANDLER_POST		0x10		// Support POST requests
#define	MPR_HANDLER_PUT			0x20		// Support PUT requests
#define	MPR_HANDLER_TRACE		0x40		// Support TRACE requests
#define	MPR_HANDLER_NEED_ENV	0x80		// Create environment
#define	MPR_HANDLER_TERMINAL	0x100		// Terminates run processing
#define	MPR_HANDLER_ALWAYS		0x200		// Always match
#define	MPR_HANDLER_MAP_VIRTUAL	0x400		// Don't map to physical storage
#define	MPR_HANDLER_EXTRA_PATH	0x800		// Do extra path processing
#define	MPR_HANDLER_OWN_GLOBAL	0x1000		// Creates own global object
#define	MPR_HANDLER_NEED_FILENAME 0x2000	// Need setFileName to be called
#define	MPR_HANDLER_ALL	\
	(MPR_HANDLER_GET | MPR_HANDLER_POST | MPR_HANDLER_DELETE | \
	 MPR_HANDLER_PUT | MPR_HANDLER_OPTIONS | MPR_HANDLER_TRACE)

//
//	Handler run() return codes
//	FUTURE -- these are currently just ignored.
//
#define MPR_HTTP_HANDLER_FINISHED_PROCESSING	1

//
// Handlers are used to process client HTTP requests in a modular fashion.
//	A handler may \em match a request by URL extension or by a custom 
//	matchRequest method.
//
class MaHandler : public MprLink {
  protected:
	MaHost			*host;							// Host for this handler	
	MprStringList	extList;						// Extensions served 
	MprStr			extensions;						// String ext list
	int				flags;
	long			hitCount;						// Usage counter 
	MprStr			name;

  public:
					MaHandler(char *name);
					MaHandler(char *name, char *ext, int flags);
	virtual			~MaHandler();
	MprStringList	*getExtList() { return &extList; };
	char			*getExtensions() { return extensions; };
	int				getFlags() { return flags; };
	char			*getName() { return name; };

	virtual	MaHandler 
					*cloneHandler();
	virtual int		matchRequest(MaRequest *rq, char *uri, int uriLen);
#if BLD_FEATURE_CONFIG_PARSE
	virtual int		parseConfig(char *key, char *value, MaServer *server, 
						MaHost *host, MaAuth *auth, MaDir* dir, 
						MaLocation *location);
#endif
	virtual void	postData(MaRequest *rq, char *buf, int buflen);
	virtual int		run(MaRequest *rq);

	virtual int		setup(MaRequest *rq);

	friend class	MaHost;
};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// MaRom ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_ROMFS || DOXYGEN
//
//	A RomInode is created for each file in the Rom file system.
//
class MaRomInode {
  public:
	MprStr			path;				// File path
	uchar			*data;				// Pointer to file data
	int				size;				// Size of file
	int				num;				// Inode number;
};

//
//	The Rom file system allows web pages and configuration files to be compiled
//	so a disk based file system is not required.
//

class MaRomFileSystem : public MprFileSystem {
  private:
	MprHashTable	*fileIndex;
	MaRomInode		*romInodes;
	MprStr			root;
	int				rootLen;

  public:
					MaRomFileSystem(MaRomInode *inodeList);
					~MaRomFileSystem();
	MprHashTable		*getFiles() { return fileIndex; };
	bool			isDir(char *path);
	MaRomInode		*lookup(char *path);
	MprFile			*newFile();
	int				stat(char *path, MprFileInfo *info);
};

//
//	Run-time hash lookup entry for each file
//
class MaRomHashEntry : public MprHashEntry {
  private:
	MaRomInode		*inode;				///< File inode descriptor
  public:
					MaRomHashEntry(char *key, MaRomInode *ri);
					~MaRomHashEntry() {};
	MaRomInode		*getInode();
};

//
//	File descriptor for a file in a Rom file system
//
class MaRomFile : public MprFile {
  private:
	MaRomFileSystem	
					*romFileSystem;
	MaRomInode		*inode;
	int				pos;						///< Current read position 
#if UNUSED
	Rom*			rom;
#endif

  public:
					MaRomFile(MaRomFileSystem* romFileSystem);
					~MaRomFile();
	int				open(char *path, int flags, int mode);
	void			close();
	int				read(void *buf, int len);
	int				write(void *buf, int len);
	long			lseek(long offset, int origin);
	int				stat(char *path, MprFileInfo *info);
};

#endif // BLD_FEATURE_ROMFS

///////////////////////////////// Prototypes ///////////////////////////////////

extern MaHttp *maGetHttp();

////////////////////////////////////////////////////////////////////////////////
#endif // _h_HTTP 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
