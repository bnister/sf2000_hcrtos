#ifndef _HCUAPI_NOTIFIER_H_
#define _HCUAPI_NOTIFIER_H_

typedef void (*worker_t)(void *arg);
typedef void (*worker2_t)(void *arg, unsigned long param);
int work_notifier_setup_proxy(unsigned int evtype, worker2_t worker, void *arg, void *qualifier);
int work_notifier_oneshot_setup_proxy(unsigned int evtype, worker2_t worker, void *arg, void *qualifier);
int work_notifier_teardown(int key);

#endif	/* _HCUAPI_NOTIFIER_H_ */
