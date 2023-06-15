#include <kernel/ld.h>
#include <kernel/io.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/module.h>
#include <kernel/lib/backtrace.h>


static void bus_err_isr(uint32_t parameter)
{
	/* clear bus_error/ack_error interrupt status */
	REG32_SET_FIELD2((void *)&SYSIO0 + 0x24, 8, 3, 0x7);
	//show_stack(NULL);
}

static void ilb_ack_isr(uint32_t parameter)
{
	REG32_SET_FIELD2((void *)&SYSIO0 + 0x24, 8, 3, 0x7);
	//show_stack(NULL);
}

static int nbdebug_init(void)
{
	/* clear bus_error/ack_error interrupt status */
	REG32_SET_FIELD2((void *)&SYSIO0 + 0x24, 8, 3, 0x7);

	/* register 0xb8800024 bit[2:0] */
	REG32_SET_FIELD2((void *)&SYSIO0 + 0x24, 0, 3, 0x7);

	xPortInterruptInstallISR((int)&CPU_ILB_ACK_INTR, ilb_ack_isr, 0);
	xPortInterruptInstallISR((int)&CPU_BUS_ERR_INTR, bus_err_isr, 0);

	return 0;
}

module_driver(nbdbg, nbdebug_init, NULL, 0)
