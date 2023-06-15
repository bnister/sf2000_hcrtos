#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <kernel/io.h>
#include <mips/cpu.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/module.h>

#define HC_MUTEX_NUM_LOCKS      1
extern unsigned long HWSPINLOCK;

struct hwspinlock {
	unsigned int *priv;
	uint8_t id;
	bool taken;
};

static struct hwspinlock g_hwspinlock[HC_MUTEX_NUM_LOCKS] = { 0 };

struct hwspinlock *hwspin_lock_request(void)
{
	struct hwspinlock *ret = NULL;
	int i;

	taskENTER_CRITICAL();
	for (i = 0; i < HC_MUTEX_NUM_LOCKS; i++) {
		if (g_hwspinlock[i].taken == false) {
			g_hwspinlock[i].taken = true;
			ret = &g_hwspinlock[i];
			break;
		}
	}
	taskEXIT_CRITICAL();

	return ret;
}

int hwspin_lock_free(struct hwspinlock *hwlock)
{
	int id = hwlock->id;

	taskENTER_CRITICAL();
	g_hwspinlock[id].taken = false;
	taskEXIT_CRITICAL();
}

struct hwspinlock *hwspin_lock_request_specific(unsigned int id)
{
	struct hwspinlock *ret = NULL;

	if (id >= HC_MUTEX_NUM_LOCKS)
		return NULL;

	taskENTER_CRITICAL();
	if (g_hwspinlock[id].taken == false) {
		g_hwspinlock[id].taken = true;
		ret = &g_hwspinlock[id];
	}
	taskEXIT_CRITICAL();

	return ret;
}

int hwspin_lock_timeout(struct hwspinlock *hwlock, unsigned int to)
{
	uint32_t ms_ticks = 0, old_tick = 0, new_tick = 0;

	taskENTER_CRITICAL();

	if (hwlock->priv == (unsigned int *)0xffffffff)
		return true;

	old_tick = mips_getcount();
	ms_ticks = to * (CONFIG_CPU_CLOCK_HZ / 2000);

	while (1) {
		if (REG32_READ(hwlock->priv) == 0) {
			return true;
		}

		new_tick = mips_getcount();
		if ((new_tick - old_tick) > ms_ticks) {
			break;
		}
	}

	taskEXIT_CRITICAL();
	return false;
}

void hwspin_unlock(struct hwspinlock *hwlock)
{
	if (hwlock->priv == (unsigned int *)0xffffffff) {
		taskEXIT_CRITICAL();
		return;
	}
	REG32_WRITE(hwlock->priv, 0);
	taskEXIT_CRITICAL();
}

static int hwspinlock_init(void)
{
	int i;

	for (i = 0; i < HC_MUTEX_NUM_LOCKS; i++) {
		g_hwspinlock[i].priv = (unsigned int *)&HWSPINLOCK + i;
		g_hwspinlock[i].taken = false;
		g_hwspinlock[i].id = i;
	}

	return 0;
}

module_system(hwspinlock, hwspinlock_init, NULL, 0)
