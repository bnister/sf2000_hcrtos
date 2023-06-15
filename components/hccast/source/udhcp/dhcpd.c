/* dhcpd.c
 *
 * udhcp Server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *          Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
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

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#ifndef HC_RTOS
#include <sys/prctl.h>
#endif

#include "debug.h"
#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "options.h"
#include "files.h"
#include "leases.h"
#include "packet.h"
#include "serverpacket.h"
#include "pidfile.h"
#include "hccast_dhcpd.h"

/* globals */
struct dhcpOfferedAddr *leases;
struct server_config_t g_hs_server_config;
struct server_config_t g_p2p_server_config;
//static int signal_pipe[2];

/* Exit and cleanup */
#ifndef HC_RTOS
static void exit_server(int retval, struct server_config_t *server_config)
{
    pidfile_delete(server_config->pidfile);
    CLOSE_LOG();
    exit(retval);
}
#endif

/* Signal handler */
#ifdef __linux__
static void signal_handler(int pipe,int sig)
{
    if (pipe > 0 && send(pipe, &sig, sizeof(sig), MSG_DONTWAIT) < 0)
    {
        LOG(LOG_ERR, "Could not send signal: %s", strerror(errno));
    }
}
#endif

#ifdef HC_RTOS

#define UDHCPD_NONE_PORT        (5001)
#define UDHCPD_WLAN0_PORT       (5002)
#define UDHCPD_WLAN1_PORT       (5003)
#define UDHCPD_P2P_PORT         (5004)

static int udhcpd_posix_signal_init(udhcp_conf_t *udhcpd_conf)
{
    struct sockaddr_in addr;
    int reuseaddr = 1;
    int fd = 0;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        LOG(LOG_ERR, "%s Could not socket",__func__);
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof (reuseaddr));

    addr.sin_family = AF_INET;

    if (UDHCP_IF_NONE == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPD_NONE_PORT);
    }
    else if (UDHCP_IF_WLAN0 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPD_WLAN0_PORT);
    }
    else if (UDHCP_IF_WLAN1 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPD_WLAN1_PORT);
    }
    else if (UDHCP_IF_P2P0 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPD_P2P_PORT);
    }

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        LOG(LOG_ERR, "%s Could not bind: %s",__func__,strerror(errno));
        close(fd);
        return -1;
    }

    if(listen(fd, 1) != 0)
    {
        LOG(LOG_ERR, "%s Could not listen: %s",__func__,strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

int udhcpd_posix_send_signal(udhcp_conf_t *udhcpd_conf)
{
    struct sockaddr_in addr;
    int sig = SIGTERM;
    int fd = 0;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        LOG(LOG_ERR, "%s Could not socket",__func__);
        return -1;
    }

    addr.sin_family = AF_INET;
    if (UDHCP_IF_NONE == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPD_NONE_PORT);
    }
    else if (UDHCP_IF_WLAN0 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPD_WLAN0_PORT);
    }
    else if (UDHCP_IF_WLAN1 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPD_WLAN1_PORT);
    }
    else if (UDHCP_IF_P2P0 == udhcpd_conf->ifname)
    {
        addr.sin_port = htons(UDHCPD_P2P_PORT);
    }

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        LOG(LOG_ERR, "%s Could not connect: %s",__func__,strerror(errno));
        close(fd);
        return -1;
    }
#if 0
    if (send(fd, &sig, sizeof(sig), MSG_DONTWAIT) < 0)
    {
        LOG(LOG_ERR, "%s Could not send signal: %s",__func__,strerror(errno));
        close(fd);
        return -1;
    }
#endif
    close(fd);
}

#endif

//static bool udhcpd_run = false;
static void* udhcpd_main(void* arg)
{
    fd_set rfds;
    struct timeval tv;
    int server_socket = -1;
    int bytes, retval;
    struct dhcpMessage packet;
    unsigned char *state;
    unsigned char *server_id, *requested;
    u_int32_t server_id_align, requested_align;
    unsigned long timeout_end;
    struct option_set *option;
    struct dhcpOfferedAddr *lease;
    int pid_fd;
    int max_sock;
    int sig;
    int signal_pipe[2] = {0};

    struct server_config_t *server_config = NULL;

    //OPEN_LOG("udhcpd");
    LOG(LOG_INFO, "udhcp server (v0.9.8) started");

    //udhcpd_run = true;
    udhcp_conf_t *udhcpd_conf = (udhcp_conf_t*) arg;

#if 0
    if (argc < 2)
        read_config(DHCPD_CONF_FILE);
    else read_config(argv[1]);
#else
    if (UDHCP_IF_WLAN0 == udhcpd_conf->ifname)
    {
#ifndef HC_RTOS
        prctl(PR_SET_NAME, "udhcpd_hs");
#endif
        load_hostapd_config();

        int ret = 1;
        if (strlen(udhcpd_conf->ip_start_def) && strlen(udhcpd_conf->ip_end_def))
        {
            ret &= read_ip(udhcpd_conf->ip_start_def, &g_hs_server_config.start);
            ret &= read_ip(udhcpd_conf->ip_end_def, &g_hs_server_config.end);
        }

        if (!ret)
        {
            LOG(LOG_INFO, "ip param error, use default");
            load_hostapd_config();
        }

        server_config = &g_hs_server_config;
    }
    else if (UDHCP_IF_P2P0 == udhcpd_conf->ifname)
    {
#ifndef HC_RTOS
        prctl(PR_SET_NAME, "udhcpd_p2p");
#endif
        load_p2p_config();

        int ret = 1;
        if (strlen(udhcpd_conf->ip_start_def) && strlen(udhcpd_conf->ip_end_def))
        {
            ret &= read_ip(udhcpd_conf->ip_start_def, &g_p2p_server_config.start);
            ret &= read_ip(udhcpd_conf->ip_end_def, &g_p2p_server_config.end);
        }

        if (!ret)
        {
            LOG(LOG_INFO, "ip param error, use default");
            load_hostapd_config();
        }

        server_config = &g_p2p_server_config;
    }
    else
    {
#ifndef HC_RTOS
        prctl(PR_SET_NAME, "udhcpd");
#endif
        load_default_config();

        server_config = &g_p2p_server_config;
    }
#endif

#ifndef HC_RTOS
    pid_fd = pidfile_acquire(server_config->pidfile);
    pidfile_write_release(pid_fd);
#endif

    if ((option = find_option(server_config->options, DHCP_LEASE_TIME)))
    {
        memcpy(&server_config->lease, option->data + 2, 4);
        server_config->lease = ntohl(server_config->lease);
    }
    else server_config->lease = LEASE_TIME;

    leases = malloc(sizeof(struct dhcpOfferedAddr) * server_config->max_leases);
    memset(leases, 0, sizeof(struct dhcpOfferedAddr) * server_config->max_leases);
#ifndef HC_RTOS
    read_leases(server_config->lease_file, server_config);
#endif
    if (read_interface(server_config->interface, &server_config->ifindex,
                       &server_config->server, server_config->arp) < 0)
        goto EXIT;

#if 0
    pid_fd = pidfile_acquire(server_config->pidfile); /* hold lock during fork. */
    if (daemon(0, 0) == -1)
    {
        perror("fork");
        exit_server(1);
    }
    pidfile_write_release(pid_fd);
#endif

#ifdef __linux__
    socketpair(AF_UNIX, SOCK_STREAM, 0, signal_pipe);
    udhcpd_conf->stop_pipe = signal_pipe[1];
#else
    signal_pipe[0] = udhcpd_posix_signal_init(udhcpd_conf);
    if(signal_pipe[0] < 0)
    {
        goto EXIT;
    }
#endif

#if 0
    signal(SIGUSR1, signal_handler);
    signal(SIGTERM, signal_handler);
#endif
    timeout_end = time(0) + server_config->auto_time;
    while (udhcpd_conf->run)  /* loop until universe collapses */
    {
        if (server_socket < 0)
            if ((server_socket = listen_socket(INADDR_ANY, SERVER_PORT, server_config->interface)) < 0)
            {
                LOG(LOG_ERR, "FATAL: couldn't create server socket, %s", strerror(errno));
                //exit_server(0);
                goto EXIT;
            }

        FD_ZERO(&rfds);
        FD_SET(server_socket, &rfds);
        FD_SET(signal_pipe[0], &rfds);
        if (server_config->auto_time)
        {
            tv.tv_sec = timeout_end - time(0);
            tv.tv_usec = 200 * 1000;

            //printf("udhcpd: %ld %ld\n", tv.tv_sec, timeout_end);
        }
        if (!server_config->auto_time || tv.tv_sec > 0)
        {
            max_sock = server_socket > signal_pipe[0] ? server_socket : signal_pipe[0];
            retval = select(max_sock + 1, &rfds, NULL, NULL, server_config->auto_time ? &tv : NULL);
        }
        else retval = 0;   /* If we already timed out, fall through */

        if (retval == 0)
        {
        #ifdef __linux__
            write_leases(server_config);
        #endif
            timeout_end = time(0) + server_config->auto_time;
            continue;
        }
        else if (retval < 0 && errno != EINTR)
        {
            DEBUG(LOG_INFO, "error on select");
            continue;
        }

        if (FD_ISSET(signal_pipe[0], &rfds))
        {
            //here just mean stop client is connected, but we not need accept and just return.
            //LOG(LOG_INFO, "udhcpd ready to exit");
            goto EXIT;
#if 0
            if (read(signal_pipe[0], &sig, sizeof(sig)) < 0)
                continue; /* probably just EINTR */
            switch (sig)
            {
                case SIGUSR1:
                    LOG(LOG_INFO, "Received a SIGUSR1");
                #ifdef __linux__
                    write_leases(server_config);
                #endif
                    /* why not just reset the timeout, eh */
                    timeout_end = time(0) + server_config->auto_time;
                    continue;
                case SIGTERM:
                    LOG(LOG_INFO, "Received a SIGTERM");
                    //exit_server(0);
                    //return NULL;
                    goto EXIT;
            }
#endif
        }

        if ((bytes = get_packet(&packet, server_socket)) < 0)   /* this waits for a packet - idle */
        {
            if (bytes == -1 && errno != EINTR)
            {
                LOG(LOG_INFO, "error on read, %s, reopening socket", strerror(errno));
                close(server_socket);
                server_socket = -1;
            }
            continue;
        }

        if ((state = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL)
        {
            DEBUG(LOG_ERR, "couldn't get option from packet, ignoring");
            continue;
        }

        /* ADDME: look for a static lease */
        lease = find_lease_by_chaddr(packet.chaddr, server_config);
        switch (state[0])
        {
            case DHCPDISCOVER:
                LOG(LOG_INFO, "received DISCOVER");

                if (sendOffer(&packet, server_config) < 0)
                {
                    LOG(LOG_ERR, "send OFFER failed");
                }
                break;
            case DHCPREQUEST:
                LOG(LOG_INFO, "received REQUEST");

                requested = get_option(&packet, DHCP_REQUESTED_IP);
                server_id = get_option(&packet, DHCP_SERVER_ID);

                if (requested) memcpy(&requested_align, requested, 4);
                if (server_id) memcpy(&server_id_align, server_id, 4);

                printf("received REQUEST requested %x server_id = %x \n",
                       requested_align, server_id);

#if 1
                if (lease)   /*ADDME: or static lease */
                {
                    if (server_id)
                    {
                        /* SELECTING State */
                        DEBUG(LOG_INFO, "server_id = %08x", ntohl(server_id_align));
                        if (server_id_align == server_config->server && requested &&
                            requested_align == lease->yiaddr)
                        {
                            retval = sendACK(&packet, lease->yiaddr, server_config);

                            if (udhcpd_conf->func && !retval)
                            {
                                udhcpd_conf->func(lease->yiaddr);
                            }
                        }
                    }
                    else //not our packet.
                    {
                        sendNAK(&packet,server_config);
                    }
                }
                else //not in lease static table.need send NAK.
                {
                    sendNAK(&packet,server_config);
                }
#else
                if (lease)   /*ADDME: or static lease */
                {
                    if (server_id)
                    {
                        /* SELECTING State */
                        DEBUG(LOG_INFO, "server_id = %08x", ntohl(server_id_align));
                        if (server_id_align == server_config->server && requested &&
                            requested_align == lease->yiaddr)
                        {
                            retval = sendACK(&packet, lease->yiaddr, server_config);

                            if (udhcpd_conf->func && !retval)
                            {
                                udhcpd_conf->func(lease->yiaddr);
                            }
                        }
                    }
                    else
                    {
                        if (requested)
                        {
                            /* INIT-REBOOT State */
                            if (lease->yiaddr == requested_align)
                            {
                                retval = sendACK(&packet, lease->yiaddr, server_config);

                                if (udhcpd_conf->func && !retval)
                                {
                                    udhcpd_conf->func(lease->yiaddr);
                                }
                            }
                            else sendNAK(&packet,server_config);
                        }
                        else
                        {
                            /* RENEWING or REBINDING State */
                            if (lease->yiaddr == packet.ciaddr)
                            {
                                retval = sendACK(&packet, lease->yiaddr, server_config);

                                if (udhcpd_conf->func && !retval)
                                {
                                    udhcpd_conf->func(lease->yiaddr);
                                }
                            }
                            else
                            {
                                /* don't know what to do!!!! */
                                sendNAK(&packet,server_config);
                            }
                        }
                    }

                    /* what to do if we have no record of the client */
                }
                else if (server_id)
                {
                    /* SELECTING State */

                }
                else if (requested)
                {
                    /* INIT-REBOOT State */
                    if ((lease = find_lease_by_yiaddr(requested_align, server_config)))
                    {
                        if (lease_expired(lease))
                        {
                            /* probably best if we drop this lease */
                            memset(lease->chaddr, 0, 16);
                            /* make some contention for this address */
                        }
                        else sendNAK(&packet,server_config);//
                    }
                    else if (requested_align < server_config->start ||
                             requested_align > server_config->end)
                    {
                        sendNAK(&packet,server_config);
                    } /* else remain silent */

                }
                else
                {
                    /* RENEWING or REBINDING State */
                }
#endif
                break;
            case DHCPDECLINE:
                DEBUG(LOG_INFO, "received DECLINE");
                if (lease)
                {
                    memset(lease->chaddr, 0, 16);
                    lease->expires = time(0) + server_config->decline_time;
                }
                break;
            case DHCPRELEASE:
                DEBUG(LOG_INFO, "received RELEASE");
                if (lease) lease->expires = time(0);
                break;
            case DHCPINFORM:
                LOG(LOG_INFO, "received INFORM");
                send_inform(&packet, server_config);
                break;
            default:
                LOG(LOG_WARNING, "unsupported DHCP message (%02x) -- ignoring", state[0]);
        }
    }

EXIT:
    LOG(LOG_INFO, "udhcpd end!");

    if (signal_pipe[0] > 0)
        close(signal_pipe[0]);
    if (signal_pipe[1] > 0)
        close(signal_pipe[1]);
    if (server_socket)
        close(server_socket);

    udhcpd_conf->run = 0;
    udhcpd_conf->pid = 0;

    return 0;
}

pthread_mutex_t g_udhcpd_mutex = PTHREAD_MUTEX_INITIALIZER;

//static pthread_t g_udhcpd_tid = 0;
int udhcpd_start(udhcp_conf_t *conf)
{
    pthread_mutex_lock(&g_udhcpd_mutex);
    //printf("run: %d, pid: %d ##### \n", conf->run, conf->pid);
    if (conf && 0 == conf->run && 0 == conf->pid)
    {
        conf->run = 1;

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x4000);
        if (pthread_create((pthread_t*)&conf->pid, NULL, (void*)udhcpd_main, conf) != 0)
        {
            perror("udhcpd_main create failed!");
            pthread_mutex_unlock(&g_udhcpd_mutex);
            return -1;
        }
        pthread_detach(conf->pid);
        pthread_mutex_unlock(&g_udhcpd_mutex);
        return 0;
    }
    pthread_mutex_unlock(&g_udhcpd_mutex);

    return -1;
}

int udhcpd_stop(udhcp_conf_t *conf)
{
    pthread_mutex_lock(&g_udhcpd_mutex);
    if (conf && conf->run && conf->pid)
    {
        conf->run = 0;
        //conf->pid = 0;
#ifdef __linux__
        signal_handler(conf->stop_pipe, SIGUSR1);
#else
        udhcpd_posix_send_signal(conf);
#endif
        pthread_mutex_unlock(&g_udhcpd_mutex);
        return 0;
    }

    pthread_mutex_unlock(&g_udhcpd_mutex);
    return -1;
}