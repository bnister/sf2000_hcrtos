#ifndef _HCUAPI_PERSISTENTMEM_H_
#define _HCUAPI_PERSISTENTMEM_H_

#include <hcuapi/iocbase.h>

#define PERSISTENTMEM_NODE_ID_UNUSED	(0)
#define PERSISTENTMEM_NODE_ID_SYSDATA	(1)
#define PERSISTENTMEM_NODE_ID_CASTAPP	(2)

#define PERSISTENTMEM_ERR_ID_DUPLICATED	(-1)
#define PERSISTENTMEM_ERR_ID_NOTFOUND	(-2)
#define PERSISTENTMEM_ERR_NO_SPACE	(-3)
#define PERSISTENTMEM_ERR_OVERFLOW	(-4)
#define PERSISTENTMEM_ERR_FAULT		(-5)

struct persistentmem_node {
	uint16_t id;
	uint16_t offset;
	uint16_t size;
	void *buf;
};

struct persistentmem_node_create {
	uint16_t id;
	uint16_t size;
};

#define PERSISTENTMEM_IOCTL_NODE_CREATE			_IOW (PERSISTENTMEM_IOCBASE, 0, struct persistentmem_node_create)
#define PERSISTENTMEM_IOCTL_NODE_DELETE			_IO  (PERSISTENTMEM_IOCBASE, 1)
#define PERSISTENTMEM_IOCTL_NODE_GET			_IOR (PERSISTENTMEM_IOCBASE, 2, struct persistentmem_node)
#define PERSISTENTMEM_IOCTL_NODE_PUT			_IOW (PERSISTENTMEM_IOCBASE, 3, struct persistentmem_node)

#endif
