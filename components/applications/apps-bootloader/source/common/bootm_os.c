#include <generated/br2_autoconf.h>

#include <stdio.h>
#include <image.h>
#include <bootm.h>
#include <bootstage.h>
#include <cpu_func.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static int do_bootm_standalone(int flag, int argc, char *const argv[],
			       bootm_headers_t *images)
{
	char *s;
	int (*appl)(int, char *const[]);

	if (flag & (BOOTM_STATE_OS_GO | BOOTM_STATE_OS_FAKE_GO)) {
		appl = (int (*)(int, char * const []))images->ep;
		taskENTER_CRITICAL();
#if defined(CONFIG_BOOT_HCRTOS)
		irq_save_all();
#elif defined(CONFIG_BOOT_HCLINUX_DUALCORE)
		irq_save_av();
#endif
		appl(argc, argv);
		taskEXIT_CRITICAL();
	}
	return 0;
}

static boot_os_fn *boot_os[] = {
	[IH_OS_U_BOOT] = do_bootm_standalone,
	[IH_OS_LINUX] = do_bootm_linux,
};

/* Allow for arch specific config before we boot */
__attribute__((weak)) void arch_preboot_os(void)
{
	/* please define platform specific arch_preboot_os() */
}

/* Allow for board specific config before we boot */
__attribute__((weak)) void board_preboot_os(void)
{
	/* please define board specific board_preboot_os() */
}

int boot_selected_os(int argc, char *const argv[], int state,
		     bootm_headers_t *images, boot_os_fn *boot_fn)
{
	arch_preboot_os();
	board_preboot_os();
	boot_fn(state, argc, argv, images);

	/* Stand-alone may return when 'autostart' is 'no' */
	if (images->os.type == IH_TYPE_STANDALONE ||
	    state == BOOTM_STATE_OS_FAKE_GO) /* We expect to return */
		return 0;
	bootstage_error(BOOTSTAGE_ID_BOOT_OS_RETURNED);
	printf("\n## Control returned to monitor - resetting...\n");

	return BOOTM_ERR_RESET;
}

boot_os_fn *bootm_os_get_boot_func(int os)
{
	return boot_os[os];
}

static void boot_prep_linux(bootm_headers_t *images)
{
}

static void boot_jump_linux(bootm_headers_t *images)
{
	typedef void __attribute__ ((__noreturn__)) (*kernel_entry_t)(int, unsigned long, unsigned long, unsigned long);
	kernel_entry_t kernel = (kernel_entry_t) images->ep;
	unsigned long linux_extra = 0;

	printf("## Transferring control to Linux (at address %p) ...\n", kernel);

	bootstage_mark(BOOTSTAGE_ID_RUN_OS);

	if (images->ft_len) {
		taskENTER_CRITICAL();
		irq_save_all();
		kernel(-2, (unsigned long)images->ft_addr, 0, 0);
	}
}

int do_bootm_linux(int flag, int argc, char *const argv[],
		   bootm_headers_t *images)
{
	/* No need for those on MIPS */
	if (flag & BOOTM_STATE_OS_BD_T)
		return -1;

	/*
	 * Cmdline init has been moved to 'bootm prep' because it has to be
	 * done after relocation of ramdisk to always pass correct values
	 * for rd_start and rd_size to Linux kernel.
	 */
	if (flag & BOOTM_STATE_OS_CMDLINE)
		return 0;

	if (flag & BOOTM_STATE_OS_PREP) {
		boot_prep_linux(images);
		return 0;
	}

	if (flag & (BOOTM_STATE_OS_GO | BOOTM_STATE_OS_FAKE_GO)) {
		boot_jump_linux(images);
		return 0;
	}

	/* does not return */
	return 1;
}
