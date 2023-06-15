#include <stdio.h>
#include <stdlib.h>
#include <kernel/irqflags.h>

extern void show_stack(void *task);

void abort(void)
{
	arch_local_irq_disable();
	while(1);
	show_stack(NULL);
	arch_local_irq_disable();
	while(1);
}

__attribute__((noinline)) void __assert_func(const char *file, int line, const char *func,
		   const char *failedexpr)
{
	arch_local_irq_disable();
	asm volatile("nop;.word 0x1000ffff;nop;");
	printf("%s:%d, %s, %s\n", file, line, func, failedexpr);
	show_stack(NULL);
	arch_local_irq_disable();
	asm volatile("nop;.word 0x1000ffff;nop;");
}
