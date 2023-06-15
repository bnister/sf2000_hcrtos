#ifndef _SERVERPACKET_H
#define _SERVERPACKET_H


int sendOffer(struct dhcpMessage *oldpacket, struct server_config_t *server_config);
int sendNAK(struct dhcpMessage *oldpacket,struct server_config_t *server_config);
int sendACK(struct dhcpMessage *oldpacket, u_int32_t yiaddr, struct server_config_t * server_config);
int send_inform(struct dhcpMessage *oldpacket, struct server_config_t *server_config);


#endif
