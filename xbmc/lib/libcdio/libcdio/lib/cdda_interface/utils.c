/*
  $Id: utils.c,v 1.3 2006/03/18 00:53:20 rocky Exp $

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
void 
cderror(cdrom_drive_t *d,const char *s)
{
  if(s && d){
    switch(d->errordest){
    case CDDA_MESSAGE_PRINTIT:
      write(STDERR_FILENO,s,strlen(s));
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
      write(STDERR_FILENO,s,strlen(s));
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
      write(STDERR_FILENO,buffer,strlen(buffer));
      if(errno){
	write(STDERR_FILENO,": ",2);
	write(STDERR_FILENO,strerror(errno),strlen(strerror(errno)));
	write(STDERR_FILENO,"\n",1);
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
      const unsigned int i_buffer=strlen(f)+strlen(s)+10;
      buffer=malloc(i_buffer);
      sprintf(buffer,f,s);
      strncat(buffer,"\n", i_buffer);
      malloced=1;
    }

  if(buffer){
    switch(messagedest){
    case CDDA_MESSAGE_PRINTIT:
      write(STDERR_FILENO,buffer,strlen(buffer));
      if(!malloced)write(STDERR_FILENO,"\n",1);
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
catstring(char *buff, const char *s) {
  if (s) {
    const unsigned int add_len = strlen(s) + 9;
    if(buff) {
      buff = realloc(buff, strlen(buff) + add_len);
    } else {
      buff=calloc(add_len, 1);
    }
    strncat(buff, s, add_len);
  }
  return(buff);
}
