/* 
   This structure is used by lib/interfaces.c to return the list of network
   interfaces on the machine
*/

#define MAX_INTERFACES 128

struct iface_struct {
	char name[16];
	struct in_addr ip;
	struct in_addr netmask;
};
