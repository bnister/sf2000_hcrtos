/* dhcpc.c
 *
 * udhcp DHCP client
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#ifdef __linux__
    #include <sys/prctl.h>
#endif
#include <pthread.h>

#include "dhcpd.h"
#include "dhcpc.h"
#include "options.h"
#include "clientpacket.h"
#include "packet.h"
#include "script.h"
#include "socket.h"
#include "debug.h"
#include "pidfile.h"

#include "hccast_dhcpd.h"

static int state;
//static unsigned long requested_ip; /* = 0 */
static unsigned long server_addr;
static unsigned long timeout;
static int packet_num; /* = 0 */
static int dhcpc_fd = -1;

#define LISTEN_NONE 0
#define LISTEN_KERNEL 1
#define LISTEN_RAW 2
static int listen_mode;

#if RUN_SCRIPT
    #define DEFAULT_SCRIPT  "/usr/share/udhcpc/default.script"
#endif

#ifdef HC_RTOS
#define UDHCPC_NONE_PORT        (5011)
#define UDHCPC_WLAN0_PORT       (5012)
#define UDHCPC_WLAN1_PORT       (5013)
#define UDHCPC_P2P_PORT         (5014)

static int udhcpc_posix_signal_init(udhcp_conf_t *udhcpd_conf)
{
    struct sockaddr_in addr;
    int reuseaddr = 1;
    int fd = 0;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        LOG(LOG_ERR, "%s Could not socket", __func__);
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

    addr.sin_family = AF_INET;

    if (UDHCP_IF_NONE == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPC_NONE_PORT);
    }
    else if (UDHCP_IF_WLAN0 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPC_WLAN0_PORT);
    }
    else if (UDHCP_IF_WLAN1 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPC_WLAN1_PORT);
    }
    else if (UDHCP_IF_P2P0 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPC_P2P_PORT);
    }

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        LOG(LOG_ERR, "%s Could not bind: %s", __func__, strerror(errno));
        close(fd);
        return -1;
    }

    if (listen(fd, 1) != 0)
    {
        LOG(LOG_ERR, "%s Could not listen: %s", __func__, strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static int udhcpc_posix_send_signal(udhcp_conf_t *udhcpd_conf)
{
    struct sockaddr_in addr;
    int sig = SIGTERM;
    int fd = 0;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        LOG(LOG_ERR, "%s Could not socket", __func__);
        return -1;
    }

    addr.sin_family = AF_INET;
    if (UDHCP_IF_NONE == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPC_NONE_PORT);
    }
    else if (UDHCP_IF_WLAN0 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPC_WLAN0_PORT);
    }
    else if (UDHCP_IF_WLAN1 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPC_WLAN1_PORT);
    }
    else if (UDHCP_IF_P2P0 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPC_P2P_PORT);
    }

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        LOG(LOG_ERR, "%s Could not connect: %s", __func__, strerror(errno));
        close(fd);
        return -1;
    }

#if 0
    if (send(fd, &sig, sizeof(sig), MSG_DONTWAIT) < 0)
    {
        LOG(LOG_ERR, "%s Could not send signal: %s", __func__, strerror(errno));
        close(fd);
        return -1;
    }
#endif
    close(fd);
}
#endif

#if 0
struct client_config_t client_config =
{
    /* Default options. */
    abort_if_no_lease: 0,
    foreground: 0,
    quit_after_lease: 0,
    background_if_no_lease: 0,
    interface: "eth0",
        pidfile: NULL,
        script: DEFAULT_SCRIPT,
        clientid: NULL,
        hostname: NULL,
        ifindex: 0,
        arp: "\0\0\0\0\0\0",        /* appease gcc-3.0 */
    };
#else
struct client_config_t g_client_config =
{
    /* Default options. */
    abort_if_no_lease: 0,
    foreground: 0,
    quit_after_lease: 0,
    background_if_no_lease: 0,
    interface: "eth0",
        pidfile: NULL,
#if RUN_SCRIPT
        script: DEFAULT_SCRIPT,
#endif
        clientid: NULL,
        hostname: NULL,
        ifindex: 0,
        arp: "\0\0\0\0\0\0",        /* appease gcc-3.0 */
    };
#endif

#ifndef BB_VER
static void show_usage(void)
{
    printf(
        "Usage: udhcpc [OPTIONS]\n\n"
        "  -c, --clientid=CLIENTID         Client identifier\n"
        "  -H, --hostname=HOSTNAME         Client hostname\n"
        "  -h                              Alias for -H\n"
        "  -f, --foreground                Do not fork after getting lease\n"
        "  -b, --background                Fork to background if lease cannot be\n"
        "                                  immediately negotiated.\n"
        "  -i, --interface=INTERFACE       Interface to use (default: eth0)\n"
        "  -n, --now                       Exit with failure if lease cannot be\n"
        "                                  immediately negotiated.\n"
        "  -p, --pidfile=file              Store process ID of daemon in file\n"
        "  -q, --quit                      Quit after obtaining lease\n"
        "  -r, --request=IP                IP address to request (default: none)\n"
        "  -s, --script=file               Run file at dhcp events (default:\n"
#if RUN_SCRIPT
        "                                  " DEFAULT_SCRIPT ")\n"
#endif
        "  -v, --version                   Display version\n"
    );
    exit(0);
}
#endif


/* just a little helper */
static void change_mode(int new_mode)
{
    DEBUG(LOG_INFO, "entering %s listen mode",
          new_mode ? (new_mode == 1 ? "kernel" : "raw") : "none");
    if(dhcpc_fd > 0)
    {
        close(dhcpc_fd);
        dhcpc_fd = -1;
    }    
    listen_mode = new_mode;
}


/* perform a renew */
static void perform_renew(void)
{
    LOG(LOG_INFO, "Performing a DHCP renew");
    switch (state)
    {
    case BOUND:
        change_mode(LISTEN_KERNEL);
    case RENEWING:
    case REBINDING:
        state = RENEW_REQUESTED;
        break;
    case RENEW_REQUESTED: /* impatient are we? fine, square 1 */
#if RUN_SCRIPT
        run_script(NULL, "deconfig");
#endif
    case REQUESTING:
    case RELEASED:
        change_mode(LISTEN_RAW);
        state = INIT_SELECTING;
        break;
    case INIT_SELECTING:
        break;
    }

    /* start things over */
    packet_num = 0;

    /* Kill any timeouts because the user wants this to hurry along */
    timeout = 0;
}


/* perform a release */
static void perform_release(struct client_config_t *client_config, unsigned long requested_ip)
{
    char buffer[16];
    struct in_addr temp_addr;

    /* send release packet */
    if (state == BOUND || state == RENEWING || state == REBINDING)
    {
        temp_addr.s_addr = server_addr;
        sprintf(buffer, "%s", inet_ntoa(temp_addr));
        temp_addr.s_addr = requested_ip;
        LOG(LOG_INFO, "Unicasting a release of %s to %s", inet_ntoa(temp_addr), buffer);
        send_release(client_config, server_addr, requested_ip); /* unicast */
#if RUN_SCRIPT
        run_script(NULL, "deconfig");
#endif
    }
    LOG(LOG_INFO, "Entering released state");

    change_mode(LISTEN_NONE);
    state = RELEASED;
    timeout = 0x7fffffff;
}


/* Signal handler */
static void signal_handler(int pipe, int sig)
{
    if (pipe > 0, send(pipe, &sig, sizeof(sig), MSG_DONTWAIT) < 0)
    {
        LOG(LOG_ERR, "Could not send signal: %s", strerror(errno));
    }
}

/* Fill dest with the text of option 'option'. */
void fill_options(char *dest, unsigned char *option, struct dhcp_option *type_p)
{
    int type, optlen;
    u_int16_t val_u16;
    int16_t val_s16;
    u_int32_t val_u32;
    int32_t val_s32;
    int len = option[OPT_LEN - 2];

    //dest += sprintf(dest, "%s=", type_p->name);

    type = type_p->flags & TYPE_MASK;
    optlen = option_lengths[type];
    for (;;)
    {
        switch (type)
        {
        case OPTION_IP_PAIR:
            //dest += sprintip(dest, "", option);
            dest += sprintf(dest, "%s%d.%d.%d.%d ", "", option[0], option[1], option[2], option[3]);
            *(dest++) = '/';
            option += 4;
            optlen = 4;
        case OPTION_IP: /* Works regardless of host byte order. */
            //dest += sprintip(dest, "", option);
            if (strstr(type_p->name, "dns"))
            {
                dest += sprintf(dest, "%s%d.%d.%d.%d ", "", option[0], option[1], option[2], option[3]);
            }
            else
            {
                dest += sprintf(dest, "%s%d.%d.%d.%d", "", option[0], option[1], option[2], option[3]);
            }
            break;
        case OPTION_BOOLEAN:
            dest += sprintf(dest, *option ? "yes " : "no ");
            break;
        case OPTION_U8:
            dest += sprintf(dest, "%u ", *option);
            break;
        case OPTION_U16:
            memcpy(&val_u16, option, 2);
            dest += sprintf(dest, "%u ", ntohs(val_u16));
            break;
        case OPTION_S16:
            memcpy(&val_s16, option, 2);
            dest += sprintf(dest, "%d ", ntohs(val_s16));
            break;
        case OPTION_U32:
            memcpy(&val_u32, option, 4);
            dest += sprintf(dest, "%lu ", (unsigned long) ntohl(val_u32));
            break;
        case OPTION_S32:
            memcpy(&val_s32, option, 4);
            dest += sprintf(dest, "%ld ", (long) ntohl(val_s32));
            break;
        case OPTION_STRING:
            memcpy(dest, option, len);
            dest[len] = '\0';
            return;  /* Short circuit this case */
        }
        option += optlen;
        len -= optlen;
        if (len <= 0) break;
    }
}

static void *udhcpc_main(void *arg)
{
    unsigned char *temp = NULL, *message = NULL;
    unsigned long t1 = 0, t2 = 0, xid = 0;
    unsigned long start = 0, lease = 0;
    unsigned long requested_ip = 0;
    unsigned long requested_ip_bak = 0;

    fd_set rfds;
    int retval = 0;
    struct timeval tv;
    int c = 0, len = 0;
    struct dhcpMessage packet = {0};
    struct in_addr temp_addr = {0};
    int pid_fd = 0;
    time_t now = 0;
    int max_fd = 0;
    int sig = 0;
    int signal_pipe[2] = {0};

    udhcp_conf_t *conf = (udhcp_conf_t *) arg;

    //OPEN_LOG("udhcpc");
    //LOG(LOG_INFO, "udhcp client (v%s) started", VERSION);

    struct client_config_t *client_config = calloc(1, sizeof(struct client_config_t));
    memcpy(client_config, &g_client_config, sizeof(struct client_config_t));

    if (conf && UDHCP_IF_WLAN0 == conf->ifname)
    {
#ifdef __linux__
        prctl(PR_SET_NAME, "udhcpc_wl0");
#endif
        client_config->interface = "wlan0";
    }
    else if (conf && UDHCP_IF_WLAN1 == conf->ifname)
    {
#ifdef __linux__
        prctl(PR_SET_NAME, "udhcpc_wl1");
#endif
        client_config->interface = "wlan1";
    }
    else if (conf && UDHCP_IF_P2P0 == conf->ifname)
    {
#ifdef __linux__
        prctl(PR_SET_NAME, "udhcpc_p2p");
#endif
        client_config->interface = "p2p0";
    }

#if 0
    if (conf->req_ip)
    {
        requested_ip = htonl(conf->req_ip);
    }
#endif

    if (conf->option & UDHCPC_ABORT_IF_NO_LEASE)
    {
        client_config->abort_if_no_lease = 1;
    }

    if (conf->option & UDHCPC_FOREGROUND)
    {
        client_config->foreground = 1;
    }

    if (conf->option & UDHCPC_QUIT_AFTER_LEASE)
    {
        client_config->quit_after_lease = 1;
    }

    if (conf->option & UDHCPC_BACKGROUND_IF_NO_LEASE)
    {
        client_config->background_if_no_lease = 1;
    }

#ifdef __linux__
    pid_fd = pidfile_acquire(client_config->pidfile);
    pidfile_write_release(pid_fd);
#endif

    if (read_interface(client_config->interface, &(client_config->ifindex), NULL, client_config->arp) < 0)
    {
        //exit_client(1);
        LOG(LOG_INFO, "[UDHCPC][ERR] [%s][%d] : read_interface failed!\n", __func__, __LINE__);
        goto EXIT1;
        //return NULL;
    }

    if (!client_config->clientid)
    {
        client_config->clientid = xmalloc(6 + 3);
        client_config->clientid[OPT_CODE]   = DHCP_CLIENT_ID;
        client_config->clientid[OPT_LEN]    = 7;
        client_config->clientid[OPT_DATA]   = 1;
        memcpy(client_config->clientid + 3, client_config->arp, 6);
    }

#ifdef __linux__
    /* setup signal handlers */
    socketpair(AF_UNIX, SOCK_STREAM, 0, signal_pipe);
    conf->stop_pipe = signal_pipe[1];
#else
    signal_pipe[0] = udhcpc_posix_signal_init(conf);
    if (signal_pipe[0] < 0)
    {
        goto EXIT2;
    }
#endif

#if 0
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGTERM, signal_handler);
#endif

    state = INIT_SELECTING;
#if RUN_SCRIPT
    run_script(NULL, "deconfig");
#endif
    change_mode(LISTEN_RAW);

    timeout = 0;

    //for (;;)
    while (conf->run)
    {
        tv.tv_sec = timeout - time(0);
        tv.tv_usec = 0;
        FD_ZERO(&rfds);

        if (!conf->run)
        {
            break;
        }

        if (listen_mode != LISTEN_NONE && dhcpc_fd < 0)
        {
            if (listen_mode == LISTEN_KERNEL)
            {
                dhcpc_fd = listen_socket(INADDR_ANY, CLIENT_PORT, client_config->interface);
            }
            else
            {
                dhcpc_fd = raw_socket(client_config->ifindex);
            }

            if (dhcpc_fd < 0)
            {
                LOG(LOG_ERR, "FATAL: couldn't listen on socket, %s", strerror(errno));
                //exit_client(0);
                goto EXIT1;
            }
        }

        if (dhcpc_fd >= 0) FD_SET(dhcpc_fd, &rfds);
        FD_SET(signal_pipe[0], &rfds);

        if (tv.tv_sec > 0)
        {
            DEBUG(LOG_INFO, "Waiting on select...\n");
            max_fd = signal_pipe[0] > dhcpc_fd ? signal_pipe[0] : dhcpc_fd;
            retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
        }
        else retval = 0;   /* If we already timed out, fall through */

        now = time(0);
        if (retval == 0)
        {
            /* timeout dropped to zero */
            switch (state)
            {
            case INIT_SELECTING:
                if (packet_num < 3)
                {
                    if (packet_num == 0)
                    {
                        xid = random_xid();
                    }

                    /* send discover packet */
                    send_discover(client_config, xid, requested_ip); /* broadcast */

                    timeout = now + ((packet_num == 2) ? 4 : 2);
                    packet_num++;
                }
                else
                {
                    /* wait to try again */
                    packet_num = 0;
                    timeout = now + 60;

                    if (client_config->background_if_no_lease)
                    {
                        LOG(LOG_INFO, "No lease, forking to background.");
                        //background();
                    }
                    else if (client_config->abort_if_no_lease)
                    {
                        LOG(LOG_INFO, "No lease, failing.");
                        //exit_client(1);
                        goto EXIT1;
                    }
                }
                break;
            case RENEW_REQUESTED:
            case REQUESTING:
                if (packet_num < 3)
                {
                    /* send request packet */
                    if (state == RENEW_REQUESTED)
                    {
                        send_renew(client_config, xid, server_addr, requested_ip); /* unicast */
                    }
                    else
                    {
                        send_selecting(client_config, xid, server_addr, requested_ip); /* broadcast */
                    }

                    timeout = now + ((packet_num == 2) ? 10 : 2);
                    packet_num++;
                }
                else
                {
                    /* timed out, go back to init state */
#if RUN_SCRIPT
                    if (state == RENEW_REQUESTED) run_script(NULL, "deconfig");
#endif
                    state = INIT_SELECTING;
                    timeout = now;
                    packet_num = 0;
                    change_mode(LISTEN_RAW);
                    //goto EXIT1;
                }
                break;
            case BOUND:
                /* Lease is starting to run out, time to enter renewing state */
                state = RENEWING;
                change_mode(LISTEN_KERNEL);
                DEBUG(LOG_INFO, "Entering renew state");
            /* fall right through */
            case RENEWING:
                /* Either set a new T1, or enter REBINDING state */
                if ((t2 - t1) <= (lease / 14400 + 1))
                {
                    /* timed out, enter rebinding state */
                    state = REBINDING;
                    timeout = now + (t2 - t1);
                    DEBUG(LOG_INFO, "Entering rebinding state");
                }
                else
                {
                    /* send a request packet */
                    send_renew(client_config, xid, server_addr, requested_ip); /* unicast */

                    t1 = (t2 - t1) / 2 + t1;
                    timeout = t1 + start;
                }
                break;
            case REBINDING:
                /* Either set a new T2, or enter INIT state */
                if ((lease - t2) <= (lease / 14400 + 1))
                {
                    /* timed out, enter init state */
                    state = INIT_SELECTING;
                    LOG(LOG_INFO, "Lease lost, entering init state");

#if RUN_SCRIPT
                    run_script(NULL, "deconfig");
#endif
                    timeout = now;
                    packet_num = 0;
                    change_mode(LISTEN_RAW);
                    //goto EXIT1;
                }
                else
                {
                    /* send a request packet */
                    send_renew(client_config, xid, 0, requested_ip); /* broadcast */

                    t2 = (lease - t2) / 2 + t2;
                    timeout = t2 + start;
                }
                break;
            case RELEASED:
                /* yah, I know, *you* say it would never happen */
                timeout = 0x7fffffff;
                break;
            }
        }
        else if (retval > 0 && listen_mode != LISTEN_NONE && FD_ISSET(dhcpc_fd, &rfds))
        {
            /* a packet is ready, read it */
            if (listen_mode == LISTEN_KERNEL)
            {
                len = get_packet(&packet, dhcpc_fd);
            }
            else
            {
                len = get_raw_packet(&packet, dhcpc_fd);
            }

            if (len == -1 && errno != EINTR)
            {
                DEBUG(LOG_INFO, "error on read, %s, reopening socket", strerror(errno));
                change_mode(listen_mode); /* just close and reopen */
            }

            if (len < 0)
            {
                continue;
            }

            if (packet.xid != xid)
            {
                DEBUG(LOG_INFO, "Ignoring XID %lx (our xid is %lx)", (unsigned long) packet.xid, xid);
                continue;
            }

            if ((message = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL)
            {
                DEBUG(LOG_ERR, "couldnt get option from packet -- ignoring");
                continue;
            }

            switch (state)
            {
            case INIT_SELECTING:
                /* Must be a DHCPOFFER to one of our xid's */
                if (*message == DHCPOFFER)
                {
                    if ((temp = get_option(&packet, DHCP_SERVER_ID)))
                    {
                        memcpy(&server_addr, temp, 4);
                        xid = packet.xid;
                        requested_ip = packet.yiaddr;

                        /* enter requesting state */
                        state = REQUESTING;
                        timeout = now;
                        packet_num = 0;
                    }
                    else
                    {
                        DEBUG(LOG_ERR, "No server ID in message");
                    }
                }
                break;
            case RENEW_REQUESTED:
            case REQUESTING:
            case RENEWING:
            case REBINDING:
                if (*message == DHCPACK)
                {
                    if (!(temp = get_option(&packet, DHCP_LEASE_TIME)))
                    {
                        LOG(LOG_ERR, "No lease time with ACK, using 1 hour lease");
                        lease = 60 * 60;
                    }
                    else
                    {
                        memcpy(&lease, temp, 4);
                        lease = ntohl(lease);
                    }

                    /* enter bound state */
                    t1 = lease / 2;

                    /* little fixed point for n * .875 */
                    t2 = (lease * 0x7) >> 3;
                    temp_addr.s_addr = packet.yiaddr;

                    start = now;
                    timeout = t1 + start;

                    if (requested_ip == requested_ip_bak)
                    {
                        state = BOUND;
                        change_mode(LISTEN_NONE);
                        break;
                    }

                    requested_ip = packet.yiaddr;
                    LOG(LOG_INFO, "Lease of %s obtained, lease time %ld", inet_ntoa(temp_addr), lease);

#if RUN_SCRIPT
                    run_script(&packet, ((state == RENEWING || state == REBINDING) ? "renew" : "bound"));
#else

                    hccast_udhcp_result_t result = {0};
                    result.stat = 1;
                    strncpy(result.ip, inet_ntoa(temp_addr), sizeof(result.ip));
                    strncpy(result.ifname, client_config->interface, sizeof(result.ifname));

                    for (int i = 0, j = 4; options[i].code; i++)
                    {
                        if ((temp = get_option(&packet, options[i].code)))
                        {
                            //fill_options(envp[j], temp, &options[i]);
                            if (!strcmp(options[i].name, "subnet"))
                            {
                                fill_options(result.mask, temp, &options[i]);
                            }
                            else if (!strcmp(options[i].name, "router"))
                            {
                                fill_options(result.gw, temp, &options[i]);
                            }
                            else if (!strcmp(options[i].name, "dns"))
                            {
                                fill_options(result.dns, temp, &options[i]);
                            }

                            j++;
                        }
                    }

                    if (conf->func)
                    {
                        conf->func((unsigned int)&result);
                    }
#endif
                    state = BOUND;
                    change_mode(LISTEN_NONE);
                    if (client_config->quit_after_lease)
                    {
                        goto EXIT2;
                        //exit_client(0);
                    }

                    //if (!client_config->foreground)
                    //    background();
                    requested_ip_bak = requested_ip;
                }
                else if (*message == DHCPNAK)
                {
                    /* return to init state */
                    LOG(LOG_INFO, "Received DHCP NAK");
#if RUN_SCRIPT
                    run_script(&packet, "nak");
                    if (state != REQUESTING)
                    {
                        run_script(NULL, "deconfig");
                    }
#endif
                    state = INIT_SELECTING;
                    timeout = now;
                    requested_ip = 0;
                    packet_num = 0;
                    change_mode(LISTEN_RAW);
                    sleep(3); /* avoid excessive network traffic */

                    //goto EXIT1;
                }
                break;
                /* case BOUND, RELEASED: - ignore all packets */
            }
        }
        else if (retval > 0 && FD_ISSET(signal_pipe[0], &rfds))
        {
            //here just mean stop client is connected, but we not need accept and just return.
            goto EXIT2;

            if (read(signal_pipe[0], &sig, sizeof(signal)) < 0)
            {
                DEBUG(LOG_ERR, "Could not read signal: %s", strerror(errno));
                continue; /* probably just EINTR */
            }
            switch (sig)
            {
            case SIGUSR1:
                perform_renew();
                break;
            case SIGUSR2:
                perform_release(client_config, requested_ip);
                break;
            case SIGTERM:
                LOG(LOG_INFO, "Received SIGTERM");
            default:
                //exit_client(0);
                goto EXIT2;
            }
        }
        else if (retval == -1 && errno == EINTR)
        {
            /* a signal was caught */
        }
        else
        {
            /* An error occured */
            DEBUG(LOG_ERR, "Error on select");
        }
    }

EXIT1:
    {
        hccast_udhcp_result_t result = {0};
        if (conf->func)
        {
            conf->func((unsigned int)&result);
        }
    }

EXIT2:
    {
        conf->run = 0;
        conf->pid = 0;
    }

    if (client_config)
    {
        free(client_config);
        client_config = NULL;
    }

    LOG(LOG_INFO, "udhcpc end!");

    if (signal_pipe[0] > 0)
    {
        close(signal_pipe[0]);
    }

    if (signal_pipe[1] > 0)
    {
        close(signal_pipe[1]);
    }

    if (dhcpc_fd > 0)
    {
        close(dhcpc_fd);
        dhcpc_fd = -1;
    }

    return NULL;
}

pthread_mutex_t g_udhcpc_mutex = PTHREAD_MUTEX_INITIALIZER;

int udhcpc_start(udhcp_conf_t *conf)
{
    pthread_mutex_lock(&g_udhcpc_mutex);
    if (conf && 0 == conf->run && 0 == conf->pid)
    {
        conf->run = 1;

        if (pthread_create((pthread_t *)&conf->pid, NULL, udhcpc_main, conf) != 0)
        {
            perror("udhcpd_main create failed!");
            pthread_mutex_unlock(&g_udhcpc_mutex);
            return -1;
        }
        pthread_detach(conf->pid);

        pthread_mutex_unlock(&g_udhcpc_mutex);
        return 0;
    }

    pthread_mutex_unlock(&g_udhcpc_mutex);
    return -1;
}

int udhcpc_stop(udhcp_conf_t *conf)
{
    pthread_mutex_lock(&g_udhcpc_mutex);
    if (conf && conf->run && conf->pid)
    {
        conf->run = 0;
        conf->pid = 0;
#ifdef __linux__
        signal_handler(conf->stop_pipe, SIGUSR1);
#else
        udhcpc_posix_send_signal(conf);
#endif
        pthread_mutex_unlock(&g_udhcpc_mutex);
        return 0;
    }

    pthread_mutex_unlock(&g_udhcpc_mutex);
    return -1;
}
