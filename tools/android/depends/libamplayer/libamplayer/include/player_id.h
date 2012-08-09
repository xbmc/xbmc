#ifndef PLAYER_ID_MGT__
#define PLAYER_ID_MGT__

int player_request_pid(void);
int player_release_pid(int pid);
int player_init_pid_data(int pid,void * data);
void * player_open_pid_data(int pid);
int player_close_pid_data(int pid);
int player_id_pool_init(void);
int player_list_pid(char id[],int size);

#endif


