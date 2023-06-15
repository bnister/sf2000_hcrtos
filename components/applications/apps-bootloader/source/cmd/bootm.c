#include <kernel/types.h>
#include <kernel/lib/console.h>
#include <image.h>
#include <bootm.h>

int bootm(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	/* determine if we have a sub command */
	argc--; argv++;

	return do_bootm_states(cmdtp, flag, argc, argv, BOOTM_STATE_START |
		BOOTM_STATE_FINDOS | BOOTM_STATE_FINDOTHER |
		BOOTM_STATE_LOADOS |
		BOOTM_STATE_OS_CMDLINE |
		BOOTM_STATE_OS_PREP | BOOTM_STATE_OS_FAKE_GO |
		BOOTM_STATE_OS_GO, &images, 1);
}

int do_bootm(int argc, char **argv)
{
	return bootm(NULL, 0, argc, argv);
}

static char bootm_help_text[] =
	"[addr [arg ...]]\n    - boot application image stored in memory\n"
	"\tpassing arguments 'arg ...'; when booting a Linux kernel,\n"
	"\t'arg' can be the address of an initrd image\n"
	"\tWhen booting a Linux kernel which requires a flat device-tree\n"
	"\ta third argument is required which is the address of the\n"
	"\tdevice-tree blob. To boot that kernel without an initrd image,\n"
	"\tuse a '-' for the second argument";

CONSOLE_CMD(bootm, NULL, do_bootm, CONSOLE_CMD_MODE_SELF, bootm_help_text)
