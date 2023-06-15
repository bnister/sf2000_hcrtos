#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <kernel/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <kernel/module.h>
#include <kernel/io.h>
#include <kernel/ld.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <kernel/soc/soc_common.h>
#include <hcuapi/amprpc.h>
#include <nuttx/wqueue.h>
#include <sys/mman.h>
#include "mmbox_reg_struct.h"
#include "smbox_reg_struct.h"
#include "mmbox_reg.h"
#include "smbox_reg.h"

#define LOG_TAG "AMPRPC"
#include <kernel/elog.h>

#define CONFIG_NMBOX 8
#define CONFIG_NMBOX_CALL 4

static void msg_other_core(uint32_t msg);
static void *msgs[CONFIG_RPCMSG_NUM] = { 0 };
#if (CONFIG_RPCMSG_NUM) > 32
#	error (CONFIG_RPCMSG_NUM) > 32
#endif
#if (CONFIG_NMBOX) > 8
#	error (CONFIG_NMBOX) > 8
#endif
static uint32_t msgbitmap = 0;
static uint32_t mboxbitmap = 0;

static SemaphoreHandle_t msgres = NULL;
static SemaphoreHandle_t mboxres = NULL;
static SemaphoreHandle_t bitmap_locker = NULL;

static mmbox_reg_t *mmbox[8] = {
	(mmbox_reg_t *)&MMBOX0,
	(mmbox_reg_t *)&MMBOX1,
	(mmbox_reg_t *)&MMBOX2,
	(mmbox_reg_t *)&MMBOX3,
	(mmbox_reg_t *)&MMBOX4,
	(mmbox_reg_t *)&MMBOX5,
	(mmbox_reg_t *)&MMBOX6,
	(mmbox_reg_t *)&MMBOX7,
};

static smbox_reg_t *smbox[8] = {
	(smbox_reg_t *)&SMBOX0,
	(smbox_reg_t *)&SMBOX1,
	(smbox_reg_t *)&SMBOX2,
	(smbox_reg_t *)&SMBOX3,
	(smbox_reg_t *)&SMBOX4,
	(smbox_reg_t *)&SMBOX5,
	(smbox_reg_t *)&SMBOX6,
	(smbox_reg_t *)&SMBOX7,
};

static inline void __lock_bitmap(void)
{
	xSemaphoreTake(bitmap_locker, portMAX_DELAY);
}

static inline void __unlock_bitmap(void)
{
	xSemaphoreGive(bitmap_locker);
}

static int rpc_unknown_id_cnt = 0;
static int rpc_unknown_dir_cnt = 0;
static void do_msg(void *msg)
{
	rpc_arg_t *rpc;

	rpc = (rpc_arg_t *)msg;
	if (rpc->dir == RPC_RET_S2M && rpc->status == RPC_STATUS_RETURN) {
		xTaskNotifyGiveIndexed((TaskHandle_t)rpc->priv,
				       configTASK_NOTIFICATION_RPC);
		return;
	} else if (rpc->dir == RPC_CALL_M2S && rpc->status == RPC_STATUS_SETUP) {
		switch (rpc->id) {
		case RPC_OPEN: {
			open_arg_t *arg = (open_arg_t *)msg;
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			arg->ret = open(arg->buf, arg->flags, arg->mode);
			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		case RPC_CLOSE: {
			close_arg_t *arg = (close_arg_t *)msg;
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			arg->ret = close(arg->fd);
			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		case RPC_READ: {
			read_arg_t *arg = (read_arg_t *)msg;
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			arg->ret = read(arg->fd, arg->buf, arg->nbytes);
			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		case RPC_WRITE: {
			write_arg_t *arg = (write_arg_t *)msg;
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			arg->ret = write(arg->fd, arg->buf, arg->nbytes);
			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		case RPC_IOCTL: {
			ioctl_arg_t *arg = (ioctl_arg_t *)msg;
			int sz = _IOC_SIZE(arg->cmd);

			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			if (sz > 0) {
				arg->ret = ioctl(arg->fd, arg->cmd,
						 (unsigned long)arg->parg);
			} else {
				arg->ret = ioctl(arg->fd, arg->cmd,
						 (unsigned long)arg->arg);
			}

			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		case RPC_POLL: {
			poll_arg_t *arg = (poll_arg_t *)msg;
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			arg->ret =
				poll(arg->fds, arg->nfds, arg->timeout_msecs);
			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		case RPC_WORK_NOTIFIER_SETUP_PROXY: {
			work_notifier_setup_arg_t *arg =
				(work_notifier_setup_arg_t *)msg;
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			arg->ret = work_notifier_setup_proxy(
				arg->evtype, arg->worker, arg->arg, arg->qualifier);
			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		case RPC_WORK_NOTIFIER_ONESHOT_SETUP_PROXY: {
			work_notifier_setup_arg_t *arg =
				(work_notifier_setup_arg_t *)msg;
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			arg->ret = work_notifier_oneshot_setup_proxy(
				arg->evtype, arg->worker, arg->arg, arg->qualifier);
			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		case RPC_WORK_NOTIFIER_TEARDOWN: {
			work_notifier_teardown_arg_t *arg =
				(work_notifier_teardown_arg_t *)msg;
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			arg->ret = work_notifier_teardown(arg->key);
			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		case RPC_MMAP: {
			mmap_arg_t *arg = (mmap_arg_t *)msg;
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_DOING);
			arg->ret = mmap(arg->start, arg->length, arg->prot,
					arg->flags, arg->fd, arg->offset);
			REG32_WRITE_SYNC(&arg->rpc.dir, RPC_RET_M2S);
			REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_RETURN);
			msg_other_core((uint32_t)arg);
			break;
		}

		default: {
			rpc_unknown_id_cnt++;
			//asm volatile("nop; .word 0x1000ffff; nop; nop;");
			break;
		}
		}
	} else {
		rpc_unknown_dir_cnt++;
	}

	return;
}

static void do_mbox_release(void *param)
{
	int i = (int)param;

	__lock_bitmap();
	mboxbitmap |= BIT(i);
	__unlock_bitmap();

	xSemaphoreGive(mboxres);
}

static struct work_s *find_work(void)
{
	static struct work_s work[CONFIG_RPCWORK_NUM] = { 0 };
	int i;

	for (i = 0; i < CONFIG_RPCWORK_NUM; i++) {
		if (work_available(&work[i]))
			break;
	}

	if (i == CONFIG_RPCWORK_NUM) {
		/* Not enough works for RPC, all works are busy */
		assert(0);
	}

	return &work[i];
}

static void amprpc_mbox_isr(uint32_t param)
{
	int i;
	volatile uint32_t val, msg;
	for (i = 0; i < CONFIG_NMBOX; i++) {
		val = smbox[i]->mbox_int_enst.val;
		if (val & F_SMBOX_0008_INT_CBK_STA) {
			if (i < 4) {
				asm volatile("nop; .word 0x1000ffff; nop; nop;");
			}
			work_queue(RPCWORK, find_work(), do_mbox_release, (void *)i, 0);
			smbox[i]->mbox_int_enst.val =
				(val & (~F_SMBOX_0008_INT_REC_STA)) | F_SMBOX_0008_INT_CBK_STA;
			while (smbox[i]->mbox_int_enst.val & F_SMBOX_0008_INT_CBK_STA);
		}

		if (val & F_SMBOX_0008_INT_REC_STA) {
			if (i >= 4) {
				asm volatile("nop; .word 0x1000ffff; nop; nop;");
			}
			msg = mmbox[i]->mbox_data0.val;
			mmbox[i]->mbox_data0.val = 0;
			while (mmbox[i]->mbox_data0.val != 0);
			work_queue(RPCWORK, find_work(), do_msg, (void *)msg, 0);

			/* First, clear interrupt status first */
			smbox[i]->mbox_int_enst.val =
				(val & (~F_SMBOX_0008_INT_CBK_STA)) | F_SMBOX_0008_INT_REC_STA;
			while (smbox[i]->mbox_int_enst.val & F_SMBOX_0008_INT_REC_STA);

			/* Second, inform other cpu to release mbox */
			smbox[i]->mbox_int_clr.int_clr = 1;
		}
	}
}

static void amprpc_mbox_init(void)
{
	int i;

	/* setup interrupt for msg in */
	for (i = 0; i < 4; i++) {
		smbox[i]->mbox_int_enst.val =
			F_SMBOX_0008_INT_REC_EN | F_SMBOX_0008_INT_REC_STA;
	}

	/* setup interrupt for msg out */
	for (i = 4; i < 8; i++) {
		smbox[i]->mbox_int_enst.val =
			F_SMBOX_0008_INT_CBK_EN | F_SMBOX_0008_INT_CBK_STA;
	}

	xPortInterruptInstallISR((uint32_t)&MBOX_INTR, amprpc_mbox_isr, (uint32_t)NULL);
}

static int amprpc_init( void )
{
	int i;

	bitmap_locker = xSemaphoreCreateMutex();
	if (bitmap_locker == NULL)
		return -EFAULT;

	msgres = xSemaphoreCreateCounting(CONFIG_RPCMSG_NUM, CONFIG_RPCMSG_NUM);
	if (msgres == NULL)
		return -EFAULT;

	mboxres = xSemaphoreCreateCounting(CONFIG_NMBOX_CALL, CONFIG_NMBOX_CALL);
	if (msgres == NULL)
		return -EFAULT;
	
	msgs[0] = memalign(32, CONFIG_RPCMSG_SIZE * CONFIG_RPCMSG_NUM);
	if (msgs[0] == NULL) {
		return -ENOMEM;
	}
	vPortCacheFlush(msgs[0], CONFIG_RPCMSG_SIZE * CONFIG_RPCMSG_NUM);
	msgs[0] = (void *)MIPS_UNCACHED_ADDR(msgs[0]);
	for (i = 1; i < CONFIG_RPCMSG_NUM; i++) {
		msgs[i] = msgs[i - 1] + CONFIG_RPCMSG_SIZE;
	}

	msgbitmap = (1 << CONFIG_RPCMSG_NUM) - 1;
	mboxbitmap = ((1 << CONFIG_NMBOX_CALL) - 1) << 4;

	amprpc_mbox_init();
	return 0;
}

static void msg_other_core(uint32_t msg)
{
	/* send a msg to other core */
	int i;

	xSemaphoreTake(mboxres, portMAX_DELAY);

	__lock_bitmap();
	i = ffs(mboxbitmap);
	assert(i != 0);
	i--;
	mboxbitmap &= ~BIT(i);
	__unlock_bitmap();

	smbox[i]->mbox_data0.data = msg;
	write_sync();
	if (smbox[i]->mbox_data0.data != msg) {
		asm volatile("nop; .word 0x1000ffff; nop; nop;");
	}
	smbox[i]->mbox_int_trig.int_trig = 1;
}

int work_notifier_oneshot_setup_proxy(unsigned int evtype, worker2_t worker, void *arg, void *qualifier)
{
	struct work_notifier_s info = { 0 };

	info.evtype = evtype;
	info.qid = HPWORK;
	info.remote = true;
	info.oneshot = true;
	info.qualifier = qualifier;
	info.arg = arg;
	info.worker2 = worker;
	return work_notifier_setup(&info);
}

int work_notifier_setup_proxy(unsigned int evtype, worker2_t worker, void *arg, void *qualifier)
{
	struct work_notifier_s info = { 0 };

	info.evtype = evtype;
	info.qid = HPWORK;
	info.remote = true;
	info.oneshot = false;
	info.qualifier = qualifier;
	info.arg = arg;
	info.worker2 = worker;
	return work_notifier_setup(&info);
}

void notifier_worker_proxy(struct work_notifier_s *info)
{
	notifier_worker_arg_t *arg = NULL;
	int sz = _IOC_SIZE(info->evtype);
	int dir = _IOC_DIR(info->evtype);
	int i;

	if (sizeof(*arg) + sz > CONFIG_RPCMSG_SIZE) {
		printf("Error parameters too large %ld, giveup\n",
		       sizeof(*arg) + sz);
		return;
	}

	xSemaphoreTake(msgres, portMAX_DELAY);

	__lock_bitmap();
	i = ffs(msgbitmap);
	assert(i != 0);
	i--;
	msgbitmap &= ~BIT(i);
	__unlock_bitmap();

	arg = msgs[i];

	arg->rpc.dir = RPC_CALL_S2M;
	arg->rpc.id = RPC_WORK_NOTIFIER_WORKER;
	arg->rpc.priv = (unsigned int)xTaskGetCurrentTaskHandle();
	arg->worker = info->worker2;
	arg->evtype = info->evtype;
	arg->arg = info->arg;

	if ((dir & _IOC_READ) && sz > 0) {
		memcpy((void *)arg->pparam, (void *)info->param, sz);
	} else {
		arg->param = info->param;
	}
	
	REG32_WRITE_SYNC(&arg->rpc.dir, RPC_CALL_S2M);
	REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_SETUP);
	msg_other_core((uint32_t)arg);

	ulTaskNotifyTakeIndexed(configTASK_NOTIFICATION_RPC, pdTRUE,
				portMAX_DELAY);

	if ((dir & _IOC_WRITE) && sz > 0) {
		memcpy((void *)info->param, (void *)arg->pparam, sz);
	}

	REG32_WRITE_SYNC(&arg->rpc.status, RPC_STATUS_FINISHED);

	__lock_bitmap();
	msgbitmap |= BIT(i);
	__unlock_bitmap();

	xSemaphoreGive(msgres);

	return ;
}

unsigned int is_amp(void)
{
	return 1;
}

module_driver_late(amprpc, amprpc_init, NULL, 0)
