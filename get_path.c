
char *get_path(char *filename)
{
	char *buff=malloc(1024);
	sprintf(buff, "Q:\\mplayer\\%s", filename);
	
	mp_msg(MSGT_GLOBAL,MSGL_V,"get_path('%s') -> '%s'\n",filename,buff);
	return buff;
}
