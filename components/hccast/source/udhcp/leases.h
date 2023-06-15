/* leases.h */
#ifndef _LEASES_H
#define _LEASES_H


struct dhcpOfferedAddr
{
    u_int8_t chaddr[16];
    u_int32_t yiaddr;   /* network order */
    u_int32_t expires;  /* host order */
};

extern unsigned char blank_chaddr[];

//void clear_lease(u_int8_t *chaddr, u_int32_t yiaddr, struct server_config_t *server_config);
//struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, unsigned long lease, struct server_config_t *server_config);
int lease_expired(struct dhcpOfferedAddr *lease);
//struct dhcpOfferedAddr *oldest_expired_lease(struct server_config_t *server_config);
//struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr, struct server_config_t *server_config);
//struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr, struct server_config_t *server_config);
//u_int32_t find_address(int check_expired, struct server_config_t *server_config);
//int check_ip(u_int32_t addr, struct server_config_t *server_config);


#endif
