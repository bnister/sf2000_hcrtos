#ifndef _KERNEL_TRACE_IRQFLAGS_H
#define _KERNEL_TRACE_IRQFLAGS_H

extern unsigned long arch_local_irq_save(void);
extern void arch_local_irq_restore(unsigned long flags);
extern unsigned long arch_local_irq_disable(void);
extern void arch_local_irq_enable(void);

#endif
