#ifndef _CLIENTPACKET_H
#define _CLIENTPACKET_H

#include "dhcpc.h"

unsigned long random_xid(void);
int send_discover(struct client_config_t *client_config, unsigned long xid, unsigned long requested);
int send_selecting(struct client_config_t *client_config, unsigned long xid, unsigned long server, unsigned long requested);
int send_renew(struct client_config_t *client_config, unsigned long xid, unsigned long server, unsigned long ciaddr);
int send_release(struct client_config_t *client_config, unsigned long server, unsigned long ciaddr);
int get_raw_packet(struct dhcpMessage *payload, int fd);

#endif
