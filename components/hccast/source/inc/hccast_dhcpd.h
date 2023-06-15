#ifndef __HDCPD_SERVICE_H__
#define __HDCPD_SERVICE_H__

typedef void (*udhcp_lease_cb)(unsigned int yiaddr);

#define BIT(nr) (1UL << (nr))

// udhcp_conf_t option
typedef enum {
	UDHCPC_ABORT_IF_NO_LEASE         = BIT(0),
	UDHCPC_FOREGROUND                = BIT(1), // unsupported
	UDHCPC_QUIT_AFTER_LEASE          = BIT(2),
	UDHCPC_BACKGROUND_IF_NO_LEASE    = BIT(3), // unsupported
} udhcp_option_e;

typedef enum
{
    UDHCP_IF_NONE = 0,
    UDHCP_IF_WLAN0,
    UDHCP_IF_WLAN1,
    UDHCP_IF_P2P0,
} udhcp_ifname_e;

typedef struct
{
    char stat;
    char res[7];
    char ip[16];
    char mask[16];
    char gw[16];
    char dns[34];
    char ifname[32];
} hccast_udhcp_result_t;

typedef struct
{
    udhcp_lease_cb func;        // in
    udhcp_ifname_e ifname;      // in
    int pid;                    // out, dont set value.
    int run;                    // out, dont set value.
#ifdef __linux__
    int stop_pipe;              // out, dont set value.
#endif
    unsigned int option;        // in
    // unsigned long req_ip;    // Reserved, use only dhcpc, IP address to request (default: none)
    char ip_start_def[32];      // in, only support udhcpd
    char ip_end_def[32];        // in, only support udhcpd
    char ip_host_def[32];       // in, only support udhcpd
} udhcp_conf_t;

#ifdef __cplusplus
extern "C" {
#endif

int udhcpd_start(udhcp_conf_t *conf);
int udhcpd_stop(udhcp_conf_t *conf);
int udhcpc_start(udhcp_conf_t *conf);
int udhcpc_stop(udhcp_conf_t *conf);

#ifdef __cplusplus
}
#endif

#endif
