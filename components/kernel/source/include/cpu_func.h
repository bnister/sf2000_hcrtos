#ifndef __CPU_LEGACY_H
#define __CPU_LEGACY_H

extern int reset(void);
extern uint32_t get_processor_id(void);
extern void hw_watchdog_reset(unsigned long delay_us);
extern void hw_watchdog_disable(void);
extern void cache_flush(void *src, uint32_t len);
extern void cache_flush_invalidate(void *src, uint32_t len);
extern void cache_flush_all(void);
extern void cache_invalidate(void *src, uint32_t len);
extern void irq_save_all(void);
extern void irq_restore_all(void);
extern void irq_save_av(void);
extern unsigned int __attribute__((noinline)) write_sync(void);
extern unsigned int is_amp(void);
extern int __cpu_clock_hz;
#define get_cpu_clock() __cpu_clock_hz
#define flush_cache(a, b) cache_flush((void *)(a), (uint32_t)(b))

#endif
