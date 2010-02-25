/* Copyright (C) 2003-2004, 2006 MySQL AB
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


extern const char	*unknown_sqlstate;
extern const char	*cant_connect_sqlstate;
extern const char	*not_error_sqlstate;

#ifdef	__cplusplus
extern "C" {
#endif

extern CHARSET_INFO *default_client_charset_info;
MYSQL_FIELD *unpack_fields(MYSQL_DATA *data,MEM_ROOT *alloc,uint fields,
			   my_bool default_value, uint server_capabilities);
void free_rows(MYSQL_DATA *cur);
void free_old_query(MYSQL *mysql);
void end_server(MYSQL *mysql);
my_bool mysql_reconnect(MYSQL *mysql);
void mysql_read_default_options(struct st_mysql_options *options,
				const char *filename,const char *group);
my_bool
cli_advanced_command(MYSQL *mysql, enum enum_server_command command,
		     const unsigned char *header, ulong header_length,
		     const unsigned char *arg, ulong arg_length,
                     my_bool skip_check, MYSQL_STMT *stmt);
unsigned long cli_safe_read(MYSQL *mysql);
void net_clear_error(NET *net);
void set_stmt_errmsg(MYSQL_STMT *stmt, NET *net);
void set_stmt_error(MYSQL_STMT *stmt, int errcode, const char *sqlstate,
                    const char *err);
void set_mysql_error(MYSQL *mysql, int errcode, const char *sqlstate);
#ifdef	__cplusplus
}
#endif

#define protocol_41(A) ((A)->server_capabilities & CLIENT_PROTOCOL_41)

