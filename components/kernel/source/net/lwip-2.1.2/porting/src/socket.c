/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <nuttx/config.h>

#include <nuttx/kmalloc.h>
#include <nuttx/fs/fs.h>

#include <assert.h>
#include <fcntl.h>
#include <nuttx/errno.h>
#include <debug.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <packet/af_packet.h>
#include <kernel/completion.h>

#ifdef CONFIG_NET_LWIP_SACK
#include <lwip/sockets.h>

#include "inode.h"

#if !LWIP_COMPAT_SOCKETS

struct socket_t {
	int family;
	union {
		int sockf_lwipfd;
		struct af_packet *sockf_pafpacket;
	} sock_family;

	wait_queue_head_t poll_wq;
	struct completion completion;
	bool closed;
};

#define sock_lwipfd		sock_family.sockf_lwipfd
#define sock_pafpacket		sock_family.sockf_pafpacket

#define CHECK_NULL_PTR(ptr)                                                    \
	do {                                                                   \
		if (ptr == NULL) {                                             \
			set_errno(EFAULT);                                     \
			return -1;                                             \
		}                                                              \
	} while (0)

static int sock_file_open(FAR struct file *filep);
static int sock_file_close(FAR struct file *filep);
static ssize_t sock_file_read(FAR struct file *filep, FAR char *buffer,
			      size_t buflen);
static ssize_t sock_file_write(FAR struct file *filep, FAR const char *buffer,
			       size_t buflen);
static int sock_file_ioctl(FAR struct file *filep, int cmd, unsigned long arg);

static int sock_file_poll(FAR struct file *filep, poll_table *wait);

static struct socket_t *file_socket(FAR struct file *filep);

static struct socket_t *sockfd_socket(int sockfd);

static const struct file_operations g_sock_fileops = {
	sock_file_open, /* open */
	sock_file_close, /* close */
	sock_file_read, /* read */
	sock_file_write, /* write */
	NULL, /* seek */
	sock_file_ioctl, /* ioctl */
	sock_file_poll /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	NULL /* unlink */
#endif
};

static struct inode g_sock_inode = {
	.i_peer = NULL, /* i_peer */
	.i_child = NULL, /* i_child */
	.i_crefs = 1, /* i_crefs */
	.i_flags = FSNODEFLAG_TYPE_SOCKET, /* i_flags */
	.u = {
	        &g_sock_fileops /* u */
	}
};

static int sock_file_open(FAR struct file *filep)
{
	return 0;
}

static int sock_file_close(FAR struct file *filep)
{
	struct socket_t *psock = file_socket(filep);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	psock->closed = true;

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_close(psock->sock_pafpacket, &psock->poll_wq);
		break;
	default:
		ret = lwip_socks_close(psock->sock_lwipfd, &psock->poll_wq);
		break;
	}

	if (!list_empty(&psock->poll_wq.task_list)) {
		wake_up(&psock->poll_wq);
		while (!list_empty(&psock->poll_wq.task_list)) {
			if (wait_for_completion_timeout(&psock->completion, 2) != 0)
				break;
		}
	}

	free(psock);

	return ret;
}

static ssize_t sock_file_read(FAR struct file *filep, FAR char *buffer,
			      size_t buflen)
{
	struct socket_t *psock = file_socket(filep);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_recv(psock->sock_pafpacket, buffer, buflen, 0);
		break;
	default:
		ret = lwip_recv(psock->sock_lwipfd, buffer, buflen, 0);
		break;
	}

	return ret;
}

static ssize_t sock_file_write(FAR struct file *filep, FAR const char *buffer,
			       size_t buflen)
{

	struct socket_t *psock = file_socket(filep);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_send(psock->sock_pafpacket, buffer, buflen, 0);
		break;
	default:
		ret = lwip_send(psock->sock_lwipfd, buffer, buflen, 0);
		break;
	}

	return ret;
}

static int sock_file_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	struct socket_t *psock = file_socket(filep);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_ioctl(psock->sock_pafpacket, cmd, (void *)arg);
		break;
	default:
		ret = lwip_socks_ioctl(psock->sock_lwipfd, (long)cmd, (void *)arg);
		break;
	}

	return ret;
}

static int sock_file_poll(FAR struct file *filep, poll_table *wait)
{
	struct socket_t *psock = file_socket(filep);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	if (psock->closed) {
		complete(&psock->completion);
		return -1;
	}

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_poll(psock->sock_pafpacket, filep, wait);
		break;
	default:
		ret = lwip_socks_poll(psock->sock_lwipfd, filep, wait);
		break;
	}

	return ret;
}

static int sockfd_allocate(FAR void *psock, int oflags)
{
	int sockfd;

	sockfd = files_allocate(&g_sock_inode, oflags, 0, psock, 0);
	if (sockfd >= 0) {
		inode_addref(&g_sock_inode);
	}

	return sockfd;
}

static struct socket_t *file_socket(FAR struct file *filep)
{
	if (filep != NULL && filep->f_inode != NULL &&
	    INODE_IS_SOCKET(filep->f_inode)) {
		return (struct socket_t *)filep->f_priv;
	}

	return NULL;
}

static struct socket_t *sockfd_socket(int sockfd)
{
	FAR struct file *filep;

	if (fs_getfilep(sockfd, &filep) < 0) {
		return NULL;
	}

	return (struct socket_t *)file_socket(filep);
}

int socket(int domain, int type, int protocol)
{
	int oflags = O_RDWR;
	struct socket_t *psock;
	int lwipfd = -1;
	int ret;

	if (type & SOCK_CLOEXEC) {
		//oflags |= O_CLOEXEC;
	}

	if (type & SOCK_NONBLOCK) {
		oflags |= O_NONBLOCK;
	}

	psock = zalloc(sizeof(*psock));
	if (!psock) {
		set_errno(ENOMEM);
		return -ENOMEM;
	}

        init_waitqueue_head(&psock->poll_wq);
	init_completion(&psock->completion);
	psock->family = domain;

	switch (domain) {
	case AF_PACKET:
		psock->sock_pafpacket = packet_socket(domain, type & SOCK_TYPE_MASK, protocol);
		if (psock->sock_pafpacket == NULL) {
			printf("ERROR: packet_socket() failed\n");
			goto __error_handle;
		}
		break;

	default:
		ret = lwip_socket(domain, type & SOCK_TYPE_MASK, protocol);
		if (ret < 0) {
			printf("ERROR: psock_socket() failed: %d\n", ret);
			goto __error_handle;
		}
		lwipfd = ret;
		psock->sock_lwipfd = ret;
		break;
	}


	ret = sockfd_allocate(psock, oflags);
	if (ret < 0) {
		printf("ERROR: Failed to allocate a socket descriptor\n");
		goto __error_handle;
	}

	return ret;

__error_handle:
	if (lwipfd > 0)
		lwip_close(lwipfd);

	if (psock->sock_pafpacket != NULL)
		packet_close(psock->sock_pafpacket, &psock->poll_wq);

	kmm_free(psock);
	return ERROR;
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int oflags = O_RDWR;
	struct socket_t *psock = sockfd_socket(s);
	struct socket_t *psock_new = NULL;
	int lwipfd = -1;
	int ret;

	CHECK_NULL_PTR(psock);

	psock_new = zalloc(sizeof(*psock_new));
	if (!psock_new) {
		set_errno(ENOMEM);
		return -ENOMEM;
	}

        init_waitqueue_head(&psock_new->poll_wq);
	init_completion(&psock_new->completion);

	switch (psock->family) {
	case AF_PACKET:
		psock_new->sock_pafpacket = packet_accept(psock->sock_pafpacket, addr, addrlen);
		if (psock_new->sock_pafpacket == NULL) {
			printf("ERROR: packet_accept() failed\n");
			goto __error_handle;
		}
		break;

	default:
		ret = lwip_accept(psock->sock_lwipfd, addr, addrlen);
		if (ret < 0) {
			printf("ERROR: psock_accept() failed: %d\n", ret);
			goto __error_handle;
		}
		lwipfd = ret;
		psock_new->sock_lwipfd = ret;
		break;
	}

	ret = sockfd_allocate(psock_new, oflags);
	if (ret < 0) {
		printf("ERROR: Failed to allocate a socket descriptor\n");
		goto __error_handle;
	}

	return ret;
	
__error_handle:
	if (lwipfd > 0)
		lwip_close(lwipfd);

	if (psock_new->sock_pafpacket != NULL)
		packet_close(psock_new->sock_pafpacket, &psock_new->poll_wq);

	free(psock_new);
	return ERROR;
}

int bind(int s, const struct sockaddr *name, socklen_t namelen)
{
	struct socket_t *psock = sockfd_socket(s);
	int ret = 0;

	CHECK_NULL_PTR(psock);
	CHECK_NULL_PTR(name);

	if (namelen < sizeof(*name)) {
		set_errno(EINVAL);
		return -1;
	}

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_bind(psock->sock_pafpacket, name, namelen);
		break;
	default:
		ret = lwip_bind(psock->sock_lwipfd, name, namelen);
		break;
	}

	return ret;
}

int shutdown(int s, int how)
{
	struct socket_t *psock = sockfd_socket(s);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_shutdown(psock->sock_pafpacket, how);
		break;
	default:
		ret = lwip_shutdown(psock->sock_lwipfd, how);
		break;
	}
	return ret;
}

int getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
	struct socket_t *psock = sockfd_socket(s);
	int ret = 0;

	CHECK_NULL_PTR(name);
	CHECK_NULL_PTR(namelen);
	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_getpeername(psock->sock_pafpacket, name, namelen);
		break;
	default:
		ret = lwip_getpeername(psock->sock_lwipfd, name, namelen);
		break;
	}
	return ret;
}

int getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
	struct socket_t *psock = sockfd_socket(s);
	int ret = 0;

	CHECK_NULL_PTR(name);
	CHECK_NULL_PTR(namelen);
	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_getsockname(psock->sock_pafpacket, name, namelen);
		break;
	default:
		ret = lwip_getsockname(psock->sock_lwipfd, name, namelen);
		break;
	}
	return ret;
}

int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
	struct socket_t *psock = sockfd_socket(s);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_getsockopt(psock->sock_pafpacket, level, optname, optval, optlen);
		break;
	default:
		ret = lwip_getsockopt(psock->sock_lwipfd, level, optname, optval, optlen);
		break;
	}
	return ret;
}

int setsockopt(int s, int level, int optname, const void *optval,
	       socklen_t optlen)
{
	struct socket_t *psock = sockfd_socket(s);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_setsockopt(psock->sock_pafpacket, level, optname, optval, optlen);
		break;
	default:
		ret = lwip_setsockopt(psock->sock_lwipfd, level, optname, optval, optlen);
		break;
	}
	return ret;
}

int closesocket(int s)
{
	close(s);
}

int connect(int s, const struct sockaddr *name, socklen_t namelen)
{
	struct socket_t *psock = sockfd_socket(s);
	int ret = 0;

	CHECK_NULL_PTR(name);
	CHECK_NULL_PTR(psock);

	if (namelen < sizeof(*name)) {
		set_errno(EINVAL);
		return -1;
	}

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_connect(psock->sock_pafpacket, name, namelen);
		break;
	default:
		ret = lwip_connect(psock->sock_lwipfd, name, namelen);
		break;
	}
	return ret;
}

int listen(int s, int backlog)
{
	struct socket_t *psock = sockfd_socket(s);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_listen(psock->sock_pafpacket, backlog);
		break;
	default:
		ret = lwip_listen(psock->sock_lwipfd, backlog);
		break;
	}
	return ret;
}

ssize_t recv(int s, void *mem, size_t len, int flags)
{
	struct socket_t *psock = sockfd_socket(s);
	ssize_t ret = 0;

	CHECK_NULL_PTR(mem);
	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_recv(psock->sock_pafpacket, mem, len, flags);
		break;
	default:
		ret = lwip_recv(psock->sock_lwipfd, mem, len, flags);
		break;
	}
	return ret;
}

ssize_t recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from,
		 socklen_t *fromlen)
{
	struct socket_t *psock = sockfd_socket(s);
	ssize_t ret = 0;

	CHECK_NULL_PTR(mem);
	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_recvfrom(psock->sock_pafpacket, mem, len, flags, from, fromlen);
		break;
	default:
		ret = lwip_recvfrom(psock->sock_lwipfd, mem, len, flags, from, fromlen);
		break;
	}
	return ret;
}

ssize_t recvmsg(int s, struct msghdr *message, int flags)
{
	struct socket_t *psock = sockfd_socket(s);
	ssize_t ret = 0;

	CHECK_NULL_PTR(message);
	CHECK_NULL_PTR(psock);

	if (message->msg_iovlen) {
		CHECK_NULL_PTR(message->msg_iov);
	}

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_recvmsg(psock->sock_pafpacket, message, flags);
		break;
	default:
		ret = lwip_recvmsg(psock->sock_lwipfd, message, flags);
		break;
	}
	return ret;
}

ssize_t send(int s, const void *dataptr, size_t size, int flags)
{
	struct socket_t *psock = sockfd_socket(s);
	ssize_t ret = 0;

	CHECK_NULL_PTR(dataptr);
	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_send(psock->sock_pafpacket, dataptr, size, flags);
		break;
	default:
		ret = lwip_send(psock->sock_lwipfd, dataptr, size, flags);
		break;
	}
	return ret;
}

ssize_t sendmsg(int s, const struct msghdr *message, int flags)
{
	struct socket_t *psock = sockfd_socket(s);
	ssize_t ret = 0;

	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_sendmsg(psock->sock_pafpacket, message, flags);
		break;
	default:
		ret = lwip_sendmsg(psock->sock_lwipfd, message, flags);
		break;
	}
	return ret;
}

ssize_t sendto(int s, const void *dataptr, size_t size, int flags,
	       const struct sockaddr *to, socklen_t tolen)
{
	struct socket_t *psock = sockfd_socket(s);
	ssize_t ret = 0;

	CHECK_NULL_PTR(dataptr);
	CHECK_NULL_PTR(psock);

	if (to && tolen < sizeof(*to)) {
		set_errno(EINVAL);
		return -1;
	}

	switch (psock->family) {
	case AF_PACKET:
		ret = packet_sendto(psock->sock_pafpacket, dataptr, size, flags, to, tolen);
		break;
	default:
		ret = lwip_sendto(psock->sock_lwipfd, dataptr, size, flags, to, tolen);
		break;
	}
	return ret;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	return lwip_inet_ntop(af, src, dst, size);
}

int inet_pton(int af, const char *src, void *dst)
{
	return lwip_inet_pton(af, src, dst);
}

#ifndef LWIP_INET_ADDR_FUNC
in_addr_t inet_addr(const char *cp)
{
	return ipaddr_addr(cp);
}
#endif

#ifndef LWIP_INET_ATON_FUNC
int inet_aton(const char *cp, struct in_addr *inp)
{
	return ip4addr_aton(cp, (ip4_addr_t *)inp);
}
#endif

#ifndef LWIP_INET_NTOA_FUNC
char *inet_ntoa(struct in_addr in)
{
	return ip4addr_ntoa((const ip4_addr_t *)&(in));
}
#endif

int psock_fstat(int s, struct stat *buf)
{
	struct socket_t *psock = sockfd_socket(s);
	int ret = 0;

	CHECK_NULL_PTR(psock);

	switch (psock->family) {
	case AF_PACKET:
		break;
	default:
		ret = lwip_socks_fstat(psock->sock_lwipfd, buf);
		break;
	}
	return ret;
}

#endif /* !LWIP_COMPAT_SOCKETS */
#endif /* LOSCFG_NET_LWIP_SACK */
