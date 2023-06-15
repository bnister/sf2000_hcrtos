#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "unity_internals.h"
#include <unity_runner.h>

extern unsigned long _runner_init_start;
extern unsigned long _runner_init_end;
extern unsigned long _test_case_init_start;
extern unsigned long _test_case_init_end;

static void run_all_group_runners(void)
{
	struct unity_runner_init *runner_start =
		(struct unity_runner_init *)&_runner_init_start;
	struct unity_runner_init *runner_end =
		(struct unity_runner_init *)&_runner_init_end;
	struct unity_runner_init *p;

	for (p = runner_start; p < runner_end; p++) {
		p->runner();
	}
}

int unity_print_groups(void)
{
	struct unity_runner_init *runner_start =
		(struct unity_runner_init *)&_runner_init_start;
	struct unity_runner_init *runner_end =
		(struct unity_runner_init *)&_runner_init_end;
	struct unity_runner_init *p;
	int count = 0;

	for (p = runner_start; p < runner_end; p++) {
		UnityPrint(p->group);
		UNITY_PRINT_EOL();
		count++;
	}

	return count;
}

int unity_print_test_cases(const char *group)
{
	struct unity_test_case_init *test_case_start =
		(struct unity_test_case_init *)&_test_case_init_start;
	struct unity_test_case_init *test_case_end =
		(struct unity_test_case_init *)&_test_case_init_end;
	struct unity_test_case_init *p;
	const char *cur = NULL;
	int count = 0;

	for (p = test_case_start; p < test_case_end; p++) {
		if ((group != NULL) && strcmp(group, p->group) != 0)
			continue;

		if ((cur == NULL) || (strcmp(cur, p->group) != 0)) {
			UnityPrint("Group: ");
			UnityPrint(p->group);
			UNITY_PRINT_EOL();
			cur = p->group;
		}

		UnityPrint(" Case: ");
		UnityPrint(p->name);
		UNITY_PRINT_EOL();
		count++;
	}

	return count;
}

int unity_run(int argc, char **argv)
{
	return UnityMain(argc, (const char **)argv, run_all_group_runners);
}

