#ifndef PY_DEFINES_H_
#define PY_DEFINES_H_

#ifdef _XBOX
struct hostent {
        char    FAR * h_name;           /* official name of host */
        char    FAR * FAR * h_aliases;  /* alias list */
        short   h_addrtype;             /* host address type */
        short   h_length;               /* length of address */
        char    FAR * FAR * h_addr_list; /* list of addresses */
#define h_addr  h_addr_list[0]          /* address, for backward compat */
};

typedef struct {
	struct hostent server;
	char name[128];
	char addr[16];
	char* addr_list[4];
} HostEnt;

typedef struct servent {
		char FAR* s_name;
		char FAR  FAR** s_aliases;
		short s_port;
		char FAR* s_proto;
} servent;

typedef struct protoent {
		char FAR* p_name;
		char FAR  FAR** p_aliases;
		short p_proto;
} protoent;

#endif
#endif //PY_DEFINES_H_