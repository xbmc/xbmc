/* 
   Unix SMB/CIFS implementation.
   Username handling
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Jeremy Allison 1997-2001.
   
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

/* internal functions */
static struct passwd *uname_string_combinations(char *s, TALLOC_CTX *mem_ctx,
						struct passwd * (*fn) (TALLOC_CTX *mem_ctx, const char *),
						int N);
static struct passwd *uname_string_combinations2(char *s, TALLOC_CTX *mem_ctx, int offset,
						 struct passwd * (*fn) (TALLOC_CTX *mem_ctx, const char *),
						 int N);

/****************************************************************************
 Get a users home directory.
****************************************************************************/

char *get_user_home_dir(const char *user)
{
	static struct passwd *pass;

	/* Ensure the user exists. */

	pass = Get_Pwnam(user);

	if (!pass)
		return(NULL);
	/* Return home directory from struct passwd. */

	return(pass->pw_dir);      
}

/****************************************************************************
 * A wrapper for sys_getpwnam().  The following variations are tried:
 *   - as transmitted
 *   - in all lower case if this differs from transmitted
 *   - in all upper case if this differs from transmitted
 *   - using lp_usernamelevel() for permutations.
****************************************************************************/

static struct passwd *Get_Pwnam_ret = NULL;

static struct passwd *Get_Pwnam_internals(TALLOC_CTX *mem_ctx,
					  const char *user, char *user2)
{
	struct passwd *ret = NULL;

	if (!user2 || !(*user2))
		return(NULL);

	if (!user || !(*user))
		return(NULL);

	/* Try in all lower case first as this is the most 
	   common case on UNIX systems */
	strlower_m(user2);
	DEBUG(5,("Trying _Get_Pwnam(), username as lowercase is %s\n",user2));
	ret = getpwnam_alloc(mem_ctx, user2);
	if(ret)
		goto done;

	/* Try as given, if username wasn't originally lowercase */
	if(strcmp(user, user2) != 0) {
		DEBUG(5,("Trying _Get_Pwnam(), username as given is %s\n",
			 user));
		ret = getpwnam_alloc(mem_ctx, user);
		if(ret)
			goto done;
	}

	/* Try as uppercase, if username wasn't originally uppercase */
	strupper_m(user2);
	if(strcmp(user, user2) != 0) {
		DEBUG(5,("Trying _Get_Pwnam(), username as uppercase is %s\n",
			 user2));
		ret = getpwnam_alloc(mem_ctx, user2);
		if(ret)
			goto done;
	}

	/* Try all combinations up to usernamelevel */
	strlower_m(user2);
	DEBUG(5,("Checking combinations of %d uppercase letters in %s\n",
		 lp_usernamelevel(), user2));
	ret = uname_string_combinations(user2, mem_ctx, getpwnam_alloc,
					lp_usernamelevel());

done:
	DEBUG(5,("Get_Pwnam_internals %s find user [%s]!\n",ret ?
		 "did":"didn't", user));

	return ret;
}

/****************************************************************************
 Get_Pwnam wrapper without modification.
  NOTE: This with NOT modify 'user'! 
  This will return an allocated structure
****************************************************************************/

struct passwd *Get_Pwnam_alloc(TALLOC_CTX *mem_ctx, const char *user)
{
	fstring user2;
	struct passwd *ret;

	if ( *user == '\0' ) {
		DEBUG(10,("Get_Pwnam: empty username!\n"));
		return NULL;
	}

	fstrcpy(user2, user);

	DEBUG(5,("Finding user %s\n", user));

	ret = Get_Pwnam_internals(mem_ctx, user, user2);
	
	return ret;  
}

/****************************************************************************
 Get_Pwnam wrapper without modification.
  NOTE: This with NOT modify 'user'! 
****************************************************************************/

struct passwd *Get_Pwnam(const char *user)
{
	struct passwd *ret;

	ret = Get_Pwnam_alloc(NULL, user);
	
	/* This call used to just return the 'passwd' static buffer.
	   This could then have accidental reuse implications, so 
	   we now malloc a copy, and free it in the next use.

	   This should cause the (ab)user to segfault if it 
	   uses an old struct. 
	   
	   This is better than useing the wrong data in security
	   critical operations.

	   The real fix is to make the callers free the returned 
	   malloc'ed data.
	*/

	if (Get_Pwnam_ret) {
		TALLOC_FREE(Get_Pwnam_ret);
	}
	
	Get_Pwnam_ret = ret;

	return ret;  
}

/* The functions below have been taken from password.c and slightly modified */
/****************************************************************************
 Apply a function to upper/lower case combinations
 of a string and return true if one of them returns true.
 Try all combinations with N uppercase letters.
 offset is the first char to try and change (start with 0)
 it assumes the string starts lowercased
****************************************************************************/

static struct passwd *uname_string_combinations2(char *s, TALLOC_CTX *mem_ctx,
						 int offset,
						 struct passwd *(*fn)(TALLOC_CTX *mem_ctx, const char *),
						 int N)
{
	ssize_t len = (ssize_t)strlen(s);
	int i;
	struct passwd *ret;

	if (N <= 0 || offset >= len)
		return(fn(mem_ctx, s));

	for (i=offset;i<(len-(N-1));i++) {
		char c = s[i];
		if (!islower_ascii((int)c))
			continue;
		s[i] = toupper_ascii(c);
		ret = uname_string_combinations2(s, mem_ctx, i+1, fn, N-1);
		if(ret)
			return(ret);
		s[i] = c;
	}
	return(NULL);
}

/****************************************************************************
 Apply a function to upper/lower case combinations
 of a string and return true if one of them returns true.
 Try all combinations with up to N uppercase letters.
 offset is the first char to try and change (start with 0)
 it assumes the string starts lowercased
****************************************************************************/

static struct passwd * uname_string_combinations(char *s, TALLOC_CTX *mem_ctx,
						 struct passwd * (*fn)(TALLOC_CTX *mem_ctx, const char *),
						 int N)
{
	int n;
	struct passwd *ret;

	for (n=1;n<=N;n++) {
		ret = uname_string_combinations2(s,mem_ctx,0,fn,n);
		if(ret)
			return(ret);
	}  
	return(NULL);
}

