
#ifndef __TS27010_NET_H__
#define __TS27010_NET_H__

#include <linux/netdevice.h>

struct ts27010_netconfig {
	char if_name[IFNAMSIZ];	
};

#define TS27010IOC_ENABLE_NET _IOW('m', 4, struct ts27010_netconfig)
#define TS27010IOC_DISABLE_NET _IO('m', 5)

struct dlci_struct;
struct ts27010_netconfig;

void ts27010_mux_rx_netchar(struct dlci_struct *dlci,
	 unsigned char *in_buf, int size);

void ts27010_destroy_network(struct dlci_struct *dlci);

int ts27010_create_network(struct dlci_struct *dlci, struct ts27010_netconfig *nc);

#endif 
