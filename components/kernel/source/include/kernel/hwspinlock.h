#ifndef _KERNEL_HWSPINLOCK_H
#define _KERNEL_HWSPINLOCK_H

#ifdef __cplusplus
 extern "C" {
#endif

struct hwspinlock;

struct hwspinlock *hwspin_lock_request(void);
int hwspin_lock_free(struct hwspinlock *hwlock);
struct hwspinlock *hwspin_lock_request_specific(unsigned int id);
int hwspin_lock_timeout(struct hwspinlock *hwlock, unsigned int to);
void hwspin_unlock(struct hwspinlock *hwlock);

#ifdef __cplusplus
}
#endif

#endif
