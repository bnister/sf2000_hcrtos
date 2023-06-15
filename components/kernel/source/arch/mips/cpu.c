#include <kernel/io.h>
#include <kernel/irqflags.h>
#include <kernel/lib/console.h>
#include <kernel/ld.h>

int reset(void)
{
	void *wdt_addr = (void *)&WDT0;

	REG32_CLR_BIT(0xb8800064, BIT1);
	REG32_SET_BIT(0xb8802000, BIT0);
	REG32_WRITE(0xb8802244, 0x80000000);
	REG32_WRITE(0xb8802248, 0x1fffffff);
	REG32_WRITE(0xb8802244, 0x00000000);

	arch_local_irq_disable();
	REG32_WRITE(wdt_addr, 0xfffffffa);
	REG32_WRITE(wdt_addr + 4, 0x26);
	asm volatile(".word 0x1000ffff; nop; nop;"); /* Wait for reboot */
}

static int do_reset(int argc, char **argv)
{
	reset();
	return 0;
}

CONSOLE_CMD(reset, NULL, do_reset, CONSOLE_CMD_MODE_SELF, "reset system")
