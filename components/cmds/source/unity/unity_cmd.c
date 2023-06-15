#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <kernel/lib/console.h>
#include <unity/unity_runner.h>

static void unity_show_help(void)
{
	printf("-h\n");
	printf("\tShow this help.\n");
	printf("-p\n");
	printf("\tList all available test groups.\n");
	printf("-l\n");
	printf("\tList all available test groups and cases.\n");
	printf("-l group_name\n");
	printf("\tList specific group's test cases.\n");
	printf("-v\n");
	printf("\tVerbose.\n");
	printf("-g group_name\n");
	printf("\tRun specific group's test cases.\n");
	printf("-n test_case\n");
	printf("\tRun specific test case.\n");
	printf("-r number\n");
	printf("\tSet how many rounds will be run.\n");
	printf("\nIf there is no specific parameter,");
	printf("default all test cases will be run.\n\n");
}

static int unity_cmd_entry(int argc, char **argv)
{
	char ch;

	opterr = 0;
	optind = 0;

	while ((ch = getopt(argc, argv, "hpl::")) != EOF) {
		switch (ch) {
		case 'h':
			unity_show_help();
			return 0;
		case 'p':
			unity_print_groups();
			return 0;
		case 'l':
			unity_print_test_cases(optarg);
			return 0;
		}
	}

	return unity_run(argc, argv);
}

CONSOLE_CMD(unity, NULL, unity_cmd_entry, CONSOLE_CMD_MODE_SELF, "Unity test command")
