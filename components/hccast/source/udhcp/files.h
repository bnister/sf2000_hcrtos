/* files.h */
#ifndef _FILES_H
#define _FILES_H

struct config_keyword
{
    char keyword[14];
    int (*handler)(char *line, void *var);
    void *var;
    char def[32];
};

int load_default_config();
int load_hostapd_config();
int load_p2p_config();
int read_config(char *file);
void write_leases(struct server_config_t *server_config);
void read_leases(char *file, struct server_config_t *server_config);
int read_ip(char *line, void *arg);

#endif
