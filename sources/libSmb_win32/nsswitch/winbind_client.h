#include "winbind_nss_config.h"
#include "winbindd_nss.h"

void init_request(struct winbindd_request *req,int rq_type);
NSS_STATUS winbindd_send_request(int req_type,
				 struct winbindd_request *request);
NSS_STATUS winbindd_get_response(struct winbindd_response *response);
NSS_STATUS winbindd_request_response(int req_type, 
			    struct winbindd_request *request,
			    struct winbindd_response *response);
int write_sock(void *buffer, int count, int recursing);
int read_reply(struct winbindd_response *response);
void close_sock(void);
void free_response(struct winbindd_response *response);

