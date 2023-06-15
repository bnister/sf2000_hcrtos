#ifndef _HCUAPI_AMPRPC_H_
#define _HCUAPI_AMPRPC_H_

#if defined(__HCRTOS__) || defined(__KERNEL__)

#include <hcuapi/notifier.h>
#ifdef __HCRTOS__
#include <poll.h>
#include <nuttx/wqueue.h>
#else
#include <linux/poll.h>
#endif

/*
 * RPC API common definition
 */
enum RPC_DIR {
	RPC_CALL_M2S = 1,
	RPC_RET_M2S = 2,
	RPC_CALL_S2M = 3,
	RPC_RET_S2M = 4,
};

enum RPC_STATUS {
	RPC_STATUS_SETUP = 1,
	RPC_STATUS_DOING = 2,
	RPC_STATUS_RETURN = 3,
	RPC_STATUS_FINISHED = 4,
};

enum RPC_ID {
	/* RPC API from linux to avp */
	RPC_OPEN = 0x1000,
	RPC_CLOSE,
	RPC_READ,
	RPC_WRITE,
	RPC_IOCTL,
	RPC_POLL,
	RPC_WORK_NOTIFIER_SETUP_PROXY,
	RPC_WORK_NOTIFIER_ONESHOT_SETUP_PROXY,
	RPC_WORK_NOTIFIER_TEARDOWN,

	/* RPC API from avp to linux */
	RPC_WORK_NOTIFIER_WORKER,

	RPC_MMAP,
};

typedef struct {
	unsigned int dir;
	unsigned int id;
	unsigned int status;
	unsigned int priv;
} rpc_arg_t;

/*
 * RPC API From linux to avp
 */

typedef struct {
	rpc_arg_t rpc;
	int ret;
	int flags;
	int mode;
	char buf[0];
} open_arg_t;

typedef struct {
	rpc_arg_t rpc;
	int ret;
	int fd;
} close_arg_t;

typedef struct {
	rpc_arg_t rpc;
	int ret;
	int fd;
	unsigned int nbytes;
	char buf[0];
} read_arg_t;

typedef struct {
	rpc_arg_t rpc;
	int ret;
	int fd;
	unsigned int nbytes;
	char buf[0];
} write_arg_t;

typedef struct {
	rpc_arg_t rpc;
	int ret;
	int fd;
	unsigned int cmd;
	unsigned int arg;
	unsigned int parg[0];
} ioctl_arg_t;

typedef struct {
	rpc_arg_t rpc;
	int ret;
	unsigned long int nfds;
	int timeout_msecs;
	struct pollfd fds[];
} poll_arg_t;

typedef struct {
	rpc_arg_t rpc;
	int ret;
	unsigned int evtype;
	worker2_t worker;
	void *arg;
	void *qualifier;
} work_notifier_setup_arg_t;

typedef struct {
	rpc_arg_t rpc;
	int ret;
	int key;
} work_notifier_teardown_arg_t;

typedef struct {
    rpc_arg_t rpc;
    void *ret;
    void *start;
    unsigned long length;
    int prot;
    int flags;
    int fd;
    long offset;
} mmap_arg_t;

/*
 * RPC API From avp to linux
 */

typedef struct {
	rpc_arg_t rpc;
	worker2_t worker;
	void *arg;
	unsigned int evtype;
	unsigned long param;
	unsigned long pparam[0];
} notifier_worker_arg_t;

#ifdef __HCRTOS__
void notifier_worker_proxy(struct work_notifier_s *info);
#else
int avp_close(int fd);
int avp_ioctl(int fd, unsigned int cmd, unsigned long args);
int avp_open(char *buf, int flags, int mode);
ssize_t avp_read(int fd, char *buf, size_t nbytes);
ssize_t avp_write(int fd, char *buf, size_t nbytes);
int avp_poll(struct pollfd *fds, unsigned int nfds, int timeout_msecs);
int avp_work_notifier_setup_proxy(unsigned int evtype, worker2_t worker, void *arg);
int avp_work_notifier_setup_proxy2(unsigned int evtype, worker2_t worker, void *arg, void *qualifier);
int avp_work_notifier_oneshot_setup_proxy(unsigned int evtype, worker2_t worker, void *arg);
int avp_work_notifier_oneshot_setup_proxy2(unsigned int evtype, worker2_t worker, void *arg, void *qualifier);
int avp_work_notifier_teardown(int key);
void *avp_mmap(void *start, size_t length, int prot, int flags, int fd, long offset);
#endif

#endif
#endif	/* _HCUAPI_AMPRPC_H_ */
