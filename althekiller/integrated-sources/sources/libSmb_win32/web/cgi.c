/* 
   some simple CGI helper routines
   Copyright (C) Andrew Tridgell 1997-1998
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "includes.h"
#include "web/swat_proto.h"

#define MAX_VARIABLES 10000

/* set the expiry on fixed pages */
#define EXPIRY_TIME (60*60*24*7)

#ifdef DEBUG_COMMENTS
extern void print_title(char *fmt, ...);
#endif

struct cgi_var {
	char *name;
	char *value;
};

static struct cgi_var variables[MAX_VARIABLES];
static int num_variables;
static int content_length;
static int request_post;
static char *query_string;
static const char *baseurl;
static char *pathinfo;
static char *C_user;
static BOOL inetd_server;
static BOOL got_request;

static char *grab_line(FILE *f, int *cl)
{
	char *ret = NULL;
	int i = 0;
	int len = 0;

	while ((*cl)) {
		int c;
	
		if (i == len) {
			char *ret2;
			if (len == 0) len = 1024;
			else len *= 2;
			ret2 = (char *)SMB_REALLOC_KEEP_OLD_ON_ERROR(ret, len);
			if (!ret2) return ret;
			ret = ret2;
		}
	
		c = fgetc(f);
		(*cl)--;

		if (c == EOF) {
			(*cl) = 0;
			break;
		}
		
		if (c == '\r') continue;

		if (strchr_m("\n&", c)) break;

		ret[i++] = c;

	}
	
	if (ret) {
		ret[i] = 0;
	}
	return ret;
}

/**
 URL encoded strings can have a '+', which should be replaced with a space

 (This was in rfc1738_unescape(), but that broke the squid helper)
**/

static void plus_to_space_unescape(char *buf)
{
	char *p=buf;

	while ((p=strchr_m(p,'+')))
		*p = ' ';
}

/***************************************************************************
  load all the variables passed to the CGI program. May have multiple variables
  with the same name and the same or different values. Takes a file parameter
  for simulating CGI invocation eg loading saved preferences.
  ***************************************************************************/
void cgi_load_variables(void)
{
	static char *line;
	char *p, *s, *tok;
	int len, i;
	FILE *f = stdin;

#ifdef DEBUG_COMMENTS
	char dummy[100]="";
	print_title(dummy);
	d_printf("<!== Start dump in cgi_load_variables() %s ==>\n",__FILE__);
#endif

	if (!content_length) {
		p = getenv("CONTENT_LENGTH");
		len = p?atoi(p):0;
	} else {
		len = content_length;
	}


	if (len > 0 && 
	    (request_post ||
	     ((s=getenv("REQUEST_METHOD")) && 
	      strequal(s,"POST")))) {
		while (len && (line=grab_line(f, &len))) {
			p = strchr_m(line,'=');
			if (!p) continue;
			
			*p = 0;
			
			variables[num_variables].name = SMB_STRDUP(line);
			variables[num_variables].value = SMB_STRDUP(p+1);

			SAFE_FREE(line);
			
			if (!variables[num_variables].name || 
			    !variables[num_variables].value)
				continue;

			plus_to_space_unescape(variables[num_variables].value);
			rfc1738_unescape(variables[num_variables].value);
			plus_to_space_unescape(variables[num_variables].name);
			rfc1738_unescape(variables[num_variables].name);

#ifdef DEBUG_COMMENTS
			printf("<!== POST var %s has value \"%s\"  ==>\n",
			       variables[num_variables].name,
			       variables[num_variables].value);
#endif
			
			num_variables++;
			if (num_variables == MAX_VARIABLES) break;
		}
	}

	fclose(stdin);
	open("/dev/null", O_RDWR);

	if ((s=query_string) || (s=getenv("QUERY_STRING"))) {
		for (tok=strtok(s,"&;");tok;tok=strtok(NULL,"&;")) {
			p = strchr_m(tok,'=');
			if (!p) continue;
			
			*p = 0;
			
			variables[num_variables].name = SMB_STRDUP(tok);
			variables[num_variables].value = SMB_STRDUP(p+1);

			if (!variables[num_variables].name || 
			    !variables[num_variables].value)
				continue;

			plus_to_space_unescape(variables[num_variables].value);
			rfc1738_unescape(variables[num_variables].value);
			plus_to_space_unescape(variables[num_variables].name);
			rfc1738_unescape(variables[num_variables].name);

#ifdef DEBUG_COMMENTS
                        printf("<!== Commandline var %s has value \"%s\"  ==>\n",
                               variables[num_variables].name,
                               variables[num_variables].value);
#endif						
			num_variables++;
			if (num_variables == MAX_VARIABLES) break;
		}

	}
#ifdef DEBUG_COMMENTS
        printf("<!== End dump in cgi_load_variables() ==>\n");   
#endif

	/* variables from the client are in UTF-8 - convert them
	   to our internal unix charset before use */
	for (i=0;i<num_variables;i++) {
		pstring dest;

		convert_string(CH_UTF8, CH_UNIX, 
			       variables[i].name, -1, 
			       dest, sizeof(dest), True);
		free(variables[i].name);
		variables[i].name = SMB_STRDUP(dest);

		convert_string(CH_UTF8, CH_UNIX, 
			       variables[i].value, -1,
			       dest, sizeof(dest), True);
		free(variables[i].value);
		variables[i].value = SMB_STRDUP(dest);
	}
}


/***************************************************************************
  find a variable passed via CGI
  Doesn't quite do what you think in the case of POST text variables, because
  if they exist they might have a value of "" or even " ", depending on the 
  browser. Also doesn't allow for variables[] containing multiple variables
  with the same name and the same or different values.
  ***************************************************************************/

const char *cgi_variable(const char *name)
{
	int i;

	for (i=0;i<num_variables;i++)
		if (strcmp(variables[i].name, name) == 0)
			return variables[i].value;
	return NULL;
}

/***************************************************************************
 Version of the above that can't return a NULL pointer.
***************************************************************************/

const char *cgi_variable_nonull(const char *name)
{
	const char *var = cgi_variable(name);
	if (var) {
		return var;
	} else {
		return "";
	}
}

/***************************************************************************
tell a browser about a fatal error in the http processing
  ***************************************************************************/
static void cgi_setup_error(const char *err, const char *header, const char *info)
{
	if (!got_request) {
		/* damn browsers don't like getting cut off before they give a request */
		char line[1024];
		while (fgets(line, sizeof(line)-1, stdin)) {
			if (strnequal(line,"GET ", 4) || 
			    strnequal(line,"POST ", 5) ||
			    strnequal(line,"PUT ", 4)) {
				break;
			}
		}
	}

	d_printf("HTTP/1.0 %s\r\n%sConnection: close\r\nContent-Type: text/html\r\n\r\n<HTML><HEAD><TITLE>%s</TITLE></HEAD><BODY><H1>%s</H1>%s<p></BODY></HTML>\r\n\r\n", err, header, err, err, info);
	fclose(stdin);
	fclose(stdout);
	exit(0);
}


/***************************************************************************
tell a browser about a fatal authentication error
  ***************************************************************************/
static void cgi_auth_error(void)
{
	if (inetd_server) {
                cgi_setup_error("401 Authorization Required", 
                                "WWW-Authenticate: Basic realm=\"SWAT\"\r\n",
                                "You must be authenticated to use this service");
	} else {
		printf("Content-Type: text/html\r\n");

		printf("\r\n<HTML><HEAD><TITLE>SWAT</TITLE></HEAD>\n");
		printf("<BODY><H1>Installation Error</H1>\n");
		printf("SWAT must be installed via inetd. It cannot be run as a CGI script<p>\n");
		printf("</BODY></HTML>\r\n");
	}
	exit(0);
}

/***************************************************************************
authenticate when we are running as a CGI
  ***************************************************************************/
static void cgi_web_auth(void)
{
	const char *user = getenv("REMOTE_USER");
	struct passwd *pwd;
	const char *head = "Content-Type: text/html\r\n\r\n<HTML><BODY><H1>SWAT installation Error</H1>\n";
	const char *tail = "</BODY></HTML>\r\n";

	if (!user) {
		printf("%sREMOTE_USER not set. Not authenticated by web server.<br>%s\n",
		       head, tail);
		exit(0);
	}

	pwd = getpwnam_alloc(NULL, user);
	if (!pwd) {
		printf("%sCannot find user %s<br>%s\n", head, user, tail);
		exit(0);
	}

	setuid(0);
	setuid(pwd->pw_uid);
	if (geteuid() != pwd->pw_uid || getuid() != pwd->pw_uid) {
		printf("%sFailed to become user %s - uid=%d/%d<br>%s\n", 
		       head, user, (int)geteuid(), (int)getuid(), tail);
		exit(0);
	}
	TALLOC_FREE(pwd);
}


/***************************************************************************
handle a http authentication line
  ***************************************************************************/
static BOOL cgi_handle_authorization(char *line)
{
	char *p;
	fstring user, user_pass;
	struct passwd *pass = NULL;

	if (!strnequal(line,"Basic ", 6)) {
		goto err;
	}
	line += 6;
	while (line[0] == ' ') line++;
	base64_decode_inplace(line);
	if (!(p=strchr_m(line,':'))) {
		/*
		 * Always give the same error so a cracker
		 * cannot tell why we fail.
		 */
		goto err;
	}
	*p = 0;

	convert_string(CH_UTF8, CH_UNIX, 
		       line, -1, 
		       user, sizeof(user), True);

	convert_string(CH_UTF8, CH_UNIX, 
		       p+1, -1, 
		       user_pass, sizeof(user_pass), True);

	/*
	 * Try and get the user from the UNIX password file.
	 */
	
	pass = getpwnam_alloc(NULL, user);
	
	/*
	 * Validate the password they have given.
	 */
	
	if NT_STATUS_IS_OK(pass_check(pass, user, user_pass, 
		      strlen(user_pass), NULL, False)) {
		
		if (pass) {
			/*
			 * Password was ok.
			 */
			
			if ( initgroups(pass->pw_name, pass->pw_gid) != 0 )
				goto err;

			become_user_permanently(pass->pw_uid, pass->pw_gid);
			
			/* Save the users name */
			C_user = SMB_STRDUP(user);
			TALLOC_FREE(pass);
			return True;
		}
	}
	
err:
	cgi_setup_error("401 Bad Authorization", 
			"WWW-Authenticate: Basic realm=\"SWAT\"\r\n",
			"username or password incorrect");

	TALLOC_FREE(pass);
	return False;
}

/***************************************************************************
is this root?
  ***************************************************************************/
BOOL am_root(void)
{
	if (geteuid() == 0) {
		return( True);
	} else {
		return( False);
	}
}

/***************************************************************************
return a ptr to the users name
  ***************************************************************************/
char *cgi_user_name(void)
{
        return(C_user);
}


/***************************************************************************
handle a file download
  ***************************************************************************/
static void cgi_download(char *file)
{
	SMB_STRUCT_STAT st;
	char buf[1024];
	int fd, l, i;
	char *p;
	char *lang;

	/* sanitise the filename */
	for (i=0;file[i];i++) {
		if (!isalnum((int)file[i]) && !strchr_m("/.-_", file[i])) {
			cgi_setup_error("404 File Not Found","",
					"Illegal character in filename");
		}
	}

	if (sys_stat(file, &st) != 0) 
	{
		cgi_setup_error("404 File Not Found","",
				"The requested file was not found");
	}

	if (S_ISDIR(st.st_mode))
	{
		snprintf(buf, sizeof(buf), "%s/index.html", file);
		if (!file_exist(buf, &st) || !S_ISREG(st.st_mode))
		{
			cgi_setup_error("404 File Not Found","",
					"The requested file was not found");
		}
	}
	else if (S_ISREG(st.st_mode))
	{
		snprintf(buf, sizeof(buf), "%s", file);
	}
	else
	{
		cgi_setup_error("404 File Not Found","",
				"The requested file was not found");
	}

	fd = web_open(buf,O_RDONLY,0);
	if (fd == -1) {
		cgi_setup_error("404 File Not Found","",
				"The requested file was not found");
	}
	printf("HTTP/1.0 200 OK\r\n");
	if ((p=strrchr_m(buf, '.'))) {
		if (strcmp(p,".gif")==0) {
			printf("Content-Type: image/gif\r\n");
		} else if (strcmp(p,".jpg")==0) {
			printf("Content-Type: image/jpeg\r\n");
		} else if (strcmp(p,".png")==0) {
			printf("Content-Type: image/png\r\n");
		} else if (strcmp(p,".css")==0) {
			printf("Content-Type: text/css\r\n");
		} else if (strcmp(p,".txt")==0) {
			printf("Content-Type: text/plain\r\n");
		} else {
			printf("Content-Type: text/html\r\n");
		}
	}
	printf("Expires: %s\r\n", http_timestring(time(NULL)+EXPIRY_TIME));

	lang = lang_tdb_current();
	if (lang) {
		printf("Content-Language: %s\r\n", lang);
	}

	printf("Content-Length: %d\r\n\r\n", (int)st.st_size);
	while ((l=read(fd,buf,sizeof(buf)))>0) {
		fwrite(buf, 1, l, stdout);
	}
	close(fd);
	exit(0);
}




/**
 * @brief Setup the CGI framework.
 *
 * Setup the cgi framework, handling the possibility that this program
 * is either run as a true CGI program with a gateway to a web server, or
 * is itself a mini web server.
 **/
void cgi_setup(const char *rootdir, int auth_required)
{
	BOOL authenticated = False;
	char line[1024];
	char *url=NULL;
	char *p;
	char *lang;

	if (chdir(rootdir)) {
		cgi_setup_error("500 Server Error", "",
				"chdir failed - the server is not configured correctly");
	}

	/* Handle the possibility we might be running as non-root */
	sec_init();

	if ((lang=getenv("HTTP_ACCEPT_LANGUAGE"))) {
		/* if running as a cgi program */
		web_set_lang(lang);
	}

	/* maybe we are running under a web server */
	if (getenv("CONTENT_LENGTH") || getenv("REQUEST_METHOD")) {
		if (auth_required) {
			cgi_web_auth();
		}
		return;
	}

	inetd_server = True;

	if (!check_access(1, lp_hostsallow(-1), lp_hostsdeny(-1))) {
		cgi_setup_error("403 Forbidden", "",
				"Samba is configured to deny access from this client\n<br>Check your \"hosts allow\" and \"hosts deny\" options in smb.conf ");
	}

	/* we are a mini-web server. We need to read the request from stdin
	   and handle authentication etc */
	while (fgets(line, sizeof(line)-1, stdin)) {
		if (line[0] == '\r' || line[0] == '\n') break;
		if (strnequal(line,"GET ", 4)) {
			got_request = True;
			url = SMB_STRDUP(&line[4]);
		} else if (strnequal(line,"POST ", 5)) {
			got_request = True;
			request_post = 1;
			url = SMB_STRDUP(&line[5]);
		} else if (strnequal(line,"PUT ", 4)) {
			got_request = True;
			cgi_setup_error("400 Bad Request", "",
					"This server does not accept PUT requests");
		} else if (strnequal(line,"Authorization: ", 15)) {
			authenticated = cgi_handle_authorization(&line[15]);
		} else if (strnequal(line,"Content-Length: ", 16)) {
			content_length = atoi(&line[16]);
		} else if (strnequal(line,"Accept-Language: ", 17)) {
			web_set_lang(&line[17]);
		}
		/* ignore all other requests! */
	}

	if (auth_required && !authenticated) {
		cgi_auth_error();
	}

	if (!url) {
		cgi_setup_error("400 Bad Request", "",
				"You must specify a GET or POST request");
	}

	/* trim the URL */
	if ((p = strchr_m(url,' ')) || (p=strchr_m(url,'\t'))) {
		*p = 0;
	}
	while (*url && strchr_m("\r\n",url[strlen(url)-1])) {
		url[strlen(url)-1] = 0;
	}

	/* anything following a ? in the URL is part of the query string */
	if ((p=strchr_m(url,'?'))) {
		query_string = p+1;
		*p = 0;
	}

	string_sub(url, "/swat/", "", 0);

	if (url[0] != '/' && strstr(url,"..")==0) {
		cgi_download(url);
	}

	printf("HTTP/1.0 200 OK\r\nConnection: close\r\n");
	printf("Date: %s\r\n", http_timestring(time(NULL)));
	baseurl = "";
	pathinfo = url+1;
}


/***************************************************************************
return the current pages URL
  ***************************************************************************/
const char *cgi_baseurl(void)
{
	if (inetd_server) {
		return baseurl;
	}
	return getenv("SCRIPT_NAME");
}

/***************************************************************************
return the current pages path info
  ***************************************************************************/
const char *cgi_pathinfo(void)
{
	char *r;
	if (inetd_server) {
		return pathinfo;
	}
	r = getenv("PATH_INFO");
	if (!r) return "";
	if (*r == '/') r++;
	return r;
}

/***************************************************************************
return the hostname of the client
  ***************************************************************************/
char *cgi_remote_host(void)
{
	if (inetd_server) {
		return get_peer_name(1,False);
	}
	return getenv("REMOTE_HOST");
}

/***************************************************************************
return the hostname of the client
  ***************************************************************************/
char *cgi_remote_addr(void)
{
	if (inetd_server) {
		return get_peer_addr(1);
	}
	return getenv("REMOTE_ADDR");
}


/***************************************************************************
return True if the request was a POST
  ***************************************************************************/
BOOL cgi_waspost(void)
{
	if (inetd_server) {
		return request_post;
	}
	return strequal(getenv("REQUEST_METHOD"), "POST");
}
