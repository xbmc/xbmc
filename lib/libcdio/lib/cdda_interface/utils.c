/*
  $Id: utils.c,v 1.2 2005/01/14 01:36:12 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "common_interface.h"
#include "utils.h"

#ifdef _XBOX
#include "xtl.h"
#else
#include <windows.h>
#endif

void 
cderror(cdrom_drive_t *d,const char *s)
{
  if(s && d){
    switch(d->errordest){
    case CDDA_MESSAGE_PRINTIT:
      //write(STDERR_FILENO,s,strlen(s));
			OutputDebugString(s);
      break;
    case CDDA_MESSAGE_LOGIT:
      d->errorbuf=catstring(d->errorbuf,s);
      break;
    case CDDA_MESSAGE_FORGETIT:
    default: ;
    }
  }
}

void 
cdmessage(cdrom_drive_t *d, const char *s)
{
  if(s && d){
    switch(d->messagedest){
    case CDDA_MESSAGE_PRINTIT:
      //write(STDERR_FILENO,s,strlen(s));
			OutputDebugString(s);
      break;
    case CDDA_MESSAGE_LOGIT:
      d->messagebuf=catstring(d->messagebuf,s);
      break;
    case CDDA_MESSAGE_FORGETIT:
    default: ;
    }
  }
}

void 
idperror(int messagedest,char **messages,const char *f,
	 const char *s)
{

  char *buffer;
  int malloced=0;
  if(!f)
    buffer=(char *)s;
  else
    if(!s)
      buffer=(char *)f;
    else{
      buffer=malloc(strlen(f)+strlen(s)+9);
      sprintf(buffer,f,s);
      malloced=1;
    }

  if(buffer){
    switch(messagedest){
    case CDDA_MESSAGE_PRINTIT:
//      write(STDERR_FILENO,buffer,strlen(buffer));
			OutputDebugString(buffer);
      if(errno){
	//write(STDERR_FILENO,": ",2);
	//write(STDERR_FILENO,strerror(errno),strlen(strerror(errno)));
	//write(STDERR_FILENO,"\n",1);
			OutputDebugString(": ");
			OutputDebugString(strerror(errno));
			OutputDebugString("\n");
      }
      break;
    case CDDA_MESSAGE_LOGIT:
      if(messages){
	*messages=catstring(*messages,buffer);
	if(errno){
	  *messages=catstring(*messages,": ");
	  *messages=catstring(*messages,strerror(errno));
	  *messages=catstring(*messages,"\n");
	}
      }
      break;
    case CDDA_MESSAGE_FORGETIT:
    default: ;
    }
  }
  if(malloced)free(buffer);
}

void 
idmessage(int messagedest,char **messages,const char *f,
	  const char *s)
{
  char *buffer;
  int malloced=0;
  if(!f)
    buffer=(char *)s;
  else
    if(!s)
      buffer=(char *)f;
    else{
      buffer=malloc(strlen(f)+strlen(s)+10);
      sprintf(buffer,f,s);
      strcat(buffer,"\n");
      malloced=1;
    }

  if(buffer){
    switch(messagedest){
    case CDDA_MESSAGE_PRINTIT:
      //write(STDERR_FILENO,buffer,strlen(buffer));
			OutputDebugString(buffer);
      if(!malloced)OutputDebugString("\n");//write(STDERR_FILENO,"\n",1);
      break;
    case CDDA_MESSAGE_LOGIT:
      if(messages){
	*messages=catstring(*messages,buffer);
	if(!malloced)*messages=catstring(*messages,"\n");
	}
      break;
    case CDDA_MESSAGE_FORGETIT:
    default: ;
    }
  }
  if(malloced)free(buffer);
}

char *
catstring(char *buff,const char *s){
  if(s){
    if(buff)
      buff=realloc(buff,strlen(buff)+strlen(s)+9);
    else
      buff=calloc(strlen(s)+9,1);
    strcat(buff,s);
  }
  return(buff);
}
