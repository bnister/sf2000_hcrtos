wqueue-y = \
	../../third_party/nuttx/sched/wqueue/kwork_notifier.o \
	../../third_party/nuttx/sched/wqueue/kwork_process.o \
	../../third_party/nuttx/sched/wqueue/kwork_queue.o \
	../../third_party/nuttx/sched/wqueue/kwork_cancel.o \
	../../third_party/nuttx/libs/libc/queue/dq_rem.o \
	../../third_party/nuttx/libs/libc/queue/dq_addlast.o \
	../../third_party/nuttx/libs/libc/queue/dq_remfirst.o

wqueue-$(CONFIG_SCHED_RPCWORK) += \
	../../third_party/nuttx/sched/wqueue/kwork_rpcthread.o
wqueue-$(CONFIG_SCHED_HPWORK) += \
	../../third_party/nuttx/sched/wqueue/kwork_hpthread.o
wqueue-$(CONFIG_SCHED_LPWORK) += \
	../../third_party/nuttx/sched/wqueue/kwork_lpthread.o

obj-y += wqueue.o
