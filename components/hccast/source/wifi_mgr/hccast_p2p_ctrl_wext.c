#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <wpa_ctrl.h>
#include <iwlib.h>

#include <miracast/miracast_api.h>

#include "sys/unistd.h"
#include "hccast_p2p_ctrl.h"
#include "hccast_dhcpd.h"
#include "hccast_p2p_ctrl.h"
#include "hccast_wifi_ctrl.h"
#include "hccast_wifi_mgr.h"
#include "hccast_mira.h"
#include "hccast_net.h"
#include "hccast_log.h"

extern int iwpriv_main(int argc, char *argv[]);

#define KILO    1e3
#define MEGA    1e6
#define GIGA    1e9

#define BUFFER_LEN (1024)

static pthread_t g_p2p_tid = 0;
static pthread_mutex_t g_p2p_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_p2p_thread_running = false;
static bool g_p2p_enable = false;
struct in_addr g_ip_addr;
static p2p_param_st g_mira_param;
static bool g_p2p_connect = false;
static bool g_p2p_connected = false;
static bool g_p2p_wps_pbc = false;
#define P2P_LISTEN_TIMEOUT (300000000LL)
#define max_args 10

#define P2P_MAX_NETWORK_LIST (10)

typedef struct _p2p_network_st_
{
    char bssid[WIFI_MAX_SSID_LEN];
    char mac[WIFI_MAX_SSID_LEN];
    char ssid[WIFI_MAX_SSID_LEN];
    char psk[WIFI_MAX_SSID_LEN];
    char pairwise[WIFI_MAX_PWD_LEN];
    unsigned long last_time;
} p2p_network_st;

typedef struct _p2p_network_list_st_
{
    p2p_network_st network[P2P_MAX_NETWORK_LIST];
} p2p_network_list_st;

p2p_network_list_st g_p2p_network_list = {0};

static void udhcpd_cb(unsigned int yiaddr)
{
    g_ip_addr.s_addr = yiaddr;
    hccast_log(LL_INFO, "offer peer ip: %s\n", inet_ntoa(g_ip_addr));

    g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
    usleep(150 * 1000);
    hccast_net_ifconfig(P2P_CTRL_IFACE_NAME, HCCAST_P2P_IP, HCCAST_P2P_MASK, NULL);
    g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTED);
}

static void udhcpc_cb(unsigned int data)
{
    hccast_udhcp_result_t *in = (hccast_udhcp_result_t *)data;
    if (in && in->stat)
    {
        inet_aton(in->gw, &g_ip_addr);
        hccast_log(LL_INFO, "got local ip: %s\n", in->ip);
        hccast_log(LL_INFO, "got local gw: %s\n", in->gw);
        hccast_log(LL_INFO, "got peer  ip: %s\n", inet_ntoa(g_ip_addr));
        hccast_net_ifconfig(in->ifname, in->ip, in->mask, NULL);
        //hccast_net_set_if_updown(WIFI_CTRL_IFACE_NAME, HCCAST_NET_IF_DOWN);

        usleep(150 * 1000);
        g_p2p_connected = true;
        g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTED);
    }
    else if (!in->stat)
    {
        g_p2p_connected = false;
        g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECT_FAILED);
    }
}

udhcp_conf_t g_p2p_udhcpd_conf =
{
    .func   = udhcpd_cb,
    .ifname = UDHCP_IF_P2P0,
    .pid    = 0,
    .run    = 0
};

static udhcp_conf_t g_p2p_udhcpc_conf =
{
    .func   = udhcpc_cb,
    .ifname = UDHCP_IF_P2P0,
    .pid    = 0,
    .run    = 0,
    .option = UDHCPC_QUIT_AFTER_LEASE | UDHCPC_ABORT_IF_NO_LEASE,
};

int p2p_ctrl_udhcpc_start()
{
    udhcpc_start(&g_p2p_udhcpc_conf);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

int p2p_ctrl_udhcpc_stop()
{
    udhcpc_stop(&g_p2p_udhcpc_conf);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

static int64_t get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;
}

static int tokenize_cmd(char *cmd, char *argv[])
{
    char *pos;
    int argc = 0;

    pos = cmd;
    for (;;)
    {
        while (*pos == ' ')
            pos++;
        if (*pos == '\0')
            break;
        argv[argc] = pos;
        argc++;
        if (argc == max_args)
            break;
        if (*pos == '"')
        {
            char *pos2 = strrchr(pos, '"');
            if (pos2)
                pos = pos2 + 1;
        }
        while (*pos != '\0' && *pos != ' ')
            pos++;
        if (*pos == ' ')
            *pos++ = '\0';
    }

    return argc;
}

static int result_get(char *str, char *key, char *val, int val_len)
{
    if (NULL == str || NULL == key || NULL == val)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    hccast_log(LL_DEBUG, "query key = %s\n", key);
    char keys[64] = {0};
    char vals[256] = {0};
    char *token;
    char *saveptr;

    char *strs = strdup(str);
    token = strtok_r(strs, "\n", &saveptr);

    while (token != NULL)
    {
        sscanf(token, "%[^=]=%[^'\n']", keys, vals);

        if (!strcmp(key, keys))
        {
            //printf("keys: %s, vals: %s.\n", keys, vals);
            strncpy(val, vals, val_len);
            break;
        }

        token = strtok_r(NULL, "\n", &saveptr);
    }

    free(strs);

    return HCCAST_WIFI_ERR_NO_ERROR;
}


/************************* SETTING ROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 * Execute a private command on the interface
 */
static int
set_private_cmd(int     skfd,       /* Socket */
                char       *args[],     /* Command line args */
                int     count,      /* Args count */
                char       *ifname,     /* Dev name */
                char       *cmdname,    /* Command name */
                iwprivargs     *priv,       /* Private ioctl description */
                int     priv_num,       /* Number of descriptions */
                char *resp)     /* Resp string of ioctl */
{
    struct iwreq  wrq;
    //u_char    buffer[4096];   /* Only that big in v25 and later */
    u_char    buffer[BUFFER_LEN];   /* Only that big in v25 and later */
    int       i = 0;      /* Start with first command arg */
    int       k;      /* Index in private description table */
    int       temp;
    int       subcmd = 0; /* sub-ioctl index */
    int       offset = 0; /* Space for sub-ioctl index */

    /* Check if we have a token index.
     * Do it now so that sub-ioctl takes precedence, and so that we
     * don't have to bother with it later on... */
    if ((count >= 1) && (sscanf(args[0], "[%i]", &temp) == 1))
    {
        subcmd = temp;
        args++;
        count--;
    }

    /* Search the correct ioctl */
    k = -1;
    while ((++k < priv_num) && strcmp(priv[k].name, cmdname));

    /* If not found... */
    if (k == priv_num)
    {
        fprintf(stderr, "Invalid command : %s\n", cmdname);
        return (-1);
    }

    /* Watch out for sub-ioctls ! */
    if (priv[k].cmd < SIOCDEVPRIVATE)
    {
        int   j = -1;

        /* Find the matching *real* ioctl */
        while ((++j < priv_num) && ((priv[j].name[0] != '\0') ||
                                    (priv[j].set_args != priv[k].set_args) ||
                                    (priv[j].get_args != priv[k].get_args)));

        /* If not found... */
        if (j == priv_num)
        {
            fprintf(stderr, "Invalid private ioctl definition for : %s\n", cmdname);
            return (-1);
        }

        /* Save sub-ioctl number */
        subcmd = priv[k].cmd;
        /* Reserve one int (simplify alignment issues) */
        offset = sizeof(__u32);
        /* Use real ioctl definition from now on */
        k = j;

#if 1
        hccast_log(LL_SPEW, "<mapping sub-ioctl %s to cmd 0x%X-%d>\n", cmdname, priv[k].cmd, subcmd);
#endif
    }

    /* If we have to set some data */
    if ((priv[k].set_args & IW_PRIV_TYPE_MASK) && (priv[k].set_args & IW_PRIV_SIZE_MASK))
    {
        switch (priv[k].set_args & IW_PRIV_TYPE_MASK)
        {
        case IW_PRIV_TYPE_BYTE:
            /* Number of args to fetch */
            wrq.u.data.length = count;
            if (wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
                wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

            /* Fetch args */
            for (; i < wrq.u.data.length; i++)
            {
                sscanf(args[i], "%i", &temp);
                buffer[i] = (char) temp;
            }
            break;

        case IW_PRIV_TYPE_INT:
            /* Number of args to fetch */
            wrq.u.data.length = count;
            if (wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
                wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

            /* Fetch args */
            for (; i < wrq.u.data.length; i++)
            {
                sscanf(args[i], "%i", &temp);
                ((__s32 *) buffer)[i] = (__s32) temp;
            }
            break;

        case IW_PRIV_TYPE_CHAR:
            if (i < count)
            {
                /* Size of the string to fetch */
                wrq.u.data.length = strlen(args[i]) + 1;
                if (wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
                    wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

                /* Fetch string */
                memcpy(buffer, args[i], wrq.u.data.length);
                buffer[sizeof(buffer) - 1] = '\0';
                i++;
            }
            else
            {
                wrq.u.data.length = 1;
                buffer[0] = '\0';
            }
            break;

        case IW_PRIV_TYPE_FLOAT:
            /* Number of args to fetch */
            wrq.u.data.length = count;
            if (wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
                wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

            /* Fetch args */
            for (; i < wrq.u.data.length; i++)
            {
                double      freq;
                if (sscanf(args[i], "%lg", &(freq)) != 1)
                {
                    printf("Invalid float [%s]...\n", args[i]);
                    return (-1);
                }
                if (strchr(args[i], 'G')) freq *= GIGA;
                if (strchr(args[i], 'M')) freq *= MEGA;
                if (strchr(args[i], 'k')) freq *= KILO;
                sscanf(args[i], "%i", &temp);
                iw_float2freq(freq, ((struct iw_freq *) buffer) + i);
            }
            break;

        case IW_PRIV_TYPE_ADDR:
            /* Number of args to fetch */
            wrq.u.data.length = count;
            if (wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
                wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

            /* Fetch args */
            for (; i < wrq.u.data.length; i++)
            {
                if (iw_in_addr(skfd, ifname, args[i], ((struct sockaddr *) buffer) + i) < 0)
                {
                    printf("Invalid address [%s]...\n", args[i]);
                    return (-1);
                }
            }
            break;

        default:
            fprintf(stderr, "Not implemented...\n");
            return (-1);
        }

        if ((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
                (wrq.u.data.length != (priv[k].set_args & IW_PRIV_SIZE_MASK)))
        {
            printf("The command %s needs exactly %d argument(s)...\n", cmdname, priv[k].set_args & IW_PRIV_SIZE_MASK);
            return (-1);
        }
    }   /* if args to set */
    else
    {
        wrq.u.data.length = 0L;
    }

    strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

    /* Those two tests are important. They define how the driver
     * will have to handle the data */
    if ((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
            ((iw_get_priv_size(priv[k].set_args) + offset) <= IFNAMSIZ))
    {
        /* First case : all SET args fit within wrq */
        if (offset)
            wrq.u.mode = subcmd;
        memcpy(wrq.u.name + offset, buffer, IFNAMSIZ - offset);
    }
    else
    {
        if ((priv[k].set_args == 0) &&
                (priv[k].get_args & IW_PRIV_SIZE_FIXED) &&
                (iw_get_priv_size(priv[k].get_args) <= IFNAMSIZ))
        {
            /* Second case : no SET args, GET args fit within wrq */
            if (offset)
                wrq.u.mode = subcmd;
        }
        else
        {
            /* Third case : args won't fit in wrq, or variable number of args */
            wrq.u.data.pointer = (caddr_t) buffer;
            wrq.u.data.flags = subcmd;
        }
    }

    /* Perform the private ioctl */
    if (ioctl(skfd, priv[k].cmd, &wrq) < 0)
    {
        fprintf(stderr, "Interface doesn't accept private ioctl...\n");
        fprintf(stderr, "%s (%X): %s\n", cmdname, priv[k].cmd, strerror(errno));
        return (-1);
    }

    /* If we have to get some data */
    if ((priv[k].get_args & IW_PRIV_TYPE_MASK) && (priv[k].get_args & IW_PRIV_SIZE_MASK))
    {
        int   j;
        int   n = 0;      /* number of args */

        //printf("%-8.16s  %s:", ifname, cmdname);

        /* Check where is the returned data */
        if ((priv[k].get_args & IW_PRIV_SIZE_FIXED) &&
                (iw_get_priv_size(priv[k].get_args) <= IFNAMSIZ))
        {
            memcpy(buffer, wrq.u.name, IFNAMSIZ);
            n = priv[k].get_args & IW_PRIV_SIZE_MASK;
        }
        else
            n = wrq.u.data.length;

        if (resp) memcpy(resp, wrq.u.data.pointer, n);
    }   /* if args to set */

    return (0);
}

/*------------------------------------------------------------------*/
/*
 * Execute a private command on the interface
 */
static inline int
set_private(int     skfd,       /* Socket */
            char   *args[],     /* Command line args */
            int     count,      /* Args count */
            char   *ifname,     /* Dev name */
            char *resp)         /* Resp string */
{
    iwprivargs   *priv;
    int       number;     /* Max of private ioctl */
    int       ret;

    /* Read the private ioctls */
    number = iw_get_priv_info(skfd, ifname, &priv);

    /* Is there any ? */
    if (number <= 0)
    {
        /* Should I skip this message ? */
        fprintf(stderr, "%-8.16s  no private ioctls.\n\n", ifname);
        if (priv)
            free(priv);
        return (-1);
    }

    /* Do it */
    ret = set_private_cmd(skfd, args + 1, count - 1, ifname, args[0], priv, number, resp);

    free(priv);
    return (ret);
}

static int iwpriv_cmd(int argc, char **argv, char *resp)
{
    int skfd;     /* generic raw socket desc. */
    int goterr = 0;

    /* Create a channel to the NET kernel. */
    if ((skfd = iw_sockets_open()) < 0)
    {
        perror("socket");
        return (-1);
    }

    //if(argc == 1)
    if (argc >= 1)
    {
        //iw_enum_devices(skfd, &print_priv_info, NULL, 0);
        goterr = set_private(skfd, argv + 2, argc - 2, argv[1], resp);
    }

    /* Close the socket. */
    iw_sockets_close(skfd);

    return (goterr);
}

#define IW_SCAN_HACK        0x8000

/*
 * Scan state and meta-information, used to decode events...
 */
typedef struct iwscan_state
{
    /* State */
    int   ap_num;     /* Access Point number 1->N */
    int   val_index;  /* Value in table 0->(N-1) */
} iwscan_state;

/*------------------------------------------------------------------*/

/*
 * Perform a scanning on one device
 */
static int iwlist_scan_cmd(int     skfd,
                           char   *ifname,
                           char   *args[],     /* Command line args */
                           int     count)      /* Args count */
{
    struct iwreq        wrq;
    struct iw_scan_req  scanopt;        /* Options for 'set' */
    int                 scanflags = 0;      /* Flags for scan */
    unsigned char      *buffer = NULL;      /* Results */
    int                 buflen = IW_SCAN_MAX_DATA; /* Min for compat WE<17 */
    struct iw_range     range;
    int                 has_range;
    struct timeval      tv;             /* Select timeout */
    int                 timeout = 15000000;     /* 15s */

    /* Avoid "Unused parameter" warning */
    args = args;
    count = count;

    /* Avoid "Unused parameter" warning */
    args = args;
    count = count;

    /* Get range stuff */
    has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);

    /* Check if the interface could support scanning. */
    if ((!has_range) || (range.we_version_compiled < 14))
    {
        hccast_log(LL_ERROR, "%-8.16s  Interface doesn't support scanning.\n\n", ifname);
        return (-1);
    }

    /* Init timeout value -> 250ms between set and first get */
    tv.tv_sec = 0;
    tv.tv_usec = 250000;

    /* Clean up set args */
    memset(&scanopt, 0, sizeof(scanopt));

    /* Parse command line arguments and extract options.
     * Note : when we have enough options, we should use the parser
     * from iwconfig... */
    while (count > 0)
    {
        /* One arg is consumed (the option name) */
        count--;

        /*
         * Check for Active Scan (scan with specific essid)
         */
        if (!strncmp(args[0], "essid", 5))
        {
            if (count < 1)
            {
                hccast_log(LL_ERROR, "Too few arguments for scanning option [%s]\n", args[0]);
                return (-1);
            }
            args++;
            count--;

            /* Store the ESSID in the scan options */
            scanopt.essid_len = strlen(args[0]);
            memcpy(scanopt.essid, args[0], scanopt.essid_len);
            /* Initialise BSSID as needed */
            if (scanopt.bssid.sa_family == 0)
            {
                scanopt.bssid.sa_family = ARPHRD_ETHER;
                memset(scanopt.bssid.sa_data, 0xff, ETH_ALEN);
            }
            /* Scan only this ESSID */
            scanflags |= IW_SCAN_THIS_ESSID;
        }
        else
            /* Check for last scan result (do not trigger scan) */
            if (!strncmp(args[0], "last", 4))
            {
                /* Hack */
                scanflags |= IW_SCAN_HACK;
            }
            else
            {
                hccast_log(LL_ERROR, "Invalid scanning option [%s]\n", args[0]);
                return (-1);
            }

        /* Next arg */
        args++;
    }

    /* Check if we have scan options */
    if (scanflags)
    {
        wrq.u.data.pointer = (caddr_t) &scanopt;
        wrq.u.data.length = sizeof(scanopt);
        wrq.u.data.flags = scanflags;
    }
    else
    {
        wrq.u.data.pointer = NULL;
        wrq.u.data.flags = 0;
        wrq.u.data.length = 0;
    }

    /* If only 'last' was specified on command line, don't trigger a scan */
    if (scanflags == IW_SCAN_HACK)
    {
        /* Skip waiting */
        tv.tv_usec = 0;
    }
    else
    {
        /* Initiate Scanning */
        if (iw_set_ext(skfd, ifname, SIOCSIWSCAN, &wrq) < 0)
        {
            if ((errno != EPERM) || (scanflags != 0))
            {
                hccast_log(LL_ERROR, "%-8.16s  Interface doesn't support scanning : %s\n\n", ifname, strerror(errno));
                return (-1);
            }
            /* If we don't have the permission to initiate the scan, we may
             * still have permission to read left-over results.
             * But, don't wait !!! */

            tv.tv_usec = 0;
        }
    }

    timeout -= tv.tv_usec;

    while (1)
    {
        fd_set    rfds;       /* File descriptors for select */
        int       last_fd;    /* Last fd */
        int       ret;

        /* Guess what ? We must re-generate rfds each time */
        FD_ZERO(&rfds);
        last_fd = -1;

        /* In here, add the rtnetlink fd in the list */

        /* Wait until something happens */
        ret = select(last_fd + 1, &rfds, NULL, NULL, &tv);

        /* Check if there was an error */
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                continue;
            }

            hccast_log(LL_ERROR, "Unhandled signal - exiting...\n");
            return (-1);
        }

        /* Check if there was a timeout */
        if (ret == 0)
        {
            unsigned char    *newbuf;
realloc:
            /* (Re)allocate the buffer - realloc(NULL, len) == malloc(len) */
            newbuf = realloc(buffer, buflen);
            if (newbuf == NULL)
            {
                if (buffer)
                {
                    free(buffer);
                }
                hccast_log(LL_ERROR, "%s: Allocation failed\n", __FUNCTION__);
                return (-1);
            }
            buffer = newbuf;

            /* Try to read the results */
            wrq.u.data.pointer = buffer;
            wrq.u.data.flags = 0;
            wrq.u.data.length = buflen;
            if (iw_get_ext(skfd, ifname, SIOCGIWSCAN, &wrq) < 0)
            {
                /* Check if buffer was too small (WE-17 only) */
                if ((errno == E2BIG) && (range.we_version_compiled > 16) && (buflen < 0xFFFF))
                {
                    /* Some driver may return very large scan results, either
                     * because there are many cells, or because they have many
                     * large elements in cells (like IWEVCUSTOM). Most will
                     * only need the regular sized buffer. We now use a dynamic
                     * allocation of the buffer to satisfy everybody. Of course,
                     * as we don't know in advance the size of the array, we try
                     * various increasing sizes. Jean II */

                    /* Check if the driver gave us any hints. */
                    if (wrq.u.data.length > buflen)
                    {
                        buflen = wrq.u.data.length;
                    }
                    else
                    {
                        buflen *= 2;
                    }

                    /* wrq.u.data.length is 16 bits so max size is 65535 */
                    if (buflen > 0xFFFF)
                    {
                        buflen = 0xFFFF;
                    }

                    /* Try again */
                    goto realloc;
                }

                /* Check if results not available yet */
                if (errno == EAGAIN)
                {
                    /* Restart timer for only 100ms*/
                    tv.tv_sec = 0;
                    tv.tv_usec = 100000;
                    timeout -= tv.tv_usec;
                    if (timeout > 0)
                    {
                        continue;   /* Try again later */
                    }
                }

                /* Bad error */
                free(buffer);
                hccast_log(LL_ERROR, "%-8.16s  Failed to read scan data : %s\n\n", ifname, strerror(errno));
                return (-2);
            }
            else
            {
                /* We have the results, go to process them */
                break;
            }
        }

        /* In here, check if event and event type
         * if scan event, read results. All errors bad & no reset timeout */
    }

    if (wrq.u.data.length)
    {
        hccast_log(LL_NOTICE, "%-8.16s  Scan completed :\n", ifname);
        struct iw_event       iwe;
        struct stream_descr   stream;
        struct iwscan_state   state = { .ap_num = 1, .val_index = 0 };
        int           ret;

        iw_init_event_stream(&stream, (char *) buffer, wrq.u.data.length);
        do
        {
            /* Extract an event and print it */
            ret = iw_extract_event_stream(&stream, &iwe,
                                          range.we_version_compiled);
            if (ret > 0)
            {
                /* Now, let's decode the event */
                switch (iwe.cmd)
                {
                case SIOCGIWAP:
                    //printf("          Cell %02d \n", state.ap_num);
                    state.ap_num++;
                    break;
                }
                //print_scanning_token(&stream, &iwe, &state, &range, has_range);
            }
        }
        while (ret > 0);
        hccast_log(LL_NOTICE, "scan ap num: %d\n", state.ap_num);
    }
    else
    {
        hccast_log(LL_ERROR, "%-8.16s  No scan results\n\n", ifname);
    }

    free(buffer);
    return (0);
}

/*------------------------------------------------------------------*/
/*
 * Map command line arguments to the proper procedure...
 */
typedef struct iwlist_entry
{
    const char       *cmd;        /* Command line shorthand */
    iw_enum_handler   fn;         /* Subroutine */
    int               max_count;
    const char       *argsname;   /* Args as human readable string */
} iwlist_cmd_st;

static const struct iwlist_entry iwlist_cmds[] =
{
    { "scanning",     iwlist_scan_cmd,    -1, "[essid NNN] [last]" },
    { NULL, NULL, 0, 0 },
};

/*------------------------------------------------------------------*/
/*
 * Find the most appropriate command matching the command line
 */
static inline const iwlist_cmd_st *find_command(const char *cmd)
{
    const iwlist_cmd_st *found = NULL;
    int  ambig = 0;
    unsigned int len = strlen(cmd);
    int i = 0;

    /* Go through all commands */
    for (i = 0; iwlist_cmds[i].cmd != NULL; ++i)
    {
        /* No match -> next one */
        if (strncasecmp(iwlist_cmds[i].cmd, cmd, len) != 0)
            continue;

        /* Exact match -> perfect */
        if (len == strlen(iwlist_cmds[i].cmd))
            return &iwlist_cmds[i];

        /* Partial match */
        if (found == NULL)
            /* First time */
            found = &iwlist_cmds[i];
        else
            /* Another time */
            if (iwlist_cmds[i].fn != found->fn)
                ambig = 1;
    }

    if (found == NULL)
    {
        fprintf(stderr, "iwlist: unknown command `%s' (check 'iwlist --help').\n", cmd);
        return NULL;
    }

    if (ambig)
    {
        fprintf(stderr, "iwlist: command `%s' is ambiguous (check 'iwlist --help').\n", cmd);
        return NULL;
    }

    return found;
}

static int iwlist_cmd(int argc, char **argv)
{
    int skfd    = -1;         /* generic raw socket desc. */
    char *dev   = NULL;            /* device name          */
    char *cmd   = NULL;            /* command          */
    char **args = NULL;          /* Command arguments */
    int count   = 0;            /* Number of arguments */
    int goterr  = 0;
    const iwlist_cmd_st *iwcmd = NULL;

    if (argc < 2)
    {
        return -1;
    }

    if (argc == 2)
    {
        cmd = argv[1];
        dev = NULL;
        args = NULL;
        count = 0;
    }
    else
    {
        cmd = argv[2];
        dev = argv[1];
        args = argv + 3;
        count = argc - 3;
    }

    /* find a command */
    iwcmd = find_command(cmd);
    if (iwcmd == NULL)
    {
        return 1;
    }

    /* Check arg numbers */
    if ((iwcmd->max_count >= 0) && (count > iwcmd->max_count))
    {
        return 1;
    }

    /* Create a channel to the NET kernel. */
    if ((skfd = iw_sockets_open()) < 0)
    {
        perror("socket");
        return -1;
    }

    /* do the actual work */
    if (dev)
    {
        goterr = (*iwcmd->fn)(skfd, dev, args, count);
    }
    else
    {
        iw_enum_devices(skfd, iwcmd->fn, args, count);
    }

    /* Close the socket. */
    iw_sockets_close(skfd);

    return goterr;
}

void sendRequest(char *iSend)
{
    char path[128] = {"\0"};
    int port = WPA_CTRL_P2P_IFACE_PORT;
    sprintf(path, WIFI_CTRL_PATH_STA"/%s", P2P_CTRL_IFACE_NAME);

    if (eloop_is_run() != 1)
    {
        hccast_log(LL_ERROR, "%s: WPAS NO RUN!\n", __func__);
        return;
    }

    pthread_mutex_lock(&g_p2p_mutex);
    hccast_log(LL_DEBUG, ">>> %s\n", iSend);

    struct wpa_ctrl *wpa_ctrl = wpa_ctrl_open(path, port);
    if (!wpa_ctrl)
    {
        hccast_log(LL_ERROR, "wpa_ctrl_open failed: %s!\n", strerror(errno));
        return;
    }

    char aResp[64];
    size_t aRespN = sizeof aResp - 1;
    int aErr = wpa_ctrl_request(wpa_ctrl, iSend, strlen(iSend), aResp, &aRespN, NULL);
    if (aErr)
    {
        hccast_log(LL_ERROR, "wpa_ctrl_request failed, ret (%d)!\n", aErr);
    }

    wpa_ctrl_close(wpa_ctrl);
    aResp[aRespN] = '\0';
    hccast_log(LL_DEBUG, "    %s\n", aResp);
    pthread_mutex_unlock(&g_p2p_mutex);
}

int p2p_ctrl_get_peer_ifa(char *ssid, size_t ssid_len, char *bssid, size_t bssid_len, char *mac, size_t mac_len)
{
    char *argv[5];
    int argc;
    char *resp = calloc(BUFFER_LEN, sizeof(char));
    char val[128] = {0};

    argc = 0;
    argv[argc++] = "iwpriv";
    argv[argc++] = "p2p0";
    argv[argc++] = "p2p_get";
    argv[argc++] = "peer_ifa";
    iwpriv_cmd(argc, argv, resp);
    usleep(1000);

    if (result_get(resp, "SSID", val, sizeof(val)) == 0)
    {
        if (ssid && ssid_len > 0)
        {
            snprintf(ssid, ssid_len, "%s", val);
        }
    }

    memset(val, 0, sizeof(val));
    if (result_get(resp, "MAC", val, sizeof(val)) == 0)
    {
        if (bssid && bssid_len > 0)
        {
            snprintf(bssid, bssid_len, "%s", val);
        }
    }

    memset(val, 0, sizeof(val));
    if (result_get(resp, "PEER_ADDR", val, sizeof(val)) == 0)
    {
        if (mac && mac_len > 0)
        {
            snprintf(mac, mac_len, "%s", val);
        }
    }

    free(resp);

    return HCCAST_WIFI_ERR_NO_ERROR;
}

int p2p_ctrl_get_current_network_id()
{
    int id = -1;

    hccast_wifi_list_net_result_t *res = NULL;
    res = (hccast_wifi_list_net_result_t *)calloc(sizeof(hccast_wifi_list_net_result_t), 1);
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return id;
    }

    wifi_ctrl_get_list_net(P2P_CTRL_IFACE_NAME, res);
    for (int i = 0; i < res->net_num; i++)
    {
        if (strstr(res->netinfo[i].flags, "[CURRENT]"))
        {
            id = strtol(res->netinfo[i].net_id, NULL, 10);
            break;
        }
    }

    free(res);
    return id;
}

int p2p_ctrl_is_persistent_current_network()
{
    bool ret = false;

    hccast_wifi_list_net_result_t *res = NULL;
    res = (hccast_wifi_list_net_result_t *)calloc(sizeof(hccast_wifi_list_net_result_t), 1);
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return ret;
    }

    wifi_ctrl_get_list_net(P2P_CTRL_IFACE_NAME, res);
    for (int i = 0; i < res->net_num; i++)
    {
        if (strstr(res->netinfo[i].flags, "[CURRENT][P2P-PERSISTENT]"))
        {
            ret = true;
        }
    }

    free(res);
    return ret;
}

int p2p_ctrl_get_persistent_network_id(char *bssid, char *ssid)
{
    int ret = -1;

    hccast_wifi_list_net_result_t *res = NULL;
    res = (hccast_wifi_list_net_result_t *)calloc(sizeof(hccast_wifi_list_net_result_t), 1);
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return ret;
    }

    wifi_ctrl_get_list_net(P2P_CTRL_IFACE_NAME, res);
    for (int i = 0; i < res->net_num; i++)
    {
        if (!memcmp(res->netinfo[i].bssid, bssid, sizeof(res->netinfo[i].bssid)) \
                && !memcmp(res->netinfo[i].ssid, ssid, sizeof(res->netinfo[i].ssid)))
        {
            ret = i;
            break;
        }
    }

    free(res);
    return ret;
}

int p2p_ctrl_get_current_bssid(char *bssid, unsigned int len)
{
    char found = 0;
    int ret = HCCAST_WIFI_ERR_NO_ERROR;

    if (!bssid || len == 0)
    {
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    hccast_wifi_status_result_t *res = NULL;
    res = (hccast_wifi_status_result_t *)calloc(sizeof(hccast_wifi_status_result_t), 1);
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return HCCAST_WIFI_ERR_MEM;
    }

    ret = wifi_ctrl_get_status(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, res);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "wifi_ctrl_get_status failed!\n");
        free(res);
        return ret;
    }

    strncpy(bssid, res->bssid, strlen(res->bssid) > len ? len : strlen(res->bssid));

    free(res);
    return ret;
}

int p2p_ctrl_get_current_ssid(char *ssid, unsigned int len)
{
    char found = 0;
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    unsigned int real_len = 0;

    if (!ssid || len == 0)
    {
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    hccast_wifi_status_result_t *res = NULL;
    res = (hccast_wifi_status_result_t *)calloc(sizeof(hccast_wifi_status_result_t), 1);
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return HCCAST_WIFI_ERR_MEM;
    }

    ret = wifi_ctrl_get_status(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, res);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "wifi_ctrl_get_status failed!\n");
        free(res);
        return ret;
    }

    real_len = strlen(res->ssid) > (len - 1) ? (len - 1) : strlen(res->ssid);
    strncpy(ssid, res->ssid, real_len);
    ssid[real_len + 1] = '\0';

    free(res);
    return ret;
}

/**
 * It gets the current network  of the WiFi interface
 *
 * @param ifname the interface name, such as wlan0, p2p0, etc.
 * @param mode the mode of the interface, STA or AP
 * @param id the network id
 * @param psk the psk of the query
 */
int p2p_ctrl_get_network_psk(char id, char *psk, unsigned int len)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char val[512] = {0};
    char reply[1024] = {0};
    char cmd[256] = {0};

    snprintf(cmd, sizeof(cmd), "GET_NETWORK %d psk", id);

    if (NULL == psk || len <= 0)
    {
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        hccast_log(LL_ERROR, "param error!\n");
        goto ERROR;
    }

    ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        goto ERROR;
    }

    if (strncmp(reply, "FAIL", 4) == 0)
    {
        ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        hccast_log(LL_ERROR, "%s (FAIL)\n", cmd);
        goto ERROR;
    }

    memcpy(psk, reply, len);
	hccast_log(LL_DEBUG, "psk == %s\n", psk);
ERROR:
    return ret;
}

int p2p_ctrl_get_network_pairwise(int id, char *pairwise, unsigned int len)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char val[512] = {0};
    char reply[1024] = {0};
    char cmd[256] = {0};

    snprintf(cmd, sizeof(cmd), "GET_NETWORK %d pairwise", id);

    if (NULL == pairwise || len <= 0)
    {
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        hccast_log(LL_ERROR, "param error!\n");
        goto ERROR;
    }

    ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        goto ERROR;
    }

    if (strncmp(reply, "FAIL", 4) == 0)
    {
        ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        hccast_log(LL_ERROR, "%s (FAIL)\n", cmd);
        goto ERROR;
    }

    hccast_log(LL_NOTICE, "pairwise == %s\n", reply);
    memcpy(pairwise, reply, len);

ERROR:
    return ret;
}

int p2p_ctrl_p2p_add_group(int group_id)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char cmd[128] = {0};

    snprintf(cmd, sizeof(cmd), "P2P_GROUP_ADD persistent=%d", group_id);

    ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
    }

    return ret;
}

int p2p_ctrl_p2p_remove_group()
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char *cmd = "P2P_GROUP_REMOVE p2p0";

    printf("[%s][%d]\n", __FUNCTION__, __LINE__);
    ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
    }

    return ret;
}

int p2p_ctrl_p2p_remove_network()
{
    char *argv[5];
    int argc;

    argc = 0;
    argv[argc++] = "iwpriv";
    argv[argc++] = "p2p0";

    argc = 2;
    argv[argc++] = "p2p_set";
    argv[argc++] = "profilefound=0";
    iwpriv_cmd(argc, argv, NULL);

    p2p_ctrl_p2p_remove_group();

    return HCCAST_WIFI_ERR_NO_ERROR;
}

int p2p_ctrl_p2p_store_network(p2p_network_list_st *list)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;

    if (!list)
    {
        hccast_log(LL_ERROR, "%s %d param error!\n", __func__, __LINE__);
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    bool isPersistent = p2p_ctrl_is_persistent_current_network();
    if (isPersistent)
    {
        char bssid[WIFI_MAX_SSID_LEN] = {0};
        char mac[WIFI_MAX_SSID_LEN] = {0};
        char ssid[WIFI_MAX_SSID_LEN] = {0};
        char psk[WIFI_MAX_SSID_LEN] = {0};
        char pairwise[WIFI_MAX_PWD_LEN] = {0};

        p2p_ctrl_get_peer_ifa(NULL, 0, NULL, 0, mac, sizeof(mac) - 1);;
        p2p_ctrl_get_current_bssid(bssid, sizeof(bssid));
        p2p_ctrl_get_current_ssid(ssid, sizeof(ssid));

        if (strlen(ssid) <= 0 || strlen(bssid) <= 0 || strlen(mac) <= 0)
        {
            //return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }

        int id = p2p_ctrl_get_current_network_id();
        if (id >= 0)
        {
            p2p_ctrl_get_network_psk(id, psk, sizeof(psk));
            p2p_ctrl_get_network_pairwise(id, pairwise, sizeof(pairwise));
        }
        else
        {
            hccast_log(LL_ERROR, "[%s %d]: get network id failed!\n", __func__, __LINE__);
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }

        bool found = false;
        unsigned fill_id = -1;
        unsigned long last_time = time(NULL);

        for (int i = P2P_MAX_NETWORK_LIST - 1; i >= 0; i--)
        {
            if (!memcmp(list->network[i].ssid, ssid, sizeof(list->network[i].ssid)))
            {
                found = true;
                list->network[i].last_time = time(NULL);
                break;
            }

            if (list->network[i].last_time <= last_time)
            {
                fill_id = i;
                last_time = list->network[i].last_time;
            }
        }

        if (!found && fill_id >= 0)
        {
            memcpy(list->network[fill_id].bssid, bssid, sizeof(list->network[fill_id].bssid));
            memcpy(list->network[fill_id].ssid, ssid, sizeof(list->network[fill_id].ssid));
            memcpy(list->network[fill_id].psk, psk, sizeof(list->network[fill_id].psk));
            memcpy(list->network[fill_id].pairwise, pairwise, sizeof(list->network[fill_id].pairwise));
            memcpy(list->network[fill_id].mac, mac, sizeof(list->network[fill_id].mac));
            list->network[fill_id].last_time = time(NULL);
        }
    }

    return ret;
}

int p2p_ctrl_p2p_add_network(bool isAll, p2p_network_list_st *list, p2p_network_st *network)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char cmd[256] = {0};
    int net_id = -1, i = 0;
    unsigned char pw[32] = {0};

    if (!list && isAll)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }
    else if (!network && !isAll)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    if (isAll)
    {
        for (int i = 0; i < P2P_MAX_NETWORK_LIST; i++)
        {
            if (strlen(list->network[i].bssid) <= 0)
            {
                continue;
            }

            sprintf(cmd, "%s", "ADD_NETWORK");
            ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
                goto EXIT;
            }

            net_id = atoi(reply);

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d ssid \"%s\"", net_id, list->network[i].ssid);
            ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d bssid %s", net_id, list->network[i].bssid);
            ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            char psk[96] = {0};
            memcpy(psk, list->network[i].psk, sizeof(list->network[i].psk));
            sprintf(cmd, "SET_NETWORK %d psk %s", net_id, psk);
            ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d pairwise %s", net_id, list->network[i].pairwise);
            ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d disabled %d", net_id, 2);
            ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d pbss %d", net_id, 0);
            ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d key_mgmt WPA-PSK", net_id);
            ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d auth_alg OPEN", net_id);
            ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
                goto EXIT;
            }

            char *argv[5];
            int argc;

            argc = 0;
            argv[argc++] = "iwpriv";
            argv[argc++] = "p2p0";

            argc = 2;
            argv[argc++] = "p2p_set";

            char profile_bssid[128] = {0};
            sprintf(profile_bssid, "profilefound=%d%s%s%d%s", 1, list->network[i].bssid, list->network[i].mac, strlen(list->network[i].ssid), list->network[i].ssid);
            argv[argc++] = profile_bssid;
            iwpriv_cmd(argc, argv, NULL);
        }
    }
    else
    {
        if (strlen(network->bssid) <= 0)
        {
            ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
            goto EXIT;
        }

        sprintf(cmd, "%s", "ADD_NETWORK");
        ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
            goto EXIT;
        }

        net_id = atoi(reply);

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d ssid \"%s\"", net_id, network->ssid);
        ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;

        char psk[96] = {0};
        memcpy(psk, list->network[i].psk, sizeof(network->psk));

        sprintf(cmd, "SET_NETWORK %d psk %s", net_id, psk);
        ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d pairwise %s", net_id, network->pairwise);
        ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d disabled %d", net_id, 2);
        ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d pbss %d", net_id, 0);
        ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d key_mgmt WPA-PSK", net_id);
        ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d auth_alg OPEN", net_id);
        ret = wifi_ctrl_run_cmd(P2P_CTRL_IFACE_NAME, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
            goto EXIT;
        }

        char cmd[128] = {0};
        char *argv[5];
        int argc;
        char *resp = calloc(BUFFER_LEN, sizeof(char));

        argc = 0;
        argv[argc++] = "iwpriv";
        argv[argc++] = "p2p0";

        argc = 2;
        argv[argc++] = "p2p_set";

        char profile_bssid[128] = {0};
        sprintf(profile_bssid, "profilefound=%d%s%s%d%s", 1, network->bssid, network->mac, strlen(network->ssid), network->ssid);

        argv[argc++] = profile_bssid;
        iwpriv_cmd(argc, argv, resp);
        free(resp);
    }

EXIT:
    return ret;
}

/*
iwpriv p2p0 p2p_set enable=0
iwpriv p2p0 p2p_set status
iwpriv p2p0 p2p_get role
iwpriv p2p0 p2p_set enable=1
iwpriv p2p0 p2p_set status
iwpriv p2p0 p2p_set intent=0
iwpriv p2p0 p2p_set op_ch=1
iwpriv p2p0 p2p_set listen_ch=1
iwpriv p2p0 p2p_set ssid=DIRECT-8188-1
iwpriv p2p0 p2p_set setDN=XXX-FFFFFF-1
iwpriv p2p0 p2p_set got_wpsinfo=3
iwpriv p2p0 p2p_set wfd_type=1
iwpriv p2p0 p2p_set wfd_enable=1
iwpriv p2p0 p2p_get role
iwpriv p2p0 p2p_set persistent=1
iwpriv p2p0 p2p_set got_wpsinfo=3
*/

/**
 * It initializes the P2P wpas attr interface
 */
void p2p_ctrl_wpas_attr_init(void)
{
    char cmd[128] = {0};
    char *argv[10];
    int argc;
	p2p_ctrl_p2p_remove_group();
	
    sendRequest("REMOVE_NETWORK all"); // wifi wpa_cli -x 9890 -i p2p0 remove_network all
    sendRequest("DISCONNECT"); // wifi wpa_cli -x 9890 -i p2p0 disconnect

    argc = 0;
    argv[argc++] = "iwpriv";
    argv[argc++] = "p2p0";

    // argc = 2
    argv[argc++] = "p2p_set";
    argv[argc++] = "enable=0";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    argv[argc++] = "status";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 2;
    argv[argc++] = "p2p_set";
    argv[argc++] = "enable=1";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    argv[argc++] = "status";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    argv[argc++] = "intent=0";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    sprintf(cmd, "op_ch=%d", g_mira_param.oper_channel);
    argv[argc++] = cmd;
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    sprintf(cmd, "listen_ch=%d", g_mira_param.listen_channel);
    argv[argc++] = cmd;
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    sprintf(cmd, "ssid=%s", g_mira_param.device_name);
    argv[argc++] = cmd;
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    sprintf(cmd, "setDN=%s", g_mira_param.device_name);
    argv[argc++] = cmd;
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    argv[argc++] = "got_wpsinfo=3";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    argv[argc++] = "wfd_type=1";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    argv[argc++] = "wfd_enable=1";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    argc = 3;
    argv[argc++] = "persistent=1";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);

    g_p2p_connected = false;
    g_p2p_wps_pbc = false;
    g_mira_param.state_update_func(HCCAST_P2P_STATE_LISTEN);
}

void p2p_ctrl_device_init(void)
{
    hccast_log(LL_NOTICE, "%s p2p device init\n", __func__);
    //p2p_ctrl_wpas_attr_init(); ref p2p_ctrl_set_enable
}

int p2p_ctrl_device_is_go(void)
{
    char *argv[10];
    int argc;
    char *resp = calloc(BUFFER_LEN, sizeof(char));
    char val[32] = {0};
    int role = 0;

    argc = 0;
    argv[argc++] = "iwpriv";
    argv[argc++] = "p2p0";
    argv[argc++] = "p2p_get";
    argv[argc++] = "role";
    iwpriv_cmd(argc, argv, resp);
    usleep(1000);

    if (result_get(resp, "Role", val, sizeof(val)) == 0)
    {
        role = strtol(val, NULL, 10);
    }

    hccast_log(LL_INFO, "role: %d\n", role);

    free(resp);
    return role;
}

void p2p_ctrl_device_enable(void)
{
#if 0
    char *argv[10];
    int argc;

    argc = 0;
    argv[argc++] = "iwpriv";
    argv[argc++] = "p2p0";
    argv[argc++] = "p2p_set";
    argv[argc++] = "enable=1";

    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);
#else
    p2p_ctrl_wpas_attr_init();
#endif
}

// iwpriv p2p0 p2p_set enable=0
void p2p_ctrl_device_disable(void)
{
    char *argv[10];
    int argc;

    sendRequest("DISCONNECT"); // wifi wpa_cli -x 9890 -i p2p0 disconnect
    usleep(1000);

    argc = 0;
    argv[argc++] = "iwpriv";
    argv[argc++] = "p2p0";
    argv[argc++] = "p2p_set";
    argv[argc++] = "enable=0";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);
}

/**
 * It sends a command to the driver to abort the p2p scan
 * note: unofficial cmd
 */
void p2p_ctrl_device_abort_scan(void)
{
    char *argv[10];
    int argc;

    argc = 0;
    argv[argc++] = "iwpriv";
    argv[argc++] = "p2p0";
    argv[argc++] = "scan_abort";
    iwpriv_cmd(argc, argv, NULL);
    usleep(1000);
}

int p2p_ctrl_get_status()
{
    char *argv[10];
    int argc;
    char *resp = calloc(BUFFER_LEN, sizeof(char));
    char val[32] = {0};
    int stat = 0;

    argc = 0;
    argv[argc++] = "iwpriv";
    argv[argc++] = "p2p0";
    argv[argc++] = "p2p_get";
    argv[argc++] = "status";
    iwpriv_cmd(argc, argv, resp);
    usleep(1000);

    if (result_get(resp, "Status", val, sizeof(val)) == 0)
    {
        stat = strtol(val, NULL, 10);
    }

    free(resp);
    return stat;
}

bool p2p_ctrl_get_enable(void)
{
    hccast_log(LL_SPEW, "%s: g_p2p_enable: %d\n", __func__, g_p2p_enable);
    return g_p2p_enable;
}

bool p2p_ctrl_set_enable(bool enable)
{
    g_p2p_enable = enable;
    hccast_log(LL_NOTICE, "%s: p2p %s!\n", __FUNCTION__, g_p2p_enable ? "enable" : "disable");
    if (enable)
    {
        p2p_ctrl_device_enable();
        p2p_ctrl_p2p_add_network(true, &g_p2p_network_list, NULL);
    }
    else
    {
		p2p_ctrl_p2p_remove_network();
        p2p_ctrl_device_disable();
        
    }

    return g_p2p_enable;
}

unsigned int p2p_ctrl_get_device_ip(void)
{
    if (g_ip_addr.s_addr)
    {
        return g_ip_addr.s_addr;
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}

int p2p_ctrl_get_device_rtsp_port(void)
{
    return MIRA_RTSP_PORT;
}

int p2p_ctrl_get_connect_stat(void)
{
    return g_p2p_connect;
}

int p2p_ctrl_set_connect_stat(bool stat)
{
    g_p2p_connect = !!stat;
    return HCCAST_WIFI_ERR_NO_ERROR;
}

int p2p_ctrl_iwlist_scan_cmd(char *inf)
{
    char *argv[10];
    int argc = 0;
    int ret = -1;

    argc = 0;
    argv[argc++] = "iwlist";

    if (inf && !strncasecmp(inf, "wlan0", 5))
    {
        argv[argc++] = "wlan0";
    }
    else if (inf && !strncasecmp(inf, "p2p0", 5))
    {
        argv[argc++] = "p2p0";
    }
    else
    {
        argv[argc++] = "p2p0";
    }

    argv[argc++] = "scanning";
    ret = iwlist_cmd(argc, argv);

    return ret;
}

// rtos
void *p2p_ctrl_thread(void *arg)
{
    // wext no support p2p go mode.
    //udhcpd_start(&g_p2p_udhcpd_conf);
    (void) arg;
    memset(&g_ip_addr, 0, sizeof(g_ip_addr));

    int p2p_state_curr      = 0;
    int p2p_state_last      = 0;

    while (g_p2p_thread_running)
    {
        if (hccast_mira_get_stat())
        {
            p2p_state_curr = p2p_ctrl_get_status();
            if (p2p_state_last != p2p_state_curr)
            {
                hccast_log(LL_SPEW, "p2p_state: %d\n", p2p_state_curr);

                if (P2P_STATE_GONEGO_ING == p2p_state_curr)   /* P2P_STATE_GONEGO_ING = 9*/
                {
                    g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
                    hccast_log(LL_INFO, "P2P_STATE_GONEGO_ING\n");
                }
                else if (!g_p2p_wps_pbc && (P2P_STATE_GONEGO_OK == p2p_state_curr \
                                            || P2P_STATE_RECV_INVITE_REQ_MATCH == p2p_state_curr))     /* P2P_STATE_GONEGO_OK = 10 */
                {
                    g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);

                    if (P2P_STATE_RECV_INVITE_REQ_MATCH == p2p_state_curr)
                    {
                        char ssid[WIFI_MAX_SSID_LEN] = {0};
                        char bssid[WIFI_MAX_SSID_LEN] = {0};

                        p2p_ctrl_get_peer_ifa(ssid, sizeof(ssid) - 1, bssid, sizeof(bssid) - 1, NULL, 0);

                        int found_id = 0;
                        for (int i = 0; i < P2P_MAX_NETWORK_LIST; i++)
                        {
                            if (!memcmp(g_p2p_network_list.network[i].ssid, ssid, sizeof(g_p2p_network_list.network[i].ssid))\
                                    && !strncasecmp(g_p2p_network_list.network[i].bssid, bssid, sizeof(g_p2p_network_list.network[i].bssid)))
                            {
                                found_id = i;
                                break;
                            }
                        }

                        p2p_ctrl_p2p_add_group(found_id);
                        hccast_log(LL_NOTICE, "P2P_STATE_RECV_INVITE_REQ_MATCH (id %d)\n", found_id);
                    }
                    else
                    {
                        hccast_wifi_mgr_p2p_wps_pbc();
                        hccast_log(LL_NOTICE, "P2P_STATE_GONEGO_OK\n");
                    }

                    g_p2p_wps_pbc = true;
                }
                else if (P2P_STATE_PROVISIONING_ING == p2p_state_curr)     /* P2P_STATE_PROVISIONING_ING = 13 */
                {
                    //g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
                    hccast_log(LL_INFO, "P2P_STATE_PROVISIONING_ING\n");
                }
                else if (g_p2p_wps_pbc && !g_p2p_connected && (P2P_STATE_PROVISIONING_DONE == p2p_state_curr))     /* P2P_STATE_PROVISIONING_DONE = 14 */
                {
                    g_p2p_connected = true;
                    if (2 == p2p_ctrl_device_is_go())
                    {
                        p2p_ctrl_udhcpc_stop();
                        p2p_ctrl_udhcpc_start();
                    }

                    p2p_ctrl_p2p_store_network(&g_p2p_network_list);
                    hccast_log(LL_NOTICE, "p2p connected!\n");
                }
                else if (g_p2p_connected && P2P_STATE_PROVISIONING_DONE != p2p_state_curr)
                {
                    g_p2p_wps_pbc = false;
                    g_p2p_connected = false;
                    g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECT_FAILED);
                    //hccast_net_set_if_updown(WIFI_CTRL_IFACE_NAME, HCCAST_NET_IF_UP);
                    hccast_log(LL_NOTICE, "p2p disconnect!\n");
                }

                p2p_state_last = p2p_state_curr;
            }
        }
        else
        {
            hccast_log(LL_INFO, "p2p device is disable\n");
        }

        usleep(200 * 1000);
    }

    g_p2p_thread_running = false;
    g_p2p_tid = 0;
    //udhcpd_stop(&g_p2p_udhcpd_conf);
    hccast_log(LL_NOTICE, "%s end!\n", __func__);

    return NULL;
}

int p2p_ctrl_init(p2p_param_st *params)
{
    if (NULL == params)
    {
        hccast_log(LL_ERROR, "miracast param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    memcpy(&g_mira_param, params, sizeof(p2p_param_st));
    p2p_ctrl_wpas_attr_init();

    pthread_mutex_lock(&g_p2p_mutex);
    if (g_p2p_thread_running)
    {
        hccast_log(LL_NOTICE, "P2P already init!\n");
        pthread_mutex_unlock(&g_p2p_mutex);
        return HCCAST_WIFI_ERR_NO_ERROR;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x4000);

    g_p2p_thread_running = true;
    if (pthread_create(&g_p2p_tid, NULL, p2p_ctrl_thread, NULL) != 0)
    {
        hccast_log(LL_ERROR, "create p2p thread error!\n");
        g_p2p_thread_running = false;
        pthread_mutex_unlock(&g_p2p_mutex);
        return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
    }

    pthread_mutex_unlock(&g_p2p_mutex);
    return HCCAST_WIFI_ERR_NO_ERROR;
}

int p2p_ctrl_uninit(void)
{
    if (g_p2p_thread_running)
    {
        pthread_mutex_lock(&g_p2p_mutex);
        g_p2p_thread_running = false;
        pthread_mutex_unlock(&g_p2p_mutex);
        if (g_p2p_tid != 0)
        {
            pthread_join(g_p2p_tid, NULL);
            g_p2p_tid = 0;
        }
    }

    return HCCAST_WIFI_ERR_NO_ERROR;
}
